
#include "precomp.h"

BOOLEAN TiVerifyDataDigest( IN PuCON_CTX Ctx, IN PPDU Pdu);
	
BOOLEAN TiVerifyHeaderDigest( IN PuCON_CTX Ctx, IN PPDU Pdu);
	
PPDU TiAllocatePDU( IN PuCON_CTX Ctx );
	
NTSTATUS PiCompletePDU (IN PDEVICE_OBJECT DevObj , IN PIRP Irp , IN PVOID Ctx);

//
// PDU Assembler
//

// Called at DISPATCH_LEVEL
// for Now PDU buffer is allocated from NonPagedPool

VOID TiReceive(
    IN PVOID  				TdiEventContext,
    IN CONNECTION_CONTEXT  	ConnectionContext,
    IN ULONG  				ReceiveFlags,
    IN ULONG 	 			BytesIndicated,
    IN ULONG  				BytesAvailable,
   OUT ULONG  				*BytesTaken,
    IN PVOID  				Tsdu,
   OUT PIRP  				*IoRequestPacket)
{
	NTSTATUS		Status;
	PUCHAR			PTsdu;
	PuCON_CTX		Ctx;
	PPDU			Pdu;
	ULONG			Remaining;
	ULONG			Copied;
	THREE_BYTE		DataSegmentLength;
	
	Status 	= STATUS_SUCCESS;	
	Ctx 	= (PuCON_CTX)ConnectionContext;
	PTsdu 	= (PUCHAR)Tsdu;
	/*
	KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,
	"%s ReceiveFlags 0x%X BytesIndicated 0x%X BytesAvailable 0x%X Tsdu 0x%p\n" , 
	__FUNCTION__ , 
	ReceiveFlags ,
	BytesIndicated ,
	BytesAvailable, 
	Tsdu));*/
		
	Remaining = BytesIndicated;

	while ( Remaining > 0)
	{
		if ( !Ctx->PendingPDU )
		{
			Pdu = TiAllocatePDU (Ctx);
			KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,"%s Process Pdu 0x%p\n" , __FUNCTION__,Pdu ));
		}
		else
			Pdu = Ctx->PendingPDU;
		
		//BHS	
		if (Pdu->Bytes < BHS_SIZE)
		{			
			Copied = min ( (BHS_SIZE - Pdu->Bytes) , Remaining);
			RtlCopyMemory ( (PUCHAR)Pdu->Bhs + Pdu->Bytes , PTsdu , Copied );
			
			PTsdu 		+= Copied;
			Remaining 	-= Copied;
			Pdu->Bytes 	+= Copied;
			*BytesTaken += Copied;
			
			if (Pdu->Bytes < BHS_SIZE)
				Ctx->PendingPDU = Pdu;	//Pending for BHS
			else 
			{
				// BHS Completed , allocate buffer for AHS or DATA
				if (!TiVerifyHeaderDigest( Ctx , Pdu ) )
				{
					// Close connection and return
				}
				/*
				TotalAHSLength:
				
				Total length of all AHS header segments in units of four byte words
				including padding, if any. p115
				*/
				Pdu->BufferSize = Pdu->Bhs->GENERICBHS.TotalAHSLength << 2;
							
				/*
				DataSegmentLength:
				
				This is the data segment payload length in bytes (excluding padding).
   				The DataSegmentLength MUST be 0 whenever the PDU has no data segment. p115
				
				The (optional) Data Segment contains PDU associated data.  Its
   				payload effective length is provided in the BHS field -
   				DataSegmentLength.  The Data Segment is also padded to an integer
   				number of 4 byte words. p118
				*/
				REVERSE_3BYTES ( &DataSegmentLength , &Pdu->Bhs->GENERICBHS.DataSegmentLength);
				Pdu->BufferSize += DataSegmentLength.AsULong;
				
				if ( Pdu->BufferSize &0x3 )
				{
					Pdu->BufferSize &= ~0x3;
					Pdu->BufferSize += 0x4;
				}	
				
				if ( Pdu->BufferSize )
				{
					Pdu->Buffer = ExAllocatePoolWithTag ( NonPagedPool, Pdu->BufferSize, USCSI_TAG );
					Ctx->PendingPDU = Pdu;	//Pending for AHS and DATA
				}
				else
					// PDU Completed
					goto QueuePdu;
			}		
		}
		//AHS and Data
		else 
		{						
			Copied = min ( (Pdu->BufferSize + BHS_SIZE - Pdu->Bytes) , Remaining );
			RtlCopyMemory ( Pdu->Buffer + ( Pdu->Bytes - BHS_SIZE ) , PTsdu , Copied);
				
			PTsdu += Copied;
			Remaining -= Copied;
			Pdu->Bytes += Copied;		
			*BytesTaken += Copied;
			
			if (Pdu->Bytes < Pdu->BufferSize + BHS_SIZE )
			{
				Ctx->PendingPDU = Pdu;		//Pending for AHS and DATA
				continue;
			}				
			//
			// Complete a PDU , queue it
			//
	QueuePdu:
	
			if ( Pdu->Bhs->GENERICBHS.TotalAHSLength )
				Pdu->Ahs = Pdu->Buffer;

			REVERSE_3BYTES ( &DataSegmentLength , &Pdu->Bhs->GENERICBHS.DataSegmentLength);
			if ( DataSegmentLength.AsULong )
				Pdu->Data = Pdu->Buffer + Pdu->Bhs->GENERICBHS.TotalAHSLength;
			
			Ctx->PendingPDU = NULL;					
			//
			KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,
			"%s Pdu 0x%X Opcode 0x%X BufferSize 0x%X\n" ,
			__FUNCTION__, 
			Pdu,
			Pdu->Bhs->GENERICBHS.Opcode,
			Pdu->BufferSize));
				
			KeQuerySystemTime ( &Pdu->ReceivingTime );
			
			if ( TiVerifyDataDigest ( Ctx , Pdu) )
				TpQueueInPDU ( Ctx , Pdu );
		}
	}
}

//
//
//

UCHAR TiDataPadding[4];
ULONG TiAllocateMdl (IN PPDU Pdu , IN PIRP Irp)
{
	PMDL			Mdl;
	ULONG			SendLen = 0 , DataSize = 0;
	UCHAR			PaddingLen;
	THREE_BYTE		DataSegmentLength;
	
	Mdl = IoAllocateMdl ( Pdu->Bhs, BHS_SIZE, FALSE , FALSE , Irp );
	__try
	{
		MmProbeAndLockPages ( Mdl , KernelMode , IoReadAccess );
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("%s Exception 0x%X" , 	__FUNCTION__ , GetExceptionCode() );
	} 
	
	SendLen += BHS_SIZE;
				
	if ( Pdu->Ahs )
	{
		Mdl = IoAllocateMdl ( Pdu->Ahs , 
							  Pdu->Bhs->GENERICBHS.TotalAHSLength << 2, 
							  TRUE , 
							  FALSE , 
							  Irp );
		__try
		{
			MmProbeAndLockPages ( Mdl , KernelMode , IoReadAccess );
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("%s Exception 0x%X" , __FUNCTION__ , GetExceptionCode());
		}
		
		SendLen += Pdu->Bhs->GENERICBHS.TotalAHSLength << 2;
	}
	
	if ( Pdu->Data )
	{
		REVERSE_3BYTES ( &DataSegmentLength , &Pdu->Bhs->GENERICBHS.DataSegmentLength );
		
		Mdl = IoAllocateMdl ( Pdu->Data , 
							  DataSegmentLength.AsULong , 
							  TRUE , 
							  FALSE , 
							  Irp );
		__try
		{
			MmProbeAndLockPages ( Mdl , KernelMode , IoReadAccess );
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("%s Exception 0x%X" , 	__FUNCTION__ , 	GetExceptionCode());
		}
		//
		// Padding if needed
		//
		if ( PaddingLen = DataSegmentLength.AsULong & 0x3 )
		{
			DataSegmentLength.AsULong &= ~0x3;
			DataSegmentLength.AsULong += 0x4;
			
			Mdl = IoAllocateMdl ( TiDataPadding , 
							   	  4 - PaddingLen , 
							  	  TRUE , 
							      FALSE , 
							      Irp );
			__try
			{
				MmProbeAndLockPages ( Mdl , KernelMode , IoReadAccess );
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				DbgPrint("%s Exception 0x%X" , __FUNCTION__ , GetExceptionCode() );
			}
		}	
		
		SendLen += DataSegmentLength.AsULong;
	}

	return SendLen;	
}

//
//
//

VOID TiCheckCmdWindow (IN PSESSION Session)
{
	KIRQL	OldIrql;
	
	KeAcquireSpinLock ( &Session->WindowLock , &OldIrql );
	
	if ( Session->CmdSN > Session->MaxCmdSN)
	{
		KeReleaseSpinLock ( &Session->WindowLock , OldIrql );
		KeWaitForSingleObject ( &Session->WindowEvent , Executive , KernelMode , FALSE , NULL );
		KeClearEvent ( &Session->WindowEvent );
	}
	
	KeReleaseSpinLock ( &Session->WindowLock , OldIrql );	
}

//
// Sender for outgoing PDUs
//

VOID TiSender(IN PVOID Parameter)
{
	NTSTATUS				Status;
	PWORKER_THREAD_CTX		Tctx;
	PuCONINFO				ConInfo;
	PuCON_CTX				Ctx;
	PPDU					Pdu;
	PIRP					Irp;
	PDEVICE_OBJECT			DevObj;
	PLIST_ENTRY				Entry;
	LONG					DataSize = 0 , SendLen = 0;
	ULONG					CmdSN , ExpStatSN;
	FOUR_BYTE				ITT;	
	KEVENT					Event;
	IO_STATUS_BLOCK			IoStatus;
	
	Status 	= STATUS_SUCCESS;	
	Tctx 	= (PWORKER_THREAD_CTX)Parameter;
	Ctx 	= Tctx->Ctx;
	ConInfo = Ctx->ConInfo;
	
	DevObj = IoGetRelatedDeviceObject (ConInfo->ConEpObj);
	
	while ( TRUE )
	{
		Status = KeWaitForSingleObject ( &Ctx->OutEvent , 
									 Executive , 
									 KernelMode , 
									 FALSE , 
									 NULL );
		
		KeClearEvent ( &Ctx->OutEvent );
		
		while ( Entry = ExInterlockedRemoveHeadList ( &Ctx->OutPDU , &Ctx->OutLock ) )
		{
			Pdu = CONTAINING_RECORD ( Entry , PDU , PDUList );
			InitializeListHead ( &Pdu->PDUList );
			
			KeInitializeEvent ( &Event , NotificationEvent , FALSE);
		
			Irp = TdiBuildInternalDeviceControlIrp( TDI_SEND, DevObj, NULL, &Event, &IoStatus );
			if (!Irp )
			{
				DbgPrint("%s Fail to allocate Irp\n" ,__FUNCTION__ );
				Ctx->FailedIrpAllocation++;
				//Retry later
				TpQueueOutPDU( Ctx , Pdu );
				continue;
			}
			// CmdSN
			if (Pdu->Bhs->GENERICBHS.Opcode != OP_DATA_OUT &&
				Pdu->Bhs->GENERICBHS.Opcode != OP_SNACK_REQ )
			{
				TiCheckCmdWindow (ConInfo->Session);
							
				if (Pdu->Bhs->GENERICBHS.Immediate)
					CmdSN = ConInfo->Session->CmdSN;
				else
					CmdSN = InterlockedIncrement ( (PLONG)&ConInfo->Session->CmdSN );
			
				REVERSE_BYTES ( &Pdu->Bhs->STAT_FIELDS.CmdSN , &CmdSN);
			}
			// State Sync
			ExpStatSN = ConInfo->ExpStatSN;
			REVERSE_BYTES ( &Pdu->Bhs->STAT_FIELDS.ExpStatSN ,&ExpStatSN);	
			
			SendLen = TiAllocateMdl ( Pdu , Irp );
			
			REVERSE_BYTES ( &ITT , &Pdu->Bhs->STAT_FIELDS.InitiatorTaskTag );			
			/*
			DbgPrint(
			"%s Pdu=0x%X ITT=0x%X Opcode=0x%X ScsiOpcode=0x%X Irp=0x%p SendLen=0x%X Bhs=0x%p Ahs=0x%p Data=0x%p CmdSN=0x%X\n" , 
			__FUNCTION__,
			Pdu,
			ITT.AsULong,
			Pdu->Bhs->GENERICBHS.Opcode,
			Pdu->Bhs->GENERICBHS.Opcode == OP_SCSI_CMD?Pdu->Bhs->SCSI_CMD.Cdb[0]:0xFF,
			Irp , 
			SendLen , 
			Pdu->Bhs,
			Pdu->Ahs,
			Pdu->Data,
			CmdSN );*/
			
			TdiBuildSend(
				Irp, DevObj, ConInfo->ConEpObj, PiCompletePDU, Pdu, Irp->MdlAddress, 0, SendLen);
							
			Status = IoCallDriver ( DevObj , Irp );						
			if ( STATUS_PENDING == Status )
				KeWaitForSingleObject ( &Event , Executive , KernelMode , FALSE , NULL );
		}	
	}
	
	KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,"Sender(0x%x) Exited\n" , Tctx->No ));
	
	ExFreePoolWithTag ( Tctx , USCSI_TAG );

}

PSN TiAllocateSN ( IN PuCONINFO ConInfo	);
	
VOID TiFreeSN ( IN PuCONINFO ConInfo , IN PSN StatSN);


__inline BOOLEAN TiUpdateStatSN(IN PuCONINFO ConInfo, IN ULONG Num )
{
	PSESSION 				Session;
	PSINGLE_LIST_ENTRY		StatSNEnt , StatSNEnt2 = NULL;
	PSN						StatSN;
	BOOLEAN					ResetTimer = FALSE;
	
	if ( !ConInfo->ExpStatSNInited )
	{
		ConInfo->ExpStatSN = Num + 1;
		ConInfo->ExpStatSNInited = TRUE;
		ResetTimer = TRUE;
	}
	// Exactly what we want
	else if ( ConInfo->ExpStatSN == Num )
	{
		// Expect next one
		ConInfo->ExpStatSN++;		
		// Maybe the others can be acked		
		while ( StatSNEnt = ConInfo->StatSN.Next )
		{	
			StatSN = CONTAINING_RECORD ( StatSNEnt , SN , Next );	
			if ( StatSN->SN  == ConInfo->ExpStatSN )
			{
				ConInfo->ExpStatSN++;			
				PopEntryList( &ConInfo->StatSN );
				TiFreeSN ( ConInfo , StatSN);	
				ResetTimer = TRUE;	
				continue;		
			}
			break;
		}	
	}	
	// Out of order
	else if ( ConInfo->ExpStatSN < Num )
	{	
		//	
		// Insert , sort by StatSN
		//	
		StatSNEnt = &ConInfo->StatSN;
						
		while ( StatSNEnt = StatSNEnt->Next )
		{
			StatSN = CONTAINING_RECORD ( StatSNEnt , SN , Next );
			
			StatSNEnt2 = StatSNEnt;
			
			if ( StatSN->SN < Num )
				break;
			else if ( StatSN->SN == Num )
			{
				// Duplicated Stat
				// Maybe Target timeouted and auto retransmit it
				// rarely happen
				ConInfo->DupStatSN++;
				return FALSE;
			}
		}
		
		if (!StatSNEnt2)
			StatSNEnt2 = &ConInfo->StatSN;
			
		StatSN = TiAllocateSN ( ConInfo );
		StatSN->SN = Num;
		
		PushEntryList ( StatSNEnt2 ,  &StatSN->Next);			
	}
	
	else if ( ConInfo->ExpStatSN > Num )
	{
		// This is a retransmit of R2T
	}
	
	if (ResetTimer)
	{
		//TODO
	}
	
	return TRUE;
}


__inline VOID TiUpdateCmdWindow ( PuCONINFO ConInfo , ULONG ExpCmdSN , ULONG MaxCmdSN)
{
	KIRQL			OldIrql;
	PSESSION 		Session;

	Session = ConInfo->Session;

	KeAcquireSpinLock (&Session->WindowLock , &OldIrql );	
	
	if ( ExpCmdSN > Session->ExpCmdSN )
		Session->ExpCmdSN = ExpCmdSN;
		
	if ( MaxCmdSN > Session->MaxCmdSN)
		Session->MaxCmdSN = MaxCmdSN ;	
		
	KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,
	"%s ExpCmdSN 0x%X MaxCmdSN 0x%X Window %d\n" , 
	__FUNCTION__ , 
	Session->ExpCmdSN,
	Session->MaxCmdSN,
	Session->MaxCmdSN - Session->ExpCmdSN ));
	
	KeReleaseSpinLock (&Session->WindowLock , OldIrql );
	//
	// TiCheckCmdWindow wait on WindowEvent
	//
	if ( Session->MaxCmdSN > Session->CmdSN)
		KeSetEvent ( &Session->WindowEvent , IO_NO_INCREMENT , FALSE);	
}

//
// Dispatcher for incoming PDUs
//

VOID TiDispatcher( IN PVOID Parameter )
{
	NTSTATUS			Status;
	PSESSION			Session;
	PWORKER_THREAD_CTX	Tctx;
	PuCONINFO			ConInfo;
	PuCON_CTX			Ctx;
	PLIST_ENTRY			PduEnt;
	PPDU				Pdu;
	FOUR_BYTE			StatSN , ExpCmdSN , MaxCmdSN;
	
	Tctx 	= (PWORKER_THREAD_CTX)Parameter;
	Ctx 	= Tctx->Ctx;
	ConInfo = Ctx->ConInfo;
	Session = ConInfo->Session;
	
	while ( TRUE )
	{		
		Status = KeWaitForSingleObject ( &Ctx->DispatchEvent , 
									 Executive , 
									 KernelMode , 
									 FALSE , 
									 NULL );
		
		KeClearEvent( &Ctx->DispatchEvent );
									 
		while ( PduEnt = ExInterlockedRemoveHeadList ( &Ctx->InPDU , &Ctx->InLock ) )
		{
			Pdu = (PPDU)CONTAINING_RECORD ( PduEnt , PDU , PDUList );
			InitializeListHead ( &Pdu->PDUList );
			// Command Window
			REVERSE_BYTES ( &ExpCmdSN , &Pdu->Bhs->STAT_FIELDS.ExpCmdSN);
			REVERSE_BYTES ( &MaxCmdSN , &Pdu->Bhs->STAT_FIELDS.MaxCmdSN);				
			TiUpdateCmdWindow (ConInfo , ExpCmdSN.AsULong ,  MaxCmdSN.AsULong);
			/*
			The presence of status
   			(and of a residual count) is signaled though the S flag bit.
   			p137	
			*/
			if ( Pdu->Bhs->GENERICBHS.Opcode == OP_DATA_IN && Pdu->Bhs->SCSI_DATA_IN.S || 
				 Pdu->Bhs->GENERICBHS.Opcode != OP_DATA_IN)
			{
				REVERSE_BYTES ( &StatSN , &Pdu->Bhs->STAT_FIELDS.StatSN);
				TiUpdateStatSN ( ConInfo , StatSN.AsULong );
			}
		
			//DbgPrint("%s Opcode=0x%X\n" , __FUNCTION__ , Pdu->Bhs->GENERICBHS.Opcode );
			switch ( Pdu->Bhs->GENERICBHS.Opcode )
			{
				case OP_SCSI_REP:
					PtProcessResponse ( Pdu , ConInfo );
					break;
				
				case OP_DATA_IN:
					PtProcessDataIn ( Pdu , ConInfo );
					break;
			
				case OP_R2T:
					PtProcessR2T ( Pdu , ConInfo );
					break;
				
				case OP_SCSI_TASK_REP:
					PtProcessTask ( Pdu , ConInfo );
					break;
			
				case OP_TXT_REP:
					PtProcessText ( Pdu , ConInfo);
					break;
			
				case OP_ASYNC_MSG:
					PtProcessAsyncMsg (Pdu , ConInfo);
					break;
												
				case OP_LOGIN_REP:
					PtProcessLogin( Pdu , ConInfo);
					break;
			
				case OP_LOGOUT_REP:
					PtProcessLogout ( Pdu , ConInfo );
					break;
			
				case OP_REJECT:
					PtProcessReject ( Pdu , ConInfo );
					break;
			
				case OP_NOP_IN:
					PtProcessNopIn ( Pdu , ConInfo );
					break;
			
				default:
					//PtProcessError ( Pdu , ConInfo );
					break;
			}
		}
	}
	
	KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,"Dispatcher(0x%x) Exited\n" , Tctx->No));
	ExFreePoolWithTag( Tctx , USCSI_TAG);
	//Ctx->WorkItemFlags |= CONCTX_INWORKER_SAFE;
}