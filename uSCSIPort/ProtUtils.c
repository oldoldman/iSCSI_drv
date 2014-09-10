#include "precomp.h"

PKEY_VALUE PiFormatKeyValue (PKEY_VALUE Val);
	
ULONG PiGetKeyState (KEY_NAME Key , PuCONINFO ConInfo );
	
VOID PiSetKeyState ( KEY_NAME Key , PuCONINFO ConInfo , UCHAR State);

BOOLEAN PiInitiatorName( PUCHAR *Name)
{
	*Name = "iqn.2009-09.yushang.com.uSCSI:vDisk.0001";
	return FALSE;
}

BOOLEAN PiInitiatorAlias( PUCHAR *Alias)
{
	*Alias = "Personal Disk of yushang";
	return FALSE;
}

BOOLEAN PiAuthMethod( PUCHAR *Method)
{
	*Method = "CHAP,KRB5,SPKM1,SPKM2,SRP,None";
	return FALSE;
}

VOID PiProtError (PuCONINFO ConInfo , PUCHAR Func )
{
	PROT_Debug (("%s\n" , Func ));
	//TODO: Colse connection
}

/*
   Memory Layout
   
   -------- <- KEY_VALUE
     ...
   -------- <- Value[0].String if any
     ...
   -------- <- Value[1].String if any
     ...
   -------- <- FormatStr if any
     ...
   --------
*/
PKEY_VALUE PiAllocateKeyVal(ULONG StrLen , ULONG StrLen2 , ULONG StrLen3)
{
	PKEY_VALUE	Kv;

	Kv = ExAllocatePoolWithTag ( PagedPool , 
								sizeof (KEY_VALUE) + StrLen + StrLen2 + StrLen3, 
								USCSI_TAG);
	if ( !Kv )
		return Kv;
		
	InitializeListHead (&Kv->Vals);
	Kv->Size = sizeof (KEY_VALUE) + StrLen + StrLen2 + StrLen3;
	Kv->Len = 0;
	Kv->Flags = 0;
	Kv->NegoRetries = 0;
	Kv->Formal = KEY_F;
	Kv->Pending = KEY_P;
		
	if ( StrLen )
		Kv->Value[0].String = (PUCHAR)(Kv+1);
	else
		Kv->Value[0].String = NULL;
		
	if (StrLen2)
		Kv->Value[1].String = (PUCHAR)(Kv+1) + StrLen;
	else
		Kv->Value[1].String = NULL;
		
	if (StrLen3)
		Kv->FormatStr = (PUCHAR)(Kv+1) + StrLen + StrLen2;
	else
		Kv->FormatStr = NULL;
	
	return Kv;
}

/*
  Called every turn of PiProcessKeys
*/
VOID PiCheckNegoState( PuCONINFO ConInfo , PLIST_ENTRY Answers )
{
	ULONG		KeyState;
	PKEY_VALUE	Kv , Kv2;
	KEY_NAME	Key;
	
	for ( Key = 0 ; Key < KEY_LastKey ; Key++)
	{
		if ( Key == KEY_MaxSessionKey )
			continue;
			
		Kv = KV( ConInfo , Key );
		
		// Some Key may not get initialized
		
		if (!Kv)
			continue;
			
		if ( KEY_Proposed == PiGetKeyState ( Key , ConInfo) )
		{
			Kv->NegoRetries++;
			
			if ( Kv->NegoRetries <= PiKeys[Key].NegoRetries )
			{				
				// Repropose it
				Kv2 = PiFormatKeyValue ( Kv );
				InsertTailList ( Answers , &Kv2->Vals );
				ConInfo->MoreWorks = TRUE;
			}
			else if ( PiKeys[Key].NegoFailAction == KEY_NEGO_FAIL_INGORE )
			{
				PiSetKeyState ( Key , ConInfo , KEY_Agreed );
			}
			else if ( PiKeys[Key].NegoFailAction == KEY_NEGO_FAIL_CLOSE )
			{
				// Close XPT
				PROT_Debug(("Close Connection\n"));
			}
		}
	}
}

//
// Allocate a PDU for LOGIN or TEXT
// Segmentation if needed
//

PPDU PiAllocateTxtPDU(PuCONINFO ConInfo , PPDU Pdu , PUCHAR Data  , ULONG Len , UCHAR Opcode)
{
	LONG		Remaining , DataLen , MaxDataSeg;
	PPDU		NewPdu , Tmp;
	PUCHAR		Data2;
	
	Data2 		= Data;
	NewPdu 		= NULL;	
	Remaining 	= Len;
	
	/*5.3 
	The default MaxRecvDataSegmentLength is used during Login
	*/	
	MaxDataSeg = (LONG)KEY_FV(KV(ConInfo,KEY_MaxRecvDataSegmentLength)).Number;
	
	do
	{		
		DataLen = min ( Remaining , MaxDataSeg );
					
		Tmp = TpAllocatePDU( ConInfo->ConCtx , Opcode , 0 , 0 );
		
		if ( !Tmp )
			goto ErrorOut;
		else if ( NewPdu )
			InsertTailList ( &NewPdu->Continue , &Tmp->Continue);	//Part of	
		else
			NewPdu = Tmp;		//Head
			
		Tmp->Data = Data2;
		
		Tmp->Bhs->GENERICBHS.Immediate = 1;
		REVERSE_3BYTES ( &Tmp->Bhs->GENERICBHS.DataSegmentLength , &DataLen);
		/*
		Tmp->Bhs->GENERICBHS.DataSegmentLength[0] = (UCHAR)(( DataLen & 0xFF0000 ) >> 16);
		Tmp->Bhs->GENERICBHS.DataSegmentLength[1] = (UCHAR)(( DataLen & 0xFF00 ) >> 8);
		Tmp->Bhs->GENERICBHS.DataSegmentLength[2] = (UCHAR)( DataLen & 0xFF );*/
		
		if ( Opcode == OP_LOGIN_REQ)
			Tmp->Bhs->LOGIN_REQUEST.Continue = 1;
		else if ( Opcode == OP_TXT_REQ )
			Tmp->Bhs->TXT_REQUEST.Continue = 1;
		
		//???
		if (Pdu)
			RtlCopyMemory ( Tmp->Bhs->TXT_REQUEST.TargetTransferTag,
							Pdu->Bhs->TXT_RESPONSE.TargetTransferTag,
							4);		
		Data2 += DataLen;
		
		Remaining -= DataLen;
			
	}while ( Remaining > 0 );
	//
	// Set last one's Buffer so we can release it later
	//
	Tmp->Buffer = Data;
	//
	// and Continue bit
	//
	if ( Opcode == OP_LOGIN_REQ)
		Tmp->Bhs->LOGIN_REQUEST.Continue = 0;
	else if ( Opcode == OP_TXT_REQ )
		Tmp->Bhs->TXT_REQUEST.Continue = 0;		
	
	return NewPdu;

ErrorOut:
	
	if (NewPdu)
	{
		TpQueuePduRelease ( ConInfo->ConCtx , NewPdu );
		/*
		while ( !IsListEmpty ( &NewPdu->Continue) )
		{
			PPDU			Tmp;
			PLIST_ENTRY		Ent;
			Ent = RemoveHeadList ( &NewPdu->Continue );
			Tmp = CONTAINING_RECORD ( Ent , PDU , Continue );
			ExFreePoolWithTag( Tmp , USCSI_TAG );
		}		
		ExFreePoolWithTag( NewPdu , USCSI_TAG );*/
	}
	
	return NULL;
}

/*
  Called for normal LOGIN or TEXT response PDU
*/
VOID PiAnswerPdu( PPDU Pdu , PuCONINFO ConInfo , UCHAR Opcode )
{
	LONG			DataLen;
	PPDU			LPdu , LPdu2;
	PLIST_ENTRY		PduEnt;

	LPdu = ConInfo->Answer;

	if ( !LPdu )
		//Allocate a PDU with DataSegmentLength = 0 , the so-called SYNC PDU
		LPdu = TpAllocatePDU( ConInfo->ConCtx , Opcode , 0 , 0 );
			
	if ( !IsListEmpty ( &LPdu->Continue) )
	{
		// Part of a logical PDU
		LPdu2 = CONTAINING_RECORD ( LPdu->Continue.Flink , PDU , Continue );
		ConInfo->Answer = LPdu2;
		InitializeListHead ( &LPdu->Continue );	
		
		if ( Opcode == OP_LOGIN_REQ )		
			LPdu->Bhs->LOGIN_REQUEST.Continue = 1;
		else if ( Opcode == OP_TXT_REQ)
			LPdu->Bhs->TXT_REQUEST.Continue = 1;
	}
	else
	{
		// End of a logical PDU
		ConInfo->Answer = NULL;
		
		if ( Opcode == OP_LOGIN_REQ && ConInfo->LoginState == LOGIN_STAT_PENDING_TRANSIT )
			LPdu->Bhs->LOGIN_REQUEST.Transit = 1;
			
		else if ( Opcode == OP_TXT_REQ && ConInfo->TextState == TXT_STAT_PENDING_COMPLETE)
			LPdu->Bhs->TXT_REQUEST.Final = 1;
	}

	if ( Opcode == OP_LOGIN_REQ )
	{
		LPdu->Bhs->LOGIN_REQUEST.CSG = ConInfo->CSG;
		/*
		10.12.3
		The next stage value is only valid  when the T bit is 1; otherwise, it is reserved
		*/
		if ( LPdu->Bhs->LOGIN_REQUEST.Transit )
			LPdu->Bhs->LOGIN_REQUEST.NSG = ConInfo->NSG;
		
		REVERSE_BYTES_SHORT ( &LPdu->Bhs->LOGIN_REQUEST.CID , &ConInfo->CID);
		if (!IsLeadingCon(ConInfo))
			REVERSE_BYTES_SHORT ( &LPdu->Bhs->LOGIN_REQUEST.TSIH , &ConInfo->Session->TSIH);
	}
	/*
	DbgPrint("%s LPdu=0x%X CSG=0x%X NSG=0x%X C=0x%X T=0x%X\n" , 
	__FUNCTION__ , LPdu ,LPdu->Bhs->LOGIN_REQUEST.CSG,LPdu->Bhs->LOGIN_REQUEST.NSG,
	LPdu->Bhs->LOGIN_REQUEST.Continue,LPdu->Bhs->LOGIN_REQUEST.Transit);*/
	
	TpQueueOutPDU( ConInfo->ConCtx , LPdu );	
}

//
// TODO Called at the end of a successful nego
//

VOID PiCommitPending( PuCONINFO ConInfo )
{
	KEY_NAME		Key;
	UCHAR			Tmp;
	PKEY_VALUE		Kv;
	
	/*
	for ( Key = 0 ; Key < KEY_LastKey ; Key++)
	{
		Kv = KV( ConInfo , Key );
		
		if (!Kv)
			continue;
		
		if (PiIsKeyDeclarative (Key))
			continue;
			
		Tmp = Kv->Formal;
		Kv->Formal = Kv->Pending;
		Kv->Pending = Tmp;
	}*/
}

//
//
//

__inline ULONG PiHashTaskTag(ULONG Task)
{
	return (Task % PENDING_TASK_HASH_SIZE);
}

//
//
//

VOID PiQueuePendingTask(PuCONINFO ConInfo , PPDU Pdu)
{
	ULONG			Slot;
	FOUR_BYTE		ITT;
	
	REVERSE_BYTES ( &ITT , &Pdu->Bhs->GENERICBHS.InitiatorTaskTag );
	Slot = PiHashTaskTag ( ITT.AsULong );
	InsertTailList ( &ConInfo->PendingTasks[Slot] , &Pdu->PDUList );
}

//
//
//

VOID PiDequeuePendingTask(PuCONINFO ConInfo , PPDU Pdu)
{
	KIRQL	OldIrql;
	
	KeAcquireSpinLock( &ConInfo->PendingTasksLock , &OldIrql);
	RemoveHeadList ( Pdu->PDUList.Blink );	
	InitializeListHead ( &Pdu->PDUList );
	KeReleaseSpinLock( &ConInfo->PendingTasksLock , OldIrql);
}

//
//
//

PPDU PiFindPendingTask(ULONG TaskTag , PuCONINFO ConInfo , BOOLEAN Remove)
{
	FOUR_BYTE		ITT;
	PLIST_ENTRY		Entry , Tmp;
	PPDU			Pdu;
	KIRQL			OldIrql;
	
	KeAcquireSpinLock( &ConInfo->PendingTasksLock , &OldIrql);
	
	Entry = &ConInfo->PendingTasks[ PiHashTaskTag(TaskTag) ];

	Tmp = Entry->Flink;

	while ( Tmp != Entry )
	{
		Pdu = CONTAINING_RECORD (Tmp , PDU , PDUList );
		
		REVERSE_BYTES ( &ITT , &Pdu->Bhs->GENERICBHS.InitiatorTaskTag);
		if (ITT.AsULong == TaskTag )
		{
			if (Remove)
			{
				RemoveHeadList ( Pdu->PDUList.Blink );	
				InitializeListHead ( &Pdu->PDUList );
			}														
			KeReleaseSpinLock( &ConInfo->PendingTasksLock , OldIrql);
			return Pdu;
		}
			
		Tmp = Tmp->Flink;
	}
	KeReleaseSpinLock( &ConInfo->PendingTasksLock , OldIrql);
	return NULL;
}

//
// Called by TDI when PDU sending complete
// This function may run at DISPATCH_LEVEL
//

NTSTATUS PiCompletePDU (PDEVICE_OBJECT DevObj , PIRP Irp , PVOID Ctx)
{
	PuCONINFO	ConInfo;
	PPDU		Pdu;
	FOUR_BYTE	ExpectedDataTansferLength , DataSegmentLength , ITT;
	UCHAR		Opcode;
			
	Pdu = (PPDU)Ctx;
	
	ConInfo = Pdu->ConCtx->ConInfo;
	Opcode = Pdu->Bhs->GENERICBHS.Opcode;
	
	KeQueryTickCount ( &Pdu->SendingTime);
	
	if ( Irp->PendingReturned )
		IoMarkIrpPending( Irp );
			
	if ( Opcode == OP_DATA_OUT)	
		goto Out;	
	
	if (Opcode == OP_SCSI_CMD || 
		Opcode == OP_SCSI_TASK_REQ || 
		Opcode == OP_SNACK_REQ )		
		goto Queue;
	
	if ( Opcode == OP_NOP_OUT)
	{
		REVERSE_BYTES ( &ITT , &Pdu->Bhs->NOP_OUT.InitiatorTaskTag );
		
		if ( ITT.AsULong != 0xFFFFFFFF )
			// Echo Expected
			// Queue it in Pending Task Queue
			goto Queue;

		// Just Release it
		goto Release;
	}		
	
	if ( Opcode == OP_LOGIN_REQ || 
		Opcode == OP_LOGOUT_REQ || 
		Opcode == OP_TXT_REQ)		
		goto Release;
	
Out:		
	return STATUS_SUCCESS;	

Release:				

	TpQueuePduRelease( Pdu->ConCtx , Pdu );
	return STATUS_SUCCESS;

Queue:

	PiQueuePendingTask ( ConInfo , Pdu );
	return STATUS_SUCCESS;
}

//
// Release PDU at PASSIVE_LEVEL
//

VOID PiPduReleaser(IN PVOID Parameter)
{
	NTSTATUS	Status;
	PLIST_ENTRY	PduEnt;
	PPDU		Pdu;
	PSESSION 	Session = (PSESSION)Parameter;
	
	while (TRUE)
	{	
		Status = 
			KeWaitForSingleObject(&Session->PduReleaseEvent, Executive, KernelMode, FALSE, NULL );		
		KeClearEvent( &Session->PduReleaseEvent );
									 
		while ( PduEnt = 
			ExInterlockedRemoveHeadList ( &Session->PduRelease , &Session->PduReleaseLock ) )
		{
			Pdu = CONTAINING_RECORD ( PduEnt , PDU , PDUList );
			InitializeListHead ( &Pdu->PDUList);
			TpFreePDU( Pdu->ConCtx , Pdu , 0 );
		}
	}
}
