
#ifndef _PROTOCOL_H
#define _PROTOCOL_H

#define BHS_SIZE			48

#define OP_NOP_OUT			0x00
#define OP_SCSI_CMD			0x01
#define OP_SCSI_TASK_REQ	0x02
#define OP_LOGIN_REQ		0x03
#define OP_TXT_REQ			0x04
#define OP_DATA_OUT			0x05
#define OP_LOGOUT_REQ		0x06
#define OP_SNACK_REQ		0x10

#define OP_NOP_IN			0x20
#define OP_SCSI_REP			0x21
#define OP_SCSI_TASK_REP	0x22
#define OP_LOGIN_REP		0x23
#define OP_TXT_REP			0x24
#define OP_DATA_IN			0x25
#define OP_LOGOUT_REP		0x26
#define OP_R2T				0x31
#define OP_ASYNC_MSG		0x32
#define OP_REJECT			0x3f

typedef union _BHS
{
	struct _BHS_BUF
	{
		UCHAR	Buf[48];
	}BHS_BUF , *PBHS_BUF;
	
	struct _GENERICBHS
	{		
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;		
		
		UCHAR	Opcode_spec:7;
		UCHAR	Final:1;	
		
		UCHAR	Opcode_spec2[2];	
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Opcode_spec3[8];
		};	
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	Opcode_spec4[28];
	}GENERICBHS , *PGENERICBHS;
	
	struct _STAT_FIELDS
	{
		UCHAR	Reserved1[16];
		
		UCHAR	InitiatorTaskTag[4];
		
		UCHAR	TargetTransferTag[4];
		
		union
		{
			// 24
			UCHAR	CmdSN[4];
			UCHAR	StatSN[4];
		};
		union
		{
			// 28
			UCHAR	ExpStatSN[4];
			UCHAR	ExpCmdSN[4];			
		};
		union
		{
			// 32
			UCHAR	Reserved2[4];
			UCHAR	MaxCmdSN[4];
		};
		union
		{
			// 36
			UCHAR	Reserved3[4];
			UCHAR	DataSN[4];
			UCHAR	R2TSN[4];
			UCHAR	ExpDataSN[4];
		};
	}STAT_FIELDS , *PSTAT_FIELDS;
	//0x01
	struct _SCSI_CMD
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;		
		UCHAR	Reserved:1;		
		
		UCHAR	Attr:3;
		UCHAR	Reserved2:2;
		UCHAR	Write:1;	
		UCHAR	Read:1;		
		UCHAR	Final:1;
		
		UCHAR	Reserved3[2];	
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	

		UCHAR	LUN[8];

		UCHAR	InitiatorTaskTag[4];	
		UCHAR	ExpectedDataTansferLength[4];
		UCHAR	CmdSN[4];
		UCHAR	ExpStatSN[4];
		
		UCHAR	Cdb[16];
		
	}SCSI_CMD , *PSCSI_CMD;
	//0x21
	struct _SCSI_RESPONSE
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved:2;
				
		UCHAR	Reserved3:1;
		UCHAR	U:1;
		UCHAR	O:1;
		UCHAR	u:1;
		UCHAR	o:1;
		UCHAR	Reserved2:2;
		UCHAR	Final:1;	
		
		UCHAR	Response;
		UCHAR	Status;
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	

		UCHAR	Reserved4[8];

		UCHAR	InitiatorTaskTag[4];	
		union
		{
			UCHAR	SNACKTag[4];
			UCHAR	Reserved5[4];
		};
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		union
		{
			UCHAR	ExpDataSN[4];
			UCHAR	Reserved6[4];
		};
		union
		{
			UCHAR	BiReadResidualCount[4];
			UCHAR	Reserved7[4];
		};
		union
		{
			UCHAR	ResidualCount[4];
			UCHAR	Reserved8[4];
		};
	}SCSI_RESPONSE , *PSCSI_RESPONSE; 
	
	//0x02
	struct _TASK_MANAGEMENT
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;	
		
		UCHAR	Function:7;
		UCHAR	Final:1;		
			
		UCHAR	Reserved2[2];	
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved3[8];
		};	
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	RefedTaskTag[4];
		UCHAR	CmdSN[4];
		UCHAR	ExpStatSN[4];
		union
		{
			UCHAR	RefCmdSN[4];
			UCHAR	Reserved4[4];
		};
		union
		{
			UCHAR	ExpDataSN[4];
			UCHAR	Reserved5[4];
		};
		UCHAR	Reserved6[28];
	}TASK_MANAGEMENT , *PTASK_MANAGEMENT;
	
	//0x05
	struct _SCSI_DATA_OUT
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;			
		UCHAR	Reserved3[2];	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	TargetTansferTag[4];
		UCHAR	Reserved5[4];
		UCHAR	ExpStatSN[4];
		UCHAR	Reserved6[4];
		UCHAR	DataSN[4];
		UCHAR	BufferOffset[4];
		UCHAR	Reserved7[4];			
	}SCSI_DATA_OUT , *PSCSI_DATA_OUT;
	//0x25
	struct _SCSI_DATA_IN
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;		
		
		UCHAR	S:1;
		UCHAR	U:1;
		UCHAR	O:1;
		UCHAR	Reserved2:3;
		UCHAR	Ack:1;
		UCHAR	Final:1;	
		
		UCHAR	Reserved3;
		union
		{
			UCHAR	Status;
			UCHAR	Reserved4;
		};	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved5[8];
		};	
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	TargetTransferTag[4];
		union
		{
			UCHAR	StatSN[4];
			UCHAR	Reserved6[4];
		};
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		UCHAR	DataSN[4];
		UCHAR	BufferOffset[4];
		UCHAR	ResidualCount[4];			
	}SCSI_DATA_IN , *PSCSI_DATA_IN;
	
	//0x31
	struct _R2T
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;					
		UCHAR	Reserved3[2];	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		UCHAR	LUN[8];

		UCHAR	InitiatorTaskTag[4];	
		UCHAR	TargetTansferTag[4];
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		UCHAR	R2TSN[4];
		UCHAR	BufferOffset[4];
		UCHAR	DesiredDataTransferLength[4];
	}R2T , *PR2T;
	
	//0x32
	struct _ASYNC_MESSAGE
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;		
		UCHAR	Reserved3[2];	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	FFFF[4];	
		UCHAR	Reserved5[4];
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		UCHAR	AsyncEvent;
		UCHAR	AsyncVCode;
		union
		{
			UCHAR	Parameter1[2];
			UCHAR	Reserved6[2];
		};
		union
		{
			UCHAR	Parameter2[2];
			UCHAR	Reserved7[2];
		};
		union
		{
			UCHAR	Parameter3[2];
			UCHAR	Reserved8[2];
		};
		UCHAR	Reserved9[4];		
	}ASYNC_MESSAGE , *PASYNC_MESSAGE;
	
	//0x04
	struct _TXT_REQUEST
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;	
		
		UCHAR	Reserved2:6;			
		UCHAR	Continue:1;
		UCHAR	Final:1;
		
		UCHAR	Reserved3[2];
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	InitiatorTaskTag[4];
		UCHAR	TargetTransferTag[4];
		UCHAR	CmdSN[4];
		UCHAR	ExpStatSN[4];	
		UCHAR	Reserved5[16];
		
	}TXT_REQUEST,*PTXT_REQUEST;
	
	//0x24
	struct _TXT_RESPONSE
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;		
		
		UCHAR	Reserved2:6;
		UCHAR	Continue:1;
		UCHAR	Final:1;			
		UCHAR	Reserved3[2];
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	InitiatorTaskTag[4];
		UCHAR	TargetTransferTag[4];
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];	
		UCHAR	Reserved5[12];
	}TXT_RESPONSE , *PTXT_RESPONSE;
	
	//0x03
	struct _LOGIN_REQUEST
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;	
		
		UCHAR	NSG:2;
		UCHAR	CSG:2;
		UCHAR	Reserved2:2;
		UCHAR	Continue:1;
		UCHAR	Transit:1;		
		
		UCHAR	VersionMax;
		UCHAR	VersionMin;	
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		struct _ISID
		{
			UCHAR	A:6;
			UCHAR	Type:2;			
			UCHAR	B[2] , C , D[2];
		}ISID;
		
		UCHAR	TSIH[2];			
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	CID[2];
		UCHAR	Reserved3[2];
		UCHAR	CmdSN[4];
		union
		{
			UCHAR	ExpStatSN[4];
			UCHAR	Reserved4[4];
		};		
		UCHAR	Reserved5[16];
	}LOGIN_REQUEST , *PLOGIN_REQUEST;
	
	//0x23
	struct _LOGIN_RESPONSE
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;
		
		UCHAR	NSG:2;
		UCHAR	CSG:2;
		UCHAR	Reserved2:2;
		UCHAR	Continue:1;
		UCHAR	Transit:1;			
		
		UCHAR	VersionMax;
		UCHAR	VersionActive;	
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		UCHAR	ISID[6];
		UCHAR	TSIH[2];
			
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	Reserved3[4];
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		
		UCHAR	StatusClass;
		UCHAR	StatusDetail;
		
		UCHAR	Reserved4[8];
				
	}LOGIN_RESPONSE , *PLOGIN_RESPONSE;
	
	//0x06
	struct _LOGOUT_REQUEST
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;		
		
		UCHAR	ReasonCode:7;
		UCHAR	Final:1;			
		UCHAR	Reserved2[2];	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		
		UCHAR	Reserved3[8];
			
		UCHAR	InitiatorTaskTag[4];
		union
		{
			UCHAR	CID[2];
			UCHAR	Reserved4[2];
		};
		UCHAR	Reserved5[2];
		UCHAR	CmdSN[4];
		UCHAR	ExpStatSN[4];
		
		UCHAR	Reserved6[16];
	}LOGOUT_REQUEST , *PLOGOUT_REQUEST;
	
	//0x26
	struct _LOGOUT_RESPONSE
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;	
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;	
		
		UCHAR	Response;
		UCHAR	Reserved3;	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		
		UCHAR	Reserved4[8];
			
		UCHAR	InitiatorTaskTag[4];
		UCHAR	Reserved5[4];
		UCHAR	StatSN[4];
		
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		
		UCHAR	Reserved6[4];
		UCHAR	Time2Wait[2];
		UCHAR	Time2Retain[2];
		UCHAR	Reserved7[4];
		
	}LOGOUT_RESPONSE , *PLOGOUT_RESPONSE;
	
	//0x10
	struct _SNACK
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;		
		
		UCHAR	Type:4;
		UCHAR	Reserved2:3;
		UCHAR	Final:1;
					
		UCHAR	Reserved3[2];
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	InitiatorTaskTag[4];
		union
		{
			UCHAR	TargetTransferTag[4];
			UCHAR	SNACKTag[4];
		};
		UCHAR	Reserved5[4];
		UCHAR	ExpStatSN[4];
		UCHAR	Reserved6[8];
		UCHAR	BegRun[4];
		UCHAR	RunLength[4];
		
	}SNACK , *PSNACK;
	
	//0x3f
	struct _REJECT
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;	
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;		
			
		UCHAR	Reason;
		UCHAR	Reserved3;
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		UCHAR	Reserved4[8];
		UCHAR	FFFF[4];
		
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		union
		{
			UCHAR	DataSN[4];
			UCHAR	R2TSN[4];
			UCHAR	Reserved5[4];
		};	
		UCHAR	Reserved6[8];
		
	}REJECT , *PREJECT;
	
	//0x00
	struct _NOP_OUT
	{
		UCHAR	Opcode:6;
		UCHAR	Immediate:1;
		UCHAR	Reserved1:1;	
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;		
		UCHAR	Reserved3[2];	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	TargetTransferTag[4];
		UCHAR	CmdSN[4];
		UCHAR	ExpStatSN[4];
		UCHAR	Reserved5[16];
	}NOP_OUT , *PNOP_OUT;
	
	//0x20
	struct _NOP_IN
	{
		UCHAR	Opcode:6;
		UCHAR	Reserved1:2;	
		
		UCHAR	Reserved2:7;
		UCHAR	Final:1;			
		UCHAR	Reserved3[2];	
		
		UCHAR	TotalAHSLength;
		UCHAR	DataSegmentLength[3];	
		union
		{
			UCHAR	LUN[8];
			UCHAR	Reserved4[8];
		};	
		UCHAR	InitiatorTaskTag[4];	
		UCHAR	TargetTransferTag[4];
		UCHAR	StatSN[4];
		UCHAR	ExpCmdSN[4];
		UCHAR	MaxCmdSN[4];
		UCHAR	Reserved5[12];
	}NOP_IN , *PNOP_IN;
	
}BHS , *PBHS;

typedef struct _AHS
{
	UCHAR	AHSLength[2];
	UCHAR	AHSType;
	UCHAR	AHS_spec[1];
}AHS, *PAHS;

#define AHS_TYPE_EX_CDB				0x01
#define AHS_TYPE_BIDIR_READ_LENGTH	0x02

//
// Keys
//	

#define KEY_DIR_INITIATOR		0x00000001
#define KEY_DIR_TARGET			0x00000002
#define KEY_DIR_BOTH			(KEY_DIR_INITIATOR|KEY_DIR_TARGET)
#define KEY_DIR_MASK			KEY_DIR_BOTH

#define KEY_PHASE_IALL			0x00000004				//by Initiator , All phases
#define KEY_PHASE_TALL			0x00000008				//by Target , All phases

#define KEY_PHASE_ILO			0x00000010				//by Initiator , Leading Connection Only
#define KEY_PHASE_IIO			0x00000020				//by Initiator , Initialization only
#define KEY_PHASE_ISEC			0x00000040				//by Initiator , Security Nego Stage only
#define KEY_PHASE_IFFPO			0x00000080				//by Initiator , Full Feature Phase only

#define KEY_PHASE_IMASK			(KEY_PHASE_ILO|KEY_PHASE_IIO|KEY_PHASE_IFFPO|KEY_PHASE_IALL)

#define KEY_PHASE_TLO			0x00000100				//by Target , Leading Connection only
#define KEY_PHASE_TIO			0x00000200
#define KEY_PHASE_TSEC			0x00000400
#define KEY_PHASE_TFFPO			0x00000800

#define KEY_PHASE_TMASK			(KEY_PHASE_TLO|KEY_PHASE_TIO|KEY_PHASE_TFFPO|KEY_PHASE_TALL)

#define KEY_DECLARATIVE			0x00001000				//

#define KEY_SCOPE_CO			0x00002000				//Connection wide
#define KEY_SCOPE_SW			0x00004000				//Session wide

#define KEY_IRRELEVANT_WHEN_DISCOVERY		0x00008000

#define KEY_FUNC_MIN			0x00010000
#define KEY_FUNC_MAX			0x00020000
#define KEY_FUNC_OR				0x00040000
#define KEY_FUNC_AND			0x00080000

#define KEY_TYPE_STRING			0x00100000
#define KEY_TYPE_BOOLEAN		0x00200000
#define KEY_TYPE_NUMBER			0x00400000
#define KEY_TYPE_LOV			0x00800000
#define KEY_TYPE_BINARY_NUMBER	0x01000000

#define KEY_DEREGISTERED		0x10000000

#define KEY_NEGO_FAIL_INGORE	0x01
#define KEY_NEGO_FAIL_CLOSE		0x02

#define KEY_NEGO_MAX_RETRIES	0x01

typedef struct _KEY_VALUE KEY_VALUE , *PKEY_VALUE;

typedef VOID (*KEYHANDLER)(KeyVal , ConInfo , Questions , Answers );

typedef struct _KEY_META
{
	PUCHAR			Key;
	UCHAR			KeyLen;
	BOOLEAN			RngValid;
	ULONG			Low , High;	
	ULONG			Attrs;
	UCHAR			NegoFailAction:2;
	UCHAR			NegoRetries:6;
	KEYHANDLER		Handler;
}KEY_META , *PKEY_META;

#define PiKeyName(K)				(PiKeys[K].Key)
#define PiKeyAttrs(K)				(PiKeys[K].Attrs)
#define PiIsKeyDeclarative(K)		(PiKeys[K].Attrs & KEY_DECLARATIVE)
#define PiIsKeyBool(K)				(PiKeys[K].Attrs & KEY_TYPE_BOOLEAN)
#define PiIsKeyString(K)			(PiKeys[K].Attrs & KEY_TYPE_STRING)
#define PiIsKeyLov(K)				(PiKeys[K].Attrs & KEY_TYPE_LOV)	
#define PiIsKeyNumber(K)			(PiKeys[K].Attrs & KEY_TYPE_NUMBER)

#define PiIsKeyDeregistered(K)		(PiKeys[K].Attrs & KEY_DEREGISTERED)

typedef enum _KEY_NAME KEY_NAME;

typedef struct _KEY_VALUE
{
	LIST_ENTRY	Vals;
	ULONG		Size;
	ULONG		Flags;
	UCHAR		Formal , Pending;
	
	struct _Key
	{
		KEY_NAME	D;	//Predefined
		union
		{		
			PUCHAR	U;	//Unknown
		};
	}Key;
	/*Negotiations MUST be handled as
  	 atomic operations.  For example, all negotiated values go into effect
   	after the negotiation concludes in agreement or are ignored if the
   	negotiation fails.
   	*/
	union
	{
		BOOLEAN		Bool;
		PUCHAR		String;	
		ULONG		Number;
		struct _Lov
		{
			PUCHAR	Val;
			PUCHAR	Sel;
		}Lov;		
		struct _Range
		{
			ULONG	Low , High;
		}Range;
	}Value[2];
				
	UCHAR		NegoRetries;
	
	ULONG		Len;		// Key Pair (key=val) Length , including null terminator
	PUCHAR		FormatStr;	// Formatted Key Pair String
}KEY_VALUE , *PKEY_VALUE;

#define KEY_F			0x00
#define KEY_P			0x01

#define KEY_FV(k)			(k)->Value[(k)->Formal]
#define KEY_PV(k)			(k)->Value[(k)->Pending]

//
// for KEY_DECLARATIVE Key 
// Value[KEY_INITIATOR] store initiator value
// Value[KEY_TARGET] store target value
//
#define KEY_I		0x00
#define KEY_T		0x01

#define KEY_IV(k)	(k)->Value[KEY_I]
#define KEY_TV(k)	(k)->Value[KEY_T]

#define KV(ConInfo , Key)					\
	( PiKeys[Key].Attrs & KEY_SCOPE_SW  ?	\
		ConInfo->Session->Parameter[Key]:	\
		ConInfo->Parameter[KEY_CON_IDX(Key)])

#define AddrKV(ConInfo , Key)				\
	( PiKeys[Key].Attrs & KEY_SCOPE_SW  ?	\
		&ConInfo->Session->Parameter[Key]:	\
		&ConInfo->Parameter[KEY_CON_IDX(Key)])

// KEY_VALUE + FormatStr
#define PiAllocateKeyValFormatStr(Len)	PiAllocateKeyVal(0,0,Len)
// KEY_VALUE + Value.String
#define PiAllocateKeyValStr(Len,Len2) 	PiAllocateKeyVal(Len,Len2,0)
#define PiAllocateKeyValStr0(Len) 		PiAllocateKeyVal(Len,0,0)
#define PiAllocateKeyValStr1(Len) 		PiAllocateKeyVal(0,Len,0)
// KEY_VALUE
#define PiAllocateKeyValue()			PiAllocateKeyVal(0,0,0)
// This function should put here , but just for convenience
PKEY_VALUE PiAllocateKeyVal(ULONG StrLen , ULONG StrLen2 , ULONG StrLen3);

//Key nego states
#define KEY_Defaulted		0x00000001
#define KEY_Proposed		0x00000002
#define KEY_Pending			0x00000004
#define KEY_Agreed			0x00000008
#define KEY_STATE_MASK		(KEY_Defaulted | KEY_Proposed | KEY_Pending |KEY_Agreed )

#define KEY_Formatted		0x00000010
// Number Format
#define KEY_FMT_NUM_DEC		0x00000020
#define KEY_FMT_NUM_HEX		0x00000040
#define KEY_FMT_NUM_BASE64	0x00000080
// Format Pending Value
#define KEY_FMT_PENDING		0x00000100

#define KEY_Illegal			0x00800000
#define KEY_Irrelevant		0x01000000
#define KEY_Reject			0x02000000
#define KEY_None			0x04000000
//The Value is "NotUnderstood"
#define KEY_NotUnderstood	0x08000000
//The Key is NotUnderstood
#define KEY_NotUnderstood2	0x10000000

// Key Indexes

typedef enum _KEY_NAME
{
	KEY_MaxConnections = 0 ,
	KEY_SendTargets	,	
	KEY_TargetName	,
	KEY_InitiatorName,
	KEY_TargetAlias	,
	KEY_InitiatorAlias,
	KEY_TargetAddress,
	KEY_TargetPortalGroupTag,
	KEY_InitialR2T	,
	KEY_ImmediateData,
	KEY_MaxBurstLength,
	KEY_FirstBurstLength,
	KEY_DefaultTime2Wait,
	KEY_DefaultTime2Retain,
	KEY_MaxOutstandingR2T,
	KEY_DataPDUInOrder,
	KEY_DataSequenceInOrder,
	KEY_ErrorRecoveryLevel,
	KEY_SessionType,
	
	KEY_MaxSessionKey,
	
	// Followings are Connection Wide
	KEY_HeaderDigest,
 	KEY_DataDigest,
	KEY_MaxRecvDataSegmentLength,
	KEY_AuthMethod,
	
	KEY_LastKey
} KEY_NAME;

#define KEYIN(k)	\
	PROT_Debug(("%s Key <<< " , __FUNCTION__ ));	\
	PiDumpKey( k )

#define KEYOUT(k)	\
	PROT_Debug(("%s Key >>> " , __FUNCTION__ ));	\
	PiDumpKey( k )
	
extern PKEY_META PiKeys;

typedef struct _PDU PDU , *PPDU;
typedef struct _uCONINFO uCONINFO,*PuCONINFO;

typedef union _THREE_BYTE
{
	struct
	{
		UCHAR	Byte0;
		UCHAR	Byte1;
		UCHAR	Byte2;
	};
	
	ULONG	AsULong:24;
	
}THREE_BYTE , *PTHREE_BYTE;

#define REVERSE_3BYTES(Destination, Source) {                \
    PTHREE_BYTE d = (PTHREE_BYTE)(Destination);               \
    PTHREE_BYTE s = (PTHREE_BYTE)(Source);                    \
    d->Byte2 = s->Byte0;                                    \
    d->Byte1 = s->Byte1;                                    \
    d->Byte0 = s->Byte2;   								\
}


//
// Response.c
//
VOID 
	PtProcessResponse ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// Data.c
//
VOID 
	PtProcessDataIn ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// R2T.c
//
VOID 
	PtProcessR2T( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// Task.c
//
VOID 
	PtProcessTask ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// Text.c
//
VOID 
	PtProcessText( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// Login.c
//
VOID 
	PtProcessLogin( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
	
NTSTATUS 
	PtLogin( 
	PuCONINFO ConInfo 
	);

VOID 
	PtProcessLogout ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// Reject.c
//
VOID 
	PtProcessReject ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
//
// Nop.c
//
VOID 
	PtProcessNopIn ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);
	
VOID 
	PtNopOut (
	PuCONINFO ConInfo , 
	BOOLEAN Echo
	);
//
// Protocol.c
//
NTSTATUS 
	PtInit();
	
NTSTATUS 
	PtUnInit();
	
VOID 
	PtProcessAsyncMsg ( 
	PPDU Pdu  , 
	PuCONINFO ConInfo
	);

// Cmd.c
PPDU 
	PtAssembleSCSICmd (
	PuCONINFO 	ConInfo , 
	PUCHAR 		Cdb , 
	ULONG 		CdbLength , 
	PUCHAR 		DataBuffer , 
	ULONG 		WriteSize ,
	ULONG 		ReadSize,
	UCHAR		TaskAttr,
	UCHAR		Dir
	);
#endif