
#include "precomp.h"

//
//
//

NTSTATUS CompletionRoutine (IN PDEVICE_OBJECT   DeviceObject, IN PIRP Irp,IN PVOID Context)
{     
    if (Irp->PendingReturned) 
    {
        IoMarkIrpPending( Irp );
        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED; 
}

//
//
//

NTSTATUS SendIrpSynchronously (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
    KEVENT      Event;
    NTSTATUS    Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, CompletionRoutine, &Event, TRUE, TRUE, TRUE);

    Status = IoCallDriver(DeviceObject, Irp);    
    if ( STATUS_PENDING == Status ) 
    {
       KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL );
       Status = Irp->IoStatus.Status;
    }

    return Status;
}

//
//
//

VOID SCSICompleteIrp(ULONG Handle, PVOID Ctx, UCHAR ScsiStatus, PUCHAR SenseData, ULONG SenseLen)
{
	PLIST_ENTRY				Ent , Ent2;
	PIO_STACK_LOCATION		Stack;
	PSCSICmd				Cmd;
	PPDO_EXT				PdoExt;	
	PSCSI_REQUEST_BLOCK		Srb;
	BOOLEAN					Found = FALSE;
	UCHAR					ScsiOpcode;
	KIRQL					OldIrql;
		
	PdoExt = (PPDO_EXT)Ctx;
	Ent = &PdoExt->CmdQueue;
	Ent2 = Ent->Flink;
	
	KeAcquireSpinLock ( &PdoExt->CmdQueueLock , &OldIrql );
	
	while ( Ent2 != Ent )
	{
		Cmd = CONTAINING_RECORD ( Ent2 , SCSICmd , List );
		if ( Cmd->Handle == Handle )
		{
			Found = TRUE;
			break;
		}
		Ent2 = Ent2->Flink;
	}
	
	if ( !Found )
	{		
		KeReleaseSpinLock ( &PdoExt->CmdQueueLock , OldIrql ); 
		DbgPrint("%s Handle=0x%X Not Found!\n", __FUNCTION__ , Handle );
		return;
	}

	RemoveHeadList ( Cmd->List.Blink );
	
	KeReleaseSpinLock ( &PdoExt->CmdQueueLock , OldIrql ); 
	
	Stack = IoGetCurrentIrpStackLocation ( Cmd->Irp );
	Srb = Stack->Parameters.Scsi.Srb;
	ScsiOpcode = Srb->Cdb[0];
	
	if ( SenseData )
	{
		Srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID;
		RtlCopyMemory ( Srb->SenseInfoBuffer , SenseData , SenseLen );
	}
	else
		Srb->SrbStatus = SRB_STATUS_SUCCESS;
		
	Srb->ScsiStatus = ScsiStatus;
	Cmd->Irp->IoStatus.Status = STATUS_SUCCESS;
	
	IoCompleteRequest ( Cmd->Irp , IO_NO_INCREMENT );
	
	ExFreePoolWithTag ( Cmd , USCSI_TAG);
}

//
//
//

NTSTATUS AllocateSCSICommand ( PIRP Irp , PPDO_EXT Ext)
{
	PIO_STACK_LOCATION			Stack;
	PSCSICmd					Cmd;
	PSCSI_REQUEST_BLOCK			Srb;
	PUCHAR						DataBuf;
	UCHAR						TaskAttr , Dir , ScsiOpcode;

	Stack = IoGetCurrentIrpStackLocation( Irp );
	Srb = Stack->Parameters.Scsi.Srb;
	ScsiOpcode = Srb->Cdb[0];
		
	if ( Srb->QueueAction == SRB_SIMPLE_TAG_REQUEST )
		TaskAttr = SCSI_TASK_SIMPLE;		
	else if ( Srb->QueueAction == SRB_HEAD_OF_QUEUE_TAG_REQUEST )
		TaskAttr = SCSI_TASK_HOQ;
	else if ( Srb->QueueAction == SRB_ORDERED_QUEUE_TAG_REQUEST)
		TaskAttr = SCSI_TASK_ORDERED;
	else
		TaskAttr = SCSI_TASK_UNTAGGED;
	
	if ( (Srb->SrbFlags & SRB_FLAGS_DATA_IN) && ( Srb->SrbFlags & SRB_FLAGS_DATA_OUT ))	
		Dir = SCSI_CMD_DIR_BOTH;
	else if ( Srb->SrbFlags & SRB_FLAGS_DATA_IN )
		Dir = SCSI_CMD_DIR_READ;
	else if ( Srb->SrbFlags & SRB_FLAGS_DATA_OUT )
		Dir = SCSI_CMD_DIR_WRITE;
	else
		Dir = SCSI_CMD_DIR_NONE;
			
	//Cmd = ExAllocateFromPagedLookasideList ( &Ext->CmdLookAside );
	Cmd = ExAllocatePoolWithTag ( PagedPool , sizeof (SCSICmd) , USCSI_TAG);
	InitializeListHead ( &Cmd->List );
	Cmd->Irp = Irp;
	/*
	if ( ScsiOpcode == SCSIOP_READ || ScsiOpcode == SCSIOP_WRITE )
		DataBuf = MmGetSystemAddressForMdlSafe (Irp->MdlAddress , NormalPagePriority);
	else 	
		DataBuf = Srb->DataBuffer;*/
	
	if ( NULL != Irp->MdlAddress )
		DataBuf = MmGetSystemAddressForMdlSafe (Irp->MdlAddress , NormalPagePriority);
	else
		DataBuf = Srb->DataBuffer;
		
	ExInterlockedInsertTailList ( &Ext->CmdQueue , &Cmd->List , &Ext->CmdQueueLock);
	/*
	DbgPrint("ScsiOpcode=0x%X SrbFlags=0x%X MdlFlags=0x%X DataBuf=0x%X DataBuffer=0x%X\n" , 
	ScsiOpcode, 
	Srb->SrbFlags,
	Irp->MdlAddress?Irp->MdlAddress->MdlFlags:0x0,
	DataBuf ,
	Srb->DataBuffer );*/
			
	uSCSIProcessSCSICmd ( Ext->Session, 
						  Srb->Cdb , 
						  Srb->CdbLength  , 
						  DataBuf,
						  Srb->DataTransferLength,
						  Srb->DataTransferLength,
						  TaskAttr,
						  Dir,
						  &Cmd->Handle );

	IoMarkIrpPending ( Irp );			
	return STATUS_PENDING;
}

//
//
//

NTSTATUS ProcessCommand (PIRP Irp , PPDO_EXT Ext)
{
	PIO_STACK_LOCATION		Stack;
	PCDB					Cdb;
	UCHAR					OpCode;
	PSCSI_REQUEST_BLOCK		Srb;
	
	Stack = IoGetCurrentIrpStackLocation( Irp );
	Srb = Stack->Parameters.Scsi.Srb;
	Cdb = (PCDB)Srb->Cdb;	
	OpCode = *((PUCHAR)Cdb);
		
	switch ( OpCode )
	{
		case SCSIOP_READ_CAPACITY:		//0x25
		case SCSIOP_TEST_UNIT_READY:	//0x00
		case SCSIOP_INQUIRY:			//0x12
		case SCSIOP_MODE_SELECT:		//0x15
		case SCSIOP_MODE_SENSE:			//0x1A		
		case SCSIOP_READ:				//0x28
		case SCSIOP_WRITE:				//0x2A
		case SCSIOP_VERIFY:				//0x2F
		case SCSIOP_SYNCHRONIZE_CACHE:  //0x35
			return AllocateSCSICommand ( Irp , Ext);
			
		default:
			DbgPrint("%s *** Unprocessed Opcode 0x%X\n" , __FUNCTION__ , OpCode);	
			break;
	}
	
	return STATUS_SUCCESS;
}