#include "pdu.h"
#include <string.h>
#include <stdio.h>


int main(void)
{
	PDU_DESC pduDesc;
	uint8_t errorType;

	char pdu_buf[512] = "07911326040000F0040B911346610089F60000208062917314080CC8F71D14969741F977FD07";
	memset(&pduDesc, 0x00, sizeof(pduDesc));
	DecodePduData(pdu_buf, &pduDesc, &errorType);

	print_decoded_pdu(&pduDesc);

	return 0;
}
