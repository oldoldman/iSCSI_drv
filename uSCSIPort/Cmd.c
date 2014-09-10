
#include "precomp.h"

PPDU PiAllocateDataOutPDU(	
PuCONINFO 	ConInfo , PUCHAR DataBuf , ULONG Offset, ULONG DataSize,PULONG DataSN, PPDU	RefCmd);

VOID PiDequeuePendingTask(PuCONINFO ConInfo , PPDU Pdu);

__inline VOID PiSendDataOut(PuCON_CTX Ctx , PPDU CmdOrR2T );

/*
	Process Immediate data and/or the first trunk of unsolicited data

 	Parms:
  		ConInfo : Connection
 		DataBuf : Read buffer / Write Buffer / RW Buffer
  	  WriteSize : 
   	   ReadSize :
   	   TaskAttr :
  	  Direction : SCSI_CMD_DIR_READ
  			      SCSI_CMD_DIR_WRITE
  			      SCSI_CMD_DIR_BOTH
  			      SCSI_CMD_DIR_NONE
 	Return:
  		Command PDU
*/
PPDU PiAllocateCmdPDU ( 
PuCONINFO ConInfo , PUCHAR 	DataBuf , ULONG WriteSize , ULONG ReadSize , UCHAR TaskAttr ,
UCHAR Direction)
{
	PPDU		Cmd , DataOut;
	PSESSION	Session;
	ULONG		FirstBurstLength;
	ULONG		MaxRecvDataSegmentLength ;
	ULONG		ExpectedDataTansferLength , DataLen , Remaining ;
	
	FOUR_BYTE	ITT;
		
	Session = ConInfo->Session;
	
	MaxRecvDataSegmentLength = KEY_TV(KV(ConInfo,KEY_MaxRecvDataSegmentLength)).Number;
	FirstBurstLength = KEY_FV(KV(ConInfo,KEY_FirstBurstLength)).Number;
		
	switch ( Direction )
	{
		// Read only
		case SCSI_CMD_DIR_READ:
			Cmd = TpAllocatePDU (ConInfo->ConCtx , OP_SCSI_CMD , 0 , 0 );
			Cmd->Bhs->SCSI_CMD.Read = 1;
			ExpectedDataTansferLength = ReadSize;			
			break;
		// Write only
		case SCSI_CMD_DIR_WRITE:
			Cmd = TpAllocatePDU (ConInfo->ConCtx , OP_SCSI_CMD , 0 , 0 );
			Cmd->Bhs->SCSI_CMD.Write = 1;
			ExpectedDataTansferLength = WriteSize;		
			break;
		// Read Write
		case SCSI_CMD_DIR_BOTH:
			Cmd = TpAllocatePDU (ConInfo->ConCtx , OP_SCSI_CMD , 8 , 0 );
			Cmd->Bhs->SCSI_CMD.Read = 1;
			Cmd->Bhs->SCSI_CMD.Write = 1;
			ExpectedDataTansferLength = WriteSize;
			break;
		// With no Read/Write
		case SCSI_CMD_DIR_NONE:
			Cmd = TpAllocatePDU (ConInfo->ConCtx , OP_SCSI_CMD , 0 , 0 );
			ExpectedDataTansferLength = 0;
			break;
	}
	//
	ITT.AsULong = InterlockedIncrement(&Session->TaskTag);
	REVERSE_BYTES ( &Cmd->Bhs->SCSI_CMD.InitiatorTaskTag , &ITT);

	Cmd->Bhs->SCSI_CMD.Attr = TaskAttr;
	
	//Write to / Read from this Buffer
	if (Direction != SCSI_CMD_DIR_NONE)
		Cmd->DataBuffer = DataBuf;
		
	/*
	For unidirectional operations, the Expected Data Transfer Length
   	field contains the number of bytes of data involved in this SCSI
   	operation.  For a unidirectional write operation (W flag set to 1 and
   	R flag set to 0), the initiator uses this field to specify the number
   	of bytes of data it expects to transfer for this operation.  For a
   	unidirectional read operation (W flag set to 0 and R flag set to 1),
   	the initiator uses this field to specify the number of bytes of data
   	it expects the target to transfer to the initiator.  It corresponds
   	to the SAM2 byte count.	
	*/

	REVERSE_BYTES ( &Cmd->Bhs->SCSI_CMD.ExpectedDataTansferLength , &ExpectedDataTansferLength);
	//
	if ( Direction == SCSI_CMD_DIR_WRITE || Direction == SCSI_CMD_DIR_BOTH )
	{
		if ( KEY_FV(KV(ConInfo , KEY_ImmediateData)).Bool )
		{
			// Immediate data
			DataLen = min ( FirstBurstLength , WriteSize );
			DataLen = min ( DataLen , MaxRecvDataSegmentLength );

			REVERSE_3BYTES ( &Cmd->Bhs->SCSI_CMD.DataSegmentLength , &DataLen );
			
			Cmd->Data = DataBuf;
		
			if ( ( DataLen < WriteSize) &&
			     ( MaxRecvDataSegmentLength < FirstBurstLength) &&
				!( KEY_FV(KV(ConInfo , KEY_InitialR2T)).Bool) )
			{	
				// More Data in Data-Out PDU
				Cmd->Bhs->SCSI_CMD.Final = 0;
				
				Remaining = WriteSize - DataLen;
				Remaining = min (Remaining , (FirstBurstLength - DataLen));
				
				DataOut = PiAllocateDataOutPDU ( ConInfo , 
												 DataBuf , 
												 DataLen , 
												 Remaining ,
												 &Cmd->DataSN,
												 NULL );

				InsertTailList ( &Cmd->DataOut , &DataOut->DataOut );						
			}
			else
				// No More Data
				Cmd->Bhs->SCSI_CMD.Final = 1;
		}
		else
		{
			if ( KEY_FV(KV(ConInfo , KEY_InitialR2T)).Bool )
				// No Immediate data  , No Unsolicited data
				// just wait for the R2T
				Cmd->Bhs->SCSI_CMD.Final = 1;
			else
			{
				// No Immediate data , But Unsolicited data
				Cmd->Bhs->SCSI_CMD.Final = 0;
				Remaining = min ( WriteSize , FirstBurstLength );
				
				DataOut = PiAllocateDataOutPDU ( ConInfo , 
												 DataBuf , 
												 0 , 
												 Remaining ,
												 &Cmd->DataSN,
												 Cmd );
												 
				InsertTailList ( &Cmd->DataOut , &DataOut->DataOut );
			}
		}
	}
	
	if ( Direction == SCSI_CMD_DIR_READ || Direction == SCSI_CMD_DIR_NONE )
	{
		/*
		Setting both the W and the F bit to 0 is an error
		
		***for read only command F bit MUST be set
		*/
		Cmd->Bhs->SCSI_CMD.Final = 1;
	}
	
	if ( Direction == SCSI_CMD_DIR_BOTH )
	{
		PAHS		Ahs;
		TWO_BYTE	AhsLen;
		
		Ahs = (PAHS)Cmd->Ahs;
		
		AhsLen.AsUShort = 0x5;		
		REVERSE_BYTES_SHORT ( &Ahs->AHSLength , &AhsLen );
		
		Ahs->AHSType = AHS_TYPE_BIDIR_READ_LENGTH;
		
		REVERSE_BYTES ( &Ahs->AHS_spec[1] , &ReadSize);
	}
	/*
	DbgPrint("%s Direction=0x%X ITT=0x%X Data=0x%p\n" , 
	__FUNCTION__ , Direction , ITT.AsULong , Cmd->Data);
		*/
	return Cmd;
}

//
//
//

BOOLEAN PiCheckCmdComplete( PPDU Cmd )
{
	KIRQL		OldIrql;
	BOOLEAN		CanComplete = FALSE;
	PLIST_ENTRY	DataInEnt,DataInEnt2;
	PPDU		DataIn;

	if ( !(Cmd->Flags & PDU_F_CMD_CAN_COMPLETE) )
		goto Out;
	//
	// Check if all transfered
	//		
	KeAcquireSpinLock( &Cmd->DataInOrR2TLock , &OldIrql);
	CanComplete = TRUE;
	DataInEnt = &Cmd->DataInOrR2T;
	DataInEnt2 = DataInEnt->Flink;
		
	while ( DataInEnt2 != DataInEnt )
	{
		DataIn = CONTAINING_RECORD ( DataInEnt2 , PDU , DataInOrR2T );
		if ( !(DataIn->Flags & PDU_F_DATA_IN_PROCESSED) )
		{
			CanComplete = FALSE;
			break;
		}
		DataInEnt2 = DataInEnt2->Flink;
	}

	KeReleaseSpinLock( &Cmd->DataInOrR2TLock ,OldIrql);

Out:
	return CanComplete;
}

/*
   Pending command completion worker
   
   Parms:
   		Parameter 	: Thread context
*/
VOID PiPendingCompleteCmdWorker (IN PVOID Parameter)
{
	NTSTATUS				Status;
	PWORKER_THREAD_CTX		Tctx;
	PSESSION				Session;
	PuCONINFO				ConInfo;
	PuCON_CTX				Ctx;
	PPDU					Resps , Cmd , LastDataIn;
	PLIST_ENTRY				CmdEnt , PduEnt;
	UCHAR					Response = 0, ScsiStatus = 0;
	PUCHAR					SenseData = NULL;
	FOUR_BYTE				ITT , BRC , RC;
	THREE_BYTE				DataSegmentLength;
	TWO_BYTE				SenseLength;
	
	Status 	= STATUS_SUCCESS;	
	Tctx 	= (PWORKER_THREAD_CTX)Parameter;
	Ctx 	= Tctx->Ctx;
	ConInfo = Ctx->ConInfo;
	Session = ConInfo->Session;
		
	SenseLength.AsUShort = 0;
	
	while (TRUE)
	{
		Status = 
		KeWaitForSingleObject ( &Ctx->PendingCompleteCmdEvent, Executive, KernelMode, FALSE, NULL );		
		KeClearEvent( &Ctx->PendingCompleteCmdEvent );
		
		while ( CmdEnt = ExInterlockedRemoveHeadList(&Ctx->PendingCompleteCmds, 
													 &Ctx->PendingCompleteCmdLock))
		{
			Cmd = CONTAINING_RECORD ( CmdEnt , PDU , PendingCmd);
			InitializeListHead ( &Cmd->PendingCmd );
		
			REVERSE_BYTES ( &ITT , &Cmd->Bhs->STAT_FIELDS.InitiatorTaskTag );
						
			if ( Cmd->Bhs->GENERICBHS.Opcode == OP_SCSI_CMD && Cmd->Bhs->SCSI_CMD.Read )
			{
				if ( !PiCheckCmdComplete(Cmd) )
				{
					//Can't complete for now , requeue it to tail
					TpQueuePendingCompleteCmd ( Cmd->ConCtx , Cmd );
					continue;
				}
			}
			
			if ( !IsListEmpty ( &Cmd->Resps ))
			{
				//	
				// Process Repsonse
				//
				PduEnt = RemoveHeadList ( &Cmd->Resps );
				Resps = CONTAINING_RECORD ( PduEnt , PDU , Resps );
				InitializeListHead ( &Resps->Resps );
			
				Response = Resps->Bhs->SCSI_RESPONSE.Response;
				ScsiStatus = Resps->Bhs->SCSI_RESPONSE.Status;
						
				KdPrintEx((DPFLTR_USCSI,DBG_PROTOCOL,
				"%s ITT 0x%X Resps 0x%p Response 0x%X Status 0x%X\n" , 
				__FUNCTION__ , 
				ITT.AsULong,
				Resps, 
				Response , 
				ScsiStatus));
			
				// How to Process these residual counts ?
				REVERSE_BYTES ( &BRC , &Resps->Bhs->SCSI_RESPONSE.BiReadResidualCount);
				REVERSE_BYTES ( &RC , &Resps->Bhs->SCSI_RESPONSE.ResidualCount);
				//
				// Sense data available?
				//
				REVERSE_3BYTES (&DataSegmentLength , &Resps->Bhs->SCSI_RESPONSE.DataSegmentLength);
			
				if ( DataSegmentLength.AsULong )
				{
					REVERSE_BYTES_SHORT ( &SenseLength, Resps->Data );
					if (SenseLength.AsUShort)
					{
						SenseData = ExAllocatePoolWithTag ( PagedPool , SenseLength.AsUShort , USCSI_TAG);
						RtlCopyMemory ( SenseData , Resps->Data + 2 , SenseLength.AsUShort);
					}			
				}
			
				TpQueuePduRelease( Ctx , Resps );
			}
			else
			{
				LastDataIn = CONTAINING_RECORD ( Cmd->DataInOrR2T.Blink , PDU , DataInOrR2T );
				ScsiStatus = LastDataIn->Bhs->SCSI_DATA_IN.Status;
			}			
			
			(*Session->CallBack.CompleteCmd)( ITT.AsULong , 
										  Session->CallBack.CompleteCtx , 
										  ScsiStatus , 
										  SenseData ,
										  SenseLength.AsUShort );
			if ( SenseData )
			{
				ExFreePoolWithTag ( SenseData , USCSI_TAG);
				SenseData = NULL;
			}
				
			PiDequeuePendingTask ( ConInfo , Cmd );
			TpQueuePduRelease( Ctx , Cmd );
		}
	}
	
	ExFreePoolWithTag (Tctx , USCSI_TAG);
}

/*
	Allocate a iSCSI command PDU and assembly it
	
	parms:
		ConInfo 	:
		Cdb			:
		CdbLength 	:
		DataBuffer 	:
		WriteSize	:
		ReadSize	:
		TaskAttr	:
		Dir			:
	
	return:
		iSCSI command PDU
*/

PPDU PtAssembleSCSICmd 
(PuCONINFO ConInfo, PUCHAR Cdb, ULONG CdbLength, PUCHAR DataBuffer, 
ULONG WriteSize, ULONG ReadSize, UCHAR TaskAttr, UCHAR Dir	)
{
	PPDU		Cmd;

	Cmd = PiAllocateCmdPDU ( ConInfo , DataBuffer , WriteSize , ReadSize , TaskAttr, Dir );			
	
	if( NULL != Cmd )
		RtlCopyMemory ( Cmd->Bhs->SCSI_CMD.Cdb , Cdb , min (16, CdbLength) );

	return Cmd;
}