
#include "precomp.h"

VOID PtSNACK (PuCONINFO ConInfo)
{
	PPDU	Pdu;
	
	Pdu = TpAllocatePDU ( ConInfo->ConCtx , OP_SNACK_REQ , 0 , 0 );	
}