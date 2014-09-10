#include "precomp.h"

PPDU PiFindPendingTask(ULONG TaskTag , PuCONINFO ConInfo , BOOLEAN Remove);

PPDU PiQueueDataInOrR2T(PuCONINFO ConInfo , PPDU Task , PPDU DataIn);

//
//
//

BOOLEAN PiCheckDataInHole ( PPDU Cmd )
{
	PLIST_ENTRY		DataOrR2TEnt , DataOrR2TEnt2 ,DataOrR2TEnt3 , Last2ndEnt;
	PPDU			DataOrR2T2 , DataOrR2T3;
	FOUR_BYTE		DataSN2 , DataSN3;
	ULONG			Gap = 0;
	BOOLEAN			Hole = FALSE ;
		
	DataOrR2TEnt = &Cmd->DataInOrR2T;
	
	return FALSE;	//Temp for test purpose
	
	if ( !Cmd->ExpDataSN )
		return 	Hole;
		
	if ( DataOrR2TEnt->Flink ==  DataOrR2TEnt )
	{
		// Zero		
		while ( Gap < Cmd->ExpDataSN )
		{
			Hole = TRUE;
			Cmd->Flags |= PDU_F_DATA_IN_RETRANSMIT;
			Gap++;
		}
	}
	
	else if ( DataOrR2TEnt->Flink->Flink == DataOrR2TEnt )
	{
		// Just one	
		DataOrR2TEnt2 = DataOrR2TEnt->Flink;
		DataOrR2T2 = CONTAINING_RECORD ( DataOrR2TEnt2 , PDU , DataInOrR2T);		
		REVERSE_BYTES ( &DataSN2 , &DataOrR2T2->Bhs->STAT_FIELDS.DataSN );
		
		if ( Cmd->ExpDataSN )
		{
			Hole = TRUE;
			Cmd->Flags |= PDU_F_DATA_IN_RETRANSMIT;
			while ( Gap < DataSN2.AsULong )
			{
				//
				Gap++;
			}
		
			Gap = DataSN2.AsULong + 1;		
			while ( Gap <= Cmd->ExpDataSN )
			{
				//
				Gap++;
			}
		}
	}
	
	else
	{
		// At least two
		Last2ndEnt = DataOrR2TEnt->Blink->Blink;
		DataOrR2TEnt2 = DataOrR2TEnt->Flink;
		DataOrR2TEnt3 = DataOrR2TEnt2->Flink;
		
		while (  TRUE )
		{
			DataOrR2T2 = CONTAINING_RECORD ( DataOrR2TEnt2 , PDU , DataInOrR2T);				
			DataOrR2T3 = CONTAINING_RECORD ( DataOrR2TEnt3 , PDU , DataInOrR2T);
		
			REVERSE_BYTES ( &DataSN2 , &DataOrR2T2->Bhs->STAT_FIELDS.DataSN);
			REVERSE_BYTES ( &DataSN3 , &DataOrR2T3->Bhs->STAT_FIELDS.DataSN);
			//
			// First DataSN MUST Start with 0
			//
			if ( DataOrR2TEnt2->Blink == DataOrR2TEnt && DataSN2.AsULong )
			{
				Hole = TRUE;
				Cmd->Flags |= PDU_F_DATA_IN_RETRANSMIT;
				while ( Gap < DataSN2.AsULong )
				{
					// Request the missing Data-In(s)
					Gap++;
				}
			}
			
			if ( DataSN2.AsULong + 1 != DataSN3.AsULong )
			{
				Hole = TRUE;
				Cmd->Flags |= PDU_F_DATA_IN_RETRANSMIT;
								
				Gap = DataSN2.AsULong + 1;
				while ( Gap < DataSN3.AsULong )
				{
					// Request the missing Data-In(s)
					Gap++;
				}
			}
			
			if ( DataOrR2TEnt2 == Last2ndEnt && DataSN3.AsULong != Cmd->ExpDataSN )
			{
				Hole = TRUE;
				Cmd->Flags |= PDU_F_DATA_IN_RETRANSMIT;
				Gap = DataSN3.AsULong + 1;
				while ( Gap < Cmd->ExpDataSN )
				{
					//
					Gap++;
				}
				break;
			}	
			else
			{
				DataOrR2TEnt2 = DataOrR2TEnt2->Flink;
				DataOrR2TEnt3 = DataOrR2TEnt2->Flink;
			}
		}
	}
	
	return Hole;
}

/*
ConCtx         |-----------------|---------|--> Cmd1
 |             |                 |         |
 |_R2Ts    - R2T1.2 - R2T2.2 - R2T1.4 - R2T1.5 - R2T2.3 
 |                      |                          |
 |                      |--------------------------|-->Cmd2
 |
 |             |--------|-----------------|--> Cmd1
 |             |        |                 |  
 |_DataIns - Di1.1  - Di1.3  - Di2.1  - Di1.6  - Di2.4
                                |                  |
                                |------------------|-->Cmd2

R2T / Data-In link in 2 queues
 1. R2Ts or DataIns queue of connection by PDUList
 2. SCSI Cmd's DataInOrR2T queue by DataInOrR2T

R2Ts queue is served by R2TWorker
DataIns queue is served by DataInWorker

*/
/*
  1. Insert Data-in PDU into a Per-Cmd List , sorted by DataSN
  2. Check PDU Final bit , if set , trigger Data-in PDUs process
*/
VOID PtProcessDataIn (PPDU Pdu, PuCONINFO ConInfo)
{
	PPDU			Cmd;
	PPDU			DataAck;
	PuCON_CTX		Ctx;
	FOUR_BYTE		DataSN , DataSize , Offset , ITT;

	Ctx = ConInfo->ConCtx;

	REVERSE_BYTES ( &ITT , &Pdu->Bhs->SCSI_DATA_IN.InitiatorTaskTag );

	Cmd = PiFindPendingTask ( ITT.AsULong , ConInfo , FALSE);		
	if ( !Cmd )	
	{
		DbgPrint( "No Matching Command for DataIn %x (ITT 0x%x)\n", Pdu , ITT.AsULong);
		TpQueuePduRelease( Ctx , Pdu );
		goto Out;
	}
	
	Pdu->Cmd = Cmd;
	
	KdPrintEx((DPFLTR_USCSI , DBG_PROTOCOL ,
	"%s ITT 0x%X Pdu 0x%p FSUO %d%d%d%d\n" , 
	__FUNCTION__ , 
	ITT.AsULong ,
	Pdu,
	Pdu->Bhs->SCSI_DATA_IN.Final,
	Pdu->Bhs->SCSI_DATA_IN.S,
	Pdu->Bhs->SCSI_DATA_IN.U,
	Pdu->Bhs->SCSI_DATA_IN.O ));
	
	// Pdu maybe dropped by PiQueueDataInOrR2T
	// in which case PiQueueDataInOrR2T return NULL
	// else return the orignal Pdu
	
	Pdu = PiQueueDataInOrR2T ( ConInfo , Cmd , Pdu );
	
	if ( !Pdu )
		goto Out;
			
	//
	// Queue to Copy data
	//
	TpQueueDataInPDU ( Ctx , Pdu );
		
	if (KEY_FV(KV(ConInfo,KEY_ErrorRecoveryLevel)).Number >0 && 
		Pdu->Bhs->SCSI_DATA_IN.Ack)
	{
		/*
		The initiator MUST ignore the A bit set to 1 for sessions with
 		ErrorRecoveryLevel=0.
		*/
		// Allocate a DataAck
		DataAck = TpAllocatePDU ( Ctx , OP_SNACK_REQ , 0 , 0 );
		RtlCopyMemory ( &DataAck->Bhs->SNACK.TargetTransferTag,
						&Pdu->Bhs->SCSI_DATA_IN.TargetTransferTag,
						4);
		//...
		TpQueueOutPDU (Ctx , DataAck);
	}	
		
	if ( Pdu->Bhs->SCSI_DATA_IN.Final)
	{
		// Trigger Data Sequence checking
		/*
			Status can accompany the last Data-In PDU if the command did not end
   			with an exception (i.e., the status is "good status" - GOOD,
   			CONDITION MET or INTERMEDIATE CONDITION MET).  The presence of status
   			(and of a residual count) is signaled though the S flag bit.
		*/
		if ( Pdu->Bhs->SCSI_DATA_IN.S )
		{
			REVERSE_BYTES ( &DataSN , &Pdu->Bhs->SCSI_DATA_IN.DataSN);
			Cmd->ExpDataSN = DataSN.AsULong;
			
			if ( !PiCheckDataInHole ( Cmd ) )
			{
				//DbgPrint("CompleteCmd=0x%X\n",Cmd);				
				Cmd->Flags |= PDU_F_CMD_CAN_COMPLETE;
				TpQueuePendingCompleteCmd ( Ctx , Cmd );
			}
		}
	}	
	else if ( (Cmd->Flags & PDU_F_DATA_IN_RETRANSMIT) && !PiCheckDataInHole ( Cmd ) )
	{
		KdPrintEx((DPFLTR_USCSI , DBG_PROTOCOL ,"%s Debug\n" , __FUNCTION__ ));
		TpQueuePendingCompleteCmd ( ConInfo->ConCtx , Cmd );
	}			
	
Out:
	
	return;
}
/*
  Insert Data-in / R2T PDU sorted by DataSN
Parms  
	Cmd 		: to which DataInOrR2T belong
	DataInOrR2T : Data-in / R2T 
Return
  	NULL , if this is a duplicated Data-In
  	DataIn , the original DataIn
*/
PPDU PiQueueDataInOrR2T(PuCONINFO ConInfo, PPDU Cmd, PPDU DataInOrR2T)
{
	KIRQL 			OldIrql;
	PLIST_ENTRY		DataInEnt , DataInEnt2;
	FOUR_BYTE		Num , Num2;
	PPDU			DataIn2;
	
	KeAcquireSpinLock( &Cmd->DataInOrR2TLock, &OldIrql);
	DataInEnt = &Cmd->DataInOrR2T;
	REVERSE_BYTES ( &Num , &DataInOrR2T->Bhs->STAT_FIELDS.DataSN);
	
	DataInEnt2 = DataInEnt->Flink;
						
	while ( DataInEnt2 != DataInEnt )
	{
		DataIn2 = CONTAINING_RECORD ( DataInEnt2 , PDU , DataInOrR2T );
			
		REVERSE_BYTES ( &Num2 , &DataIn2->Bhs->STAT_FIELDS.DataSN );
			
		if ( Num2.AsULong < Num.AsULong )
			break;
		else if ( Num2.AsULong == Num.AsULong)
		{									
			KeReleaseSpinLock( &Cmd->DataInOrR2TLock, OldIrql);
			// Duplicated Data-In
			// Drop it?
			DbgPrint("Duplicated DataIn=0x%X Droped\n",DataInOrR2T);
			Cmd->DupDataInCount++;
			TpQueuePduRelease( ConInfo->ConCtx , DataInOrR2T );
			return NULL;
		}
		DataInEnt2 = DataInEnt2->Flink;
	}
		
	DataInEnt = &DataInOrR2T->DataInOrR2T;
	
	InsertHeadList ( DataInEnt2 , DataInEnt);
	KeReleaseSpinLock( &Cmd->DataInOrR2TLock, OldIrql);
	
	return DataInOrR2T;		
}

//
//
//

VOID PiDataInWorker(IN PVOID Parameter)
{
	NTSTATUS				Status;
	PWORKER_THREAD_CTX		Tctx;
	PSESSION				Session;
	PuCONINFO				ConInfo;
	PuCON_CTX				Ctx;
	PLIST_ENTRY				DataInEnt , DataInEnt2;
	PPDU					DataIn , Cmd;
	ULONG					BufferOffset;
	THREE_BYTE				DataSegmentLength;
	FOUR_BYTE				ITT;
	BOOLEAN					AllTransfered;
	
	Status 	= STATUS_SUCCESS;
	Tctx 	= (PWORKER_THREAD_CTX)Parameter;
	Ctx 	= Tctx->Ctx;
	ConInfo = Ctx->ConInfo;
	Session = ConInfo->Session;
	
	while (TRUE)
	{	
		Status = 
			KeWaitForSingleObject (&Ctx->DataInEvent, Executive, KernelMode, FALSE, NULL);		
		KeClearEvent( &Ctx->DataInEvent );
									 
		while ( DataInEnt = ExInterlockedRemoveHeadList ( &Ctx->DataIns , &Ctx->DataInLock ) )
		{
			DataIn = CONTAINING_RECORD ( DataInEnt , PDU , PDUList );
			InitializeListHead ( &DataIn->PDUList);
			
			Cmd = DataIn->Cmd;
			/*
			{
				LARGE_INTEGER Tick;
				KeQueryTickCount( &Tick);
			DbgPrint("%s Time=0x%X-%X Cmd=0x%X Pdu=0x%X Bhs=0x%X\n" , 
			__FUNCTION__ , Tick.HighPart,Tick.LowPart , Cmd , DataIn,DataIn->Bhs);
			}*/
			REVERSE_BYTES  ( &ITT , &DataIn->Bhs->SCSI_DATA_IN.InitiatorTaskTag);
			REVERSE_3BYTES ( &DataSegmentLength , &DataIn->Bhs->SCSI_DATA_IN.DataSegmentLength);
			REVERSE_BYTES  ( &BufferOffset , &DataIn->Bhs->SCSI_DATA_IN.BufferOffset );
			/*
			DbgPrint(
			"%s Cmd=0x%X ITT=0x%X DataBuffer=0x%p Offset=0x%X Length=0x%X Data=0x%X S=0x%X Final=0x%X\n",
			__FUNCTION__,
			Cmd,
			ITT.AsULong,
			Cmd->DataBuffer,
			BufferOffset,
			DataSegmentLength.AsULong,
			DataIn->Data,
			DataIn->Bhs->SCSI_DATA_IN.S,
			DataIn->Bhs->SCSI_DATA_IN.Final);*/
			// Copy Data			
			if (Cmd->DataBuffer)			
				RtlCopyMemory( Cmd->DataBuffer + BufferOffset, 
							   DataIn->Data, 
							   DataSegmentLength.AsULong );
							   
			DataIn->Flags |= PDU_F_DATA_IN_PROCESSED;
		}
	}
	
	ExFreePoolWithTag (Tctx , USCSI_TAG);
}

//
// DataBuf : Data Buffer
// Offset  : Offset
// DataSize: Data size count from Offset
//           for Unsolicited data , DataSize range is (0 , FirstBurstLength]
//           for Solicited data , DataSize range is (0 , MaxBurstLength]
/*
   ----------
   /|\
    |
    |
 DataSize
    |
    |
   \|/
   ---------- <- Offset
   /|\
    |
    |
   \|/
   ---------- <- DataBuf

*/

PPDU PiAllocateDataOutPDU(
PuCONINFO ConInfo, PUCHAR DataBuf, ULONG Offset, ULONG DataSize, PULONG DataSN, PPDU RefCmd)
{
	ULONG			DataLen , MaxRecvDataSegmentLength;
	PPDU			DataOut = NULL , Tmp ;
	PLIST_ENTRY		Ent;

	MaxRecvDataSegmentLength = KEY_TV(KV(ConInfo,KEY_MaxRecvDataSegmentLength)).Number;
	
	do
	{
		DataLen = min (DataSize , MaxRecvDataSegmentLength);
					
		Tmp = TpAllocatePDU ( ConInfo->ConCtx , OP_DATA_OUT , 0 , 0 );
		
		if ( !Tmp )
			goto ErrorOut;			
		else if ( DataOut )
			InsertTailList ( &DataOut->DataOut , &Tmp->DataOut );		
		else
			DataOut = Tmp;
		
		Tmp->Bhs->SCSI_DATA_OUT.Final = 0;
		
		REVERSE_3BYTES ( &Tmp->Bhs->SCSI_DATA_OUT.DataSegmentLength , &DataLen );							
		REVERSE_BYTES  ( &Tmp->Bhs->SCSI_DATA_OUT.BufferOffset , &Offset);
		
		*DataSN++;
		REVERSE_BYTES ( &Tmp->Bhs->SCSI_DATA_OUT.DataSN , DataSN);
		
		RtlCopyMemory ( &Tmp->Bhs->SCSI_DATA_OUT.InitiatorTaskTag , 
						&RefCmd->Bhs->GENERICBHS.InitiatorTaskTag ,
						4);
		
		if ( RefCmd->Bhs->GENERICBHS.Opcode == OP_R2T )		
			RtlCopyMemory ( &Tmp->Bhs->SCSI_DATA_OUT.TargetTansferTag , 
							&RefCmd->Bhs->R2T.TargetTansferTag , 
							4);

		Tmp->Data = DataBuf + Offset;
					
		Offset += DataLen;					
		DataSize -= DataLen;	// DataSize will NEVER < 0			
					
	}while ( DataSize > 0);
				
	Tmp->Bhs->SCSI_DATA_OUT.Final = 1;	
	
	return DataOut;
	
ErrorOut:

	if (DataOut)
		//TpFreePDU( ConInfo->ConCtx , DataOut , 0 );
		TpQueuePduRelease( ConInfo->ConCtx , DataOut );
	
	return NULL;
}
/*
   Send DataOut Queue of command
   if this is a SCSI command with unsolicited data
   SCSI command MUST be sent first
*/
VOID PiSendDataOut(PuCON_CTX Ctx , PPDU CmdOrR2T )
{
	PPDU		DataOut;
	PLIST_ENTRY	PduEnt , PduEnt2;
	
	PduEnt = &CmdOrR2T->DataOut;
	PduEnt2 = PduEnt->Flink;
		
	while (  PduEnt2 != PduEnt )
	{
		DataOut = CONTAINING_RECORD ( PduEnt2 , PDU , DataOut );
		TpQueueOutPDU( Ctx , DataOut );	
		PduEnt2 = PduEnt2->Flink;
	}
}

