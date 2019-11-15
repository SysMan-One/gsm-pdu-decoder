#ifndef PDU_H
#define PDU_H


//###########################################################################
// @INCLUDE
//###########################################################################
#include <stdint.h>

//###########################################################################
// @DEFINES
//###########################################################################
#define SMS_PDU_MAX_LEN					175
#define SMS_PDU_USER_DATA_MAX_LEN		140  /* 140 octets */
#define TRUNCATED_PDU_DATA_LEN			134
#define SMS_GSM7BIT_MAX_LEN				160
#define TRUNCATED_GSM_DATA_LEN			153
#define ADDR_OCTET_MAX_LEN				20	 /* 10 Octets */
#define TIME_STAMP_OCTET_MAX_LEN		14	 /* 7 Octets */
#define UTF8_CHAR_LEN					4	 /* 4 bytes */
#define GSM_7BIT						0x00
#define ANSI_8BIT						0x01  // TODO
#define UCS2_16BIT						0x02  // TODO
#define USER_DATA_HEADER_INDICATION		0x40
#define STATUS_REPORT_INDICATOR			0x20
#define MSG_REF_NO_DEFAULT				0x00
#define UDH_CONCATENATED_MSG_LEN		0x05
#define IE_CONCATENATED_MSG_LEN			0x03
#define MORE_MSG_TO_SEND				0x04
#define TRUE							 1
#define FALSE							 0
#define LONG_SMS_TEXT_MAX_LEN			700

//###########################################################################
// @ENUMERATOR
//###########################################################################
/* Error Values */
enum
{
	ERR_MSG_TYPE = 0,
	ERR_CHAR_SET,
	ERR_PHONE_TYPE_OF_ADDR,
	ERR_PHONE_NUM_PLAN,
	ERR_PROTOCOL_ID,
	ERR_DATA_CODE_SCHEME
};

/* Message Type indication */
enum	
{
	MSG_TYPE_SMS_DELIVER = 0x00,
	MSG_TYPE_SMS_SUBMIT = 0x01,
	MSG_TYPE_SMS_STATUS_REPORT = 0x02
};

/* Type of Number */
enum
{
   NUM_TYPE_UNKNOWN = 0x00,
   NUM_TYPE_INTERNATIONAL = 0x01,
   NUM_TYPE_NATIONAL = 0x02,
   NUM_TYPE_ALPHANUMERIC = 0x05
};

/* Numbering Plan Indicator */
enum
{
   NUM_PLAN_UNKNOWN = 0x00,
   NUM_PLAN_ISDN = 0x01
};

/* Validity Period type */
enum
{
   VLDTY_PERIOD_DEFAULT = 0x00,
   VLDTY_PERIOD_RELATIVE = 0x10,
   VLDTY_PERIOD_ABSOLUTE = 0x18
};

/* User Data Header Information Element identifier */
enum
{
   IE_CONCATENATED_MSG = 0x00,
   IE_PORT_ADDR_8BIT = 0x04,
   IE_PORT_ADDR_16BIT = 0x05
};

/* Message State */
enum
{
	MSG_DELIVERY_FAIL = 0,
	MSG_DELIVERY_SUCCESS
};

//###########################################################################
// @DATATYPE
//###########################################################################
typedef struct
{
	uint8_t day;
	uint8_t month;
	uint8_t year;

} DATE_DESC;

typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;

} TIME_DESC;


/* PDU Decode Descriptor */
typedef struct
{
	uint8_t smscAddrLen;						/* Length of Service Center Number  */
	uint8_t smscNpi;							/* Numbering Plan Indicactor */
	uint8_t smscTypeOfAddr;					/* Type of Address of Service Center Number */
	char smscAddr[ADDR_OCTET_MAX_LEN + 1];	/* Service Center Number */
	uint8_t firstOct;							/* First octet of PDU SMS */
	uint8_t isHeaderPrsnt;						/* User data header indicator */
	uint8_t msgRefNo;							/* Message Reference Number */
	
	uint8_t phoneAddrLen;						/* Lenght of Phone Number */
					
	uint8_t phoneTypeOfAddr;					/* Type of Address of Phone Number */
	
	char phoneAddr[ADDR_OCTET_MAX_LEN + 1];	/* Phone Number */
	
	uint8_t protocolId;						/* Protocol Identifier */
	uint8_t dataCodeScheme; 					/* Data Coding scheme */
	uint8_t msgType;						    /* Message Type */
	uint8_t isWapPushMsg;						/* WAP-PUSH SMS */
	uint8_t isFlashMsg;						/* FLASH SMS */
	uint8_t isStsReportReq;					/* Staus Report Flag */
	uint8_t isMsgWait;							/* Message Waiting */
	
	uint8_t usrDataFormat;					/* User Data Coding Format */
	
	char timeStamp[TIME_STAMP_OCTET_MAX_LEN + 1];		 /* Service Center Time Stamp */
	char dischrgTimeStamp[TIME_STAMP_OCTET_MAX_LEN + 1]; /* Discharge Time Stamp */
	
	uint8_t vldtPrd;							 /* Validity Period */
	uint8_t vldtPrdFrmt;						 /* Validity Period Format */
	
	uint8_t usrDataLen;							    				/* User Data Length */
	uint8_t usrData[SMS_GSM7BIT_MAX_LEN * UTF8_CHAR_LEN + 1];   		/* User Data for GSM_7bit, ANSI_8bit & UCS2_16bit*/
	uint8_t udhLen;									/* User Data Header Length */
	uint8_t udhInfoType;								/* Type of User Data Header */
	uint8_t udhInfoLen;								/* User Data Header information length */
	uint8_t concateMsgRefNo; 							/* Concatenated Message Reference Number */
	uint8_t concateTotalParts;						/* Maximum Number of concatenated messages */
	uint8_t concateCurntPart;							/* Sequence Number of concatenated messages */
	uint8_t isConcatenatedMsg;							/* Concatenated Msg or Not */
	uint8_t smsSts;					  				/* Status of SMS */
	uint16_t srcPortAddr;								/* Source Port Address */
	uint16_t destPortAddr;							/* Destination Port Address */
	uint8_t isDeliveryReq;
	DATE_DESC date;
	TIME_DESC time;
	
} PDU_DESC;

//###########################################################################
// @PROTOTYPE
//###########################################################################
uint8_t DecodePduData(char *pGsmPduStr, PDU_DESC *pPduDecodeDesc, uint8_t *pError);
uint8_t EncodePduData(PDU_DESC *pPduEncodeDesc, uint8_t *pGsmPduStr, uint16_t gsmPduStrLen);

void print_decoded_pdu(PDU_DESC *pPduDecodeDesc);

#endif	// PDU_H

