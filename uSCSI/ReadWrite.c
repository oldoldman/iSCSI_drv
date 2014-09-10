
#include "precomp.h"

//
//
//

NTSTATUS uFlush (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION	Stack;
	
	Stack = IoGetCurrentIrpStackLocation( Irp );
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest( Irp , IO_NO_INCREMENT );
	return Status;
}

//
//
//

NTSTATUS uCreateClose (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION	Stack;
	
	Stack = IoGetCurrentIrpStackLocation( Irp );
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest( Irp , IO_NO_INCREMENT );
	return Status;
}

//
//
//

NTSTATUS uReadWrite (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION	Stack;
	
	Stack = IoGetCurrentIrpStackLocation( Irp );

	Irp->IoStatus.Status = Status;
	IoCompleteRequest( Irp , IO_NO_INCREMENT );
	return Status;
}