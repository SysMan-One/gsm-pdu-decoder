#ifndef GSMLIB_PDU_H
#define GSMLIB_PDU_H


//###########################################################################
// @INCLUDE
//###########################################################################
#include <stdint.h>

//###########################################################################
// @DEFINES
//###########################################################################
#define SMS_PDU_MAX_LEN					140
#define SMS_UDL_MAX_LEN					153
#define LONG_SMS_TEXT_MAX_LEN			700
#define TRUE							 1
#define FALSE							 0
#define ADDR_MAX_LEN					20	 /* 10 Octets */
#define TIME_STAMP_LEN					14	 /* 7 Octets */

//###########################################################################
// @DATATYPE
//###########################################################################
typedef struct {

} PDU_DECODE_DESC;


//###########################################################################
// @PROTOTYPE
//###########################################################################
void GsmLib_DecodeSmsSubmitPduFrmt(PDU_DECODE_DESC *pPduDecodeDesc);


#endif	//GSMLIB_PDU_H
