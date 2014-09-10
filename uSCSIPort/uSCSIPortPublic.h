#ifndef _USCSI_PORT_PUBLIC_H
#define _USCSI_PORT_PUBLIC_H

typedef struct _TGT
{
	union
	{
		PUCHAR	TargetName;
		ULONG	NameOffset;
	};
	ULONG	Addr;
	USHORT	Port;
}TGT , *PTGT;

typedef struct _TGTS
{
	ULONG	Count;
	ULONG	Size;
}TGTS , *PTGTS;


//STATUS CODE
#define STATUS_USCSI_SESSION_FAILED 0xE
#endif