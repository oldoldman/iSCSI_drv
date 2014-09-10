
#include "precomp.h"

//
// IRP_MJ_SCSI
//

NTSTATUS uScsi (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS				Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION		Stack;
	PPDO_EXT				Ext;
	PSCSI_REQUEST_BLOCK 	Srb;
	PSENSE_DATA				Sense;
	PCDB					Cdb;
		
	Stack = IoGetCurrentIrpStackLocation( Irp );
	Ext = (PPDO_EXT)DeviceObject->DeviceExtension;	
	Srb = Stack->Parameters.Scsi.Srb;
	Sense = (PSENSE_DATA)Srb->SenseInfoBuffer;
	Cdb = (PCDB)Srb->Cdb;
	/*
	DbgPrint("%s Srb.Function=%d SrbStatus=%d ScsiStatus=%d PathId.TargetId.Lun=%d.%d.%d QueueTag=%d QueueAction=%d CdbLength=%d SenseInfoBufferLength=%d SrbFlags=0x%X DataTransferLength=%d TimeOutValue=%d DataBuffer=0x%p SenseInfoBuffer=0x%p NextSrb=0x%p OriginalRequest=0x%p SrbExtension=0x%p cdb[0]=0x%X\n" , 
	__FUNCTION__ , 
	Srb->Function,
	Srb->SrbStatus,
	Srb->ScsiStatus,
	Srb->PathId,Srb->TargetId,Srb->Lun,
	Srb->QueueTag,
	Srb->QueueAction,
	Srb->CdbLength,
	Srb->SenseInfoBufferLength,
	Srb->SrbFlags,
	Srb->DataTransferLength,
	Srb->TimeOutValue,
	Srb->DataBuffer,
	Srb->SenseInfoBuffer,
	Srb->NextSrb,
	Srb->OriginalRequest,
	Srb->SrbExtension,
	Srb->Cdb[0]);*/
	
	// 0x01					
	if ( SRB_FUNCTION_CLAIM_DEVICE == Srb->Function )
	{
		if (!Ext->Claimed )
		{
			Ext->Claimed = TRUE;
			Srb->DataBuffer = DeviceObject;
		}
		else
		{
			//Status = STATUS_DEVICE_BUSY;
		}		
	}
	// 0x06
	else if ( SRB_FUNCTION_RELEASE_DEVICE == Srb->Function )
	{
		if ( Ext->Claimed ) 
			Ext->Claimed = FALSE;
		else
			Status = STATUS_UNSUCCESSFUL;
	}
	// 0x00
	else if ( SRB_FUNCTION_EXECUTE_SCSI == Srb->Function )
	{		
		Status = ProcessCommand( Irp , Ext );
		if ( Status == STATUS_PENDING )
			return Status;		
	}
	// 0x02
	else if ( SRB_FUNCTION_IO_CONTROL == Srb->Function )
	{
		PSRB_IO_CONTROL 	IoCtl;
		PSENDCMDOUTPARAMS	OutParms;
		PSENDCMDINPARAMS	InParms;
		IoCtl = Srb->DataBuffer;
					
		if ( IOCTL_SCSI_MINIPORT_IDENTIFY == IoCtl->ControlCode)
		{
			int i;
			InParms = (PSENDCMDINPARAMS)((PUCHAR)IoCtl + IoCtl->HeaderLength);
			DbgPrint(
			"SENDCMDINPARAMS cBufferSize=0x%X irDriveRegs=(0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X) bDriveNumber=0x%X\n" , 
			InParms->cBufferSize, 
			InParms->irDriveRegs.bFeaturesReg,
			InParms->irDriveRegs.bSectorCountReg,
    		InParms->irDriveRegs.bSectorNumberReg,
    		InParms->irDriveRegs.bCylLowReg,
    		InParms->irDriveRegs.bCylHighReg,
    		InParms->irDriveRegs.bDriveHeadReg,
    		InParms->irDriveRegs.bCommandReg,
			InParms->bDriveNumber);
			
			OutParms = (PSENDCMDOUTPARAMS)((PUCHAR)IoCtl + IoCtl->HeaderLength);
			DbgPrint("SENDCMDOUTPARAMS cBufferSize=0x%X\n" , OutParms->cBufferSize);
			//for Buffer structure refer to ATA-3 p49
			//
			OutParms->bBuffer[0] = 0x80;
			OutParms->bBuffer[1] = 0x40;
			//
			OutParms->bBuffer[2] = 1;	//Num of logical cylinders
			OutParms->bBuffer[6] = 1;	//Num of logical heads
			
			//10-19 Serial Number
			RtlCopyMemory( &OutParms->bBuffer[20] , "uSCSIDemo1234567890" , 19);			
			// Model number
			for ( i = 54 ; i < 92 ; i++ )
				OutParms->bBuffer[i] = 'M';
			//Command set supported
			OutParms->bBuffer[164] = 0x0;	//do not support
			OutParms->DriverStatus.bDriverError = (UCHAR)SMART_NO_ERROR;
		}
		
		else if ( IOCTL_SCSI_MINIPORT_ENABLE_SMART == IoCtl->ControlCode )
		{
			DbgPrint("IOCTL_SCSI_MINIPORT_ENABLE_SMART\n");
		}
		
		else if ( IOCTL_SCSI_MINIPORT_RETURN_STATUS == IoCtl->ControlCode )
		{
			DbgPrint("IOCTL_SCSI_MINIPORT_RETURN_STATUS\n");
		}
		
		else
		{
			DbgPrint("*** Unprocessed SRB_FUNCTION_IO_CONTROL HeaderLength=0x%X Signature=%c%c%c%c%c%c%c%c Timeout=0x%X ControlCode=0x%X ReturnCode=0x%X Length=0x%X\n",
			IoCtl->HeaderLength,
			IoCtl->Signature[0],IoCtl->Signature[1],IoCtl->Signature[2],IoCtl->Signature[3],
			IoCtl->Signature[4],IoCtl->Signature[5],IoCtl->Signature[6],IoCtl->Signature[7],
			IoCtl->Timeout , 
			IoCtl->ControlCode,
			IoCtl->ReturnCode,
			IoCtl->Length);
		}
		Irp->IoStatus.Information = 0;
	}	
	// 0x07
	else if ( SRB_FUNCTION_SHUTDOWN == Srb->Function )
	{
		DbgPrint("TODO SRB_FUNCTION_SHUTDOWN\n");
	}
	//0x08
	else if ( SRB_FUNCTION_FLUSH == Srb->Function )
	{
		DbgPrint("TODO SRB_FUNCTION_FLUSH\n");
	}
	else 
		DbgPrint("%s **** Unprocessed Srb function=0x%X CdbLength=0x%X\n" , 
		__FUNCTION__ , Srb->Function , Srb->CdbLength);
	
	Srb->SrbStatus = SRB_STATUS_SUCCESS;
		
	Irp->IoStatus.Status = Status;
	IoCompleteRequest ( Irp , IO_NO_INCREMENT );
	return Status;
}