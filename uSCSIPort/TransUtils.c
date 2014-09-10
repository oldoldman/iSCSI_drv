
#include "precomp.h"

#define TCP_DEVICE	L"\\Device\\Tcp"

NTSTATUS TiReceive(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  BytesIndicated,
    IN ULONG  BytesAvailable,
    OUT ULONG  *BytesTaken,
    IN PVOID  Tsdu,
    OUT PIRP  *IoRequestPacket);

//
//
//
    
NTSTATUS TiOpenAddress ( PuCONINFO Coninfo )
{
	NTSTATUS						Status;	
	OBJECT_ATTRIBUTES				Attrs;
	UNICODE_STRING					Tcp;
	PFILE_FULL_EA_INFORMATION 		Ea;	
	PTA_IP_ADDRESS					IpAddr;
	ULONG							EaSize;	
	PIRP							Irp;
	IO_STATUS_BLOCK					IoStatus;	
	KEVENT							Event;
	
	Status = STATUS_SUCCESS;
	
	RtlInitUnicodeString ( &Tcp , TCP_DEVICE );
		
	InitializeObjectAttributes ( &Attrs , 
								 &Tcp , 
								 OBJ_CASE_INSENSITIVE |
								 OBJ_KERNEL_HANDLE , 
								 NULL , 
								 NULL );

	EaSize = sizeof (FILE_FULL_EA_INFORMATION) + 
				TDI_TRANSPORT_ADDRESS_LENGTH + 
				sizeof (TA_IP_ADDRESS);			
	Ea = ExAllocatePoolWithTag ( PagedPool , EaSize , USCSI_TAG);	
	if ( Ea )
	{
		RtlZeroMemory ( Ea , EaSize );
		
		Ea->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
		RtlCopyMemory( &Ea->EaName , TdiTransportAddress , Ea->EaNameLength );
				
		Ea->EaValueLength = sizeof (TA_IP_ADDRESS);		
		IpAddr = (PTA_IP_ADDRESS)((PUCHAR)Ea->EaName + Ea->EaNameLength + 1);
		IpAddr->TAAddressCount = 1;
		IpAddr->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
		IpAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;		
	}
	else
	{	
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto ErrorOut;		
	}
	
	Status = ZwCreateFile ( &Coninfo->Addr , 
							FILE_READ_EA | FILE_WRITE_EA,
							&Attrs,
							&IoStatus,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							0,
							FILE_OPEN,
							0,
							Ea,
							EaSize);	
	if (!NT_SUCCESS(Status))
		goto ErrorOut;

	Status = ObReferenceObjectByHandle ( Coninfo->Addr , 
										 0,
										 *IoFileObjectType,
										 KernelMode,
										 &Coninfo->AddrObj,
										 NULL );
	if (!NT_SUCCESS(Status))
		goto ErrorOut;
	
	ExFreePoolWithTag ( Ea , USCSI_TAG );
	
	return Status;
	
ErrorOut:

	if (Coninfo->Addr)
		ZwClose ( Coninfo->Addr );

	if (Coninfo->AddrObj )
		ObDereferenceObject ( Coninfo->AddrObj );

	if ( Ea )
		ExFreePoolWithTag ( Ea , USCSI_TAG );

	return Status;
}

//
//
//

NTSTATUS TiOpenConnectionEndpoint( PuCONINFO Coninfo)
{
	NTSTATUS					Status;
	UNICODE_STRING				Tcp;
	OBJECT_ATTRIBUTES			Attrs;
	PFILE_FULL_EA_INFORMATION	Ea;
	ULONG						EaSize;	
	PIRP						Irp;
	IO_STATUS_BLOCK				IoStatus;
	KEVENT						Event;
	
	Status = STATUS_SUCCESS;
	
	RtlInitUnicodeString ( &Tcp , TCP_DEVICE );
	
	InitializeObjectAttributes ( &Attrs , 
								 &Tcp , 
								 OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE , 
								 NULL , 
								 NULL );
	
	EaSize = sizeof (FILE_FULL_EA_INFORMATION) + 
				TDI_CONNECTION_CONTEXT_LENGTH +
				sizeof (PuCON_CTX);
	Ea = ExAllocatePoolWithTag ( PagedPool , EaSize , USCSI_TAG);
	if ( Ea )
	{
		PuCON_CTX *Ctx;
		RtlZeroMemory (Ea , EaSize);
		Ea->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
		RtlCopyMemory( &Ea->EaName , TdiConnectionContext , Ea->EaNameLength );		

		Ea->EaValueLength = sizeof (PuCON_CTX);	
		Ctx = (PuCON_CTX*)((PUCHAR)Ea->EaName + Ea->EaNameLength + 1);
		*Ctx = Coninfo->ConCtx;
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto ErrorOut;
	}
	Status = ZwCreateFile ( &Coninfo->Ep , 
							FILE_READ_EA | FILE_WRITE_EA,
							&Attrs,
							&IoStatus,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							0,
							FILE_OPEN,
							0,
							Ea,
							EaSize);
	if (!NT_SUCCESS(Status))
		goto ErrorOut;
		
	Status = ObReferenceObjectByHandle ( Coninfo->Ep , 
										 0,
										 *IoFileObjectType,
										 KernelMode,
										 &Coninfo->ConEpObj,
										 NULL );
	if (!NT_SUCCESS(Status))
		goto ErrorOut;
	
	ExFreePoolWithTag ( Ea , USCSI_TAG );
	 
	KdPrintEx((DPFLTR_USCSI,DBG_TDI,"%s DevObj 0x%p\n" , __FUNCTION__ , IoGetRelatedDeviceObject(Coninfo->ConEpObj)));
	
	return Status;

ErrorOut:

	if ( Coninfo->Ep )
		ZwClose (Coninfo->Ep);

	if ( Coninfo->ConEpObj )
		ObDereferenceObject ( Coninfo->ConEpObj );

	if ( Ea )
		ExFreePoolWithTag ( Ea , USCSI_TAG ); 

	return Status;
}

//
//
//

NTSTATUS TiOpenControl( PuCONINFO Coninfo)
{
	NTSTATUS					Status;
	UNICODE_STRING				Tcp;
	OBJECT_ATTRIBUTES			Attrs;
	PIRP						Irp;
	IO_STATUS_BLOCK				IoStatus;
	KEVENT						Event;
	
	Status = STATUS_SUCCESS;
	
	RtlInitUnicodeString ( &Tcp , TCP_DEVICE );	
	
	InitializeObjectAttributes ( &Attrs , 
								 &Tcp , 
								 OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE , 
								 NULL , 
								 NULL );
	
	Status = ZwCreateFile ( &Coninfo->Ctl , 
							FILE_GENERIC_READ | FILE_GENERIC_WRITE,
							&Attrs,
							&IoStatus,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							0,
							FILE_OPEN,
							0,
							NULL,
							0 );
	if (!NT_SUCCESS(Status))
		goto ErrorOut;
		
	Status = ObReferenceObjectByHandle ( Coninfo->Ctl , 
										 0,
										 *IoFileObjectType,
										 KernelMode,
										 &Coninfo->CtlObj,
										 NULL );
	if (!NT_SUCCESS(Status))
		goto ErrorOut;
	
	return Status;
	
ErrorOut:

	if ( Coninfo->Ctl )
		ZwClose (Coninfo->Ctl);

	if ( Coninfo->CtlObj )
		ObDereferenceObject ( Coninfo->CtlObj );

	return Status;
}

//
//
//

NTSTATUS TiAssociate( PuCONINFO Coninfo )
{
	NTSTATUS			Status;
	PIRP				Irp;
	KEVENT				Event;
	IO_STATUS_BLOCK		IoStatus;
	PDEVICE_OBJECT		DevObj;
	
	Status = STATUS_SUCCESS;
	
	DevObj = IoGetRelatedDeviceObject (Coninfo->ConEpObj);
	
	Irp = TdiBuildInternalDeviceControlIrp (TDI_ASSOCIATE_ADDRESS,
											DevObj,
											NULL , 
											&Event,
											&IoStatus);
	if ( Irp )
	{
		TdiBuildAssociateAddress (Irp , 
								DevObj,
								Coninfo->ConEpObj,
								NULL,
								NULL,
								Coninfo->Addr);
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto ErrorOut;
	}
	
	KeInitializeEvent ( &Event , NotificationEvent , FALSE);
	Status = IoCallDriver ( DevObj , Irp);	
	if ( STATUS_PENDING == Status )
	{
		KeWaitForSingleObject ( &Event , Executive , KernelMode , TRUE , NULL );
		Status = Irp->IoStatus.Status;
	}

	return Status;

ErrorOut:

	return Status;
}

//
//
//

NTSTATUS TiQueryTransportInfo ( IN PuCONINFO Coninfo , IN ULONG InfoType)
{
	NTSTATUS					Status;
	PIRP						Irp;
	KEVENT						Event;
	IO_STATUS_BLOCK				IoStatus = {0};
	PFILE_OBJECT				FileObj;
	PDEVICE_OBJECT				DevObj;
	PVOID						MdlAddr;
	ULONG						MdlAddrLen;
	PMDL						Mdl;
	
	switch (InfoType)
	{
		case TDI_QUERY_CONNECTION_INFO:
			DevObj = IoGetRelatedDeviceObject (Coninfo->ConEpObj);
			FileObj = Coninfo->ConEpObj;
			MdlAddr = &Coninfo->ConnectionInfo;
			MdlAddrLen = sizeof (Coninfo->ConnectionInfo);
			break;
			
		case TDI_QUERY_PROVIDER_INFO:		
			DevObj = IoGetRelatedDeviceObject (Coninfo->CtlObj);
			FileObj = Coninfo->CtlObj;
			MdlAddr = &Coninfo->ProviderInfo;
			MdlAddrLen = sizeof (Coninfo->ProviderInfo);
			break;
			
		default:
			Status = STATUS_INVALID_PARAMETER;
			goto Out;
	}
	
	Irp = 
		TdiBuildInternalDeviceControlIrp( TDI_QUERY_INFORMATION, DevObj, NULL, &Event, &IoStatus );
	
	if ( Irp )
	{
		Mdl = IoAllocateMdl( MdlAddr , MdlAddrLen , FALSE , FALSE , Irp);				
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Out;
	}
	
	if ( Mdl )
	{
		__try
		{
			MmProbeAndLockPages ( Mdl , KernelMode , IoReadAccess );
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("%s Exception 0x%X" , __FUNCTION__ , GetExceptionCode());
		}
		
		TdiBuildQueryInformation(Irp, DevObj, FileObj, NULL, NULL, InfoType, Mdl);
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Out;
	}

	KeInitializeEvent ( &Event , NotificationEvent , FALSE);

	Status = IoCallDriver ( DevObj , Irp );
	if ( STATUS_PENDING == Status )
	{
		KeWaitForSingleObject ( &Event , Executive , KernelMode , FALSE , NULL );
		Status = Irp->IoStatus.Status;
	}
	
	if ( NT_SUCCESS(Status) )
	{
		switch ( InfoType )
		{
			case TDI_QUERY_PROVIDER_INFO:
				DbgPrint(
				"Version=0x%X MaxSendSize=0x%X MaxConnectionUserData=0x%X MaxDatagramSize=0x%X ServiceFlags=0x%X MinimumLookaheadData=0x%X MaximumLookaheadData=0x%X NumberOfResources=0x%X\n",
				Coninfo->ProviderInfo.Version,
				Coninfo->ProviderInfo.MaxSendSize,
				Coninfo->ProviderInfo.MaxConnectionUserData,
				Coninfo->ProviderInfo.MaxDatagramSize,
				Coninfo->ProviderInfo.ServiceFlags,
				Coninfo->ProviderInfo.MinimumLookaheadData,
				Coninfo->ProviderInfo.MaximumLookaheadData,
				Coninfo->ProviderInfo.NumberOfResources);
				break;
		}		
	}
		
Out:
	return Status;
}

//
//
//

NTSTATUS TiTransportError (__in PVOID TdiEventCtx ,__in NTSTATUS Status)
{
	PuCONINFO	ConInfo;
	ConInfo = (PuCONINFO)TdiEventCtx;
	KdPrintEx((DPFLTR_USCSI,DBG_TRANSPORT,"%s - 0x%X\n" , __FUNCTION__ , Status));
	return STATUS_SUCCESS;
}

//
//
//

NTSTATUS TiTransportDisconnect(
    __in PVOID  				TdiEventContext,
    __in CONNECTION_CONTEXT  	ConnectionContext,
    __in LONG  					DisconnectDataLength,
    __in PVOID  				DisconnectData,
    __in LONG  					DisconnectInformationLength,
    __in PVOID  				DisconnectInformation,
    __in ULONG  				DisconnectFlags)
{
	PuCONINFO	ConInfo;
	ConInfo = (PuCONINFO)TdiEventContext;
	
	KdPrintEx((DPFLTR_USCSI,DBG_TDI,
	"%s ConInfo 0x%p DisconnectDataLength 0x%X DisconnectInformationLength 0x%X DisconnectFlags 0x%X\n" , 
	__FUNCTION__ , 
	ConInfo , 
	DisconnectDataLength,
	DisconnectInformationLength,
	DisconnectFlags));
	
	ConInfo->State = CON_STAT_UNINITIALIZED;
	
	return STATUS_SUCCESS;
}

//
//
//
    	    
NTSTATUS TiRegisterCallBack (PuCONINFO Coninfo , LONG EventType , PVOID ClientCallBack,PVOID Ctx)
{
	NTSTATUS			Status;
	PIRP				Irp;
	KEVENT				Event;
	IO_STATUS_BLOCK		IoStatus = {0};
	PDEVICE_OBJECT		DevObj;
	
	Status = STATUS_SUCCESS;
	
	DevObj = IoGetRelatedDeviceObject ( Coninfo->AddrObj );
	
	KeInitializeEvent ( &Event , NotificationEvent , FALSE);
	
	Irp = TdiBuildInternalDeviceControlIrp(	TDI_SET_EVENT_HANDLER,
											DevObj,
											NULL,
											&Event,
											&IoStatus );
	if ( Irp )
	{
		TdiBuildSetEventHandler ( Irp , 
								  DevObj,
								  Coninfo->AddrObj,
								  NULL,
								  NULL,
								  EventType,
								  ClientCallBack,
								  Ctx );
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		return Status;
	}
	
	Status = IoCallDriver ( DevObj , Irp );
	
	if ( STATUS_PENDING == Status )
	{
		KeWaitForSingleObject ( &Event , Executive , KernelMode , FALSE , NULL );
		Status = Irp->IoStatus.Status;
	}
	
	return Status;
}

VOID PiPduReleaser(IN PVOID Parameter);

//
// Session
//

VOID TiInitSession (PSESSION Session)
{
	NTSTATUS	Status;
	ULONG		i;
	
	for ( i = 0 ; i < KEY_MaxSessionKey ; i++ )
	{
		if ( ! (PiKeys[i].Attrs & KEY_TYPE_STRING) )
		{
			Session->Parameter[i] = PiAllocateKeyValue();
			Session->Parameter[i]->Key.D = i;
			Session->Parameter[i]->Pending = KEY_P;
		}
		else
			Session->Parameter[i] = NULL;
	}
	
	KEY_FV(Session->Parameter[KEY_ImmediateData]).Bool = TRUE;
	Session->Parameter[KEY_ImmediateData]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_InitialR2T]).Bool = TRUE;	
	Session->Parameter[KEY_InitialR2T]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_DataPDUInOrder]).Bool = TRUE;	
	Session->Parameter[KEY_DataPDUInOrder]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_DataSequenceInOrder]).Bool = TRUE;	
	Session->Parameter[KEY_DataSequenceInOrder]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_MaxConnections]).Number = 1;	
	Session->Parameter[KEY_MaxConnections]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_MaxBurstLength]).Number = 262144;	
	Session->Parameter[KEY_MaxBurstLength]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_FirstBurstLength]).Number = 262144;	
	Session->Parameter[KEY_FirstBurstLength]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_DefaultTime2Wait]).Number = 2;	
	Session->Parameter[KEY_DefaultTime2Wait]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_DefaultTime2Retain]).Number = 20;	
	Session->Parameter[KEY_DefaultTime2Retain]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_MaxOutstandingR2T]).Number = 1;	
	Session->Parameter[KEY_MaxOutstandingR2T]->Flags |= KEY_Defaulted;
	
	KEY_FV(Session->Parameter[KEY_ErrorRecoveryLevel]).Number = 0;	
	Session->Parameter[KEY_ErrorRecoveryLevel]->Flags |= KEY_Defaulted;
	
	Session->Ns100Unit = KeQueryTimeIncrement();
	
	Session->CID = 0;	
	Session->NormalSession = TRUE;
	
	InitializeListHead( &Session->Connections);	
	KeInitializeSpinLock ( &Session->WindowLock );	
	KeInitializeSpinLock ( &Session->ConLock );	
			
	KeInitializeEvent ( &Session->WindowEvent , NotificationEvent , FALSE);
	//
	InitializeListHead( &Session->PduRelease);	
	KeInitializeEvent( &Session->PduReleaseEvent , NotificationEvent , FALSE);
	KeInitializeSpinLock ( &Session->PduReleaseLock );
	Status = PsCreateSystemThread( &Session->PduReleaser ,
									0 ,
									NULL ,
									NULL ,
									NULL ,
									PiPduReleaser ,
									Session );
}

//
//
//

VOID TpFreeSession (PSESSION Session)
{
	KEY_NAME	i;
	for ( i = 0 ; i < KEY_MaxSessionKey ; i++ )
	{
		if ( Session->Parameter[i] )
			ExFreePoolWithTag ( Session->Parameter[i] ,USCSI_TAG);
	}
	ExFreePoolWithTag ( Session , USCSI_TAG);
}

//
// Allocate an uCONINFO from NonPagedPool
//

PuCONINFO TiAllocateConInfo()
{
	ULONG		Size;
	PuCONINFO	ConInfo;
	
	Size = sizeof (uCONINFO) + sizeof (uCON_CTX);
	//
	// uCONINFO will be accessed from TDI ClientCallbacks
	//
	ConInfo = ExAllocatePoolWithTag ( NonPagedPool , Size , USCSI_TAG );
	
	if (ConInfo)
	{
		RtlZeroMemory ( ConInfo , Size );
		ConInfo->ConCtx = (PuCON_CTX)(ConInfo + 1);	
		ConInfo->ConCtx->ConInfo = ConInfo;	
	}
		
	return ConInfo;
}

NTSTATUS TiInitializeTransport( PuCONINFO Coninfo);

VOID TiSender(IN PVOID Parameter);
VOID TiDispatcher(IN PVOID Parameter);
	
VOID PiR2TWorker(IN PVOID Parameter);
VOID PiDataInWorker(IN PVOID Parameter);
VOID PiRespsWorker(IN PVOID Parameter);
VOID PiPendingCompleteCmdWorker(IN PVOID Parameter);

//
//
//

NTSTATUS TiInitWorkers (PuCONINFO ConInfo)
{
	NTSTATUS				Status;
	PuCON_CTX				Ctx;
	PWORKER_THREAD_CTX		Tctx;
	UCHAR					k , Cnt , Cnt2;
	KAFFINITY				Aff;
	
	Status = STATUS_SUCCESS;
	Ctx = ConInfo->ConCtx;
		
	KeInitializeEvent( &Ctx->DispatchEvent , NotificationEvent , FALSE );
	KeInitializeEvent( &Ctx->OutEvent , NotificationEvent , FALSE );
	KeInitializeEvent( &Ctx->R2TEvent , NotificationEvent , FALSE );
	KeInitializeEvent( &Ctx->DataInEvent , NotificationEvent , FALSE );
	KeInitializeEvent( &Ctx->PendingCompleteCmdEvent , NotificationEvent , FALSE );
	
	Ctx->DispatcherCount 	= 0;
	Ctx->SenderCount 		= 0;
	Ctx->R2TWorkerCount		= 0;
	Ctx->DataInWorkerCount	= 0;
	
	Aff = KeQueryActiveProcessors();
	k 	= 0;
	Cnt = 0;	
	while ( Aff >>= k++ )
		if ( Aff & 0x1)
			Cnt++;
	//
	//Create Dispatcher Thread
	//
	
	Cnt2 = min ( Cnt, CTX_MAX_DISPATCH_WORKERS );
	for ( k = 0 ; k < Cnt2 ; k++ )
	{		
		Tctx = ExAllocatePoolWithTag ( PagedPool , sizeof (*Tctx) , USCSI_TAG );
		
		if ( Tctx )
		{
			Tctx->Ctx 	= Ctx;
			Tctx->No 	= k;
			Status = PsCreateSystemThread( &Ctx->Dispatchers[k] ,
										0 ,
										NULL ,
										NULL ,
										NULL ,
										TiDispatcher ,
										Tctx );
			if ( NT_SUCCESS(Status))
				Ctx->DispatcherCount++;
			else
				ExFreePoolWithTag ( Tctx , USCSI_TAG );
		}
	}
	
	if ( !Ctx->DispatcherCount )
		goto ErrorOut;
	
	//
	//Create Sender Thread
	//
	
	Cnt2 = min ( Cnt, CTX_MAX_SEND_WORKERS );
	for ( k = 0 ; k < Cnt2 ; k++ )
	{		
		Tctx = ExAllocatePoolWithTag ( PagedPool , sizeof (*Tctx) , USCSI_TAG );
		
		if ( Tctx )
		{
			Tctx->Ctx 	= Ctx;
			Tctx->No 	= k;
			Status = PsCreateSystemThread( &Ctx->Senders[k] ,
										0 ,
										NULL ,
										NULL ,
										NULL ,
										TiSender ,
										Tctx );
			if ( NT_SUCCESS(Status))
				Ctx->SenderCount++;
			else
				ExFreePoolWithTag ( Tctx , USCSI_TAG );
		}
	}
	
	if ( !Ctx->SenderCount )
		goto ErrorOut;
		
	//
	//Create R2T Thread
	//
	
	Cnt2 = min ( Cnt, CTX_MAX_R2T_WORKERS );
	for ( k = 0 ; k < Cnt2 ; k++ )
	{		
		Tctx = ExAllocatePoolWithTag ( PagedPool , sizeof (*Tctx) , USCSI_TAG );
		
		if ( Tctx )
		{
			Tctx->Ctx 	= Ctx;
			Tctx->No 	= k;
			Status = PsCreateSystemThread( &Ctx->R2TWorkers[k] ,
										0 ,
										NULL ,
										NULL ,
										NULL ,
										PiR2TWorker ,
										Tctx );
			if ( NT_SUCCESS(Status))
				Ctx->R2TWorkerCount++;
			else
				ExFreePoolWithTag ( Tctx , USCSI_TAG );
		}
	}

	if ( !Ctx->R2TWorkerCount )
		goto ErrorOut;
			
	//
	//Create DataIn Thread
	//
	
	Cnt2 = min ( Cnt, CTX_MAX_DATAIN_WORKERS );
	for ( k = 0 ; k < Cnt2 ; k++ )
	{		
		Tctx = ExAllocatePoolWithTag ( PagedPool , sizeof (*Tctx) , USCSI_TAG );
		
		if ( Tctx )
		{
			Tctx->Ctx 	= Ctx;
			Tctx->No 	= k;
			Status = PsCreateSystemThread( &Ctx->DataInWorkers[k] ,
										0 ,
										NULL ,
										NULL ,
										NULL ,
										PiDataInWorker ,
										Tctx );
			if ( NT_SUCCESS(Status))
				Ctx->DataInWorkerCount++;
			else
				ExFreePoolWithTag ( Tctx , USCSI_TAG );
		}
	}

	if ( !Ctx->DataInWorkerCount )
		goto ErrorOut;
	
	//
	//Create Pending Command Thread
	//
	
	Cnt2 = min ( Cnt, CTX_MAX_PENDING_CMD_WORKERS );
	for ( k = 0 ; k < Cnt2 ; k++ )
	{		
		Tctx = ExAllocatePoolWithTag ( PagedPool , sizeof (*Tctx) , USCSI_TAG );
		
		if ( Tctx )
		{
			Tctx->Ctx 	= Ctx;
			Tctx->No 	= k;
			Status = PsCreateSystemThread( &Ctx->PendingCompleteCmdWorkers[k] ,
										0 ,
										NULL ,
										NULL ,
										NULL ,
										PiPendingCompleteCmdWorker ,
										Tctx );
			if ( NT_SUCCESS(Status))
				Ctx->PendingCompleteCmdCount++;
			else
				ExFreePoolWithTag ( Tctx , USCSI_TAG );
		}
	}
	
	if ( !Ctx->PendingCompleteCmdCount )
		goto ErrorOut;
	
	return STATUS_SUCCESS;
	
ErrorOut:
		
	return STATUS_UNSUCCESSFUL;
}

//
// Initialize Connection
//

VOID TiInitConInfo (PuCONINFO ConInfo)
{
	NTSTATUS				Status;
	KEY_NAME				i;
	ULONG					j;
	PuCON_CTX				Ctx;
	PKEY_VALUE				Kv;
		
	Ctx = ConInfo->ConCtx;
	
	ConInfo->State = CON_STAT_UNINITIALIZED;	
	
	ConInfo->SSG = LOGIN_STAGE_SecurityNegotiation;
	
	ConInfo->MoreWorks = FALSE;
	
	ConInfo->SkipOperationalNegotiation = FALSE;
		
	for ( i = KEY_MaxSessionKey + 1 ; i < KEY_LastKey ; i++ )
	{
		if ( ! (PiKeys[i].Attrs & KEY_TYPE_STRING) )
		{
			ConInfo->Parameter[KEY_CON_IDX(i)] =PiAllocateKeyValue();
			ConInfo->Parameter[KEY_CON_IDX(i)]->Key.D = i;
			ConInfo->Parameter[KEY_CON_IDX(i)]->Pending = KEY_P;
		}
		else
			ConInfo->Parameter[KEY_CON_IDX(i)] = NULL;
	}	
	
	for ( j = 0 ; j < PENDING_TASK_HASH_SIZE ; j++)
		InitializeListHead ( &ConInfo->PendingTasks[j]);
	
	KEY_IV(KV(ConInfo,KEY_MaxRecvDataSegmentLength)).Number = 8192;
	KEY_TV(KV(ConInfo,KEY_MaxRecvDataSegmentLength)).Number = 8192;
	
	Kv = PiAllocateKeyValStr0 (sizeof("None"));
	if (Kv)
	{
		ConInfo->Parameter[KEY_CON_IDX(KEY_HeaderDigest)] = Kv;
		Kv->Key.D = KEY_HeaderDigest;
		RtlCopyMemory ( KEY_FV(Kv).String , "None" , sizeof ("None"));
	}
	
	Kv = PiAllocateKeyValStr0 (sizeof("None"));
	if (Kv)
	{
		ConInfo->Parameter[KEY_CON_IDX(KEY_DataDigest)] = Kv;
		Kv->Key.D = KEY_DataDigest;
		RtlCopyMemory ( KEY_FV(Kv).String , "None" , sizeof ("None"));
	}
	
	Kv = PiAllocateKeyValStr0 (sizeof("None"));
	if (Kv)
	{
		ConInfo->Parameter[KEY_CON_IDX(KEY_AuthMethod)] = Kv;
		Kv->Key.D = KEY_AuthMethod;
		RtlCopyMemory ( KEY_FV(Kv).String , "None" , sizeof ("None"));
	}
	
	KeInitializeEvent ( &ConInfo->LoginEvent , NotificationEvent , FALSE);
		
	ConInfo->StatSN.Next = NULL;

	InitializeListHead (&ConInfo->ConList);
	KeInitializeSpinLock(&ConInfo->PendingTasksLock);

	ConInfo->Answer = NULL;
	/*
	ExInitializeNPagedLookasideList ( 	
		&ConInfo->SNLookAside , NULL , NULL , 0 , sizeof (SN) , USCSI_TAG , 0);*/
		
	InitializeListHead (&Ctx->InPDU);	
	InitializeListHead (&Ctx->OutPDU);	
	InitializeListHead (&Ctx->R2Ts);
	InitializeListHead (&Ctx->DataIns);
	InitializeListHead (&Ctx->PendingCompleteCmds);
	
	KeInitializeSpinLock (&Ctx->InLock);	
	KeInitializeSpinLock (&Ctx->OutLock);
	KeInitializeSpinLock (&Ctx->R2TLock);	
	KeInitializeSpinLock (&Ctx->DataInLock);	
	KeInitializeSpinLock (&Ctx->PendingCompleteCmdLock);
		
	ExInitializeNPagedLookasideList ( 	
		&Ctx->BHSLookAside , NULL , NULL , 0 , sizeof (BHS) , USCSI_TAG , 0);	
	ExInitializeNPagedLookasideList ( 	
		&Ctx->PDULookAside , NULL , NULL , 0 , sizeof (PDU) , USCSI_TAG , 0);
	
	TiInitializeTransport (ConInfo);
	
	TiInitWorkers( ConInfo );
}

//
//
//

VOID TiFreeConnection (PuCONINFO ConInfo)
{
	KEY_NAME	i;
	for ( i = KEY_MaxSessionKey + 1 ; i < KEY_LastKey ; i++ )
		if ( ConInfo->Parameter[KEY_CON_IDX(i)] )
			ExFreePoolWithTag ( ConInfo->Parameter[KEY_CON_IDX(i)] ,USCSI_TAG );
}

//
//
//

BOOLEAN TiVerifyDataDigest( PuCON_CTX Ctx, PPDU Pdu)
{
	return TRUE;
}

//
//
//

BOOLEAN TiVerifyHeaderDigest( PuCON_CTX Ctx, PPDU Pdu)
{
	return TRUE;
}

//
//
//

PPDU TiAllocatePDU( PuCON_CTX Ctx )
{
	PPDU Pdu;
	
	Pdu = ExAllocateFromNPagedLookasideList ( &Ctx->PDULookAside );
	//Pdu = ExAllocatePoolWithTag ( NonPagedPool , sizeof (PDU) , USCSI_TAG);
	if ( NULL == Pdu )
		goto Out;		

	RtlZeroMemory ( Pdu , sizeof (PDU) );
	Pdu->ConCtx = Ctx;
	
	InitializeListHead (&Pdu->PDUList);	
	InitializeListHead (&Pdu->Continue);
	InitializeListHead (&Pdu->Resps);	
	InitializeListHead (&Pdu->DataInOrR2T);
	InitializeListHead (&Pdu->DataOut);
		
	KeInitializeSpinLock(&Pdu->DataInOrR2TLock);		
			
	Pdu->Bhs = ExAllocateFromNPagedLookasideList( &Ctx->BHSLookAside);
	//Pdu->Bhs = ExAllocatePoolWithTag ( NonPagedPool , sizeof (BHS) , USCSI_TAG);
	if ( NULL == Pdu->Bhs )
	{
		ExFreeToNPagedLookasideList( &Ctx->PDULookAside, Pdu );
		//ExFreePoolWithTag ( Pdu , USCSI_TAG );
		Pdu = NULL;
		goto Out;
	}

	RtlZeroMemory ( Pdu->Bhs , sizeof (BHS) );

Out:
	
	return Pdu;
}

//
//
//

PSN TiAllocateSN ( PuCONINFO ConInfo)
{
	PSN		NSN;
	
	//NSN = ExAllocateFromNPagedLookasideList ( &ConInfo->SNLookAside );
	NSN = ExAllocatePoolWithTag ( NonPagedPool , sizeof (SN) , USCSI_TAG);
	//RtlZeroMemory ( StatSN , sizeof (STATSN) );
	
	return NSN;
}

VOID TiFreeSN ( PuCONINFO ConInfo , PSN SN)
{
	//ExFreeToNPagedLookasideList (&ConInfo->SNLookAside , SN );
	ExFreePoolWithTag ( SN , USCSI_TAG);
}

//
// TDI stuffs
//

NTSTATUS TiInitializeTransport( PuCONINFO Coninfo )
{
	NTSTATUS	Status;
	
	Status = TiOpenAddress ( Coninfo );
	if (!NT_SUCCESS( Status ))
		goto Out;	

	Status = TiRegisterCallBack (Coninfo , TDI_EVENT_RECEIVE , TiReceive , Coninfo );
	if (!NT_SUCCESS(Status))
		goto Out;
	
	Status = TiRegisterCallBack (Coninfo , TDI_EVENT_ERROR , TiTransportError , Coninfo );
	if (!NT_SUCCESS(Status))
		goto Out;	

	Status = TiRegisterCallBack (Coninfo , TDI_EVENT_DISCONNECT , TiTransportDisconnect , Coninfo );
	if (!NT_SUCCESS(Status))
		goto Out;	
		
	Status = TiOpenConnectionEndpoint ( Coninfo );
	if (!NT_SUCCESS( Status ))
		goto Out;	
	
	Status = TiAssociate (Coninfo );
	if (!NT_SUCCESS( Status ))
		goto Out;	
			
	Status = TiOpenControl (Coninfo );
	if (!NT_SUCCESS( Status ))
		goto Out;
	
	Coninfo->State = CON_STAT_INITIALIZED;

Out:

	KdPrintEx((DPFLTR_USCSI,DBG_TDI,"%s Coninfo 0x%p - 0x%X\n" , __FUNCTION__ , Coninfo,Status));

	return Status;
}

NTSTATUS TiPnPPowerHandler(
PUNICODE_STRING DeviceName,PNET_PNP_EVENT PowerEvent,PTDI_PNP_CONTEXT Context1,PTDI_PNP_CONTEXT Context2)
{
	KdPrintEx((DPFLTR_USCSI,DBG_TDI,"%s DeviceName %ws\n" , __FUNCTION__ , DeviceName));
	return STATUS_SUCCESS;
}

VOID TiBindHandler(
IN TDI_PNP_OPCODE  PnPOpcode,IN PUNICODE_STRING  DeviceName,IN PWSTR  MultiSZBindList)
{
	KdPrintEx((DPFLTR_USCSI,DBG_TDI,"%s DeviceName %ws PnPOpcode 0x%X\n" , __FUNCTION__,
		DeviceName , PnPOpcode ));
}

VOID TiAddAddressHandler(
IN PTA_ADDRESS  Address,IN PUNICODE_STRING  DeviceName,IN PTDI_PNP_CONTEXT  Context)
{
	KdPrintEx((DPFLTR_USCSI,DBG_TDI,"%s DeviceName %ws\n" , __FUNCTION__ , DeviceName));
}


VOID TiDelAddressHandler(
IN PTA_ADDRESS Address,IN PUNICODE_STRING DeviceName,IN PTDI_PNP_CONTEXT Context)
{
	KdPrintEx((DPFLTR_USCSI,DBG_TDI,"%s DeviceName %ws\n" , __FUNCTION__ , DeviceName));
}

//
// Public Functions
//

PPDU TpAllocatePDU( PuCON_CTX Ctx , UCHAR Opcode , UCHAR AHSLen , ULONG DataLen)
{
	PPDU Pdu;
	
	Pdu = TiAllocatePDU(Ctx);	
	if ( NULL == Pdu )
		goto Out;
	
	Pdu->Bhs->GENERICBHS.Opcode = Opcode & 0x3F;

	if ( AHSLen )
		Pdu->Bhs->GENERICBHS.TotalAHSLength = AHSLen;
	
	if ( DataLen & 0xFFFFFF )
	{
		REVERSE_3BYTES ( &Pdu->Bhs->GENERICBHS.DataSegmentLength , &DataLen );
	}
		
	if (AHSLen + DataLen > 0 )
	{
		Pdu->Buffer = ExAllocatePoolWithTag ( NonPagedPool, AHSLen + DataLen, USCSI_TAG );
		if ( NULL == Pdu->Buffer )
		{
			TpQueuePduRelease( Ctx , Pdu );
			Pdu = NULL;
			goto Out;
		}
		
		if ( AHSLen )
			Pdu->Ahs = Pdu->Buffer;
			
		Pdu->Data = Pdu->Buffer + AHSLen;
	}

Out:
	/*
	{
		
		LARGE_INTEGER Tick;
		KeQueryTickCount( &Tick);
		DbgPrint("%s Time=0x%X-%X Pdu=0x%X Bhs=0x%X Ahs=0x%X Data=0x%X\n",
		__FUNCTION__,Tick.HighPart,Tick.LowPart,Pdu,Pdu->Bhs,Pdu->Ahs,Pdu->Data);
	}*/
	return Pdu;
}

//
// Free PDU cascade
// Because some data allocate from PagedPool it MUST be Called at PASSIVE_LEVEL
//

VOID TpFreePDU(PuCON_CTX Ctx, PPDU Pdu, ULONG Level)
{
	PPDU			APdu;
	PLIST_ENTRY		PduEnt;
	//
	while ( !IsListEmpty ( &Pdu->Continue ))
	{
		PduEnt = RemoveHeadList ( &Pdu->Continue);
		APdu = CONTAINING_RECORD ( PduEnt , PDU , Continue);
		InitializeListHead ( &APdu->Continue);		
		TpFreePDU ( Ctx , APdu , Level + 1 );
	}
	//
	while ( !IsListEmpty ( &Pdu->DataInOrR2T))
	{
		PduEnt = RemoveHeadList ( &Pdu->DataInOrR2T);
		APdu = CONTAINING_RECORD ( PduEnt , PDU , DataInOrR2T);
		InitializeListHead ( &APdu->DataInOrR2T);
		TpFreePDU ( Ctx , APdu , Level + 1 );
	}
	// 
	while ( !IsListEmpty ( &Pdu->DataOut))
	{
		PduEnt = RemoveHeadList ( &Pdu->DataOut);
		APdu = CONTAINING_RECORD ( PduEnt , PDU , DataOut);
		InitializeListHead ( &APdu->DataOut);
		TpFreePDU ( Ctx , APdu , Level + 1 );
	}
	/*
	{
		LARGE_INTEGER Tick;
		KeQueryTickCount( &Tick);
		DbgPrint("%s Time=0x%X-%X Level=0x%X Pdu=0x%X Opcode=0x%X Bhs=0x%X Ahs=0x%X Data=0x%X\n" , 
		__FUNCTION__ , 
		Tick.HighPart,Tick.LowPart,
		Level ,
		Pdu , 
		Pdu->Bhs->GENERICBHS.Opcode,
		Pdu->Bhs,
		Pdu->Ahs,Pdu->Data);
	}*/

	if ( NULL != Pdu->Buffer)
	{
		//for TEXT PDU , Buffer is allocated from PagedPool
		ExFreePoolWithTag (Pdu->Buffer , USCSI_TAG);
		Pdu->Buffer = NULL;
	}
	ExFreeToNPagedLookasideList (&Pdu->ConCtx->BHSLookAside , Pdu->Bhs);
	//ExFreePoolWithTag ( Pdu->Bhs , USCSI_TAG);
	Pdu->Bhs = NULL;
	
	ExFreeToNPagedLookasideList (&Pdu->ConCtx->PDULookAside , Pdu);
	//ExFreePoolWithTag ( Pdu , USCSI_TAG);
	Pdu = NULL;	
}

//
//
//

VOID TpQueuePDU( PuCON_CTX Ctx , PPDU Pdu , UCHAR Which )

{
	PSESSION Session = Ctx->ConInfo->Session;
	
	if ( Which == WI_PDU_IN )
	{
		ExInterlockedInsertTailList ( &Ctx->InPDU , &Pdu->PDUList, &Ctx->InLock );
		KeSetEvent( &Ctx->DispatchEvent , IO_NO_INCREMENT , FALSE );
	}
	
	else if ( Which == WI_PDU_OUT )
	{
		ExInterlockedInsertTailList ( &Ctx->OutPDU , &Pdu->PDUList, &Ctx->OutLock );
		KeSetEvent( &Ctx->OutEvent , IO_NO_INCREMENT , FALSE );
	}
	
	else if ( Which == WI_PENDING_COMPLETE_CMD )
	{
		ExInterlockedInsertTailList ( &Ctx->PendingCompleteCmds , 
								&Pdu->PendingCmd, &Ctx->PendingCompleteCmdLock );
		KeSetEvent( &Ctx->PendingCompleteCmdEvent , IO_NO_INCREMENT , FALSE );
	}
	
	else if ( Which == WI_DATAIN )
	{
		ExInterlockedInsertTailList ( &Ctx->DataIns , &Pdu->PDUList, &Ctx->DataInLock );
		KeSetEvent( &Ctx->DataInEvent , IO_NO_INCREMENT , FALSE );
	}
	
	else if ( Which == WI_R2T )
	{
		ExInterlockedInsertTailList ( &Ctx->R2Ts , &Pdu->PDUList, &Ctx->R2TLock );
		KeSetEvent( &Ctx->R2TEvent , IO_NO_INCREMENT , FALSE );
	}
	
	else if ( Which == WI_PDU_RELEASE )
	{		
		ExInterlockedInsertTailList ( &Session->PduRelease , &Pdu->PDUList, &Session->PduReleaseLock );
		KeSetEvent( &Session->PduReleaseEvent , IO_NO_INCREMENT , FALSE );
	}	
}

NTSTATUS TiConnectTo( PuCONINFO Coninfo , ULONG Address , USHORT Port);

//
//
//

PuCONINFO TpAddConInfo( PSESSION Session )
{
	PuCONINFO	ConInfo = NULL;

	if ( ConInfo = TiAllocateConInfo() )
	{
		TiInitConInfo (ConInfo);
		ConInfo->Session = Session;
		ConInfo->CID = Session->CID++;
		
		if ( !Session->LeadingCon)
		{
			Session->LeadingCon = ConInfo;
			Session->BestCon = ConInfo;
		}
		
		TiConnectTo ( ConInfo , Session->Tgt->Tgt.Addr , Session->Tgt->Tgt.Port);
		//
		TiQueryTransportInfo (ConInfo , TDI_QUERY_PROVIDER_INFO );
				
		ExInterlockedInsertTailList ( &Session->Connections , &ConInfo->ConList, &Session->ConLock );
	}
	
	return ConInfo;
}

//
//
//

PSESSION TpAllocateSession (PITGT Itgt)
{
	PSESSION	Session = NULL;
	
	Session = ExAllocatePoolWithTag (NonPagedPool , sizeof (SESSION) , USCSI_TAG );
	
	RtlZeroMemory ( Session , sizeof (SESSION));
	
	if (Session)
	{
		TiInitSession (Session);
		Session->Tgt = Itgt;
	}	
	
	return Session;
}

//
// Establish the TCP Connection
//

NTSTATUS TiConnectTo( PuCONINFO Coninfo , ULONG Address , USHORT Port)
{
	NTSTATUS					Status;
	PIRP						Irp;
	TDI_CONNECTION_INFORMATION 	Info = {0};
	TA_IP_ADDRESS				Addr = {0};
	ULONG						InfoSize;
	KEVENT						Event;
	IO_STATUS_BLOCK				IoStatus = {0};
	PDEVICE_OBJECT				DevObj;
	
	Status = STATUS_SUCCESS;
	
	DevObj = IoGetRelatedDeviceObject (Coninfo->ConEpObj);

	Addr.TAAddressCount = 1;
	Addr.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
	Addr.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
	Addr.Address[0].Address[0].sin_port = Port;
	Addr.Address[0].Address[0].in_addr = Address;
	Info.RemoteAddressLength = sizeof (Addr);
	Info.RemoteAddress = &Addr;	
	
	Irp = TdiBuildInternalDeviceControlIrp(	TDI_CONNECT, DevObj, NULL, &Event, &IoStatus );
	if ( Irp )
	{
		TdiBuildConnect(Irp , DevObj , Coninfo->ConEpObj, NULL, NULL, NULL, &Info, &Info );
	}
	else
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		return Status;
	}
		
	KeInitializeEvent ( &Event , NotificationEvent , FALSE);

	Status = IoCallDriver ( DevObj , Irp );	
	if ( STATUS_PENDING == Status )
	{		
		KeWaitForSingleObject ( &Event , Executive , KernelMode , FALSE , NULL );
		Status = Irp->IoStatus.Status;
	}
	
	if (NT_SUCCESS (Status))
		Coninfo->State = CON_STAT_CONNECTED;
	
	return Status;
}

//
//
//

PuCONINFO TpBestConnection(PSESSION Session)
{
	return Session->BestCon;
}

//
//
//

VOID TiLoadBalancer( PVOID Parameter)
{
	
}

//
//
//

NTSTATUS TpTest()
{	
	NTSTATUS Status;
	PuCONINFO Coninfo;
	
	Coninfo = TiAllocateConInfo();
	if (!Coninfo)
		return STATUS_INSUFFICIENT_RESOURCES;
	
	Status = TiInitializeTransport( Coninfo );
	if (!NT_SUCCESS(Status))
		return Status;
	
	Status = TiConnectTo( Coninfo , INETADDR(192,168,1,1) , HTONS(80) );	
	//
	// Query infos
	//
	Status = TiQueryTransportInfo (Coninfo , TDI_QUERY_CONNECTION_INFO );
	if (!NT_SUCCESS( Status ))
		return Status;	
		
	Status = TiQueryTransportInfo (Coninfo , TDI_QUERY_PROVIDER_INFO );
	if (!NT_SUCCESS( Status ))
		return Status;

	ExFreePoolWithTag ( Coninfo , USCSI_TAG);
	
	return Status;
}

//
//
//

VOID TpRegisterPnPHandlers( PSESSION Session )
{
	NTSTATUS					Status;
	UNICODE_STRING				ClientName;
	TDI20_CLIENT_INTERFACE_INFO	TdiInfo = {0};
	
	RtlInitUnicodeString (&ClientName , L"uSCSI");
	TdiInfo.MajorTdiVersion = 2;
	TdiInfo.MinorTdiVersion = 0;
	TdiInfo.ClientName = &ClientName;
	TdiInfo.PnPPowerHandler = TiPnPPowerHandler;
	TdiInfo.BindingHandler = TiBindHandler;
	TdiInfo.AddAddressHandlerV2 = TiAddAddressHandler;
	TdiInfo.DelAddressHandlerV2 = TiDelAddressHandler;
	
	Status = TdiRegisterPnPHandlers( &TdiInfo , sizeof (TdiInfo) , &Session->TdiPnP);
	
}

//
//
//

VOID TpSetSessionParms( PSESSION Session , PLIST_ENTRY Parms)
{
	PLIST_ENTRY				ParmEnt;
	PKEY_VALUE				Parm;
	KEY_NAME				Key;
	
	ParmEnt = Parms->Flink;
	while ( ParmEnt != Parms)
	{
		Parm = CONTAINING_RECORD (ParmEnt , KEY_VALUE , Vals);
		Key = Parm->Key.D;
		
		if ( Key >= KEY_MaxSessionKey )
			continue;
		
		if ( !(PiKeys[Key].Attrs & KEY_DIR_INITIATOR))
			continue;	
		
		if ( !(PiKeys[Key].Attrs & KEY_SCOPE_SW) )
			continue;
		
		ParmEnt = ParmEnt->Flink;
	}
}

//
//
//

PuCONINFO TiFindConnection (PSESSION Session , USHORT Cid)
{
	PuCONINFO		ConInfo;
	PLIST_ENTRY		ConEnt;
	
	ConEnt = Session->Connections.Flink;
	while ( ConEnt != &Session->Connections)
	{
		ConInfo = CONTAINING_RECORD(ConEnt , uCONINFO , ConList);
		if (ConInfo->CID == Cid)
			return ConInfo;
		ConEnt = ConEnt->Flink;
	}
	return NULL;
}

//
//
//

VOID TpSetConnectionParms ( PSESSION Session , USHORT Cid , PLIST_ENTRY Parms)
{
	PLIST_ENTRY				ParmEnt;
	PKEY_VALUE				Parm;
	KEY_NAME				Key;
	PuCONINFO				ConInfo;
	
	ParmEnt = Parms->Flink;
	while ( ParmEnt != Parms)
	{
		Parm = CONTAINING_RECORD (ParmEnt , KEY_VALUE , Vals);
		Key = Parm->Key.D;
		
		if ( Key <= KEY_MaxSessionKey || Key >= KEY_LastKey )
			continue;
		
		if ( !(PiKeys[Key].Attrs & KEY_DIR_INITIATOR))
			continue;
				
		if ( !(PiKeys[Key].Attrs & KEY_SCOPE_CO) )
			continue;
			
		ConInfo = TiFindConnection( Session , Cid);
		
		if (!ConInfo)
			continue;
		
		if ( ConInfo->State > CON_STAT_IN_LOGIN && 
			 (KEY_HeaderDigest == Key || KEY_DataDigest == Key || KEY_AuthMethod ==Key  ) )
			continue;
			
		if ( PiKeys[Key].Attrs & KEY_TYPE_STRING )
		{
			ULONG			Len;
			PKEY_VALUE		New;
			
			Len = strlen (KEY_FV(Parm).String) + 1;	//'\0'
			if ( Len )
			{
				New = ExAllocatePoolWithTag (PagedPool , sizeof (KEY_VALUE) + Len , USCSI_TAG);
				if ( New )
				{				
					if ( ConInfo->Parameter[KEY_CON_IDX(Key)] )
						ExFreePoolWithTag (ConInfo->Parameter[KEY_CON_IDX(Key)] , USCSI_TAG);
					KEY_FV(New).String = (PUCHAR)(New+1);
					RtlCopyMemory ( KEY_FV(New).String , KEY_FV(Parm).String , Len -1 );
					*(KEY_FV(New).String + Len - 1) = '\0';
					ConInfo->Parameter[KEY_CON_IDX(Key)] = New;
				}
			}
		}
		
		else if (PiKeys[Key].Attrs & KEY_TYPE_LOV)
		{
			ULONG		Len;
			PKEY_VALUE	New;
			
			Len = strlen (KEY_FV(Parm).Lov.Val) + 1;//'\0'
			if ( Len )
			{
				New = ExAllocatePoolWithTag (PagedPool , sizeof (KEY_VALUE) + Len , USCSI_TAG);
				if ( New )
				{				
					if ( ConInfo->Parameter[KEY_CON_IDX(Key)])
						ExFreePoolWithTag (ConInfo->Parameter[KEY_CON_IDX(Key)] , USCSI_TAG);
					KEY_FV(New).String = (PUCHAR)(New+1);
					RtlCopyMemory ( KEY_FV(New).String , KEY_FV(Parm).Lov.Val , Len - 1 );
					*(KEY_FV(New).String + Len - 1) = '\0';
					ConInfo->Parameter[KEY_CON_IDX(Key)] = New;
				}
			}
		}
		
		else if (PiKeys[Key].Attrs & KEY_TYPE_NUMBER)
			KEY_FV(ConInfo->Parameter[KEY_CON_IDX(Key)]).Number = KEY_FV(Parm).Number;
		
		else if ( PiKeys[Key].Attrs & KEY_TYPE_BOOLEAN )
			KEY_FV(ConInfo->Parameter[KEY_CON_IDX(Key)]).Bool = KEY_FV(Parm).Bool;
		
		if (ConInfo->State == CON_STAT_LOGGED_IN && KEY_MaxRecvDataSegmentLength == Key );
			//TODO : Notify Target
		ParmEnt = ParmEnt->Flink;
	}
}