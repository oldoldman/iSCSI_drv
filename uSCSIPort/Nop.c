
#include "precomp.h"

PPDU PiFindPendingTask(ULONG TaskTag , PuCONINFO ConInfo , BOOLEAN Remove );

/*
Parms
      TPing : Target Nopin PDU if not NULL
       Echo : TRUE  Target Nopin expected
              FALSE Target Nopin not expected
  Immediate : Relevant only when Echo == TRUE
                
Return
	  Nopout PDU 
      NULL if failed
*/

PPDU PiAllocateNopOut(PuCONINFO ConInfo , PPDU TPing , BOOLEAN 	Echo, BOOLEAN Immediate)
{
	PPDU			Pdu;
	PSESSION		Session;
	PuCON_CTX		Ctx;
	FOUR_BYTE		F;
	
	Session 	= ConInfo->Session;
	Ctx 		= ConInfo->ConCtx;
	F.AsULong 	= 0xFFFFFFFF;
		
	if ( TPing )
	{
		// Reuse the Nopin PDU
		Pdu = TPing;
		Pdu->Bhs->NOP_OUT.Opcode = OP_NOP_OUT;
		RtlZeroMemory ( &Pdu->Bhs->NOP_OUT.Reserved5 , 16 );
	}
	else
	{
		Pdu = TpAllocatePDU ( Ctx , OP_NOP_OUT , 0 , 0x400 );
		if ( !Pdu )
			goto Out;
			
		REVERSE_BYTES ( &Pdu->Bhs->NOP_OUT.TargetTransferTag , &F);		
	}
	/*
	   The NOP-Out MUST have the Initiator Task Tag set to a valid value
	   only if a response in the form of NOP-In is requested
	*/
	if ( Echo )
	{
		// Echo Expected
		// 1. This is an Initiator Ping requiring Echo
		// 2. This is an Initaitor Echo requiring Echo
		Session->TaskTag++;
		REVERSE_BYTES ( &Pdu->Bhs->NOP_OUT.InitiatorTaskTag , &Session->TaskTag );
		Pdu->Bhs->NOP_OUT.Immediate = Immediate;	
	}
	else
	{
		// No Echo expected
		// 1. This is an a State Sync 
		// 2. This is an Initaitor Echo requiring no Echo
		REVERSE_BYTES ( &Pdu->Bhs->NOP_OUT.InitiatorTaskTag , &F);
		/*
		If the Initiator Task Tag contains 0xffffffff, the I bit MUST be set
	  	to 1 ... 
		*/
		Pdu->Bhs->NOP_OUT.Immediate = 1;
	}
	
	Pdu->Bhs->NOP_OUT.Final = 1;

Out:

	return Pdu;
}

/*
Parms
	Pdu : Nopin
*/

VOID PtProcessNopIn ( PPDU Pdu, PuCONINFO ConInfo)
{
	PPDU			NopOut;
	PuCON_CTX		Ctx;
	FOUR_BYTE		F , TTT , ITT;
	LARGE_INTEGER	Time;
	ULONG			Tmp;
	EIGHT_BYTE		Tmp2 , Tmp3 , Tmp4;
		
	Ctx = ConInfo->ConCtx;
	
	F.AsULong = 0xFFFFFFFF;	
	REVERSE_BYTES ( &TTT , &Pdu->Bhs->NOP_IN.TargetTransferTag);
	
	//DbgPrint("%s ConInfo=0x%p Pdu=0x%p TTT=0x%X\n" ,__FUNCTION__ , ConInfo , Pdu ,TTT.AsULong);
	// Target Echo
	if ( TTT.AsULong == F.AsULong )
	{		
		KeQueryTickCount ( &Time );
		
		REVERSE_BYTES ( &ITT , &Pdu->Bhs->NOP_IN.InitiatorTaskTag);
		NopOut = PiFindPendingTask ( ITT.AsULong , ConInfo , TRUE);		
		if ( NULL == NopOut )
		{
			DbgPrint("No Matching NopOut for Pdu=0x%x ITT=0x%x\n" , Pdu , ITT.AsULong);
			goto Out;
		}
			
		// Caculate Time Span
		
		REVERSE_BYTES_QUAD ( &Tmp4 , &Time);
		REVERSE_BYTES_QUAD ( &Tmp2 , &Tmp4);
		REVERSE_BYTES_QUAD ( &Tmp4 , &NopOut->SendingTime);
		REVERSE_BYTES_QUAD ( &Tmp3 , &Tmp4);
		
		Tmp = (ULONG)(Tmp2.AsULongLong - Tmp3.AsULongLong) * ConInfo->Session->Ns100Unit / 10;
		
		// Update Connection Statistics
		
		if ( ConInfo->Stats[CON_STATS_MIN] )
			ConInfo->Stats[CON_STATS_MIN] = min ( ConInfo->Stats[CON_STATS_MIN] , Tmp );
		else
			ConInfo->Stats[CON_STATS_MIN] = Tmp;
			
		ConInfo->Stats[CON_STATS_AVG] = (ConInfo->Stats[CON_STATS_AVG] + Tmp ) >> 1;
		
		if (ConInfo->Stats[CON_STATS_MAX])
			ConInfo->Stats[CON_STATS_MAX] = max (ConInfo->Stats[CON_STATS_MAX] , Tmp);
		else
			ConInfo->Stats[CON_STATS_MAX] = Tmp;
		/*
		DbgPrint("%s Traffics ( Min %d Max %d Avg %d) ms\n" , 
		__FUNCTION__ , 
		ConInfo->Stats[CON_STATS_MIN],
		ConInfo->Stats[CON_STATS_MAX],
		ConInfo->Stats[CON_STATS_AVG]);*/
		
		TpQueuePduRelease( Ctx , NopOut );

		// Test
		NopOut = PiAllocateNopOut ( ConInfo , NULL , TRUE , FALSE);
		if ( !NopOut )
			goto Out;
			
		TpQueueOutPDU ( Ctx , NopOut );
	}
	//Target Ping Requiring Nopout
	else
	{		
		/*
		Upon receipt of a NOP-In with the Target Transfer Tag set to a valid
   		value (not the reserved 0xffffffff), the initiator MUST respond with
   		a NOP-Out.
		*/
		NopOut = PiAllocateNopOut ( ConInfo , Pdu , FALSE , FALSE);
		if ( !NopOut )
			goto Out;
			
		TpQueueOutPDU ( Ctx , NopOut );
	}

Out:
	
	TpQueuePduRelease( Ctx , Pdu );
	return;	
}

/*
Parms
	Echo : Does this Nop-Out expect a Nop-In ? 
*/
VOID PtNopOut (PuCONINFO ConInfo , BOOLEAN Echo)
{
	PPDU			Pdu;
	
	Pdu = PiAllocateNopOut ( ConInfo , NULL , TRUE , TRUE);	
	
	if ( Pdu )
		TpQueueOutPDU ( ConInfo->ConCtx , Pdu );	
}