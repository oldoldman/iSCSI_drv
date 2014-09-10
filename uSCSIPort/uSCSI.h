
#ifndef USCSI_H
#define USCSI_H

#define USCSI_TAG   (ULONG)'uSCI'

#define INETADDR(a, b, c, d) (a | (b<<8) | (c<<16) | (d<<24))
#define HTONL(a) (((a&0xFF)<<24) | ((a&0xFF00)<<8) | ((a&0xFF0000)>>8) | ((a&0xFF000000)>>24)) 
#define HTONS(a) (((0xFF&a)<<8) | ((0xFF00&a)>>8))

//
// debug module
//
//#define DBG_USCSI
//#define DBG_XPT
//#define DBG_PROT

ULONG DbgSeq;

#define TMP_Debug( c )
/*
#define TMP_Debug( c )	\
		{	\
			DbgPrint("%08X ",DbgSeq++);	\
			DbgPrint c;	\
		}*/

#ifdef DBG_USCSI
#define USCSI_Debug( c )	\
		DbgPrint c
#else
#define USCSI_Debug( c )
#endif

#ifdef DBG_XPT
#define XPT_Debug( c )	\
		DbgPrint c
#else
#define XPT_Debug( c )
#endif

#ifdef DBG_PROT
#define PROT_Debug( c )	\
		DbgPrint c
#else
#define PROT_Debug( c )
#endif

#define DPFLTR_USCSI	DPFLTR_IHVDRIVER_ID

#define DBG_TRANSPORT	0x4
#define DBG_TDI			0x5
#define DBG_PROTOCOL	0x6

typedef struct _ITGT
{
	LIST_ENTRY	List;
	TGT			Tgt;
}ITGT , *PITGT;

#endif
