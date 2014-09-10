#ifndef _TRANSPORT_H
#define _TRANSPORT_H

//
// An iSCSI Session
//

typedef struct _SESSION
{
	LIST_ENTRY			List;
	//
	// iSCSI Command Sequence
	//
	ULONG				CmdSN;	
	//
	// Outstanding R2T Counter
	//
	ULONG				R2TCount;
	//
	// Command Window
	//
	ULONG				ExpCmdSN;
	ULONG				MaxCmdSN;
	KSPIN_LOCK			WindowLock;
	KEVENT				WindowEvent;
	
	ULONG				TaskTag;
	
	UCHAR				ISID[6];
	UCHAR				TSIH[2];
	//
	BOOLEAN				NormalSession;
	//
	// CID Counter
	//
	USHORT				CID;
	//
	// Session State
	//
	UCHAR				State;
	//
	// Target
	//
	PITGT				Tgt;
	//
	// Session Connections
	//
	KSPIN_LOCK			ConLock;
	LIST_ENTRY			Connections;
	PuCONINFO			LeadingCon;
	PuCONINFO			BestCon;
	//
	// Pdu Release Thread
	//	
	LIST_ENTRY			PduRelease;
	HANDLE				PduReleaser;
	KEVENT				PduReleaseEvent;
	KSPIN_LOCK			PduReleaseLock;
	//
	//
	//
	HANDLE				TdiPnP;
	//
	//
	//
	USCSI_CALL_BACK		CallBack;
	//
	// For traffic stats
	//
	ULONG				Ns100Unit;
	//	
	// Session Wide Parameters
	//
	PKEY_VALUE			Parameter[KEY_MaxSessionKey];
}SESSION , *PSESSION;

// Session States

#define SESSION_STAT_FREE			0x01
#define SESSION_STAT_LOGGED_IN		0x03
#define SESSION_STAT_FAILED			0x04

// Connection
#define CTX_MAX_DISPATCH_WORKERS	1
#define CTX_MAX_SEND_WORKERS		1
#define CTX_MAX_R2T_WORKERS			1
#define CTX_MAX_DATAIN_WORKERS		1
#define CTX_MAX_PENDING_CMD_WORKERS	1

typedef struct _uCON_CTX
{
	//
	// in PDU queue
	//
	LIST_ENTRY				InPDU;
	KSPIN_LOCK				InLock;
	KEVENT					DispatchEvent;
	HANDLE					Dispatchers[CTX_MAX_DISPATCH_WORKERS];
	ULONG					DispatcherCount;
	//
	// out PDU queue
	//
	LIST_ENTRY				OutPDU;	
	KSPIN_LOCK				OutLock;
	KEVENT					OutEvent;
	HANDLE					Senders[CTX_MAX_SEND_WORKERS];
	ULONG					SenderCount;
	//
	// Outstanding R2Ts
	//
	LIST_ENTRY				R2Ts;
	KSPIN_LOCK				R2TLock;	
	KEVENT					R2TEvent;
	HANDLE					R2TWorkers[CTX_MAX_R2T_WORKERS];
	ULONG					R2TWorkerCount;
	//
	// Incoming Data-In
	//
	LIST_ENTRY				DataIns;
	KSPIN_LOCK				DataInLock;
	KEVENT					DataInEvent;
	HANDLE					DataInWorkers[CTX_MAX_DATAIN_WORKERS];
	ULONG					DataInWorkerCount;
	//
	// Command that has been acked
	//
	LIST_ENTRY				PendingCompleteCmds;
	KSPIN_LOCK				PendingCompleteCmdLock;
	KEVENT					PendingCompleteCmdEvent;
	HANDLE					PendingCompleteCmdWorkers[CTX_MAX_PENDING_CMD_WORKERS];
	ULONG					PendingCompleteCmdCount;
	//
	// Pools
	//
	NPAGED_LOOKASIDE_LIST	PDULookAside;	
	NPAGED_LOOKASIDE_LIST  	BHSLookAside;	
	//
	// For PDU assembling
	//
	PPDU					PendingPDU;
	//
	// Outstanding Continue PDU
	//
	PPDU					LinkPDU;
	//
	// Performance Counter
	//
	ULONG					FailedIrpAllocation;
	//
	// back to
	//
	PuCONINFO				ConInfo;	
}uCON_CTX , *PuCON_CTX;

//
// Worker context
//

typedef struct _WORKER_THREAD_CTX
{
	PuCON_CTX				Ctx;
	UCHAR					No;	
}WORKER_THREAD_CTX , *PWORKER_THREAD_CTX;

#define WI_PDU_IN					0x01
#define WI_PDU_OUT					0x02
#define WI_R2T						0x03
#define WI_DATAIN					0x04
#define WI_PENDING_COMPLETE_CMD		0x05
#define WI_PDU_RELEASE				0x06

//
// Traffic stats
//

typedef enum _CON_STATS
{
	CON_STATS_MIN = 0,
	CON_STATS_MAX,
	CON_STATS_AVG,
	CON_STATS_TIMEOUT,
	
	CON_STATS_LAST
}CON_STATS;

#define PENDING_TASK_HASH_SIZE	0x100

//
// iSCSI connection
//

typedef struct _uCONINFO
{
	//
	// On Session list
	//
	LIST_ENTRY				ConList;
	//
	// Status Numbering
	//
	ULONG					ExpStatSN;
	//
	// TRUE if ExpStatSN has been initied with first StatSN
	//
	BOOLEAN					ExpStatSNInited;
	//
	// StatSN Queue sorted by StatSN
	//
	SINGLE_LIST_ENTRY		StatSN;
	//
	//
	//
	NPAGED_LOOKASIDE_LIST  	SNLookAside;	
	//
	// Connection State
	//
	UCHAR					State;
	//
	// ID
	//
	USHORT					CID;	
	
	ULONG					DupResponseCount;
	ULONG					DupStatSN;
	//
	// Transport address , endpoint , control channel
	//
	HANDLE					Addr , Ep , Ctl;		
	PFILE_OBJECT			AddrObj , ConEpObj , CtlObj;			
	//
	// Context
	//
	PuCON_CTX				ConCtx;
	//
	// Session belonging
	//
	PSESSION				Session;
	//
	// State of Text Conversation
	//
	UCHAR					TextState;
	//
	// State of Login Conversation
	//
	UCHAR					LoginState;
	UCHAR					SkipOperationalNegotiation:1;
	UCHAR					SSG:2;
	UCHAR					CSG:2;
	UCHAR					NSG:2;	
	KEVENT					LoginEvent;
	//
	// For Login and Text
	//
	BOOLEAN					MoreWorks;	
	PPDU					Answer;	
	//
	// Used by 
	// 1. Login/Text to hold the head of a logical PDU
	// 2. and Data-In to hold the head of a data sequence
	//
	PPDU					Logical;
	//
	//
	//
	UCHAR					LogoutState;
	//
	// Hashed By InitiatorTaskTag
	//
	LIST_ENTRY				PendingTasks[PENDING_TASK_HASH_SIZE];
	KSPIN_LOCK				PendingTasksLock;
	//
	// Connection Stats
	//
	ULONG					Stats[CON_STATS_LAST];
	//	
	// Connection Wide Parameters
	//
	PKEY_VALUE				Parameter[ KEY_LastKey - KEY_MaxSessionKey - 1];	
	//
	// Transport Parameters
	//
	TDI_CONNECTION_INFO		ConnectionInfo;
	TDI_PROVIDER_INFO		ProviderInfo;
	
}uCONINFO , *PuCONINFO;

#define IsLeadingCon(c)		( (c)->Session->LeadingCon == (c) )

#define KEY_CON_IDX(k)	( k - KEY_MaxSessionKey - 1 )

// Text Exchange State
#define TXT_STAT_IDLE				0x00
#define TXT_STAT_STARTED			0x01
#define TXT_STAT_PENDING_COMPLETE	0x02

// Login Stages
#define LOGIN_STAGE_SecurityNegotiation				0x00
#define LOGIN_STAGE_LoginOperationalNegotiation		0x01
#define LOGIN_STAGE_FullFeaturePhase				0x03

// Login Stage State
#define LOGIN_STAT_IDLE				0x00
#define LOGIN_STAT_STARTED			0x01	// A new stage has started
#define LOGIN_STAT_PENDING_TRANSIT	0x02	// A transit has been requested

//
#define CON_STAT_UNINITIALIZED		0x00
#define CON_STAT_INITIALIZED		0x01
#define CON_STAT_CONNECTED			0x02

// RFC 3720 Connection States

#define CON_STAT_FREE				0x11
#define CON_STAT_XPT_WAIT			0x12
#define CON_STAT_IN_LOGIN			0x14	//Login  , Initialization
#define CON_STAT_LOGGED_IN			0x15	//Full Feature
#define CON_STAT_LOGOUT_REQED		0x16
#define CON_STAT_CLEANUP_WAIT		0x17

//
// Protocol Data Unit
//
typedef struct _PDU
{
	//
	// On In\Out\R2Ts\DataIns Queue or in Pending Cmd Hash
	//
	LIST_ENTRY				PDUList;
	//
	// On Continue List or PendingCompleteCmds Queue
	//
	union
	{
		LIST_ENTRY			Continue;
		LIST_ENTRY			PendingCmd;
	};
	//
	// Referenced SCSI Cmd PDU
	//
	struct _PDU*			Cmd;	
	//
	//
	//
	ULONG					Flags;
	//
	// Response of this command
	//
	LIST_ENTRY				Resps;
	//
	// Data ins or R2Ts
	//
	LIST_ENTRY				DataInOrR2T;	
	ULONG					DupDataInCount;
	KSPIN_LOCK				DataInOrR2TLock;
	//
	// Data outs of SCSI Command / R2T
	//
	LIST_ENTRY				DataOut;
	//
	// For SCSI Command and R2T
	// Numbering Data-Out(s) within a SCSI Command (Unsolicited Do) 
	// or within a R2T (Solicited Do)
	ULONG					DataSN;
	//
	// Data Offset in Data-Out buffer
	//
	ULONG					DoOffset;
	//
	// SCSI staff
	//
	PIRP					OrigIrp;
	PSCSI_REQUEST_BLOCK		Srb;
	PUCHAR					DataBuffer;
	//
	// Sending/Receiving Time of PDU
	//
	union
	{
		LARGE_INTEGER		SendingTime;
		LARGE_INTEGER		ReceivingTime;
	};	
	//
	// SCSI_RESPONSE ExpDataSN
	//
	ULONG					ExpDataSN;
	//
	// Back to
	//
	PuCON_CTX				ConCtx;
	// 
	PBHS					Bhs;
	//
	// Buffer for both Ahs and Data
	//
	PUCHAR					Buffer;	
	PUCHAR					Ahs;		
	PUCHAR					Data;
	// TiReceive stuffs	
	ULONG					BufferSize;	
	ULONG					Bytes;
}PDU , *PPDU;

#define PDU_F_DATA_IN_PROCESSED		0x00000001
#define PDU_F_DATA_IN_RETRANSMIT	0x00000002
#define PDU_F_CMD_CAN_COMPLETE		0x00000004
//
//
//
typedef struct _SN
{
	SINGLE_LIST_ENTRY	Next;
	ULONG				SN;
}SN , *PSN;

//
// Public Functions
//

// TransUtils.c 

VOID TpRegisterPnPHandlers( PSESSION Session);

PPDU TpAllocatePDU(PuCON_CTX Ctx , UCHAR Opcode , UCHAR AHSLen , ULONG DataLen);

VOID TpFreePDU(PuCON_CTX Ctx , PPDU Pdu ,ULONG Level);
	
VOID TpQueuePDU(PuCON_CTX Ctx , PPDU Pdu , UCHAR Which );

PuCONINFO TpBestConnection(PSESSION Session);
	
#define TpQueueInPDU(Ctx , Pdu )					TpQueuePDU( Ctx , Pdu , WI_PDU_IN )
#define TpQueueOutPDU(Ctx , Pdu )					TpQueuePDU( Ctx , Pdu , WI_PDU_OUT )
#define TpQueueR2TPDU(Ctx , Pdu )					TpQueuePDU( Ctx , Pdu , WI_R2T )
#define TpQueueDataInPDU(Ctx , Pdu )				TpQueuePDU( Ctx , Pdu , WI_DATAIN )
#define TpQueuePendingCompleteCmd(Ctx , Pdu )		TpQueuePDU( Ctx , Pdu , WI_PENDING_COMPLETE_CMD )
#define TpQueuePduRelease(Ctx , Pdu)				TpQueuePDU( Ctx , Pdu , WI_PDU_RELEASE)

PuCONINFO TpAddConInfo(PSESSION Session );

NTSTATUS TpConnectTo( PuCONINFO Coninfo , ULONG Address , USHORT Port);

PSESSION TpAllocateSession ();

VOID TpFreeSession (PSESSION Session);

NTSTATUS TpTest();



		
#endif