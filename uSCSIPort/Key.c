
#include "precomp.h"

VOID PiInternalHandler ( PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers);

VOID PiAuthMethodHandler ( PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers);

PPDU PiAllocateTxtPDU ( PuCONINFO ConInfo , PPDU Pdu , PUCHAR Data  , LONG Len , UCHAR Opcode);

VOID PiCheckNegoState( PuCONINFO ConInfo , PLIST_ENTRY Answers );

#define KEY_META_INFO(name , val ,low , high , attrs , nego_fail_action , retries , hdlr)	\
		{ name , sizeof (name)-1 , val , low , high , attrs , nego_fail_action , retries , hdlr}

ULONG		PiKeyIdx , PiKeyCaps;
PKEY_META	PiKeys;

//Type Func Irrelevant Scope Decl TPhase IPhase  Dir	

KEY_META PiPreKeys[] = 
{	
	KEY_META_INFO("MaxConnections", TRUE , 1, 0xFFFF, 
				KEY_TYPE_NUMBER|
				KEY_FUNC_MIN|
				KEY_IRRELEVANT_WHEN_DISCOVERY |
				KEY_SCOPE_SW|
				KEY_PHASE_TLO|KEY_PHASE_ILO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("SendTargets", FALSE , 0, 0, 
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_PHASE_IFFPO|
				KEY_DIR_INITIATOR,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("TargetName", FALSE , 0, 0, 
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_TFFPO|KEY_PHASE_IIO|KEY_PHASE_ISEC|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("InitiatorName", FALSE , 0 , 0 ,	
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_IIO|KEY_PHASE_ISEC|
				KEY_DIR_INITIATOR,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("TargetAlias", FALSE, 0, 0, 
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_TALL|KEY_PHASE_TSEC|
				KEY_DIR_TARGET,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("InitiatorAlias"	, FALSE,	0 , 0 , 
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_IALL|KEY_PHASE_ISEC|
				KEY_DIR_INITIATOR,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("TargetAddress", FALSE,	0 , 0 , 
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_TFFPO|KEY_PHASE_TSEC|
				KEY_DIR_TARGET,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
					
	KEY_META_INFO("TargetPortalGroupTag", FALSE,	0 , 0 , 
				KEY_TYPE_BINARY_NUMBER|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_TIO|KEY_PHASE_TSEC|
				KEY_DIR_TARGET,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("InitialR2T" , FALSE, 0 ,0 ,
				KEY_TYPE_BOOLEAN|
				KEY_FUNC_OR|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("ImmediateData" , FALSE, 0 , 0 ,
				KEY_TYPE_BOOLEAN|
				KEY_FUNC_AND|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("MaxBurstLength" , TRUE, 512 , 0xFFFFFF,
				KEY_TYPE_NUMBER|
				KEY_FUNC_MIN|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("FirstBurstLength", TRUE,512 , 0xFFFFFF,
				KEY_TYPE_NUMBER|
				KEY_FUNC_MIN|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("DefaultTime2Wait", TRUE,0 , 3600,
				KEY_TYPE_NUMBER|
				KEY_FUNC_MAX|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("DefaultTime2Retain", TRUE,0 , 3600,
				KEY_TYPE_NUMBER|
				KEY_FUNC_MIN|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("MaxOutstandingR2T", TRUE,1 , 65535,
				KEY_TYPE_NUMBER|
				KEY_FUNC_MIN|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("DataPDUInOrder", FALSE,0 , 0,
				KEY_TYPE_BOOLEAN|
				KEY_FUNC_OR|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("DataSequenceInOrder", FALSE,0 , 0,
				KEY_TYPE_BOOLEAN|
				KEY_FUNC_OR|
				KEY_IRRELEVANT_WHEN_DISCOVERY|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("ErrorRecoveryLevel", TRUE,0 , 2,
				KEY_TYPE_NUMBER|
				KEY_FUNC_MIN|
				KEY_SCOPE_SW|
				KEY_PHASE_ILO|KEY_PHASE_TLO|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				
	KEY_META_INFO("SessionType", FALSE,0 , 0 , 
				KEY_TYPE_STRING|
				KEY_SCOPE_SW|
				KEY_DECLARATIVE|
				KEY_PHASE_ILO|KEY_PHASE_ISEC|
				KEY_DIR_INITIATOR,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("MaxKeys" , FALSE, 0 , 0 , 0 , 0 , 0 , NULL),
	
	KEY_META_INFO("HeaderDigest", FALSE, 0, 0, 
				  KEY_TYPE_LOV|
				  KEY_SCOPE_CO|
				  KEY_PHASE_IIO|KEY_PHASE_TIO|
				  KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
	
	KEY_META_INFO("DataDigest", FALSE,	0, 0, 
				  KEY_TYPE_LOV|
				  KEY_SCOPE_CO|
				  KEY_PHASE_IIO|KEY_PHASE_TIO|
				  KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	
				  				
	KEY_META_INFO("MaxRecvDataSegmentLength" , TRUE, 512 , 0xFFFFFF ,
				KEY_TYPE_NUMBER|
				KEY_SCOPE_CO|
				KEY_DECLARATIVE|
				KEY_PHASE_IALL | KEY_PHASE_TALL|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiInternalHandler),	

	KEY_META_INFO("AuthMethod" , FALSE, 0 , 0 ,
				KEY_TYPE_LOV|
				KEY_SCOPE_CO|
				KEY_PHASE_IIO|KEY_PHASE_ISEC|KEY_PHASE_TIO|KEY_PHASE_TSEC|
				KEY_DIR_BOTH,
				KEY_NEGO_FAIL_INGORE,
				KEY_NEGO_MAX_RETRIES,
				PiAuthMethodHandler),	
	//
	KEY_META_INFO("LastKey" , FALSE, 0 , 0 , 0 , 0 , 0 , NULL)
};
/*
  Construct a Logical PDU
  Return : Text Data , Caller released
*/
PUCHAR PiExtractText ( PPDU Pdu  , PULONG PSize)
{
	PLIST_ENTRY		PduEnt , PduEnt2;
	PPDU			Text;
	ULONG			Size = 0 , Size2 = 0;
	PUCHAR			Buff = NULL, Buff2 = NULL;
	
	PduEnt = PduEnt2 = &Pdu->Continue;	
	do
	{
		Text = (PPDU)CONTAINING_RECORD ( PduEnt2 , PDU , Continue );
		Size += ( Text->Bhs->GENERICBHS.DataSegmentLength[0]<<16 | 
				  Text->Bhs->GENERICBHS.DataSegmentLength[1]<< 8 | 
				  Text->Bhs->GENERICBHS.DataSegmentLength[2]);
		PduEnt2 = PduEnt2->Flink;
	}while ( PduEnt2 != PduEnt);
	
	if (Size)
		Buff = ExAllocatePoolWithTag (PagedPool , Size , USCSI_TAG);
	
	if (!Buff)
	{
		if (PSize)
			*PSize = 0;
		return NULL;
	}
	
	Buff2 = Buff;	
	PduEnt = PduEnt2 = &Pdu->Continue;	
	do
	{
		Text = (PPDU)CONTAINING_RECORD ( PduEnt2 , PDU , Continue );
		Size2 = ( Text->Bhs->GENERICBHS.DataSegmentLength[0]<<16 | 
				  Text->Bhs->GENERICBHS.DataSegmentLength[1]<< 8 | 
				  Text->Bhs->GENERICBHS.DataSegmentLength[2]);
		RtlCopyMemory ( Buff2 , Text->Data , Size2 );
		
		Buff2 += Size2;		
		PduEnt2 = PduEnt2->Flink;
	}while (PduEnt2 != PduEnt);
	
	if (PSize)
		*PSize = Size;
		
	return Buff;
}

//TODO : Implement following rule
/*
If not otherwise specified, the maximum length of a simple-value (not
   its encoded representation) is 255 bytes, not including the delimiter
   (comma or zero byte).
*/

VOID PiDecodeBoolean (PUCHAR Val  , PKEY_VALUE KeyVal)
{
	if (!strcmp (Val , "Yes") )
		KEY_FV(KeyVal).Bool = TRUE;
	else if (!strcmp (Val , "No"))
		KEY_FV(KeyVal).Bool = FALSE;
	else
		KeyVal->Flags |= KEY_Illegal;
}

VOID PiDecodeString (PUCHAR Val  , PKEY_VALUE KeyVal)
{
	KEY_FV(KeyVal).String = Val;
}

VOID PiDecodeLov (PUCHAR Val  , PKEY_VALUE KeyVal)
{
	KEY_FV(KeyVal).Lov.Val = Val;
}

UCHAR Base64[] = 
{
	0x00 , 0x01 , 0x02 , 0x03 , 0x04 , 0x05 , 0x06 , 0x07 ,
	0x08 , 0x09 , 0x0A , 0x0B , 0x0C , 0x0D , 0x0E , 0x0F ,
	0x10 , 0x11 , 0x12 , 0x13 , 0x14 , 0x15 , 0x16 , 0x17 ,
	0x18 , 0x19 , 0x1A , 0x1B , 0x1C , 0x1D , 0x1E , 0x1F ,
	0x20 , 0x21 , 0x22 , 0x23 , 0x24 , 0x25 , 0x26 , 0x27 ,
	0x28 , 0x29 , 0x2A , 0x2B , 0x2C , 0x2D , 0x2E , 0x2F ,
	0x30 , 0x31 , 0x32 , 0x33 , 0x34 , 0x35 , 0x36 , 0x37 ,
	0x38 , 0x39 , 0x3A , 0x3B , 0x3C , 0x3D , 0x3E , 0x3F
};
//42 9496 7295
ULONG Pow10[] = 
{
	1 , 
	10 , 
	100 , 
	1000 , 
	10000 , 
	100000 , 
	1000000 ,
	10000000 ,
	100000000 ,
	1000000000 ,
	//0xFFFFFFFF
	4294967295	
};

ULONG PiPow10 (ULONG p)
{
	ULONG Val = 0xa;
	/*
	if ( !p ) return 1;
	
	while ( --p > 0 )
		Val = ( Val << 3 )  + Val + Val ;*/

	return Pow10[p];
}

ULONG PiDecodeNum (PUCHAR Val)
{
	PUCHAR	Start;
	ULONG	Count = 0;
	UCHAR	C , Idx;
	ULONG	Value = 0;
	
	C = *Val;
	// Skip leading zero(s)
	while ( C == '0')
		C = *Val++;
	
	if ( C == '\0')
		return 0;
	else if ( C == 'x' || C == 'X' )
	{
		//Hex-Constant
		Start = Val;
		while ( *Val++ != '\0' );
		
		while ( --Val >= Start )
		{
			C = *Val;
			
			if ( C >= '0' && C <= '9')
				 Value = Value | ( C - '0' ) << Count++*4;
			else if ( C >= 'a' && C <= 'f')
				Value = Value | ( C - 'a' + 0xa ) << Count++*4;
			else if ( C >= 'A' && C <= 'F' )
				Value = Value | ( C - 'A' + 0xa ) << Count++*4;
		}
		return Value;		
	}
	else if ( C == 'B' || C == 'b')
	{
		//Base64
		Start = Val;
		while ( *Val++ != '\0' );
		
		while ( --Val >= Start )
		{
			C = *Val;
			
			if ( C >= 'A' && C <= 'Z')
				Idx = C - 'A';
			else if ( C >= 'a' && C <= 'z')
				Idx = C- 'a' + 26;
			else if ( C >= '0' && C <= '9' )
				Idx = C - '0' + 52;
			else if ( C == '+' )
				Idx = 62;
			else if (C == '/' )
				Idx = 63;
			else
				Idx = -1;	// padding '='
			if (Idx > 0)	
				Value = Value | Base64[Idx] << Count++*6;
		}
		return Value;
	}
	else
	{
		//Decimal-Constant
		Start = Val;
		
		while ( *Val++ != '\0' );
		
		Val--;
		
		while ( --Val >= Start )
		{
			C = *Val;
			if ( C >= '0' && C <= '9')
				Value += ( C - '0' ) * PiPow10(Count++);
		}
		return Value;
	}	
}

VOID PiDecodeNumber(PUCHAR Val , PKEY_VALUE KeyVal)
{
	ULONG		Value;
	KEY_NAME	Key;
	
	Key = KeyVal->Key.D;
	
	Value = PiDecodeNum ( Val );
	
	if ( PiKeys[Key].RngValid && 
	    ( Value < PiKeys[Key].Low || Value > PiKeys[Key].High ) )
	    KeyVal->Flags |= KEY_Illegal;
	    
	KEY_FV(KeyVal).Number = Value;
}

//
//
//

VOID PiDecodeBinNumber(PUCHAR Val , PKEY_VALUE KeyVal)
{
	KEY_FV(KeyVal).Number = PiDecodeNum( Val );
}

//
//
//

VOID PiDecodeRange (PUCHAR Val , PKEY_VALUE KeyVal)
{
	ULONG		Value;
	KEY_NAME	Key;
	
	Key = KeyVal->Key.D;
	//TODO: Range is something like this 1~100 
}

//
//
//

VOID PiDecodeKeyValue (PUCHAR Key , PUCHAR Val , PKEY_VALUE KeyVal)
{
	PKEY_META	Meta;
	ULONG		i;	
	
	for ( i = 0 ; i < PiKeyIdx ; i++ )
	{
		if (PiIsKeyDeregistered(i))
			continue;
			
		if ( !strcmp (Key , (PiKeys+i)->Key ))
		{
			Meta = PiKeys+i;
			
			KeyVal->Key.D = i;
			
			if ( !strcmp(Val , "NotUnderstood"))
			{
				/*
					All keys in this document, except for the X extension formats, MUST
 					be supported by iSCSI initiators and targets when used as specified
 					here.  If used as specified, these keys MUST NOT be answered with
 					NotUnderstood.
   				
 					***that is to say predefined Keys will never be answered with NotUnderstood
				*/
				// Assert ( i > KEY_LastKey )
				KeyVal->Flags |= KEY_NotUnderstood;
				return;
			}
					
			if ( !strcmp(Val , "Irrelevant"))
			{
				KeyVal->Flags |= KEY_Irrelevant;
				return;
			}
			
			if ( !strcmp(Val , "Reject") )
			{
				KeyVal->Flags |= KEY_Reject;
				return;
			}			
						
			if ( !strcmp(Val , "None"))
			{
				KeyVal->Flags |= KEY_None;
				return;
			}
			
			if ( Meta->Attrs & KEY_TYPE_BOOLEAN )
			{		
				PiDecodeBoolean(Val , KeyVal);
				return;
			}
			
			if ( Meta->Attrs & KEY_TYPE_STRING )
			{
				PiDecodeString (Val , KeyVal);
				return;
			}
			
			if ( Meta->Attrs & KEY_TYPE_LOV )
			{
				PiDecodeLov (Val , KeyVal);
				return;
			}
				
			if ( Meta->Attrs & KEY_TYPE_NUMBER )
			{
				PiDecodeNumber (Val , KeyVal );
				return;
			}
			
			if (Meta->Attrs & KEY_TYPE_BINARY_NUMBER )
			{
				PiDecodeBinNumber ( Val , KeyVal );
				return;
			}
			
		}		
	}
		
	// This is an UnKnown Key
	KeyVal->Flags |= KEY_NotUnderstood2;
	KeyVal->Key.U = Key;

}

PKEY_VALUE PiFormatKeyValue (PKEY_VALUE Val);

//
// State : KEY_Defaulted or KEY_Proposed or KEY_Agreed
//
VOID PiSetKeyState ( KEY_NAME Key , PuCONINFO ConInfo , UCHAR State)
{
	PKEY_VALUE	Kv;
	
	Kv= KV(ConInfo,Key);
	if (Kv)
	{
		UCHAR OldState = Kv->Flags & KEY_STATE_MASK;
		Kv->Flags &= ~KEY_STATE_MASK;
		Kv->Flags |= State;
		/*DbgPrint("%s %s Key NegoState 0x%X->0x%X\n" , 
			__FUNCTION__ , PiKeyName(Key) , OldState , State );*/
	}
	else
		PROT_Debug(("%s %s not initialized\n" , __FUNCTION__ , PiKeyName(Key)));
}

//
//
//

ULONG PiGetKeyState (  KEY_NAME Key , PuCONINFO ConInfo )
{
	PKEY_VALUE Kv;
		
	Kv = KV(ConInfo,Key);
	if (Kv)
		return Kv->Flags & KEY_STATE_MASK;
	else
	{		
		// In case of Key is not initialized , return KEY_Defaulted
		PROT_Debug(("%s %s not initialized\n" , __FUNCTION__ , PiKeyName(Key)));
		return KEY_Defaulted;
	}
}

/*
   KeyState State Machine
   
        |-------------------------|
        |                         |
    Defaulted ---------|          |
        |              v          v
        |        |-> Pending -> Agreed 
        v        | 
    Proposed  ---|

*/
VOID PiUpdateKeyNegoState ( KEY_NAME Key , PuCONINFO ConInfo , PLIST_ENTRY Answers)
{
	PKEY_VALUE		Kv , Kv2;
	BOOLEAN			Flag = FALSE;
	ULONG			NegoState;
	
	NegoState = PiGetKeyState (Key , ConInfo);
		
	// Target initialized nego	
	if ( KEY_Defaulted == NegoState )
	{	
		// Need an answer	
		if ( !PiIsKeyDeclarative(Key) )
		{
			Kv2 = KV(ConInfo , Key);
			
			if ( !(Kv2->Flags & KEY_FMT_PENDING) )
			{
				Kv2->Flags |= KEY_FMT_PENDING;
				Flag = TRUE;
			}
				
			Kv = PiFormatKeyValue( Kv2 );
			
			if (Flag)
			{
				Kv2->Flags &= ~KEY_FMT_PENDING;
				Kv->Flags &= ~KEY_FMT_PENDING;
			}
			InsertTailList ( Answers , &Kv->Vals);
			PiSetKeyState ( Key , ConInfo , KEY_Pending);				
		}		
	}
	// Initiator initialized nego
	else if ( KEY_Proposed == NegoState )
	{
		PiSetKeyState ( Key , ConInfo , KEY_Pending);
	}	
}

//
//
//

BOOLEAN PiIsKeyIrrelevant(PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers)
{
	PSESSION	Session;
	KEY_NAME	Key;
	PKEY_VALUE	Kv;
	KEY_VALUE	Kv2 = {0};
	
	Session = ConInfo->Session;
	Key = KeyVal->Key.D;		
	
    // Rule 1 : Stage checking
	if ( ConInfo->State != CON_STAT_IN_LOGIN) 
	{
		 if ((PiKeyAttrs(Key) & KEY_PHASE_TIO) ||
		     (PiKeyAttrs(Key) & KEY_PHASE_TSEC) ||
		     (PiKeyAttrs(Key) & KEY_PHASE_TLO) )
			goto IrrelevantOut;
		if  ( (ConInfo->State != CON_STAT_LOGGED_IN ) && 
		 		(PiKeyAttrs(Key) & KEY_PHASE_TFFPO))
			goto IrrelevantOut;  
	}
	else
	{
		if ( !IsLeadingCon(ConInfo) && ( PiKeyAttrs(Key) & KEY_PHASE_TLO))
			goto IrrelevantOut;
			
		if (PiKeyAttrs(Key) & KEY_PHASE_TFFPO)
			goto IrrelevantOut;
		
		if ( !(PiKeyAttrs(Key) & KEY_PHASE_TSEC) &&
		     (ConInfo->LoginState == LOGIN_STAGE_SecurityNegotiation) )
			goto IrrelevantOut;
	}	   
	// Rule 2 : Irrelevant when SessionType=Discovery	
	if ( (PiKeyAttrs(Key) & KEY_IRRELEVANT_WHEN_DISCOVERY)&& 
	    !ConInfo->Session->NormalSession)
		goto IrrelevantOut;
	
	// Rule 3 : Irrelevant when ( InitialR2T=Yes and ImmediateData=No )
	if ( Key == KEY_FirstBurstLength && 
		KEY_FV(KV(ConInfo , KEY_InitialR2T)).Bool &&
		!KEY_FV(KV(ConInfo , KEY_ImmediateData)).Bool)
		goto IrrelevantOut;
	
	return FALSE;
	
IrrelevantOut:
	
	Kv2.Key.D = KeyVal->Key.D;
	Kv2.Flags |= KEY_Irrelevant;
	Kv = PiFormatKeyValue( &Kv2 );			
	InsertTailList ( Answers , &Kv->Vals);
	PROT_Debug(("%s is Irrelevant\n" , PiKeyName(Key) ));
		
	return TRUE;
}

//
//
//

VOID PiHandleNotUnderstood2 ( PKEY_VALUE KeyVal , PuCONINFO ConInfo , PLIST_ENTRY Questions , PLIST_ENTRY Answers )
{
	PKEY_VALUE		Kv;
	KEY_VALUE		Kv2= {0};
	PSESSION 		Session;
	Session = ConInfo->Session;
	
	PROT_Debug(("NotUnderstood %s=%s\n" , KeyVal->Key.U , KEY_FV(KeyVal).String));
	
	Kv = PiFormatKeyValue ( KeyVal );
	InsertTailList ( Answers , &Kv->Vals);
}

PVOID PiFormatKeyValues(PLIST_ENTRY KeyVals , PULONG DataLen);

VOID PiDumpKey (PKEY_VALUE KeyVal);

//
//
//

VOID PiAnswerKeys( PPDU Pdu , PuCONINFO ConInfo , PLIST_ENTRY Answers)
{
	PUCHAR			Key , Val , Buff;
	PKEY_VALUE		TmpKeyVal ;
	ULONG			Size , Len;
	LIST_ENTRY		Questions;
	PLIST_ENTRY		QEnt , QEnt2;
	
	InitializeListHead( &Questions );
	
	Buff = Key = Val = PiExtractText( Pdu , &Size);
	/*
	if ( Buff )
	{
		PUCHAR Tmp = Buff , End = Buff + Size - 1;
		do
		{
			DbgPrint("Question : %s\n" ,Tmp);
			while( *Tmp++ != '\0');
		}while( Tmp < End );
	}*/
	
	if ( Buff )
	{		
		while ( Val < ( Buff + Size ))
		{
			if ( *Val++ == '=' )
			{
				*(Val - 1 ) = '\0';
				Len = strlen (Val);
				//
				// Getting the Key and Legal checking
				//
				TmpKeyVal = PiAllocateKeyValue();
								
				PiDecodeKeyValue ( Key ,  Val , TmpKeyVal);
				
				Key = Val + Len + 1;
				Val = Key;
				
				//KEYIN(TmpKeyVal);	
				
				InsertTailList ( &Questions , &TmpKeyVal->Vals);				
			}
		}
		//
		// Begin processing
		//
		QEnt = &Questions; 
		QEnt2 = Questions.Flink;
		
		while ( QEnt2 != QEnt)
		{
			TmpKeyVal = CONTAINING_RECORD (QEnt2 , KEY_VALUE , Vals );
			//
			// Answer a key
			//
			if ( !(TmpKeyVal->Flags & KEY_NotUnderstood2) )
			{
				if (PiIsKeyIrrelevant(TmpKeyVal , ConInfo , &Questions , Answers))
					continue;
				
				if (PiKeys[TmpKeyVal->Key.D].Handler)
					(*(PiKeys[TmpKeyVal->Key.D].Handler))( TmpKeyVal  , ConInfo , 
																	&Questions , Answers );
				
				else
					PROT_Debug(("%s %s missing handler" , __FUNCTION__ , 
										PiKeyName(TmpKeyVal->Key.D)));
			}							
			else					
				PiHandleNotUnderstood2 (TmpKeyVal  , ConInfo , &Questions , Answers );					
						
			QEnt2 = QEnt2->Flink;
			
		}
		//
		// Free Questions
		//
		while ( (QEnt = RemoveHeadList (&Questions)) != &Questions)
		{
			TmpKeyVal = CONTAINING_RECORD (QEnt , KEY_VALUE , Vals);
			ExFreePoolWithTag ( TmpKeyVal ,USCSI_TAG);
		}
		ExFreePoolWithTag ( Buff , USCSI_TAG );
	}
}

/*
   Process Key Pairs in Pdu
   If told so , ALWAYS queue an answer PDU , in case of no answer data , 
   this PDU function as an sync signal
*/
VOID PiProcessKey( PPDU Pdu , PuCONINFO ConInfo , BOOLEAN AllocatePDU , UCHAR Opcode )
{
	PUCHAR				Data = NULL;
	LONG				DataLen = 0;
	PKEY_VALUE			AnswerVal;
	PPDU				NewPdu = NULL;
	LIST_ENTRY			Answers;
	PLIST_ENTRY			KeyValEnt;
	
	InitializeListHead (&Answers);	
	
	PiAnswerKeys ( Pdu , ConInfo , &Answers);
	
	PiCheckNegoState ( ConInfo , &Answers );
		
	if (!AllocatePDU) goto NoPdu;		

	Data = PiFormatKeyValues (&Answers , &DataLen);
	/*
	if ( Data )
	{
		PUCHAR Tmp = Data , End = Data + DataLen - 1;
		do
		{
			DbgPrint("Answer : %s\n" ,Tmp);
			while( *Tmp++ != '\0');
		}while( Tmp < End );
	}	*/
	
	if ( ! ( NewPdu = PiAllocateTxtPDU ( ConInfo , Pdu , Data , DataLen , Opcode) ) )
		goto ErrorOut;	
	//
	// Queue the answer
	//
	ConInfo->Answer  = NewPdu;

NoPdu:	
	//
	// Free answer values
	//
	while ( ( KeyValEnt = RemoveHeadList ( &Answers )) != &Answers )
	{
		AnswerVal = CONTAINING_RECORD ( KeyValEnt , KEY_VALUE , Vals );
		if ( AllocatePDU ) KEYOUT( AnswerVal );
		ExFreePoolWithTag ( AnswerVal ,USCSI_TAG);
	}	

	return;
	
ErrorOut:

	if ( Data )
		ExFreePoolWithTag ( Data , USCSI_TAG );
		
	while ( ( KeyValEnt = RemoveHeadList ( &Answers )) != &Answers )
	{
		AnswerVal = CONTAINING_RECORD ( KeyValEnt , KEY_VALUE , Vals );
		ExFreePoolWithTag ( AnswerVal ,USCSI_TAG);
	}
	
	return;
}

//
//
//

PUCHAR PiFormatNumDec ( ULONG Num )
{
	CHAR		Idx = 10;
	UCHAR		VLen , Offset = 0 , D;
	PUCHAR		FormatStr;
	
	if ( !Num )
	{
		VLen = 2;
		FormatStr = ExAllocatePoolWithTag ( PagedPool , VLen , USCSI_TAG );
		*(FormatStr + Offset++) = '0';
		*(FormatStr + Offset ) = '\0';
		goto Out;		
	}		
	
	while ( Idx-- )
		if ( Pow10[Idx] <= Num && Num <= Pow10[Idx+1] )
			break;
	
	VLen = Idx + 1;
	
	FormatStr = ExAllocatePoolWithTag ( PagedPool , VLen , USCSI_TAG );
	
	while ( Idx >= 0 )
	{
		D = 1;
		while ( ( Num -= Pow10[Idx] ) >= Pow10[Idx] )
			D++;
		
		*(FormatStr + Offset++) = D + '0';
		
		if ( !Num )
			while ( Idx-- )
				*(FormatStr + Offset++ ) = '0';
		else
			while ( Idx-- )
				if ( Pow10[Idx] <= Num && Num <= Pow10[Idx+1] )
					break;
				else
					*(FormatStr + Offset++ ) = '0';
	}
	
	*(FormatStr + Offset ) = '\0';	

Out:	
	return FormatStr;
}

//
//
//

PUCHAR PiFormatNumHex ( ULONG Num)
{
	UCHAR		VLen = 0 , D;
	PUCHAR		FormatStr;
	ULONG		Offset = 0;
	
	if (!Num)
		VLen = 1;
	else
		while(VLen++ < 8)
			if ( Num >> 4*VLen )
				break;

	FormatStr = ExAllocatePoolWithTag ( PagedPool , VLen + 3, USCSI_TAG );//'0x' and '\0'
	*(FormatStr + Offset++ ) = '0';
	*(FormatStr + Offset++ ) = 'x';

	while ( VLen > 0)
	{
		D = ( Num >> 4*(VLen-1)) & 0xF;
		if ( D >= 0 && D <= 9 )
			*(FormatStr + Offset++) = '0' + D;
		else if ( D >= 0xA && D <= 0xF )
			*(FormatStr + Offset++) = 'A' + D - 0xA;
		VLen--;
	}	
	*(FormatStr + Offset ) = '\0';
	
	return FormatStr;
}

//
//
//

PUCHAR PiFormatNumBase64 ( ULONG Num )
{
	return NULL;
}
/*
  This function serve for 2 purposes
   1. To display incoming KeyValues
   2. To format answers
  Return : 
   Formatted Val , Caller will release the returning value
  Comments :
   1. Formatted Value is a copy of Val , exept that it is formatted
   2. Caller choose which value (Formal/Pending) to be formatted  
      by setting KEY_FMT_PENDING flag
*/

#define KEY_WHICH(Kv)		((Kv->Flags&KEY_FMT_PENDING)?KEY_PV(Kv):KEY_FV(Kv))

//
//
//

PKEY_VALUE PiFormatKeyValue (PKEY_VALUE Val)
{
	LONG			Len = 0 ;
	PUCHAR			V , Tmp;
	KEY_NAME		Key;
	PKEY_VALUE		Kv;
	ULONG			VLen = 0 , KLen = 0 , ValSize;
	
	Key 	= Val->Key.D;
	ValSize = Val->Size;
	
	if ( Val->Flags & (	KEY_Irrelevant | 
						KEY_Reject | 
						KEY_None |
						KEY_NotUnderstood |												
						KEY_NotUnderstood2 ) )
	{
		if (Val->Flags & KEY_Irrelevant)
			V = "Irrelevant";
		else if (Val->Flags & KEY_Reject )
			V = "Reject";
		else if (Val->Flags & KEY_None)
			V = "None";
		else if (Val->Flags & ( KEY_NotUnderstood2 | KEY_NotUnderstood ) )
			V = "NotUnderstood";
			
		VLen = strlen (V) + 1;	//'\0'
		
		if (Val->Flags & KEY_NotUnderstood2)
			KLen = strlen ( Val->Key.U )+1;
		else
			KLen = PiKeys[Key].KeyLen+1;
			
		Len = ValSize + VLen + KLen;
		Kv = PiAllocateKeyValFormatStr( Len );
		Tmp = Kv->FormatStr;
		RtlCopyMemory ( Kv , Val , ValSize );
		Kv->FormatStr = Tmp;
		Kv->Size = Len;
		Kv->Len = Len - ValSize;
		//
		//
		//
		if ( Val->Flags & (KEY_NotUnderstood2| KEY_NotUnderstood) )
			RtlCopyMemory ( Kv->FormatStr , Val->Key.U , KLen-1);
		else
			RtlCopyMemory ( Kv->FormatStr , PiKeys[Key].Key , KLen-1);		
		
		*(Kv->FormatStr + KLen - 1) = '=';
		
		RtlCopyMemory ( Kv->FormatStr + KLen  , V , VLen - 1);
		*(Kv->FormatStr + Kv->Len - 1) = '\0';
	}
	
	else if ( PiKeys[Key].Attrs & (KEY_TYPE_STRING|KEY_TYPE_LOV))
	{
		(PiKeys[Key].Attrs & KEY_TYPE_STRING ) ?
			(V = KEY_WHICH(Val).String ): (V = KEY_WHICH(Val).Lov.Val );
		
		VLen = strlen (V) + 1;	//'\0'		
		KLen = 	PiKeys[Key].KeyLen + 1;//'='
		
		Len = ValSize + VLen + KLen;
		Kv = PiAllocateKeyValFormatStr ( Len);
		Tmp = Kv->FormatStr;
		RtlCopyMemory ( Kv , Val , ValSize );
		Kv->FormatStr = Tmp;
		Kv->Size = Len;
		Kv->Len = Len - ValSize;
		//
		//
		//
		if ( PiKeys[Key].Attrs & KEY_TYPE_STRING)
		{
			if (Val->Value[0].String)
				Kv->Value[0].String = (PUCHAR)Kv + (Val->Value[0].String - (PUCHAR)Val );
			if (Val->Value[1].String)
				Kv->Value[1].String = (PUCHAR)Kv + (Val->Value[1].String - (PUCHAR)Val ); 
		}			
		
		if ( PiKeys[Key].Attrs & KEY_TYPE_LOV)
		{
			if (Val->Value[0].Lov.Val)
				Kv->Value[0].Lov.Val = (PUCHAR)Kv + (Val->Value[0].Lov.Val - (PUCHAR)Val );
			if (Val->Value[1].String)
				Kv->Value[1].Lov.Val = (PUCHAR)Kv + (Val->Value[1].Lov.Val - (PUCHAR)Val ); 
		}	
		
		RtlCopyMemory (Kv->FormatStr , PiKeys[Key].Key , KLen - 1 );
				
		*(Kv->FormatStr + KLen - 1) = '=';
		
		RtlCopyMemory (Kv->FormatStr + KLen , V , VLen - 1);
		*(Kv->FormatStr + Kv->Len - 1 ) = '\0';		
	}
	
	else if ( PiKeys[Key].Attrs & KEY_TYPE_BOOLEAN )
	{
		KEY_WHICH(Val).Bool ? (V = "Yes" ) : (V = "No");
		
		VLen = strlen (V) + 1;	//'\0'
		KLen = PiKeys[Key].KeyLen + 1;	//'='
		
		Len = sizeof (KEY_VALUE) + VLen + KLen;
		Kv = PiAllocateKeyValFormatStr ( Len );
		Tmp = Kv->FormatStr;
		RtlCopyMemory ( Kv , Val , sizeof (KEY_VALUE));
		Kv->FormatStr = Tmp;
		Kv->Len = Len - sizeof (KEY_VALUE);		
		//
		//
		//
		RtlCopyMemory (Kv->FormatStr , PiKeys[Key].Key , KLen - 1 );
		
		*(Kv->FormatStr + KLen - 1) = '=';
		
		RtlCopyMemory (Kv->FormatStr + KLen , V , VLen - 1);
		*(Kv->FormatStr + Kv->Len - 1) = '\0';
	}
	
	else if (PiKeys[Key].Attrs & (KEY_TYPE_NUMBER|KEY_TYPE_BINARY_NUMBER))
	{
		ULONG	Offset = 0;
		PUCHAR	Num;
		//
		// VLen Counts the digits
		//
		
		if ( Val->Flags & KEY_FMT_NUM_DEC )
			Num = PiFormatNumDec ( KEY_WHICH(Val).Number );
		else if ( Val->Flags & KEY_FMT_NUM_HEX )
			Num = PiFormatNumHex ( KEY_WHICH(Val).Number );
		else if (  Val->Flags & KEY_FMT_NUM_BASE64 )
			Num = PiFormatNumBase64 ( KEY_WHICH(Val).Number );
		else
			Num = PiFormatNumDec ( KEY_WHICH(Val).Number );
			
		VLen = strlen ( Num ) + 1;
		KLen = PiKeys[Key].KeyLen + 1;	//'='
		Len = ValSize + VLen + KLen;
		
		Kv = PiAllocateKeyValFormatStr ( Len );
		Tmp = Kv->FormatStr;
		RtlCopyMemory ( Kv , Val , ValSize);
		Kv->FormatStr = Tmp;
		Kv->Size = Len;
		Kv->Len = Len - ValSize;		

		RtlCopyMemory (Kv->FormatStr , PiKeys[Key].Key , KLen - 1 );
		
		*(Kv->FormatStr + KLen - 1) = '=';
		
		RtlCopyMemory (Kv->FormatStr + KLen, Num , VLen);
		ExFreePoolWithTag ( Num , USCSI_TAG);		
	}
	
	if (Kv)
		Kv->Flags |= KEY_Formatted;
	
	return Kv;
}

/*
  KeyVals: Listhead of KEY_VALUE , formatted or not
  Return : Total bytes needed
*/
ULONG PiKeyBufferLength( PLIST_ENTRY KeyVals)
{
	ULONG			Length = 0;
	PLIST_ENTRY		KeyEnt;
	PKEY_VALUE		KeyVal , FmtVal;
	KEY_NAME		Key;
	
	KeyEnt = KeyVals->Flink;
	
	while (KeyEnt != KeyVals )
	{
		KeyVal = CONTAINING_RECORD (KeyEnt , KEY_VALUE , Vals );
				
		if ( KeyVal->Flags & KEY_Formatted )
			Length += KeyVal->Len;			//this is the most frequently code path
		else
		{
			FmtVal = PiFormatKeyValue (KeyVal);
			Length += FmtVal->Len;
			ExFreePoolWithTag ( FmtVal , USCSI_TAG );
		}			
		
		KeyEnt = KeyEnt->Flink;
	}
	// Rounding 4 bytes aligned
	if (Length & 0x3 )
	{
		Length &= ~0x3;
		Length += 0x4;
	}
	return Length;
}

//  Format KEY_VALUE in KeyVals list
  
//  KeyVals : IN  Listhead of KEY_VALUE , formatted or not
//  DataLen : OUT Buffer length in byte

//   Return : Formatted buffer , Allocate from Paged pool

PVOID PiFormatKeyValues(PLIST_ENTRY KeyVals , PULONG DataLen)
{
	ULONG			Length , Len;
	PUCHAR			DataBuffer = NULL, DataBuffer2 = NULL;
	PLIST_ENTRY		KeyEnt;
	PKEY_VALUE		KeyVal , FmtVal;
	KEY_NAME		Key;
		
	Length = PiKeyBufferLength (KeyVals);
	if ( !Length )
		goto Out;
		
	DataBuffer = 
		DataBuffer2 = 
			ExAllocatePoolWithTag (PagedPool , Length , USCSI_TAG);
	if (!DataBuffer)
		goto Out;
		
	RtlZeroMemory (DataBuffer + Length - 4  , 4 ); //for digest
		
	KeyEnt = KeyVals->Flink;
			
	while (KeyEnt != KeyVals )
	{
		KeyVal = CONTAINING_RECORD (KeyEnt , KEY_VALUE , Vals );
												
		if ( KeyVal->Flags & KEY_Formatted)			
		{	
			//Most Case			
			RtlCopyMemory (DataBuffer2 , KeyVal->FormatStr , KeyVal->Len );
			DataBuffer2 += KeyVal->Len;				
		}
		else
		{
			FmtVal = PiFormatKeyValue (KeyVal);
			RtlCopyMemory (DataBuffer2 , FmtVal->FormatStr , FmtVal->Len );
			DataBuffer2 += FmtVal->Len;
			ExFreePoolWithTag (FmtVal , USCSI_TAG);
		}
		KeyEnt = KeyEnt->Flink;			
	}

Out:

	if (DataLen)  
		*DataLen = Length;
		
	return DataBuffer;
}

#define KEY_EXTEND	20

//
//
//

BOOLEAN PiInitKeys()
{
	ULONG Count;
	Count = KEY_LastKey + 1 + KEY_EXTEND;
	
	PiKeys = ExAllocatePoolWithTag ( PagedPool , Count * sizeof (KEY_META) , USCSI_TAG );
	if ( PiKeys)
	{
		RtlCopyMemory ( PiKeys , PiPreKeys , (KEY_LastKey + 1)*sizeof (KEY_META));
		PiKeyIdx = KEY_LastKey + 1;
		PiKeyCaps = Count;
		return TRUE;
	}
	
	return FALSE;
}

//
//
//

BOOLEAN PiRegisterKey( PKEY_META Meta ,PULONG KeyNum)
{
	PKEY_META	Tmp;
	ULONG		Len;
	
	Len = strlen (Meta->Key) + 1;
	
	if (!Len)
		return FALSE;
	
	if ( Meta->Attrs & KEY_DEREGISTERED)
		return FALSE;
			
	if ( PiKeyIdx == PiKeyCaps )
	{
		Tmp = ExAllocatePoolWithTag ( PagedPool , 
				(PiKeyCaps + KEY_EXTEND) * sizeof (KEY_META) , USCSI_TAG );
		
		if ( !Tmp )
			return FALSE;
			
		RtlCopyMemory ( Tmp , PiKeys , PiKeyCaps * sizeof (KEY_META));
		
		ExFreePoolWithTag ( PiKeys , USCSI_TAG);
		PiKeys = Tmp;
		
		PiKeyCaps += KEY_EXTEND;
		
	}
	
	PiKeys[PiKeyIdx].Key = ExAllocatePoolWithTag ( PagedPool , Len , USCSI_TAG);
		
	if ( !PiKeys[PiKeyIdx].Key )
		return FALSE;
		
	RtlCopyMemory ( &PiKeys[PiKeyIdx] , Meta , sizeof (KEY_META));
	RtlCopyMemory ( PiKeys[PiKeyIdx].Key , Meta->Key , Len);
	
	if (KeyNum)
		*KeyNum = PiKeyIdx;
		
	PiKeyIdx++;
	return TRUE;
}

//
//
//

BOOLEAN PiDeregisterKey ( ULONG KeyNum)
{
	PiKeys[KeyNum].Attrs |= KEY_DEREGISTERED;
	return TRUE;
}

//
//
//

VOID PiDumpKey (PKEY_VALUE KeyVal)
{
	PKEY_VALUE	Tmp;
	
	if ( KeyVal->Flags & KEY_Formatted )
		PROT_Debug (("%s.\n" , KeyVal->FormatStr));
	else
	{
		Tmp = PiFormatKeyValue ( KeyVal );
		PROT_Debug (("%s.\n" , Tmp->FormatStr));
		ExFreePoolWithTag ( Tmp , USCSI_TAG);
	}
}


