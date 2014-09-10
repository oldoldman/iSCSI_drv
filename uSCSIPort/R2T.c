
#include "precomp.h"

PPDU PiFindPendingTask(ULONG TaskTag , PuCONINFO ConInfo , BOOLEAN Remove);

__inline VOID PiSendDataOut(PuCON_CTX Ctx , PPDU CmdOrR2T );

PPDU PiQueueDataInOrR2T(PuCONINFO ConInfo , PPDU Cmd , PPDU DataInOrR2T);

PPDU PiAllocateDataOutPDU(
PuCONINFO ConInfo , PUCHAR DataBuf , ULONG Offset, ULONG DataSize,PULONG DataSN,PPDU RefCmd);

//
//
// 

VOID PtProcessR2T(PPDU Pdu  , PuCONINFO ConInfo)
{
	PPDU			Cmd;
	PSESSION		Session;
	PuCON_CTX		Ctx;
	FOUR_BYTE		ITT;
	
	Ctx = ConInfo->ConCtx;
	Session = ConInfo->Session;
	
	InterlockedIncrement ( (PLONG)&Session->R2TCount );
	
	REVERSE_BYTES ( &ITT , &Pdu->Bhs->R2T.InitiatorTaskTag);	
	Cmd = PiFindPendingTask ( ITT.AsULong , ConInfo , FALSE);	
	Pdu->Cmd = Cmd;
	
	Pdu = PiQueueDataInOrR2T ( ConInfo , Cmd , Pdu );
	if ( Pdu )
	{
		TpQueueR2TPDU ( Ctx , Pdu );
	}
}

//
// Sender for R2Ts
//

VOID PiR2TWorker(IN PVOID Parameter)
{
	NTSTATUS				Status;
	PSESSION				Session;
	PWORKER_THREAD_CTX		Tctx;
	PuCONINFO				ConInfo;
	PuCON_CTX				Ctx;
	PLIST_ENTRY				R2TEnt;
	PPDU					R2T, Cmd, DataOut;
	ULONG					DesiredDataTransferLength;
	ULONG					MaxRecvDataSegmentLength , MaxBurstLength;
	ULONG					Offset = 0 ;
	
	Status = STATUS_SUCCESS;
	
	Tctx 	= (PWORKER_THREAD_CTX)Parameter;
	Ctx 	= Tctx->Ctx;
	ConInfo = Ctx->ConInfo;
	Session = ConInfo->Session;
	
	MaxRecvDataSegmentLength = KEY_TV(KV(ConInfo , KEY_MaxRecvDataSegmentLength)).Number;
	MaxBurstLength = KEY_FV(KV(ConInfo,KEY_MaxBurstLength)).Number;
	/*
	Within a connection, outstanding R2Ts MUST be fulfilled by the
   	initiator in the order in which they were received
	*/
	while (TRUE )
	{
		Status = KeWaitForSingleObject ( &Ctx->R2TEvent , 
									 Executive , 
									 KernelMode , 
									 FALSE , 
									 NULL );
		
		KeClearEvent(&Ctx->R2TEvent);
									 
		while ( R2TEnt = ExInterlockedRemoveHeadList ( &Ctx->R2Ts , &Ctx->R2TLock ) )
		{
			R2T = (PPDU)CONTAINING_RECORD ( R2TEnt , PDU , PDUList );
			InitializeListHead ( &R2T->PDUList);
			Cmd = R2T->Cmd;
			/*
			The Desired Data
	   		Transfer Length MUST NOT be 0 and MUST not exceed MaxBurstLength p143
			*/
			REVERSE_BYTES ( &DesiredDataTransferLength , 
						    &R2T->Bhs->R2T.DesiredDataTransferLength);
			REVERSE_BYTES ( &Offset , &R2T->Bhs->R2T.BufferOffset);
		
#ifdef STRICT_PROT_CHECK
			if ( DesiredDataTransferLength > MaxBurstLength || !DesiredDataTransferLength)
				PiProtError( ConInfo , __FUNCTION__ );
#endif
			/*
			DbgPrint("%s DataBuffer=0x%X Offset=0x%X DesiredDataTransferLength=0x%X\n",
			__FUNCTION__ , Cmd->DataBuffer,Offset,DesiredDataTransferLength);*/
			DataOut	 = PiAllocateDataOutPDU ( ConInfo , 
											  Cmd->DataBuffer,
											  Offset ,
											  DesiredDataTransferLength,
											  &R2T->DataSN ,
											  R2T );		
		
			InsertTailList ( &R2T->DataOut , &DataOut->DataOut );
			
			InterlockedDecrement ( (PLONG)&Session->R2TCount );
		
			PiSendDataOut ( Ctx , R2T );
		}
	}
	
	ExFreePoolWithTag ( Tctx , USCSI_TAG );
	//Ctx->WorkItemFlags |= CONCTX_R2T_SAFE;
}