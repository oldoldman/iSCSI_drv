
#include "precomp.h"
						
NTSTATUS uStartFdo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS					Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION			Stack;
	PFDO_EXT					FdoExt;
	PDEVICE_RELATIONS			Rels;
				
	Stack = IoGetCurrentIrpStackLocation( Irp );
	FdoExt = DeviceObject->DeviceExtension;
	
	Status = SendIrpSynchronously(FdoExt->LowerDev,Irp);		
	if (NT_ERROR(Status))
		goto ErrorOut;
		
	Status = IoSetDeviceInterfaceState (&FdoExt->InterfaceName , TRUE );
	if (NT_ERROR(Status))
		goto ErrorOut;
	
	uSCSIInitialize();
	
	return Status;
	
ErrorOut:

	return Status;
}

NTSTATUS uPnPFdo (IN PDEVICE_OBJECT	DeviceObject, IN PIRP Irp)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	ULONG					i = 0 , OldCount;
	PIO_STACK_LOCATION		Stack;
	PLIST_ENTRY				PdoEnt , PdoEnt2;
	PFDO_EXT				FdoExt;
	PPDO_EXT				PdoExt;
	PDEVICE_RELATIONS		OldRels , Rels;
		
	Stack = IoGetCurrentIrpStackLocation( Irp );
	FdoExt = DeviceObject->DeviceExtension;
	
	KdPrintEx((DPFLTR_USCSI,DBG_USCSI,
	"%s MinorFunction 0x%X\n" , __FUNCTION__ , Stack->MinorFunction));
	
	switch (Stack->MinorFunction )
	{
		case IRP_MN_START_DEVICE:
				
			Status = uStartFdo (DeviceObject , Irp);
			
            break;	
            			
		case IRP_MN_QUERY_DEVICE_RELATIONS:
		
			if ( BusRelations == Stack->Parameters.QueryDeviceRelations.Type )
			{
				OldRels = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        		if (OldRels) 
            		OldCount = OldRels->Count; 
       			else  
            		OldCount = 0;
        
				Rels = ExAllocatePoolWithTag(PagedPool , 
											sizeof (DEVICE_RELATIONS) + 
												(FdoExt->PDOCount + OldCount - 1)*sizeof (PDEVICE_OBJECT), 
											USCSI_TAG );		
				
				if ( Rels )
				{
					if (OldCount) 
            			RtlCopyMemory (Rels->Objects, OldRels->Objects,
                                      	OldCount * sizeof (PDEVICE_OBJECT));
				
					Rels->Count = OldCount + FdoExt->PDOCount;
				
					PdoEnt = &FdoExt->PDOList;
					PdoEnt2 = PdoEnt->Flink;
									
					while ( PdoEnt2 != PdoEnt )
					{
						PdoExt = CONTAINING_RECORD ( PdoEnt2, PDO_EXT , PDOList);
						if ( !PdoExt->Reported )
						{
							ObReferenceObject( PdoExt->Self );
							Rels->Objects[OldCount++] = PdoExt->Self;
							PdoExt->Reported = TRUE;
						}
						PdoEnt2 = PdoEnt2->Flink;
					}
					
					Irp->IoStatus.Information = (ULONG_PTR)Rels;						
				}
			}
			break;
		
		case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
		case IRP_MN_QUERY_CAPABILITIES:
		case IRP_MN_QUERY_PNP_DEVICE_STATE:
	
			IoSkipCurrentIrpStackLocation( Irp );
	
			Status = IoCallDriver ( FdoExt->LowerDev , Irp );
	
			return Status;

		case 0x18:
			//IRP_MN_QUERY_LEGACY_BUS_INFORMATION
			Status = SendIrpSynchronously( FdoExt->LowerDev , Irp );				
			break;
	}
	
	Irp->IoStatus.Status = Status;
	
    IoCompleteRequest( Irp , IO_NO_INCREMENT );
 
    return Status;
}

NTSTATUS uStartPdo (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{	
	NTSTATUS		Status = STATUS_SUCCESS;
	PPDO_EXT		PdoExt;
	
	PdoExt = (PPDO_EXT)DeviceObject->DeviceExtension;
	
	InitializeListHead ( &PdoExt->PDOList);
   	InitializeListHead ( &PdoExt->CmdQueue);
   	KeInitializeSpinLock ( &PdoExt->CmdQueueLock);
   	/*
   	ExInitializePagedLookasideList ( 	
		&PdoExt->CmdLookAside , NULL , NULL , 0 , sizeof (SCSICmd) , USCSI_TAG , 0);*/	
   						
	return Status;
	
}

//
//
//

NTSTATUS uPnPPdo (IN PDEVICE_OBJECT	DeviceObject, IN PIRP Irp)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION		Stack;
	PPDO_EXT				PdoExt;	
	PWCHAR					Id = NULL;
	
	Stack 	= IoGetCurrentIrpStackLocation( Irp );
	PdoExt 	= DeviceObject->DeviceExtension;
	
	KdPrintEx((DPFLTR_USCSI,DBG_USCSI,
	"%s MinorFunction 0x%X\n" , __FUNCTION__ , Stack->MinorFunction ));
	
	switch (Stack->MinorFunction)
	{
		case IRP_MN_START_DEVICE:
			
			uStartPdo ( DeviceObject , Irp );				
			break;
			
		case IRP_MN_QUERY_ID:
		
			if (BusQueryHardwareIDs == Stack->Parameters.QueryId.IdType  ||
				BusQueryCompatibleIDs == Stack->Parameters.QueryId.IdType )
			{
				Id = ExAllocatePoolWithTag (PagedPool , HARDWAREID_LEN , USCSI_TAG);
				if ( Id )
				{
					RtlCopyMemory ( Id , HARDWAREID , HARDWAREID_LEN );
					Irp->IoStatus.Information = (ULONG_PTR)Id;											
				}
				else
					Status = STATUS_UNSUCCESSFUL;
				
			}
			else if ( BusQueryDeviceID == Stack->Parameters.QueryId.IdType )
			{
				Id = ExAllocatePoolWithTag (PagedPool ,DEVICEID_LEN ,USCSI_TAG);
				if ( Id )
				{
					RtlCopyMemory ( Id , DEVICEID , DEVICEID_LEN );
					Irp->IoStatus.Information = (ULONG_PTR)Id;											
				}
				else
					Status = STATUS_UNSUCCESSFUL;
			}	
			/*		
			else if ( BusQueryInstanceID == Stack->Parameters.QueryId.IdType )
			{
				Id = ExAllocatePoolWithTag ( PagedPool , 2 , USCSI_TAG);
				if ( Id )
				{
					RtlCopyMemory ( Id , L"\0" , 2 );
					Irp->IoStatus.Information = (ULONG_PTR)Id;											
				}
				else
					Status = STATUS_UNSUCCESSFUL;
			}*/
			KdPrintEx((DPFLTR_USCSI,DBG_USCSI,
			"%s IdType %d Id %ws\n" , __FUNCTION__ , 
			Stack->Parameters.QueryId.IdType , Id ));
			break;
			
		case IRP_MN_QUERY_DEVICE_TEXT:
			
			// Description
			if (DeviceTextDescription == 
							Stack->Parameters.QueryDeviceText.DeviceTextType)
			{
				Id = ExAllocatePoolWithTag ( PagedPool , DEVICEDESC_LEN , USCSI_TAG);
				if (Id)
				{
					RtlCopyMemory ( Id , DEVICEDESC , DEVICEDESC_LEN );
					Irp->IoStatus.Information = (ULONG_PTR)Id;											
				}
				else
					Status = STATUS_UNSUCCESSFUL;
			}
			// Location info
			else if (DeviceTextLocationInformation ==
					Stack->Parameters.QueryDeviceText.DeviceTextType)
			{
				Id = ExAllocatePoolWithTag ( PagedPool , DEVICELOC_LEN , USCSI_TAG);
				if (Id)
				{
					RtlCopyMemory ( Id , DEVICELOC , DEVICELOC_LEN );
					Irp->IoStatus.Information = (ULONG_PTR)Id;											
				}
				else
					Status = STATUS_UNSUCCESSFUL;
			}				
			KdPrintEx((DPFLTR_USCSI,DBG_USCSI,
			"%s Text %d Id %ws\n" , __FUNCTION__ , 
			Stack->Parameters.QueryDeviceText.DeviceTextType , Id ));
			break;

		case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
			
			Status = Irp->IoStatus.Status;
			
			IoCompleteRequest( Irp , IO_NO_INCREMENT );    
			
			return Status;
			
		case IRP_MN_QUERY_RESOURCES:
			{
				PCM_RESOURCE_LIST	Res;
				Res = ExAllocatePoolWithTag ( PagedPool , sizeof (CM_RESOURCE_LIST) , 
											USCSI_TAG);
				if ( Res )
				{
					Res->Count = 1;
					Res->List[0].PartialResourceList.Version = 1;
					Res->List[0].PartialResourceList.Revision = 1;
					Res->List[0].PartialResourceList.Count = 0;
					Irp->IoStatus.Information = (ULONG_PTR)Res;
				}
				else
				{
					Irp->IoStatus.Information = 0;
					Status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			break;
			
		case IRP_MN_QUERY_BUS_INFORMATION:
			{
				PPNP_BUS_INFORMATION businfo;
				businfo = ExAllocatePoolWithTag (PagedPool, sizeof(PNP_BUS_INFORMATION),
                                        USCSI_TAG);
				//businfo->BusTypeGuid = GUID_USCSI_BUS;
				businfo->LegacyBusType = PNPBus;
				businfo->BusNumber = 0;
			}
			break;
			
		case IRP_MN_QUERY_CAPABILITIES:
			{
				PDEVICE_CAPABILITIES Caps;
				Caps = Stack->Parameters.DeviceCapabilities.Capabilities;
				
				Caps->LockSupported = FALSE;
				Caps->EjectSupported = FALSE;
				Caps->Removable = FALSE;
				Caps->DockDevice = FALSE;
				Caps->D1Latency = Caps->D2Latency = Caps->D3Latency = 0;
				Caps->NoDisplayInUI = 1;
				Irp->IoStatus.Information = sizeof (DEVICE_CAPABILITIES);
			}
			break;
			
		case IRP_MN_QUERY_PNP_DEVICE_STATE:
			{
				Irp->IoStatus.Information = 0;
			}
			break;
	}
	
	Irp->IoStatus.Status = Status;
 
    IoCompleteRequest( Irp , IO_NO_INCREMENT );    
	
	return Status;
}

//
//
//

NTSTATUS uPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{	
	PCOMMON_EXT Ext;
	
	Ext = (PCOMMON_EXT)DeviceObject->DeviceExtension;

	if ( Ext->IsFDO )
		return uPnPFdo (DeviceObject , Irp );
	else
		return uPnPPdo (DeviceObject , Irp );
}