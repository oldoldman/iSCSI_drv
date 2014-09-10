
#ifndef USCSI_H
#define USCSI_H

#define USCSI_TAG   (ULONG)'uSCI'

#define FUNC_CODE(n)  ((n&0x3FFC)>>2) 

// {BD97F566-02E4-4f6e-B29B-9B87E662A3E7}
DEFINE_GUID(GUID_USCSI_BUS, 
	0xBD97F566,0x02E4,0x4f6e,0xB2,0x9B,0x9B,0x87,0xE6,0x62,0xA3,0xE7);

#define HARDWAREID  	L"uSCSI\\Disk\0GenDisk\0\0"
#define HARDWAREID_LEN	sizeof (HARDWAREID)

#define COMPATIBLEID  HARDWAREID
#define COMPATIBLEID_LEN  HARDWAREID_LEN 

#define DEVICEDESC  	L"uSCSI Disk\0"
#define DEVICEDESC_LEN  sizeof(DEVICEDESC)   

#define DEVICEID		L"uSCSI\\Disk\0"
#define DEVICEID_LEN	sizeof(DEVICEID)

#define DEVICELOC   	L"uSCSI at cloud end\0"
#define DEVICELOC_LEN 	sizeof (DEVICELOC)

//
//
//
#define DBG_SRB
#define DBG_PNP
#define DBG_IOCTL

#define TMP_Debug( c )	\
		DbgPrint c
		
#ifdef DBG_USCSI
#define USCSI_Debug( c )	\
		DbgPrint c
#else
#define USCSI_Debug( c )
#endif

#ifdef DBG_SRB
#define SRB_Debug( c )	\
		DbgPrint c
#else
#define SRB_Debug( c )
#endif

#ifdef DBG_PNP
#define PNP_Debug( c )	\
		DbgPrint c
#else
#define PNP_Debug( c )
#endif

#ifdef DBG_IOCTL
#define IOCTL_Debug( c )	\
		DbgPrint c
#else
#define IOCTL_Debug( c )
#endif

#define DPFLTR_USCSI	DPFLTR_IHVDRIVER_ID

#define DBG_USCSI		0x1f

typedef struct _COMMON_EXT
{
	BOOLEAN			IsFDO;
	
	PDEVICE_OBJECT	Self;
	PDEVICE_OBJECT	LowerDev;
	
}COMMON_EXT , *PCOMMON_EXT;
	
typedef struct _FDO_EXT
{
	COMMON_EXT;
	
	UNICODE_STRING	InterfaceName;
	
	ULONG			PDOCount;	
	KSPIN_LOCK		PDOLock;
	LIST_ENTRY		PDOList;
		
}FDO_EXT , *PFDO_EXT;

typedef struct _PDO_EXT
{
	COMMON_EXT;
	
	LIST_ENTRY				PDOList;
	
	BOOLEAN					Claimed;
	BOOLEAN					Reported;
	
	PVOID					Session;
	PUCHAR					Target;
	
	PAGED_LOOKASIDE_LIST	CmdLookAside;
	
	KSPIN_LOCK				CmdQueueLock;
	LIST_ENTRY				CmdQueue;
	
}PDO_EXT , *PPDO_EXT;

typedef struct _SCSICmd
{
	LIST_ENTRY	List;
	PIRP		Irp;
	ULONG		Handle;
}SCSICmd , *PSCSICmd;
//
// uSCSI.c
//
DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath);
DRIVER_ADD_DEVICE uAddDevice;
NTSTATUS uAddDevice( IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT Pdo);
//
// IoCtl.c
//
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH uIoCtl;
NTSTATUS uIoCtl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

//
// Pnp.c
//
__drv_dispatchType(IRP_MJ_PNP) DRIVER_DISPATCH uPnP;
NTSTATUS uPnP (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
//    
// Utils.c
//
IO_COMPLETION_ROUTINE CompletionRoutine;
NTSTATUS CompletionRoutine (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS SendIrpSynchronously (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS ProcessCommand (PIRP Irp , PPDO_EXT Ext);

//
// ReadWrite.c
__drv_dispatchType(IRP_MJ_READ) DRIVER_DISPATCH uReadWrite;
__drv_dispatchType(IRP_MJ_WRITE) DRIVER_DISPATCH uReadWrite;
NTSTATUS uReadWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

__drv_dispatchType(IRP_MJ_CLOSE) DRIVER_DISPATCH uCreateClose;  
NTSTATUS uCreateClose (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

__drv_dispatchType(IRP_MJ_FLUSH_BUFFERS) DRIVER_DISPATCH uFlush;
NTSTATUS uFlush (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

//
// Srb.c
__drv_dispatchType(IRP_MJ_SCSI) DRIVER_DISPATCH uScsi;
NTSTATUS uScsi (IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
#endif
