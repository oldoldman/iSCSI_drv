
#include "precomp.h"

VOID PiProcessKey( PPDU Pdu , PuCONINFO ConInfo , BOOLEAN AllocatePDU , UCHAR Opcode );
	
PVOID PiFormatKeyValues(PLIST_ENTRY KeyVals , PULONG DataLen);
	
PPDU PiAllocateTxtPDU ( PuCONINFO ConInfo , PPDU Pdu , PUCHAR Data  , ULONG Len , UCHAR Opcode);
	
VOID PiSetKeyState ( KEY_NAME Key , PuCONINFO ConInfo , UCHAR State);
	
VOID PiProtError (PuCONINFO ConInfo , PUCHAR Func );
	
VOID PiCommitPending( PuCONINFO ConInfo );

VOID PiCalcNextStage(PuCONINFO ConInfo);

//
//Forward declaration
//
	
VOID PiAnswerPdu( PPDU Pdu , PuCONINFO ConInfo , UCHAR Opcode );
	
BOOLEAN PiCanLoginTransit (PuCONINFO ConInfo );
	
PPDU PiCollectStgParms(PuCONINFO ConInfo , BOOLEAN Answer);
	
VOID PiLoginComplete( PuCONINFO ConInfo );

#define PiAnswerLoginPdu( Pdu , ConInfo)	\
	PiAnswerPdu ( Pdu , ConInfo , OP_LOGIN_REQ)

#define PiLoginProcessKey(Pdu , ConInfo , AllocatePDU )	\
	PiProcessKey (Pdu , ConInfo , AllocatePDU , OP_LOGIN_REQ)

//
//
//
VOID PtProcessLogin( PPDU Pdu , PuCONINFO ConInfo)
{
	PPDU			NewPdu , CPdu , LPdu;
	PLIST_ENTRY		PduEnt , PduEnt2;
	ULONG			DataSegLen;

	DataSegLen = (Pdu->Bhs->GENERICBHS.DataSegmentLength[0] << 16 ) |
				 (Pdu->Bhs->GENERICBHS.DataSegmentLength[1] << 8 ) |
				 (Pdu->Bhs->GENERICBHS.DataSegmentLength[2] );
	
	DbgPrint(
	"%s Pdu=0x%p VersionMax=0x%X VersionActive=0x%X CSG=0x%X NSG=0x%X C=0x%X T=0x%X DataSegLen=0x%X\n" , 
	__FUNCTION__ , 
	Pdu ,
	Pdu->Bhs->LOGIN_RESPONSE.VersionMax,
	Pdu->Bhs->LOGIN_RESPONSE.VersionActive,
	Pdu->Bhs->LOGIN_RESPONSE.CSG,
	Pdu->Bhs->LOGIN_RESPONSE.NSG,
	Pdu->Bhs->LOGIN_RESPONSE.Continue,
	Pdu->Bhs->LOGIN_RESPONSE.Transit,
	DataSegLen);
	//
	// There are some errors
	//	
	if ( Pdu->Bhs->LOGIN_RESPONSE.StatusClass )
	{
		/*DbgPrint("%s Status=0x%X%X\n", 
		__FUNCTION__ ,Pdu->Bhs->LOGIN_RESPONSE.StatusClass,Pdu->Bhs->LOGIN_RESPONSE.StatusDetail);
		*/
		ConInfo->LoginState = LOGIN_STAT_IDLE;
		goto Error;
	}
	//
	// Continue sending Initiator Logical PDU if any
	//
	if ( ConInfo->Answer )
	{
#ifdef STRICT_PROT_CHECK
		if ( DataSegLen )
			PiProtError ( ConInfo , __FUNCTION__ );	// This must be a SYNC Pdu
#endif
		goto Answer;
	}
	//
	// Part of Target Logical PDU
	//
	/*An initiator receiving a Text or Login
 	Response with the C bit set to 1 MUST answer with a Text or Login
 	Request with no data segment (DataSegmentLength 0).*/	
	if ( Pdu->Bhs->LOGIN_RESPONSE.Continue )
	{
#ifdef STRICT_PROT_CHECK
		if ( Pdu->Bhs->LOGIN_RESPONSE.Transit )
			PiProtError( ConInfo , __FUNCTION__ );
#endif
		if ( !ConInfo->Logical )
			// Head
			ConInfo->Logical = Pdu;	
		else
			// Part of
			InsertTailList ( &ConInfo->Logical->Continue, &Pdu->Continue );
		goto Answer;
	}
	// Logical PDU completed
	else if ( ConInfo->Logical )
	{		
		LPdu = ConInfo->Logical;
		ConInfo->Logical = NULL;
	}
	else
		LPdu = Pdu;
	//
	// Process Target Logical PDU
	//	
	PiLoginProcessKey ( LPdu , 
					ConInfo , 
					/*A Login Response with a T bit set to 1 MUST NOT contain key=value
 					pairs that may require additional answers from the initiator within
 					the same stage*/
					!Pdu->Bhs->LOGIN_RESPONSE.Transit );
#ifdef STRICT_PROT_CHECK		
	if (ConInfo->LoginState == LOGIN_STAT_STARTED && 
		Pdu->Bhs->LOGIN_RESPONSE.Transit )
	{
		PiProtError( ConInfo , __FUNCTION__ );
	}
	else if 
#else
	if
#endif
	(ConInfo->LoginState == LOGIN_STAT_STARTED && 
	 !Pdu->Bhs->LOGIN_RESPONSE.Transit)
	{
		if ( PiCanLoginTransit (ConInfo) )
		{			
			PiCalcNextStage( ConInfo );
			PiCommitPending( ConInfo );			
			ConInfo->LoginState = LOGIN_STAT_PENDING_TRANSIT;
			/*DbgPrint("%s Stage Transition 0x%X -> 0x%X Proposed\n" , 
			__FUNCTION__ , ConInfo->CSG , ConInfo->NSG);*/
		}
	}
	else if ( ConInfo->LoginState == LOGIN_STAT_PENDING_TRANSIT &&
			  Pdu->Bhs->LOGIN_RESPONSE.Transit )
	{		
		if ( LOGIN_STAGE_FullFeaturePhase == Pdu->Bhs->LOGIN_RESPONSE.NSG )
		{
			// Final Login Response
			//DbgPrint("%s Login Successfully CID=0x%X\n" , __FUNCTION__ , ConInfo->CID);
			
			ConInfo->LoginState = LOGIN_STAT_IDLE;
			ConInfo->State = CON_STAT_LOGGED_IN;
			
			if ( IsLeadingCon(ConInfo) )
			{
				ConInfo->Session->State = SESSION_STAT_LOGGED_IN;
				REVERSE_BYTES_SHORT( &ConInfo->Session->TSIH , 
										&Pdu->Bhs->LOGIN_RESPONSE.TSIH);
			}
			
			goto LoginComplete;
		}
		/*
		DbgPrint("%s Stage Transition 0x%X -> 0x%X Agreed\n" , 
		__FUNCTION__, ConInfo->CSG , Pdu->Bhs->LOGIN_RESPONSE.NSG);*/
			
		ConInfo->CSG = Pdu->Bhs->LOGIN_RESPONSE.NSG;			
		PiCalcNextStage(ConInfo);
			
		ConInfo->LoginState = LOGIN_STAT_STARTED;
		//
		// Prepare next stage nego parms
		//
		PiCollectStgParms ( ConInfo , TRUE);
	}
	else if ( ConInfo->LoginState == LOGIN_STAT_PENDING_TRANSIT &&
				!Pdu->Bhs->LOGIN_RESPONSE.Transit )
	{
		/*
		5.3.3
		If the target responds to a Login Request that has the T bit set to 1
   		with a Login Response that has the T bit set to 0, the initiator
   		should keep sending the Login Request (even empty) with the T bit set
   		to 1, while it still wants to switch stage, until it receives the
   		Login Response that has the T bit set to 1 or it receives a key that
  		requires it to set the T bit to 0.
		*/
		//DbgPrint("%s Stage Transition Pending\n" , __FUNCTION__);
	}	
	//
	//
	//
Answer:

	PiAnswerLoginPdu ( Pdu , ConInfo );	
	return;
		
Error:	
	//TODO : may need to free the remaining Initiator's Logical PDU
LoginComplete:

	PiLoginComplete(ConInfo);		
	return;	
}

BOOLEAN PiInitiatorName( PUCHAR *Name);
	
BOOLEAN PiInitiatorAlias( PUCHAR *Alias);
	
BOOLEAN PiAuthMethod( PUCHAR *Method);
	
VOID PiDumpKey (PKEY_VALUE KeyVal);


#define SETNEGOSTATE(Key,ConInfo)					\
	if ( (PiKeys[Key].Attrs & KEY_DECLARATIVE) )	\
		PiSetKeyState( Key , ConInfo , KEY_Agreed);	\
	else											\
		PiSetKeyState( Key , ConInfo , KEY_Proposed)

/*
   This function will ALWAYS allocate at least 1 PDU ,
   even if there should be no nego parms , in which case , 
   the PDU function as a sync signal
*/
		
PPDU PiCollectStgParms(PuCONINFO 	ConInfo , BOOLEAN 	Answer)
{
	PPDU			NewPdu = NULL;
	PKEY_VALUE		*Parm;
	PUCHAR			Val , DataOut;
	ULONG			Len , DataLen;
	LIST_ENTRY		ParmList;
	PLIST_ENTRY		ParmEnt;	
	BOOLEAN			Dynamic;
	
	InitializeListHead ( &ParmList );
	//
	// Security Nego
	//
	if (ConInfo->CSG == LOGIN_STAGE_SecurityNegotiation )
	{		

		Parm = AddrKV(ConInfo , KEY_InitiatorName);
		if (!*Parm)
		{
			Dynamic = PiInitiatorName(&Val);
			if (Dynamic)
			{		
				Len = strlen (Val) + 1;		
				*Parm = PiAllocateKeyValStr0( Len );
				RtlCopyMemory (KEY_IV(*Parm).String , Val , Len-1 );
				*(KEY_IV(*Parm).String + Len-1 ) = '\0';
				ExFreePoolWithTag (Val , USCSI_TAG);
			}
			else
			{
				*Parm = PiAllocateKeyValue();
				KEY_IV(*Parm).String = Val;
			}
			
		}		
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_InitiatorName;
		SETNEGOSTATE( KEY_InitiatorName , ConInfo);		
		InsertTailList (&ParmList , &(*Parm)->Vals );
		
		Parm = AddrKV(ConInfo , KEY_SessionType);
		if (!*Parm)
			*Parm = PiAllocateKeyValue();
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_SessionType;
		SETNEGOSTATE( KEY_SessionType , ConInfo);	
		InsertTailList (&ParmList ,&(*Parm)->Vals );
					
		if ( ConInfo->Session->NormalSession )
		{	
			KEY_IV(*Parm).String = "Normal";		
			
			Parm = AddrKV(ConInfo , KEY_TargetName);
			if (!*Parm)
				*Parm = PiAllocateKeyValue();
			(*Parm)->Flags = 0;
			(*Parm)->Key.D = KEY_TargetName;
			SETNEGOSTATE( KEY_TargetName , ConInfo);	
			KEY_IV(*Parm).String = ConInfo->Session->Tgt->Tgt.TargetName;
			InsertTailList (&ParmList ,&(*Parm)->Vals );
		}
		else
			KEY_IV(*Parm).String = "Discovery";
		
		Parm = AddrKV(ConInfo , KEY_InitiatorAlias);
		if (!*Parm)
		{
			Dynamic = PiInitiatorAlias ( &Val);
			if (Dynamic)
			{
				Len = strlen (Val) + 1;		
				*Parm = PiAllocateKeyValStr0( Len );
				RtlCopyMemory (KEY_IV(*Parm).String , Val , Len-1 );
				*(KEY_IV(*Parm).String + Len-1 ) = '\0';
				ExFreePoolWithTag (Val , USCSI_TAG);
			}
			else
			{
				*Parm = PiAllocateKeyValue();
				KEY_IV(*Parm).String = Val;
			}			
		}
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_InitiatorAlias;
		SETNEGOSTATE( KEY_InitiatorAlias , ConInfo);	
		InsertTailList (&ParmList ,&(*Parm)->Vals );
		
		Parm = AddrKV(ConInfo , KEY_AuthMethod);
		if (!*Parm)
		{
			Dynamic = PiAuthMethod ( &Val);
			if (Dynamic)
			{
				Len = strlen (Val) + 1;		
				*Parm = PiAllocateKeyValStr0( Len );
				RtlCopyMemory (KEY_FV(*Parm).String , Val , Len-1 );
				*(KEY_FV(*Parm).String + Len-1 ) = '\0';
				ExFreePoolWithTag (Val , USCSI_TAG);
			}
			else
			{
				*Parm = PiAllocateKeyValue();
				KEY_FV(*Parm).String = Val;
			}			
		}
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_AuthMethod;
		SETNEGOSTATE( KEY_AuthMethod , ConInfo);	
		InsertTailList (&ParmList ,&(*Parm)->Vals );
		
		
	}
	//
	// Operational Nego
	//
	if ( ConInfo->CSG == LOGIN_STAGE_LoginOperationalNegotiation )
	{
	
		Parm = AddrKV(ConInfo , KEY_MaxConnections);
		
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_MaxConnections;
		SETNEGOSTATE( KEY_MaxConnections , ConInfo);	
		InsertTailList (&ParmList ,&(*Parm)->Vals );
		
		Parm = AddrKV(ConInfo , KEY_HeaderDigest);
		
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_HeaderDigest;
		SETNEGOSTATE( KEY_HeaderDigest , ConInfo);	
		InsertTailList (&ParmList ,&(*Parm)->Vals );
		
		Parm = AddrKV(ConInfo , KEY_DataDigest);
		
		(*Parm)->Flags = 0;
		(*Parm)->Key.D = KEY_DataDigest;
		SETNEGOSTATE( KEY_DataDigest , ConInfo);	
		InsertTailList (&ParmList ,&(*Parm)->Vals );
		
		
	}
	//
	// Full Feature Nego
	//	
	if ( ConInfo->CSG == LOGIN_STAGE_FullFeaturePhase )
	{
		
	}
	
	DataOut = PiFormatKeyValues( &ParmList , &DataLen);
	if (!DataOut)
		goto Out;
	/*	
	{		
		PUCHAR Tmp = DataOut ,End = DataOut + DataLen - 1;
		do
		{
			DbgPrint("Parm : %s\n" ,Tmp);
			while( *Tmp++ != '\0');
		}while( Tmp < End );
	}*/
	
	NewPdu = PiAllocateTxtPDU (	ConInfo , 
								NULL , 
								DataOut , 
								DataLen , 
								OP_LOGIN_REQ);
	
	if ( !NewPdu )							
		goto Out;
		
	ParmEnt = ParmList.Flink;
	while ( ParmEnt != &ParmList )
	{
		PKEY_VALUE	Answer;
		Answer = CONTAINING_RECORD (ParmEnt , KEY_VALUE , Vals);
		KEYOUT(Answer);
		ParmEnt = ParmEnt->Flink;		
	}
	
	if (Answer)
		ConInfo->Answer = NewPdu;
		
	return NewPdu;

Out:

	if ( DataOut )
		ExFreePoolWithTag (DataOut , USCSI_TAG);
		
	return NewPdu;
}

//

VOID PiCalcNextStage(PuCONINFO ConInfo)
{
	if ( LOGIN_STAGE_SecurityNegotiation == ConInfo->CSG )
	{
		if (ConInfo->SkipOperationalNegotiation)
			ConInfo->NSG = LOGIN_STAGE_FullFeaturePhase;
		else
			ConInfo->NSG = LOGIN_STAGE_LoginOperationalNegotiation;
	}
	else if ( LOGIN_STAGE_LoginOperationalNegotiation == ConInfo->CSG )
		ConInfo->NSG = LOGIN_STAGE_FullFeaturePhase;
}
/*
   Logic for a login stage transition
*/
BOOLEAN PiCanLoginTransit (PuCONINFO ConInfo )
{
	if ( ConInfo->MoreWorks )
	{
		ConInfo->MoreWorks = FALSE;
		return FALSE;
	}
	
	return TRUE;		
}


VOID PiLoginComplete( PuCONINFO ConInfo )
{
	KeSetEvent ( &ConInfo->LoginEvent , IO_NO_INCREMENT , FALSE );
}

PPDU PiAllocateCmdPDU (
PuCONINFO ConInfo , PUCHAR DataBuf , ULONG DataSize , ULONG ReadSize , UCHAR Direction);

//
//
//

NTSTATUS PtLogin( PuCONINFO ConInfo )
{
	PPDU 			Login , CPdu;
	PLIST_ENTRY		PduEnt , PduEnt2;
	BOOLEAN			LeadingCon;
	
	ConInfo->State = CON_STAT_IN_LOGIN;
		
	ConInfo->CSG = ConInfo->SSG;	
	ConInfo->LoginState = LOGIN_STAT_STARTED;	
	ConInfo->SkipOperationalNegotiation = 1;
	
	Login = PiCollectStgParms (ConInfo , FALSE);
		
	PduEnt2 = PduEnt = &Login->Continue;	
	LeadingCon = IsLeadingCon( ConInfo );
	
	do
	{	
		CPdu = CONTAINING_RECORD (PduEnt2 , PDU , Continue);
		
		REVERSE_BYTES_SHORT (&CPdu->Bhs->LOGIN_REQUEST.CID , &ConInfo->CID);
		
		if ( !LeadingCon )
			REVERSE_BYTES_SHORT ( &CPdu->Bhs->LOGIN_REQUEST.TSIH , 
								  &ConInfo->Session->TSIH);
								  
		CPdu->Bhs->LOGIN_REQUEST.CSG = ConInfo->CSG;
		CPdu->Bhs->LOGIN_REQUEST.ISID.Type = 0x00;
		CPdu->Bhs->LOGIN_REQUEST.ISID.B[0]=0x12;
		CPdu->Bhs->LOGIN_REQUEST.ISID.B[1]=0x34;
		
		PduEnt2 = PduEnt2->Flink;
			
	} while ( PduEnt2 != PduEnt );
	
	TpQueueOutPDU ( ConInfo->ConCtx , Login);
	
	KeWaitForSingleObject ( &ConInfo->LoginEvent , Executive , KernelMode , FALSE , NULL );

	// Test
	//PtNopOut( ConInfo , TRUE );
					
	if ( CON_STAT_LOGGED_IN == ConInfo->State )
		return STATUS_SUCCESS;
	else
		return STATUS_UNSUCCESSFUL;
}