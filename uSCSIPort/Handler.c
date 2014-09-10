
#include "precomp.h"

//Key Handlers

VOID PiUpdateKeyNegoState ( KEY_NAME Key , PuCONINFO ConInfo , PLIST_ENTRY Answers);
ULONG PiGetKeyState (  KEY_NAME Key , PuCONINFO ConInfo );
VOID PiSetKeyState ( KEY_NAME Key , PuCONINFO ConInfo , UCHAR State);
PKEY_VALUE PiFormatKeyValue (PKEY_VALUE Val);

VOID PiHandleReject ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers )
{
	KEY_NAME	Key;
	Key = KeyVal->Key.D;
	
	PROT_Debug(("%s Reject\n" , PiKeyName(Key)));	
	// Assert ( PiGetKeyState(KeyVal->Key.D , ConInfo) == KEY_Proposed )
	KV(ConInfo , Key)->Flags |= KEY_Reject;
		
	PiSetKeyState ( Key , ConInfo , KEY_Defaulted );
}

VOID PiHandleIrrelevant ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers )
{
	KEY_NAME	Key;
	Key = KeyVal->Key.D;
	
	PROT_Debug(("%s Irrelevant\n"  , PiKeyName(Key)));
	// Assert ( PiGetKeyState(KeyVal->Key.D , ConInfo) == KEY_Proposed )
	KV(ConInfo,Key)->Flags |= KEY_Irrelevant;
	PiSetKeyState ( Key , ConInfo , KEY_Defaulted );
}

VOID PiHandleNone ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers )
{
	PSESSION 	Session;
	KEY_NAME	Key;
	Session = ConInfo->Session;
	Key = KeyVal->Key.D;
	
	PROT_Debug(("%s None\n"  , PiKeyName(Key)));
	
	if ( PiKeys[Key].Attrs & KEY_TYPE_LOV )
	{
		KV(ConInfo , Key)->Flags |= KEY_None;
		PiUpdateKeyNegoState ( Key , ConInfo , Answers );
		//PiSetKeyState ( Key , ConInfo , KEY_Agreed );
	}
	else
	{
		PROT_Debug(("%s Protocol Error\n" , __FUNCTION__));
		PiSetKeyState ( Key , ConInfo , KEY_Defaulted );
	}
}

VOID PiHandleNotUnderstood ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers )
{
	KEY_NAME	Key;
	Key = KeyVal->Key.D;
	
	PROT_Debug(("%s NotUnderstood\n" ,KeyVal->Key.U , KEY_FV(KeyVal).String));
	/*
	KV(ConInfo , Key)->Flags |= KEY_NotUnderstood;
			
	PiSetKeyState ( Key , ConInfo , KEY_Defaulted );*/
}



VOID PiHandleIllegal ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers )
{
	PKEY_VALUE		Kv;
	KEY_VALUE		Kv2 = {0};
	KEY_NAME		Key;
	
	Key = KeyVal->Key.D;
	PROT_Debug(("%s Illegal\n"  , PiKeyName(Key)));
	//Mark Reject
	KeyVal->Flags &= ~KEY_Illegal;
	KeyVal->Flags |= KEY_Reject;
	
	Kv = PiFormatKeyValue ( KeyVal );
	InsertTailList ( Answers , &Kv->Vals);
}


VOID PiHandleDeclarative ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers)
{
	ULONG			TLen , ILen;
	PUCHAR			Tmp;
	KEY_NAME		Key;
	PKEY_VALUE		Kv , *Kv2 , Kv3;
	
	Key = KeyVal->Key.D;
	if ( Key == KEY_TargetAlias ) return;//temp
	Kv = KV(ConInfo , Key);
	Kv2 = AddrKV(ConInfo , Key);
		
	if (PiIsKeyBool(Key))
	{
		KEY_TV(Kv).Bool = KEY_FV(KeyVal).Bool;
		PROT_Debug (("Boolean 0x%X accepted\n" , KEY_TV(Kv).Bool));
	}
	else if ( PiIsKeyNumber(Key))
	{
		KEY_TV(Kv).Number = KEY_FV(KeyVal).Number;
		PROT_Debug (("Number 0x%X accepted\n" , KEY_TV(Kv).Number));
	}
	else if ( PiIsKeyString(Key))
	{		
		TLen = strlen ( KEY_FV(KeyVal).String );
		if (TLen)
		{
			ILen = strlen ( Kv->Value[KEY_I].String );
			if ( ILen)
			{
				Kv3 = PiAllocateKeyValStr( ILen + 1 , TLen + 1 );
				Tmp = KEY_IV(Kv3).String;
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE) + ILen + 1);
				KEY_IV(Kv3).String = Tmp;					
			}
			else
			{
				Kv3 = PiAllocateKeyValStr1( TLen + 1 );
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE));					
			}
			RtlCopyMemory ( KEY_TV(Kv3).String , KEY_FV(KeyVal).String , TLen + 1);
			ExFreePoolWithTag (Kv , USCSI_TAG);
			*Kv2 = Kv3;
			PROT_Debug (("String '%s' accepted\n" , KEY_TV(Kv3).String));
		}
	}
	else if (PiIsKeyLov(Key))
	{
		TLen = strlen ( KEY_FV(KeyVal).Lov.Val );
		if (TLen)
		{
			ILen = strlen ( Kv->Value[KEY_I].Lov.Val );
			if ( ILen)
			{
				Kv3 = PiAllocateKeyValStr( ILen + 1  , TLen + 1 );
				Tmp = KEY_IV(Kv3).Lov.Val;
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE) + ILen + 1);
				KEY_IV(Kv3).String = Tmp;					
			}
			else
			{
				Kv3 = PiAllocateKeyValStr1( TLen + 1 );
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE));	
			}
			RtlCopyMemory ( KEY_TV(Kv3).Lov.Val , KEY_FV(KeyVal).Lov.Val , TLen + 1);
			ExFreePoolWithTag (Kv , USCSI_TAG);
			*Kv2 = Kv3;
			PROT_Debug (("Lov '%s' accepted\n" , KEY_TV(Kv3).Lov.Val));				
		}
	}
}
/*
  Handler for most predefined Keys
*/
VOID PiInternalHandler ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers	)
{	
	PSESSION		Session;
	KEY_NAME		Key;
	PKEY_VALUE		Kv, *Kv2 , Kv3;
	
	Session = ConInfo->Session;
	Key = KeyVal->Key.D;
	
	PROT_Debug(("%s "  , PiKeyName(Key)));
	
	Kv = KV(ConInfo , Key);
	Kv2 = AddrKV(ConInfo , Key);
	
	if ( PiIsKeyDeclarative( Key ) )
	{
		PiHandleDeclarative(KeyVal  , ConInfo , Questions , Answers);
		return;
	}
					
	// Answer value is Reject
	if ( KeyVal->Flags & KEY_Reject )										 
		PiHandleReject( KeyVal  , ConInfo , Questions , Answers );				
	// Answer value is Irrelevant
	else if ( KeyVal->Flags & KEY_Irrelevant )					
		PiHandleIrrelevant( KeyVal  , ConInfo , Questions , Answers );					
	// Answer value is None
	else if ( KeyVal->Flags & KEY_None )					
		PiHandleNone ( KeyVal  , ConInfo , Questions , Answers );
	// only for incoming Keys				
	// Answer value is "NotUnderstood"
	else if ( KeyVal->Flags & KEY_NotUnderstood )						
		PiHandleNotUnderstood (KeyVal  , ConInfo , Questions , Answers );	
	// Value is Illegal , for example , out of range	
	else if ( KeyVal->Flags & KEY_Illegal)
		PiHandleIllegal( KeyVal  , ConInfo , Questions , Answers );
	
	else if ( PiIsKeyBool(Key) )
	{
		if ( PiKeys[Key].Attrs & KEY_FUNC_OR )
			KEY_PV(Kv).Bool = KEY_FV(Kv).Bool | KEY_FV(KeyVal).Bool;		
		else if ( PiKeys[Key].Attrs & KEY_FUNC_AND )
			KEY_PV(Kv).Bool = KEY_FV(Kv).Bool & KEY_FV(KeyVal).Bool;
		PROT_Debug (("Boolean 0x%X->0x%X\n" , KEY_FV(Kv).Bool , KEY_PV(Kv).Bool));
	}
	else if ( PiIsKeyNumber(Key) )
	{
		if ( PiKeys[Key].Attrs & KEY_FUNC_MIN )
			KEY_PV(Kv).Number = min ( KEY_FV(Kv).Number , KEY_FV(KeyVal).Number);
		else if ( PiKeys[Key].Attrs & KEY_FUNC_MAX )
			KEY_PV(Kv).Number = max ( KEY_FV(Kv).Number , KEY_FV(KeyVal).Number);
		PROT_Debug (("Number 0x%X->0x%X\n" , KEY_FV(Kv).Number , KEY_PV(Kv).Number));
	}
	else if ( PiIsKeyString(Key) )
	{
		PUCHAR		Tmp , Tmp2;
		ULONG		PLen , FLen , Tmp3;
		PLen = strlen ( KEY_FV(KeyVal).String );
		if (PLen)
		{
			FLen = strlen ( KEY_FV(Kv).String);
			if (FLen)
			{
				Kv3 = PiAllocateKeyValStr( FLen + 1, PLen + 1 );
				Tmp = KEY_FV(Kv3).String;
				Tmp2 = KEY_PV(Kv3).String;
				Tmp3 = Kv3->Size;
								
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE) );
				
				KEY_FV(Kv3).String = Tmp;
				RtlCopyMemory ( KEY_FV(Kv3).String , KEY_FV(Kv).String , FLen + 1);
				
				KEY_PV(Kv3).String = Tmp2;
				Kv3->Size = Tmp3;
				
			}
			else
			{
				Kv3 = PiAllocateKeyValStr1( PLen + 1 );
				Tmp2 = KEY_PV(Kv3).String;
				Tmp3 = Kv3->Size;
								
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE) );
				
				KEY_PV(Kv3).String = Tmp2;
				Kv3->Size = Tmp3;
			}
			
			RtlCopyMemory ( KEY_PV(Kv3).String , KEY_FV(KeyVal).String , PLen + 1);
			
			PROT_Debug (("String 0x%X->0x%X\n" , KEY_FV(Kv).String , KEY_PV(Kv).String));
		}
	}
	else if ( PiIsKeyLov(Key) )
	{
		PUCHAR		Tmp , Tmp2;
		ULONG		PLen , FLen , Tmp3;
		PLen = strlen ( KEY_FV(KeyVal).Lov.Val );
		if (PLen)
		{
			FLen = strlen ( KEY_FV(Kv).Lov.Val);
			if (FLen)
			{
				Kv3 = PiAllocateKeyValStr( FLen + 1, PLen + 1 );
				Tmp = KEY_FV(Kv3).Lov.Val;
				Tmp2 = KEY_PV(Kv3).Lov.Val;
				Tmp3 = Kv3->Size;
								
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE) );
				
				KEY_FV(Kv3).Lov.Val = Tmp;
				RtlCopyMemory ( KEY_FV(Kv3).Lov.Val , KEY_FV(Kv).Lov.Val , FLen + 1);
				
				KEY_FV(Kv3).Lov.Sel = KEY_FV(Kv).Lov.Sel;
				 
				KEY_PV(Kv3).Lov.Val = Tmp2;
				Kv3->Size = Tmp3;
				
			}
			else
			{
				Kv3 = PiAllocateKeyValStr1( PLen + 1 );
				Tmp2 = KEY_PV(Kv3).Lov.Val;
				Tmp3 = Kv3->Size;
								
				RtlCopyMemory ( Kv3 , Kv , sizeof (KEY_VALUE) );
				
				KEY_PV(Kv3).Lov.Val = Tmp2;
				Kv3->Size = Tmp3;
			}
			
			RtlCopyMemory ( KEY_PV(Kv3).Lov.Val , KEY_FV(KeyVal).Lov.Val , PLen + 1);
			
			PROT_Debug (("Lov 0x%X->0x%X\n" , KEY_FV(Kv).Lov.Val , KEY_PV(Kv).Lov.Val));
		}
	}
	
	PiUpdateKeyNegoState ( Key , ConInfo , Answers );
	
}

VOID PiAuthMethodHandler ( 
PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers	)
{
	KEY_NAME	Key;
	
	Key = KeyVal->Key.D;
	
	//DbgPrint("%s\n" , __FUNCTION__);
	// TODO : Dispatch to different AuthMethod handler
	if (KeyVal->Flags & KEY_None )
	{
		KV(ConInfo , Key)->Flags |= KEY_None;
		PiUpdateKeyNegoState ( Key , ConInfo , Answers );
	}	
}
