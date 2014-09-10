
#include "precomp.h"

PPDU PiFindPendingTask(ULONG TaskTag , PuCONINFO ConInfo , BOOLEAN Remove);

BOOLEAN PiCheckDataInHole(PPDU Cmd );

//
//
//
BOOLEAN PiCheckCmdComplete( PPDU Cmd );

VOID PtProcessResponse(PPDU Pdu, PuCONINFO ConInfo)
{
	PPDU		Cmd;
	FOUR_BYTE	ITT , ExpDataSN , BRC , RC;
	
	REVERSE_BYTES ( &BRC , &Pdu->Bhs->SCSI_RESPONSE.BiReadResidualCount);
	REVERSE_BYTES ( &RC , &Pdu->Bhs->SCSI_RESPONSE.ResidualCount);
		
	REVERSE_BYTES ( &ITT , &Pdu->Bhs->SCSI_RESPONSE.InitiatorTaskTag);
	Cmd = PiFindPendingTask ( ITT.AsULong , ConInfo , FALSE);
	Pdu->Cmd = Cmd;
	/*
	DbgPrint(
	"%s ITT=0x%X Pdu=0x%p UOuo=(%d%d%d%d) Response=0x%X Status=0x%X BRC=0x%X RC=0x%X\n" , 
	__FUNCTION__ , 
	ITT.AsULong , 
	Pdu,
	Pdu->Bhs->SCSI_RESPONSE.U,
	Pdu->Bhs->SCSI_RESPONSE.O,
	Pdu->Bhs->SCSI_RESPONSE.u,
	Pdu->Bhs->SCSI_RESPONSE.o,
	Pdu->Bhs->SCSI_RESPONSE.Response,
	Pdu->Bhs->SCSI_RESPONSE.Status,
	BRC.AsULong , 
	RC.AsULong);*/	
			
	REVERSE_BYTES ( &ExpDataSN , &Pdu->Bhs->SCSI_RESPONSE.ExpDataSN);
	Cmd->ExpDataSN = ExpDataSN.AsULong;	
	//
	// Queue Response , its processing may be delayed
	// if there are any missing Data-In/R2T(s)
	//	
	InsertTailList ( &Cmd->Resps , &Pdu->Resps );	
	
	if ( !PiCheckDataInHole ( Cmd ) )
	{
		Cmd->Flags |= PDU_F_CMD_CAN_COMPLETE;
		TpQueuePendingCompleteCmd ( Cmd->ConCtx , Cmd );
	}
}