
#ifndef _USCSI_PORT_H
#define _USCSI_PORT_H

#define SCSI_TASK_UNTAGGED							0x00
#define SCSI_TASK_SIMPLE							0x01
#define SCSI_TASK_ORDERED							0x02
#define SCSI_TASK_HOQ								0x03
#define SCSI_TASK_ACA								0x04

// OP_SCSI_CMD direction
#define SCSI_CMD_DIR_READ							0x01
#define SCSI_CMD_DIR_WRITE							0x02
#define SCSI_CMD_DIR_BOTH							0x03
#define SCSI_CMD_DIR_NONE							0x04

typedef VOID (*uSCSIComplete)( ULONG Handle , 
							   PVOID Context , 
							   UCHAR ScsiStatus , 
							   PUCHAR SenseData,
							   ULONG SenseLength );

typedef struct _USCSI_CALL_BACK
{
	uSCSIComplete		CompleteCmd;
	PVOID				CompleteCtx;
		
}USCSI_CALL_BACK , *PUSCSI_CALL_BACK;

typedef struct _USCSI_SESSION_INFO
{
	PVOID	PlaceHolder;
}USCSI_SESSION_INFO , *PUSCSI_SESSION_INFO;

VOID uSCSIInitialize();

VOID uSCSIAddTarget(PUCHAR Tgt , ULONG Addr , USHORT Port);

ULONG uSCSIGetTargetsSize();

NTSTATUS uSCSIPopTargets( PTGTS Tgts);

PVOID uSCSICreateSession( PUCHAR Target , PUSCSI_CALL_BACK CallBack );

VOID uSCSIProcessSCSICmd ( PVOID 	Session , 
							PUCHAR 	Cdb , 
							ULONG 	CdbLength , 
							PUCHAR 	DataBuf , 
							ULONG 	WriteSize,
							ULONG 	ReadSize ,
							UCHAR	TaskAttr,
							UCHAR	Dir ,
							PULONG	Handle);

#endif