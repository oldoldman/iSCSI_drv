
#include "precomp.h"

BOOLEAN PiInitKeys();

//
// Protocol Handlers
//

VOID PtProcessAsyncMsg ( PPDU Pdu  , PuCONINFO ConInfo)
{
	KdPrintEx((DPFLTR_USCSI,DBG_PROTOCOL,
	"%s ConInfo 0x%p Pdu 0x%p\n" , 
	__FUNCTION__ , 
	ConInfo , 
	Pdu));	
}


VOID PtProcessLogout ( PPDU Pdu  , PuCONINFO ConInfo)
{
	KdPrintEx((DPFLTR_USCSI,DBG_PROTOCOL,
	"%s ConInfo 0x%p Pdu 0x%p\n" , 
	__FUNCTION__ , 
	ConInfo , 
	Pdu));
}

NTSTATUS PtInit()
{
	if ( PiInitKeys() )
		return STATUS_SUCCESS;

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS PtUnInit()
{
	//TODO:
	return STATUS_SUCCESS;
}





