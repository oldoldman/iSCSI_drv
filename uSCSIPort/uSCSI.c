
#include "precomp.h"

NTSTATUS DriverEntry(IN PDRIVER_OBJECT 	DriverObject,IN PUNICODE_STRING	RegistryPath)
{
	return STATUS_SUCCESS;
}

//
// Global
//

BOOLEAN		uSCSIPortInitialized = FALSE;
KSPIN_LOCK	SessionListLock;
LIST_ENTRY	SessionList;

KSPIN_LOCK	TgtsLock;
LIST_ENTRY	Targets;
ULONG		TgtCount;

//
// Exported function
//

ULONG TiCountTargetSize()
{
	PITGT		Itgt;
	ULONG		Size = 0;
	PLIST_ENTRY	TgtEnt , TgtEnt2;
	
	TgtEnt = &Targets;
	TgtEnt2 = TgtEnt->Flink;
	
	while ( TgtEnt2 != TgtEnt)
	{
		Itgt = CONTAINING_RECORD ( TgtEnt2 , ITGT , List );
		Size += strlen (Itgt->Tgt.TargetName) + 1;
		Size += sizeof ( TGT );
		TgtEnt2 = TgtEnt2->Flink;
	}
	
	if (!Size)
		return Size;
		
	return Size + sizeof (TGTS);
}

//
//
//

ULONG uSCSIGetTargetsSize()
{
	KIRQL		OldIrql;
	ULONG		Size = 0;

	ExAcquireSpinLock ( &TgtsLock , &OldIrql );
	Size = TiCountTargetSize();
	ExReleaseSpinLock ( &TgtsLock , OldIrql );
	
	return Size;
}

//
//
//

NTSTATUS uSCSIPopTargets( PTGTS Tgts)
{
	KIRQL		OldIrql;
	PTGT		Tgt;
	PITGT		Itgt;
	PUCHAR		NameBuf;
	ULONG		Len , NameOffset;
	PLIST_ENTRY	TgtEnt , TgtEnt2;
	
	ExAcquireSpinLock ( &TgtsLock , &OldIrql );
	
	if ( Tgts->Size != TiCountTargetSize() )
		return STATUS_INVALID_PARAMETER;
	
	Tgts->Count = TgtCount;
	Tgt = (PTGT)(Tgts + 1);
	NameBuf = (PUCHAR)(Tgt + Tgts->Count);
	NameOffset = NameBuf - (PUCHAR)Tgts;
	
	TgtEnt = &Targets;
	TgtEnt2 = TgtEnt->Flink;
	
	while ( TgtEnt2 != TgtEnt)
	{
		Itgt = CONTAINING_RECORD ( TgtEnt2 , ITGT , List );
		
		Len = strlen ( Itgt->Tgt.TargetName ) + 1 ;	
			
		RtlCopyMemory ( NameBuf , Itgt->Tgt.TargetName ,  Len);
		
		Tgt->NameOffset = NameOffset;
		Tgt->Addr = Itgt->Tgt.Addr;
		Tgt->Port = Itgt->Tgt.Port;
		
		Tgt++;
		NameBuf += Len;
		NameOffset += Len;
		
		TgtEnt2 = TgtEnt2->Flink;
	}
	
	ExReleaseSpinLock ( &TgtsLock , OldIrql );

	return STATUS_SUCCESS;		
}

//
//
//

VOID uSCSIAddTarget(PUCHAR TgtName , ULONG Addr , USHORT Port)
{
	ULONG		Len;
	PITGT		Itgt;
	KIRQL		OldIrql;
	
	Len = strlen (TgtName);
	
	if ( Len )
	{
		Itgt = ExAllocatePoolWithTag ( PagedPool , sizeof (ITGT) + Len + 1, USCSI_TAG);
		Itgt->Tgt.TargetName = (PUCHAR)( Itgt + 1);
		RtlCopyMemory ( Itgt->Tgt.TargetName , TgtName , Len + 1);
		Itgt->Tgt.Addr =Addr;
		Itgt->Tgt.Port = Port;
		
		ExAcquireSpinLock ( &TgtsLock , &OldIrql );
		InsertTailList ( &Targets , &Itgt->List);
		TgtCount++;
		ExReleaseSpinLock ( &TgtsLock , OldIrql );
	}	
}

//
//
//

VOID uSCSIInitialize()
{	
	if ( uSCSIPortInitialized )
		return;

	PtInit();
	InitializeListHead ( &SessionList );
	InitializeListHead ( &Targets );
	KeInitializeSpinLock ( &SessionListLock);
	KeInitializeSpinLock ( &TgtsLock);
		
	uSCSIPortInitialized = TRUE;
	TgtCount = 0;
	//uSCSIAddTarget ( "iqn.com.yushang:vdisk.0001" , INETADDR (192,168,1,100) , HTONS(3260) );
}

//
//
//

PVOID uSCSICreateSession( PUCHAR Target , PUSCSI_CALL_BACK CallBack )
{
	NTSTATUS		Status;
	PITGT			Itgt;
	PLIST_ENTRY		TgtEnt , TgtEnt2;
	PSESSION		Session;
	PuCONINFO		ConInfo;
	ULONG			Cons;
	
	if (!uSCSIPortInitialized)
		return NULL;
		
	TgtEnt = &Targets;
	TgtEnt2 = TgtEnt->Flink;
	
	while ( TgtEnt != TgtEnt2 )
	{
		Itgt = CONTAINING_RECORD ( TgtEnt2 , ITGT , List);
		if ( !strcmp( Itgt->Tgt.TargetName , Target ))
			goto Found;
		TgtEnt2 = TgtEnt2->Flink;
	}
	
	return NULL;
	
Found:

	if ( !CallBack->CompleteCmd )
		return NULL;

	Session = TpAllocateSession( Itgt );
	Session->CallBack = *CallBack;
	
	TpAddConInfo (Session);
		
	Status = PtLogin (Session->LeadingCon);
	if (NT_SUCCESS (Status) )
	{
		ExInterlockedInsertTailList ( &SessionList , &Session->List , &SessionListLock);			
		return Session;
	}
	else
	{
		TpFreeSession (Session);
		return NULL;
	}
}

VOID PiSendDataOut(PuCON_CTX Ctx , PPDU CmdOrR2T );

//
//
//

VOID uSCSIProcessSCSICmd ( PVOID 	Session , 
						   PUCHAR 	Cdb , 
						   ULONG 	CdbLength , 
						   PUCHAR 	DataBuf , 
						   ULONG 	WriteSize,
						   ULONG 	ReadSize ,
						   UCHAR	TaskAttr,
						   UCHAR	Dir ,
						   PULONG	Handle)
{
	PuCONINFO	ConInfo;
	PPDU		Cmd;
	
	ConInfo = TpBestConnection (Session);
	
	Cmd = PtAssembleSCSICmd ( ConInfo , 
							  Cdb , 
							  CdbLength , 
							  DataBuf , 
							  WriteSize , 
							  ReadSize , 
							  TaskAttr ,
							  Dir );
	if ( Handle )
		REVERSE_BYTES ( Handle , &Cmd->Bhs->STAT_FIELDS.InitiatorTaskTag);
		
	TpQueueOutPDU ( ConInfo->ConCtx , Cmd );
	PiSendDataOut ( ConInfo->ConCtx , Cmd );

}

//
//
//

VOID uSCSIDiscover( ULONG Portal , USHORT Port)
{
	
}

//
//
//

PLIST_ENTRY uSCSIGetTargets()
{
	PLIST_ENTRY		Tgts = NULL;
	
	return Tgts;
}

//
//
//

PLIST_ENTRY uSCSIGetSessions()
{
	PLIST_ENTRY		Sessions = NULL;
	
	return Sessions;
}

//
//
//

PUSCSI_SESSION_INFO uSCSIGetSessionInfo ( PVOID Session )
{
	PUSCSI_SESSION_INFO	Info = NULL;
	
	return Info;
}
//
// For Extension Module
//

