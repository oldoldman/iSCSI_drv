
#include "precomp.h"

VOID PiProcessKey( PPDU Pdu , PuCONINFO ConInfo , BOOLEAN AllocatePDU ,UCHAR Opcode);

VOID PiAnswerPdu( PPDU Pdu , PuCONINFO ConInfo , UCHAR Opcode );

BOOLEAN PiCanTxtComplete (PuCONINFO ConInfo );

VOID PiProtError (PuCONINFO ConInfo , PUCHAR Func );

#define PiAnswerTextPdu( Pdu , ConInfo)	\
	PiAnswerPdu ( Pdu , ConInfo , OP_TXT_REQ)

#define PiTextProcessKey(Pdu , ConInfo , AllocatePDU )	\
	PiProcessKey (Pdu , ConInfo , AllocatePDU , OP_TXT_REQ)

//
//
//
		
VOID PtProcessText( PPDU Pdu , PuCONINFO ConInfo)
{
	PPDU	LPdu = NULL;

	PROT_Debug(("%s ConInfo 0x%p Pdu 0x%p\n" , __FUNCTION__ , ConInfo , Pdu));

	if ( ConInfo->Answer )
	{
#ifdef STRICT_PROT_CHECK
		if ( DataSegLen )
			PiProtError ( ConInfo , __FUNCTION__ );
#endif
		goto Answer;
	}
		
	/*An initiator receiving a Text or Login
 	Response with the C bit set to 1 MUST answer with a Text or Login
 	Request with no data segment (DataSegmentLength 0).*/ 	
	if ( Pdu->Bhs->TXT_RESPONSE.Continue )
	{
#ifdef STRICT_PROT_CHECK
		if ( Pdu->Bhs->TXT_RESPONSE.Transit )
			PiProtError( ConInfo , __FUNCTION__ );
#endif
		if ( !ConInfo->Logical )
			ConInfo->Logical = Pdu;	//Head
		else
			// Part of
			InsertTailList ( &ConInfo->Logical->Continue, &Pdu->Continue );			
		goto Answer;
	}
	//
	// Logical Pdu completed
	//
	if (ConInfo->Logical )
	{
		LPdu = ConInfo->Logical;
		ConInfo->Logical = NULL;
	}
	else
		LPdu = Pdu;
		
	PiTextProcessKey ( LPdu , 
					ConInfo ,
					/*
					A Text Response with the F bit set to 1 MUST NOT contain key=value
   					pairs that may require additional answers from the initiator
					*/ 
					!LPdu->Bhs->TXT_RESPONSE.Final );
#ifdef STRICT_PROT_CHECK
	if ( ConInfo->TextState == TXT_STAT_STARTED && Pdu->Bhs->TXT_RESPONSE.Final)
	{
		PiProtError (ConInfo , __FUNCTION__);
	}
	else if 
#else
	if
#endif
	( ConInfo->TextState == TXT_STAT_STARTED && 
				!Pdu->Bhs->TXT_RESPONSE.Final)
	{
		if ( PiCanTxtComplete(ConInfo) )
		{
			PROT_Debug(("%s Completion Requested\n" , __FUNCTION__));
			ConInfo->TextState = TXT_STAT_PENDING_COMPLETE;
		}
	}
	else if ( ConInfo->TextState == TXT_STAT_PENDING_COMPLETE && 
		 Pdu->Bhs->TXT_RESPONSE.Final )
	{
		// End Conversation
		PROT_Debug(("%s Completion Agreed\n" , __FUNCTION__));
		ConInfo->TextState = TXT_STAT_IDLE;
		return;
	}
	else if ( ConInfo->TextState == TXT_STAT_PENDING_COMPLETE && 
		!Pdu->Bhs->TXT_RESPONSE.Final)
	{
		//NewPdu->Bhs->TXT_REQUEST.Final = 1;
	}

Answer:
	
	PiAnswerTextPdu( Pdu , ConInfo );
}

/*
  Logic for ending a Text conversation
*/
BOOLEAN PiCanTxtComplete (PuCONINFO ConInfo )
{
	if (ConInfo->MoreWorks)
		return FALSE;

	return TRUE;
}