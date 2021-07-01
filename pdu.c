/*
 *   DESCRIPTION:	3GPP PDU decode/encode API
 *
 *   ABSTRACT:
 *
 *   AUTHOR: https://github.com/mehul-m-prajapati
 *
 *   CREATION DATE: 22-APR-2021
 *
 *   USAGE:
 *
 *   MODIFICATION HISTORY:
 *
 *	31-MAR-2021	RRL	Small formating corrections.
 *
 *	 2-JUN-2021	RRL	Fixed bug in the EncodingPDU()
 *
 *	 7-JUN-2021	ALX,RRL	Added a local version of the strnlen() routine
 *
 */


//###########################################################################
// @INCLUDES
//###########################################################################
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"pdu.h"




//###########################################################################
// @DEFINES
//###########################################################################
#define ESC_CHR							0x1B
#define TIME_STAMP_LEN						7
#define MSG_CLASS0						0x00
#define MSG_CLASS1						0x01

//###########################################################################
// @ENUMERATOR
//###########################################################################
/* Data Coding Scheme Groups */
enum
{
	GROUP1_WITH_MSG_CLASS = 0x10,
	GROUP1_WITH_NO_MSG_CLASS = 0x00,
	GROUP2_WITH_MSG_CLASS = 0xF0
};

//###########################################################################
// @GLOBAL VARIABLE
//###########################################################################

//###########################################################################
// @FUNCTIONS
//###########################################################################
static uint8_t i_Hex2Ascii(uint8_t hexNibble);
static uint8_t i_Ascii2Hex(uint8_t asciiChar);
static int	__bin2hex(unsigned char *hexBuf, int hexBufLen, unsigned char *asciiStrng);
static uint8_t	__hex2bin(uint8_t *asciiStrng, uint8_t *hexBuf);
static uint8_t i_DecSemiOctet2Ascii(uint8_t *decSemiOctetBuf, uint8_t *asciiStrng);
static uint8_t i_Ascii2DecSemiOctet(uint8_t *asciiStrng, uint8_t *decSemiOctetBuf);

static void	i_Gsm7BitDfltChrToUtf8Chr(uint8_t cIn, uint8_t *utf8Char, int *utf8CharLen);
static void	i_GsmExt7BitChrToUtf8Chr(uint8_t cIn, uint8_t *utf8Char, int *utf8CharLen);
static uint16_t	i_GsmStrToUtf8Str(uint8_t *pStrInGsm, int strInGsmLen, uint8_t *pStrOutUtf);
static void	i_Utf8StrToGsmStr(uint8_t *cIn, uint16_t cInLen, uint8_t *gsmOut, int *gsmLen);

static uint8_t i_Text2Pdu(uint8_t *pAsciiBuf, uint8_t asciiLen, uint8_t *pPduBuf);
static int i_Pdu2Text(uint8_t *pPduBuf, uint8_t pduLen, uint8_t *pAsciiBuf);


/*  DESCRIPTION: a local version equivalent of the C RTL strnlen() routine
 *
 *   INPUTS:
 *	str:	a source string
 *	maxlen:	a maximum length to scan for NIL char
 *
 *   OUTPUS:
 *	NONE:
 *
 *   RETURNS:
 *	length of the string
 */
static	inline size_t __strnlen(char *str, size_t maxlen)
{
char	*cp;

	for(cp = str ;maxlen && *cp; maxlen--, cp++);

	return	cp - str;
}




//***************************************************************************
// @NAME        : Hex2Ascii
// @PARAM       : hexNibble
// @RETURNS     : Ascii Character or FALSE
// @DESCRIPTION : This function converts hex nibble to ascii character,
//				  and if fails than return FALSE.
//***************************************************************************
static inline uint8_t i_Hex2Ascii(uint8_t hexNibble)
{
	if (hexNibble <= 0x09)
		return (hexNibble + '0');
	else  if ((hexNibble >= 0x0A) && (hexNibble <= 0x0F))
		return (hexNibble + 'A');

	return	0;
}

//***************************************************************************
// @NAME        : i_Ascii2Hex
// @PARAM       : uint8_t asciiChar
// @RETURNS     : hex nibble or FALSE
// @DESCRIPTION : This function converts ascii pack to hex nibble.
//***************************************************************************
static inline uint8_t i_Ascii2Hex(uint8_t asciiChar)
{
	if ((asciiChar >= '0') && (asciiChar <= '9'))
		return (asciiChar - '0');
	else  if ((asciiChar >= 'A') && (asciiChar <= 'F'))
		return (asciiChar - 'A');

	return	0;
}

//***************************************************************************
// @NAME        : HexBuf2AsciiBuf
// @PARAM       : hexBuf - Pointer to buffer containg hex data
//				  hexBufLen - length of hexBuf
// @RETURNS     : Length of Ascii string, if fails in between than returns FALSE.
// @DESCRIPTION : This function converts series of hex data in to ascii string.
//***************************************************************************
static int __bin2hex(unsigned char  *pHexBuf, int hexBufLen, unsigned char *pAsciiStrng)
{
int	idx = 0;
unsigned char  *hp, hn, ln, asciiChar, *cp;

	hp = pHexBuf;
	cp = pAsciiStrng;

	for (idx = 0; idx < hexBufLen; idx++, hp++)
		{
		hn = (*hp) >> 4;
		ln = (*hp) & 0x0F;

		if ( (asciiChar = i_Hex2Ascii(hn)) )
			*cp++ = asciiChar;
		else	return 0;

		if ( (asciiChar = i_Hex2Ascii(ln)) )
			*cp++ = asciiChar;
		else	return 0;;
		}

	*cp = '\0';

	return (cp - pAsciiStrng);
}

//***************************************************************************
// @NAME        : AsciiBuf2HexBuf
// @PARAM       : asciiStrng - Pointer to buffer string buffer.
//				  hexBuf - Pointer to hex buffer.
// @RETURNS     : Length of hex buffer, if fails in between than returns FALSE.
// @DESCRIPTION : This function converts ascii string in to siries of hex data.
//***************************************************************************
static uint8_t __hex2bin(uint8_t *pAsciiStrng, uint8_t *pHexBuf)
{
int	idx = 0, hidx = 0, asciiLen = 0;
uint8_t hexData = 0, higherNibble = 0, lowerNibble = 0, asciiChar = 0;

	asciiLen = strlen ( (char *) pAsciiStrng);
	asciiLen = asciiLen >> 1;

	for (idx =0; idx < asciiLen; idx++)
		{
		/* Process higher nibble */
		asciiChar = pAsciiStrng[idx*2];
		higherNibble = i_Ascii2Hex(asciiChar);
		higherNibble = higherNibble << 4;

		/* Process lower nibble */
		asciiChar = pAsciiStrng[(idx*2)+1];
		lowerNibble = i_Ascii2Hex(asciiChar);

		/* Prepare complete hex byte */
		hexData = higherNibble | lowerNibble;
		pHexBuf[hidx++] = hexData;
		}

	return (hidx);
}

//***************************************************************************
// @NAME        : i_Utf8StrToGsmStr
// @PARAM       : uint8_t *cIn - the pointer to buffer containing UTF8 characters.
//				  uint16_t cInLen - length of data in cIn buffer.
//				  uint8_t *gsmOut - The pointer to buffer which carries converetd
//								  string in Gsm character set.
//				  uint8_t *gsmLen - The pointer to length of converetd data with
//								  Gsm character set.
// @RETURNS     : void
// @DESCRIPTION : This function converts string in UTF8 character set to
//				  Gsm cgaracter set.
//***************************************************************************
void i_Utf8StrToGsmStr(uint8_t *cIn, uint16_t cInLen, uint8_t *gsmOut, int *gsmLen)
{
int	gsmIdx = 0, cInidx = 0;
uint8_t charVar = 0;


	while (cInidx < cInLen)
	{
		if (gsmIdx >= LONG_SMS_TEXT_MAX_LEN)
		{
			gsmIdx = LONG_SMS_TEXT_MAX_LEN;
			break;
		}

		charVar = cIn[cInidx++];
		switch(charVar)
		{
			// Characters not listed here are equal to those in the
			// UTF8 charset OR not present in it.

			case 0x40:
				gsmOut[gsmIdx++] = 0;
				break;

			case 0x24:
				gsmOut[gsmIdx++] = 2;
				break;

			case 0x5F:
				gsmOut[gsmIdx++] = 17;
				break;

			case 0xc2:
				switch (cIn[cInidx++])
				{
					case 0xA3:
						gsmOut[gsmIdx++] = 1;
						break;

					case 0xA5:
						gsmOut[gsmIdx++] = 3;
						break;

					case 0xA4:
						gsmOut[gsmIdx++] = 36;
						break;

					case 0xA1:
						gsmOut[gsmIdx++] = 64;
						break;

					case 0xA7:
						gsmOut[gsmIdx++] = 95;
						break;

					case 0xBF:
						gsmOut[gsmIdx++] = 96;
						break;

					default:
						gsmOut[gsmIdx++] = ' ';
						break;
				}
				break;

			case 0xc3:
				switch (cIn[cInidx++])
				{
					case 0xA8:
						gsmOut[gsmIdx++] = 4;
						break;

					case 0xA9:
						gsmOut[gsmIdx++] = 5;
						break;

					case 0xB9:
						gsmOut[gsmIdx++] = 6;
						break;

					case 0xAC:
						gsmOut[gsmIdx++] = 7;
						break;

					case 0xB2:
						gsmOut[gsmIdx++] = 8;
						break;

					case 0x87:
						gsmOut[gsmIdx++] = 9;
						break;

					case 0x98:
						gsmOut[gsmIdx++] = 11;
						break;

					case 0xB8:
						gsmOut[gsmIdx++] = 12;
						break;

					case 0x85:
						gsmOut[gsmIdx++] = 14;
						break;

					case 0xA5:
						gsmOut[gsmIdx++] = 15;
						break;

					case 0x86:
						gsmOut[gsmIdx++] = 28;
						break;

					case 0xA6:
						gsmOut[gsmIdx++] = 29;
						break;

					case 0x9F:
						gsmOut[gsmIdx++] = 30;
						break;

					case 0x89:
						gsmOut[gsmIdx++] = 31;
						break;

					case 0x84:
						gsmOut[gsmIdx++] = 91;
						break;

					case 0x96:
						gsmOut[gsmIdx++] = 92;
						break;

					case 0x91:
						gsmOut[gsmIdx++] = 93;
						break;

					case 0x9C:
						gsmOut[gsmIdx++] = 94;
						break;

					case 0xA4:
						gsmOut[gsmIdx++] = 123;
						break;

					case 0xB6:
						gsmOut[gsmIdx++] = 124;
						break;

					case 0xB1:
						gsmOut[gsmIdx++] = 125;
						break;

					case 0xBC:
						gsmOut[gsmIdx++] = 126;
						break;

					case 0xA0:
						gsmOut[gsmIdx++] = 127;
						break;

					default:
						gsmOut[gsmIdx++] = ' ';
						break;
				}
				break;


			case 0xce:
				switch (cIn[cInidx++])
				{
					case 0x94:
						gsmOut[gsmIdx++] = 16;
						break;

					case 0xA6:
						gsmOut[gsmIdx++] = 18;
						break;

					case 0x9B:
						gsmOut[gsmIdx++] = 20;
						break;

					case 0xA9:
						gsmOut[gsmIdx++] = 21;
						break;

					case 0xA0:
						gsmOut[gsmIdx++] = 22;
						break;

					case 0xA8:
						gsmOut[gsmIdx++] = 23;
						break;

					case 0xA3:
						gsmOut[gsmIdx++] = 24;
						break;

					case 0x98:
						gsmOut[gsmIdx++] = 25;
						break;

					case 0x9E:
						gsmOut[gsmIdx++] = 26;
						break;

					default:
						gsmOut[gsmIdx++] = ' ';
						break;
				}
				break;

			case 0xe2:
				if ((cIn[cInidx++] == 0x82) /*&& (cIn[cInidx++] == 0x03)*/)
				{
						switch (cIn[cInidx++])
						{
							case 0xAC:
								gsmOut[gsmIdx++] = ESC_CHR;
								gsmOut[gsmIdx++] = 0x65;
								break;

							default:
								gsmOut[gsmIdx++] = ' ';
								break;
						}
				}
				break;

			// extension table
			case '\f':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 10;
				break; // form feed, 0x0C

			case '^':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 20;
				break;

			case '{':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 40;
				break;

			case '}':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 41;
				break;

			case '\\':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 47;
				break;

			case '[':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 60;
				break;

			case '~':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 61;
				break;

			case ']':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 62;
				break;

			case '|':
				gsmOut[gsmIdx++] = ESC_CHR;
				gsmOut[gsmIdx++] = 64;
				break;

			default:
				gsmOut[gsmIdx++] = charVar;
				break;
		}
	}

	*gsmLen = gsmIdx;
}

//***************************************************************************
// @NAME        : i_Gsm7BitDfltChrToUtf8Chr
// @PARAM       : char cIn - Gsm7 bit default Character.
//				  uint8_t *utf8Char - The pointer to converted UTF character.
//				  uint8_t *utf8CharLen - The pointer to length of converted UTF
//				 					   character.
// @RETURNS     : void
// @DESCRIPTION : This function converts Gsm 7-bit default character to UTF
//				  characetr set.
//***************************************************************************
void i_Gsm7BitDfltChrToUtf8Chr(uint8_t cIn, uint8_t *utf8Char, int *utf8CharLen)
{
int idx = 0;

	switch(cIn)
	{
		// Characters not listed here are equal to those in the
		// UTF8 charset OR not present in it.

		case 0:
			utf8Char[idx++] = 0x40;
			break;

		case 1:
			utf8Char[idx++] = 0xc2;
			utf8Char[idx++] = 0xA3;
			break;

		case 2:
			utf8Char[idx++] = 0x24;
			break;

		case 3:
			utf8Char[idx++] = 0xc2;
			utf8Char[idx++] = 0xA5;
			break;

		case 4:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xA8;
			break;

		case 5:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xA9;
			break;

		case 6:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xB9;
			break;

		case 7:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xAC;
			break;

		case 8:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xB2;
			break;

		case 9:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x87;
			break;

		case 11:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x98;
			break;

		case 12:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xB8;
			break;

		case 14:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x85;
			break;

		case 15:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xA5;
			break;

		case 16:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0x94;
			break;	/* Added */

		case 17:
			utf8Char[idx++] = 0x5F;
			break;

		case 18:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0xA6;
			break;	/* Added */

		case 19:
			/* Pending */
			break;	/* Added */

		case 20:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0x9B;
			break;	/* Added */

		case 21:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0xA9;
			break;	/* Added */

		case 22:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0xA0;
			break;	/* Added */

		case 23:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0xA8;
			break;	/* Added */

		case 24:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0xA3;
			break;	/* Added */

		case 25:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0x98;
			break;	/* Added */
		case 26:
			utf8Char[idx++] = 0xce;
			utf8Char[idx++] = 0x9E;
			break;	/* Added */

		case 28:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x86;
			break;

		case 29:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xA6;
			break;

		case 30:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x9F;
			break;

		case 31:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x89;
			break;

		case 36:
			utf8Char[idx++] = 0xc2;
			utf8Char[idx++] = 0xA4;
			break; // 164 in UTF8

		case 64:
			utf8Char[idx++] = 0xc2;
			utf8Char[idx++] = 0xA1;
			break;

		// 65-90 capital letters
		case 91:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x84;
			break;

		case 92:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x96;
			break;

		case 93:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x91;
			break;

		case 94:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0x9C;
			break;

		case 95:
			utf8Char[idx++] = 0xc2;
			utf8Char[idx++] = 0xA7;
			break;

		case 96:
			utf8Char[idx++] = 0xc2;
			utf8Char[idx++] = 0xBF;
			break;

		// 97-122 small letters
		case 123:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xA4;
			break;

		case 124:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xB6;
			break;

		case 125:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xB1;
			break;

		case 126:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xBC;
			break;

		case 127:
			utf8Char[idx++] = 0xc3;
			utf8Char[idx++] = 0xA0;
			break;

		default:
			utf8Char[idx++] = cIn;
			break;
	}

	*utf8CharLen = idx;
}

//***************************************************************************
// @NAME        : i_GsmExt7BitChrToUtf8Chr
// @PARAM       : char cIn - Gsm extended 7-bit default Character.
//				  uint8_t *utf8Char - The pointer to converted UTF character.
//				  uint8_t *utf8CharLen - The pointer to length of converted UTF
//				 					   character.
// @RETURNS     : void
// @DESCRIPTION : This function converts Gsm extended 7-bit character to UTF
//				  characetr set.
//***************************************************************************
void i_GsmExt7BitChrToUtf8Chr(uint8_t cIn, uint8_t *utf8Char, int *utf8CharLen)
{
int idx = 0;

	switch(cIn)
	{
		// Characters not listed here are equal to those in the
		// UTF8 charset OR not present in it.

		// extension table
		case 10:
			utf8Char[idx++] = '\f';
			break; // form feed, 0x0C

		case 20:
			utf8Char[idx++] = '^';
			break;

		case 40:
			utf8Char[idx++] = '{';
			break;

		case 41:
			utf8Char[idx++] = '}';
			break;

		case 47:
			utf8Char[idx++] = '\\';
			break;

		case 60:
			utf8Char[idx++] = '[';
			break;

		case 61:
			utf8Char[idx++] = '~';
			break;

		case 62:
			utf8Char[idx++] = ']';
			break;

		case 64:
			utf8Char[idx++] = '|';
			break;

		case 101:
			utf8Char[idx++] = 0xe2;
			utf8Char[idx++] = 0x82;
			utf8Char[idx++] = 0xAC;
			break; // 164 in UTF8

		default:
			utf8Char[idx++] = cIn;
			break;
	}

	*utf8CharLen = idx;
}

//***************************************************************************
// @NAME        : i_GsmStrToUtf8Str
// @PARAM       : uint8_t *pStrInGsm - The pointer to string with Gsm character set.
//				  uint8_t strInGsmLen - length of string with gsm character set..
//				  uint8_t *pStrOutUtf - The pointer to buffer containing converted
//									  UTF8 character set.
// @RETURNS     : void
// @DESCRIPTION : This function converts Gsm 7-bit characters to UTF8 characters.
//***************************************************************************
uint16_t i_GsmStrToUtf8Str(uint8_t *pStrInGsm, int strInGsmLen, uint8_t *pStrOutUtf)
{
int index = 0, utf8CharLen = 0, cnvrtdStrIndex = 0;

	for (index = 0; index < strInGsmLen; index++)
		{
		if (pStrInGsm[index] == ESC_CHR)
			{
			index++;
			i_GsmExt7BitChrToUtf8Chr(pStrInGsm[index], &pStrOutUtf[cnvrtdStrIndex], &utf8CharLen);
			}
		else	i_Gsm7BitDfltChrToUtf8Chr(pStrInGsm[index], &pStrOutUtf[cnvrtdStrIndex], &utf8CharLen);

		cnvrtdStrIndex += utf8CharLen;

		}

	if (cnvrtdStrIndex > 550)
		cnvrtdStrIndex = 550;

	pStrOutUtf[cnvrtdStrIndex] = '\0';

	return (cnvrtdStrIndex);
}

//***************************************************************************
// @NAME        : i_DecSemiOctet2Ascii
// @PARAM       : decSemiOctetBuf - Pointer to decimal semi octet buffer.
//				  asciiStrng - Pointer to ascii buffer.
// @RETURNS     : void.
// @DESCRIPTION : This function converts decimal semi octet in to ascii.
//***************************************************************************
static uint8_t i_DecSemiOctet2Ascii(uint8_t *pDecSemiOctetBuf, uint8_t *pAsciiStrng)
{
int	idx, loopCnt, decSemiOctetLen;
uint8_t asciiChar, asciiNextChar;


	decSemiOctetLen = strlen ( (char *) pDecSemiOctetBuf);
	loopCnt = decSemiOctetLen >> 1;

	for (idx = 0; idx < loopCnt; idx++)
	{
		asciiChar = pDecSemiOctetBuf[idx*2];
		asciiNextChar = pDecSemiOctetBuf[(idx*2) + 1];

		pAsciiStrng[idx*2] = asciiNextChar;
		if (idx == (loopCnt - 1))
		{
			if (asciiChar == 'F')
			{
				pAsciiStrng[(idx*2) + 1] = '\0';
				return (decSemiOctetLen - 1);
			}
			else
			{
				pAsciiStrng[(idx*2) + 1] = asciiChar;
			}
		}
		else
		{
			pAsciiStrng[(idx*2) + 1] = asciiChar;
		}
	}

	pAsciiStrng[idx*2] = '\0';

	return (decSemiOctetLen);
}


//***************************************************************************
// @NAME        : i_Ascii2DecSemiOctet
// @PARAM       : asciiStrng - Pointer to ascii buffer.
//				  decSemiOctetBuf - Pointer to decimal semi octet buffer.
// @RETURNS     : void.
// @DESCRIPTION : This function converts ascii in to decimal semi octet.
//***************************************************************************
static uint8_t i_Ascii2DecSemiOctet(uint8_t *pAsciiStrng, uint8_t *pDecSemiOctetBuf)
{
	uint8_t idx = 0;
	uint8_t asciiChar = 0;
	uint8_t asciiNextChar = 0;
	uint8_t asciiLen= 0;
	uint8_t loopCnt = 0;
	uint8_t isToAddF = 0;

	asciiLen = strlen ( (char *) pAsciiStrng);

	if ((asciiLen % 2) != 0)
	{
		asciiLen++;
		isToAddF = TRUE;
	}
	loopCnt = asciiLen >> 1;

	for (idx = 0; idx < loopCnt; idx++)
	{
		asciiChar = pAsciiStrng[idx*2];
		if (idx == (loopCnt - 1))
		{
			if (isToAddF == TRUE)
			{
				asciiNextChar = 'F';
			}
			else
			{
				asciiNextChar = pAsciiStrng[(idx*2) + 1];
			}
		}
		else
		{
			asciiNextChar = pAsciiStrng[(idx*2) + 1];
		}

		pDecSemiOctetBuf[idx*2] = asciiNextChar;
		pDecSemiOctetBuf[(idx*2) + 1] = asciiChar;
	}
	pDecSemiOctetBuf[idx*2] = '\0';

	return (asciiLen);
}


//***************************************************************************
// @NAME        : i_TextToPdu
// @PARAM       : asciiBuf- Pointer to ascii buffer containing text data.
//				  pduBuf - Pointer to pdu buffer(for converted pdu data).
// @RETURNS     : uint8_t,pdu data length.
// @DESCRIPTION : This function converts Text data(7bit) to pdu data(8bit).
//***************************************************************************
static uint8_t i_Text2Pdu(uint8_t *pAsciiBuf, uint8_t asciiLen, uint8_t *pPduBuf)
{
int	idx = 0, index = 0, pduIndex = 0;
uint8_t procChar = 0;
uint8_t procNextChar = 0;
uint8_t procVariable = 0;
uint8_t tempVariable;
uint8_t asciiChar = 0;

	for (idx = 0; idx < asciiLen; idx++)
		{
		asciiChar = pAsciiBuf[idx];
		procChar = asciiChar >> index;

		if (index != 7)
			{
			procNextChar = pAsciiBuf[idx + 1];
			tempVariable = procNextChar;
			procVariable = 0xFF << (index + 1);
			tempVariable = tempVariable & (~procVariable);
			tempVariable = tempVariable << (7 - index);

			pPduBuf[pduIndex++] = procChar | tempVariable;
			}

		index++;
			if (index > 7)	index = 0;
		}

	return (pduIndex);
}

//***************************************************************************
// @NAME        : i_PduToText
// @PARAM       : pduBuf - Pointer to pdu buffer(for converted pdu data).
//				: pduLen - length of pdu data.
//				  asciiBuf- Pointer to ascii buffer containing text data.
// @RETURNS     : uint8_t,pdu data length.
// @DESCRIPTION : This function converts pdu data(8bit) to text data(7bit).
//***************************************************************************
static int i_Pdu2Text(uint8_t *pPduBuf, uint8_t pduLen, uint8_t *pAsciiBuf)
{
int	idx = 0, index = 0;
uint8_t asciiIndex = 0;
uint8_t procByte = 0;
uint8_t procPrevByte = 0;
uint8_t procVariable = 0;
uint8_t tempVariable = 0;
uint8_t pduByte = 0;


	for (idx = 0; idx < pduLen; idx++)
		{
		pduByte = pPduBuf[idx];
		procByte = pduByte & (0xFF >> (index + 1));
		procByte = procByte << index;

		if (idx > 0)
			{
			procPrevByte = pPduBuf[idx - 1];
			tempVariable = procPrevByte;
			procVariable = 0xFF >> index;
			tempVariable = tempVariable & (~procVariable);
			tempVariable = tempVariable >> (7 - (index - 1));
			}
		else	tempVariable = 0;

		pAsciiBuf[asciiIndex++] = procByte | tempVariable;

		if (index == 6)
			{
			if (idx == (pduLen - 1))
				{
				if ((pduByte >> 1) != 0)
					pAsciiBuf[asciiIndex++] = pduByte >> 1;
				}
			else	pAsciiBuf[asciiIndex++] = pduByte >> 1;
			}

		index++;
		if (index  > 6)
			index = 0;
		}

	pAsciiBuf[asciiIndex] = '\0';

	return (asciiIndex);
}

//***************************************************************************
// @NAME        : DecodePduData
// @PARAM       : pGsmPduStr-Reference To PDU String,
//				  gsmPduStrLen-Length of PDU String,
//				  PDU_DECODE_DESC-Object Pointer
// @RETURNS     : TRUE/FALSE
// @DESCRIPTION : This function extracts Pdu String data & fills relevant parameters in Descriptor
//				  if fails then return FALSE.
//***************************************************************************
int	DecodePduData(unsigned char *pdu, PDU_DESC *pdsc, int *pError)
{
 int	idx = 0, length = 0, addrLen = 0, asciiLen = 0;
 uint8_t npi = 0;
 uint8_t udl = 0;
 unsigned char  obuf[SMS_PDU_MAX_LEN];
 uint8_t oct = 0;
 uint8_t grpId = 0;

	memset(pdsc, 0, sizeof(PDU_DESC));					/* Zeroing output structure */

	__hex2bin(pdu, obuf);						/* Converting whole Ascii String to Hex String */

	pdsc->smscAddrLen = obuf[idx++];					/* Service center Number Length */

	pdsc->smscTypeOfAddr = obuf[idx++];					/* Service Center Type of Address (Eg: 91 , 81) */

	npi = pdsc->smscTypeOfAddr & 0x0F;					/* Numbering Plan Identification */

	pdsc->smscTypeOfAddr = (pdsc->smscTypeOfAddr & 0x70) >> 4;		/* Type of Number */

										/* Service Center Number */
	addrLen = pdsc->smscAddrLen - 1;					/* Subtracting Type of Addr octet length */
	__bin2hex(&obuf[idx], addrLen, pdsc->smscAddr);
	pdsc->smscAddrLen = i_DecSemiOctet2Ascii(pdsc->smscAddr, pdsc->smscAddr);/* Internal Swapping */
	idx += addrLen;

	/* First Octet of SMS_DELIVER PDU */
	pdsc->firstOct = obuf[idx++];
	if ((pdsc->firstOct & 0x40) == USER_DATA_HEADER_INDICATION)
		pdsc->isHeaderPrsnt = TRUE;

	/* Message Type Indicator */
	switch (pdsc->firstOct & 0x03)
		{
		case MSG_TYPE_SMS_DELIVER:
			pdsc->msgType = MSG_TYPE_SMS_DELIVER;
			break;

		case MSG_TYPE_SMS_STATUS_REPORT:				/* Message Reference Number TP-MR of SMS_STATUS_REPORT PDU */
			pdsc->msgType = MSG_TYPE_SMS_STATUS_REPORT;
			pdsc->msgRefNo = obuf[idx++];
			break;

		default:
			*pError = ERR_MSG_TYPE;
			return (FALSE);
			break;
		}


	pdsc->phoneAddrLen = obuf[idx++];					/* Phone Number Length */

	pdsc->phoneTypeOfAddr = obuf[idx++];					/* Phone Number Type of Address (Eg: 91 , 81) */

	npi = pdsc->phoneTypeOfAddr & 0x0F;					/* Numbering Plan Identification */

	pdsc->phoneTypeOfAddr = (pdsc->phoneTypeOfAddr & 0x70) >> 4;		/* Type of Number */


	switch (pdsc->phoneTypeOfAddr)						/* Check type of number */
		{
		case NUM_TYPE_UNKNOWN:
		case NUM_TYPE_INTERNATIONAL:
		case NUM_TYPE_NATIONAL:
			/** for Alphanumeric type of Address, Numbering plan is fix 0x00 */
			if (npi != NUM_PLAN_ISDN)
				{
				*pError = ERR_PHONE_NUM_PLAN;
				return (FALSE);
				}
			/** Phone Number (Source Address) */
			if (((addrLen = pdsc->phoneAddrLen) % 2) != 0)
				{
				/** Eg: For "46708251358" Number Length will be 11 ("6407281553F8") */
				/** Eg: For "9898907249" Number Length will be 12 ("8989092794") */
				addrLen++;
				}

			addrLen = addrLen >> 1; // addrLen / 2
			__bin2hex(&obuf[idx], addrLen, pdsc->phoneAddr);
			i_DecSemiOctet2Ascii(pdsc->phoneAddr, pdsc->phoneAddr); // Internal Swapping
			idx = idx + addrLen;
			break;

		case NUM_TYPE_ALPHANUMERIC:
			addrLen = pdsc->phoneAddrLen; // length is in terms of Ascii characters
			addrLen = addrLen >> 1; // addrLen / 2
			pdsc->phoneAddrLen = i_Pdu2Text(&obuf[idx], addrLen, pdsc->phoneAddr);
			break;

		default:
			return	*pError = ERR_PHONE_TYPE_OF_ADDR, (FALSE);
			break;
		}

	 if (pdsc->msgType == MSG_TYPE_SMS_DELIVER)
		{
		pdsc->protocolId = obuf[idx++];					/* Protocol Identifier */

		if (pdsc->protocolId != 0x00)
			return	*pError = ERR_PROTOCOL_ID, (FALSE);

		pdsc->dataCodeScheme = obuf[idx++];				/* Data Coding Scheme */
		grpId = pdsc->dataCodeScheme & 0xF0;

		switch (grpId)
			{
			case GROUP1_WITH_MSG_CLASS:
			case GROUP1_WITH_NO_MSG_CLASS:
				/** Check Character Set */
				switch ((pdsc->dataCodeScheme & 0x0C) >> 2)
					{
					case GSM_7BIT:
						pdsc->usrDataFormat = GSM_7BIT;
						break;

					case ANSI_8BIT:
						pdsc->usrDataFormat = ANSI_8BIT;
						break;

					case UCS2_16BIT:
						pdsc->usrDataFormat = UCS2_16BIT;
						break;

					default:
						return	*pError = ERR_CHAR_SET, (FALSE);
						break;
					}

				if (grpId == GROUP1_WITH_MSG_CLASS)
				/** Special case consideration Flash Messsage */
					if ((pdsc->dataCodeScheme & 0x03) == MSG_CLASS0)
						pdsc->isFlashMsg = TRUE;
				break;

			case GROUP2_WITH_MSG_CLASS:
				/** Special case consideration Flash Messsage */
				if ((pdsc->dataCodeScheme & 0x03) == MSG_CLASS0)
					 pdsc->isFlashMsg = TRUE;

				switch ((pdsc->dataCodeScheme & 0x04) >> 2)
					{
					case GSM_7BIT:
						pdsc->usrDataFormat = GSM_7BIT;
						break;

					case ANSI_8BIT:
						pdsc->usrDataFormat = ANSI_8BIT;
						/** Special case consideration WAP_PUSH Messsage */
						if ((pdsc->dataCodeScheme & 0x03) == MSG_CLASS1)
							pdsc->isWapPushMsg = TRUE;
						break;
					 }
				 break;

			default:
				return	*pError = ERR_DATA_CODE_SCHEME, (FALSE);
				break;
			}

		}


	__bin2hex(&obuf[idx], TIME_STAMP_LEN, pdsc->timeStamp);				/* Service Center Time Stamp */
	i_DecSemiOctet2Ascii(pdsc->timeStamp, pdsc->timeStamp);					/* Internal Swapping */

	pdsc->date.year = (pdsc->timeStamp[0] - '0') * 10 + (pdsc->timeStamp[1] - '0');
	pdsc->date.month = (pdsc->timeStamp[2] - '0') * 10 + (pdsc->timeStamp[3] - '0');
	pdsc->date.day = (pdsc->timeStamp[4] - '0') * 10 + (pdsc->timeStamp[5] - '0');

	pdsc->time.hour = (pdsc->timeStamp[6] - '0') * 10 + (pdsc->timeStamp[7] - '0');
	pdsc->time.minute = (pdsc->timeStamp[8] - '0') * 10 + (pdsc->timeStamp[9] - '0');
	pdsc->time.second = (pdsc->timeStamp[10] - '0') * 10 + (pdsc->timeStamp[11] - '0');

	idx += TIME_STAMP_LEN;

	if (pdsc->msgType == MSG_TYPE_SMS_STATUS_REPORT)
		{
		/** Discharge Time Stamp */
		__bin2hex(&obuf[idx], TIME_STAMP_LEN, pdsc->dischrgTimeStamp);
		i_DecSemiOctet2Ascii(pdsc->dischrgTimeStamp, pdsc->dischrgTimeStamp); // Internal Swapping
		idx += TIME_STAMP_LEN;

		/** Status of SMS */
		pdsc->smsSts = obuf[idx++];

		pdsc->smsSts = (pdsc->smsSts == 0x00) ? MSG_DELIVERY_SUCCESS : MSG_DELIVERY_FAIL;

		return (TRUE);
		}

	/* User Data Length */
	pdsc->usrDataLen = obuf[idx++];
	udl = pdsc->usrDataLen;

	/* User Data */

	/*****************************************************************************
	* Below section of code process user data header information
	*****************************************************************************/

	if (pdsc->isHeaderPrsnt) 		// Check whether Header Present
		{
		pdsc->udhLen = obuf[idx++];

		for (length = idx; length < (idx + pdsc->udhLen); length += idx)
			{
			pdsc->udhInfoType = obuf[idx++];
			pdsc->udhInfoLen = obuf[idx++];

			if (pdsc->udhInfoType == IE_CONCATENATED_MSG) // whether Concatenated Message
				{
				pdsc->isConcatenatedMsg = TRUE;
				pdsc->concateMsgRefNo = obuf[idx++];
				pdsc->concateTotalParts = obuf[idx++];
				pdsc->concateCurntPart = obuf[idx++];
				}
			else if (pdsc->udhInfoType == IE_PORT_ADDR_8BIT) // Port Address 8bit
				{
				pdsc->srcPortAddr = obuf[idx++];
				pdsc->destPortAddr = obuf[idx++];
				}
			else if (pdsc->udhInfoType == IE_PORT_ADDR_16BIT) // Port Address 16bit
				{
				pdsc->srcPortAddr = obuf[idx++];
				pdsc->srcPortAddr = pdsc->srcPortAddr << 8;
				pdsc->srcPortAddr |= obuf[idx++];

				pdsc->destPortAddr = obuf[idx++];
				pdsc->destPortAddr = pdsc->destPortAddr << 8;
				pdsc->destPortAddr |= obuf[idx++];
				}
			else	idx = idx + pdsc->udhInfoLen; // Ignoring other Header Information
			}

		if (pdsc->usrDataFormat == GSM_7BIT)
			{
			uint8_t fillBitsNum = 0;
			uint8_t udhSeptet = 0;

			/* Derive number of octet to be filled */
			fillBitsNum = ((1 + pdsc->udhLen) * 8) % 7;
			fillBitsNum = 7 - fillBitsNum;

			udhSeptet = (((1 + pdsc->udhLen) * 8) + fillBitsNum) / 7;
			udl -= udhSeptet;
			}
		}

	 /* Extract user data */
	if (pdsc->usrDataFormat == GSM_7BIT)
		{
		oct = (udl*7) / 8;

		if ( (udl = (udl*7) % 8) )
			oct++;	 // Deriving no. of octes from septets

		udl = oct;
		asciiLen = i_Pdu2Text(&obuf[idx], udl, pdsc->usrData);
		pdsc->usrDataLen = i_GsmStrToUtf8Str(pdsc->usrData, asciiLen, pdsc->usrData);
		}
	else 	{ // for 8/16bit data
		memcpy(pdsc->usrData, &obuf[idx], pdsc->usrDataLen);
		pdsc->usrData[pdsc->usrDataLen] = '\0';
		}

	return (TRUE);
}

//***********************************************************************************************
// @NAME        : EncodePduData
// @PARAM       : pGsmPduStr-Reference To PDU String
//				  gsmPduStrLen-Length of PDU String,
//				  PDU_ENCODE_DESC-Object Pointer
// @RETURNS     : TRUE/FALSE
// @DESCRIPTION : This function extracts PDU data from Descriptor & Prepares PDU String
//				  if fails then return FALSE.
//***********************************udl= udl - udhSeptet;****************************************
int	EncodePduData(PDU_DESC *pdsc, unsigned char *pdu, int pdusz, int *tpdulen)
{
int	idx, tidx, addrLen, gsmLen;
unsigned char  obuf[SMS_PDU_MAX_LEN + 1], *tpdu;

	*tpdulen = idx = tidx = addrLen = gsmLen = 0;

	if (pdsc->smscAddrLen != 0)					/* Check whether Service Centre Present */
		{
		if ((pdsc->smscAddrLen % 2) != 0)			/* Service center Number Length */
			pdsc->smscAddrLen++;

		pdsc->smscAddrLen = 1 + (pdsc->smscAddrLen / 2);	/* Adding length of Type of Addr */
		obuf[idx++] = pdsc->smscAddrLen;

		if (pdsc->smscTypeOfAddr == NUM_TYPE_INTERNATIONAL)	/* Service Center Type of Address (Eg: 91 , 81) */
			obuf[idx++] = 0x91;
		else if (pdsc->smscTypeOfAddr == NUM_TYPE_NATIONAL)
			obuf[idx++] = 0xA1;
		else	obuf[idx++] = 0x81;				/* Unknown */


		i_Ascii2DecSemiOctet(pdsc->smscAddr, pdsc->smscAddr);	/* Service Center Number */
		addrLen = __hex2bin(pdsc->smscAddr, &obuf[idx]);
		idx = idx + addrLen;
		}
	else	obuf[idx++] = 0x00;					/* SMSC stored on phone is used */

	/* So at this point TP-SCA field has been formed , we can fix TPDU area for the future use*/
	tpdu = &obuf[idx];

	pdsc->firstOct |= MSG_TYPE_SMS_SUBMIT;				/* First Octet of SMS_SUBMIT PDU */
	pdsc->firstOct |= pdsc->vldtPrdFrmt;

	if (pdsc->isConcatenatedMsg)
		{
		pdsc->firstOct |= USER_DATA_HEADER_INDICATION;		/* Indicate that UDH is present */

		if (pdsc->concateTotalParts == pdsc->concateCurntPart)	/* Is last part to send? */
			if(pdsc->isDeliveryReq)				/* Indicate that delivery report is require */
				pdsc->firstOct |= STATUS_REPORT_INDICATOR;
		}
	 else if(pdsc->isDeliveryReq)					/* Set status report indication bit */
		pdsc->firstOct |= STATUS_REPORT_INDICATOR;		/* Indicate that delivery report is require */

	 obuf[idx++] = pdsc->firstOct;

	 obuf[idx++] = MSG_REF_NO_DEFAULT;				/* Allow Mobile to set Message Reference No. */


	 obuf[idx++] = pdsc->phoneAddrLen;				/* Phone Number Length */

	 if (pdsc->phoneTypeOfAddr == NUM_TYPE_INTERNATIONAL)		/* Phone Number Type of Address (Eg: 91 , 81) */
		obuf[idx++] = 0x91;
	 else if (pdsc->phoneTypeOfAddr == NUM_TYPE_NATIONAL)
		obuf[idx++] = 0xA1;
	 else	obuf[idx++] = 0x81;


	 i_Ascii2DecSemiOctet(pdsc->phoneAddr, pdsc->phoneAddr);	/* Phone Number (Source Address) */
	 addrLen = __hex2bin(pdsc->phoneAddr, &obuf[idx]);
	 idx += addrLen;

	 obuf[idx++] = 0x00;						/* Protocol Identifier */

	 pdsc->dataCodeScheme |= (pdsc->usrDataFormat << 2);		/* Data Coding Scheme */


	 if (pdsc->isFlashMsg)						/* Special case considerations WAP-PUSH & Flash Messsage */
		pdsc->dataCodeScheme |= 0x10;
	 else if (pdsc->isWapPushMsg)
		pdsc->dataCodeScheme = 0xF5;

	 obuf[idx++] = pdsc->dataCodeScheme;

	 switch (pdsc->vldtPrdFrmt)
		{
		case VLDTY_PERIOD_RELATIVE:				/* One octet */
			obuf[idx++] = pdsc->vldtPrd;
			break;

		case VLDTY_PERIOD_DEFAULT:				/* Validity Period not preset */
		default:
			break;
		}

	 pdsc->usrDataLen = pdsc->usrDataLen				/* User Data Length */
				? pdsc->usrDataLen			/* Has been defined before calling */
				: __strnlen( (char *) pdsc->usrData, sizeof(pdsc->usrData));

	 /* User Data */

	 /* Check whether length is sufficient */
	 if (pdsc->usrDataFormat == GSM_7BIT)
		{
		gsmLen = strlen( (char *) pdsc->usrData);		/* for GSM_7bit data */
		tidx = idx;
		obuf[idx++] = gsmLen;					/* TP-UDL */

		if (gsmLen > SMS_GSM7BIT_MAX_LEN)
			gsmLen = TRUNCATED_GSM_DATA_LEN;
		}
	 else // for 8bit & 16bit Data
		{
		obuf[idx++] = pdsc->usrDataLen;				/* TP-UDL */

		if (pdsc->usrDataLen > SMS_PDU_USER_DATA_MAX_LEN)
				pdsc->usrDataLen = TRUNCATED_PDU_DATA_LEN;
		}

	 /*****************************************************************************
	 * Below section of code process user data header information
	 *****************************************************************************/

	 if (pdsc->isConcatenatedMsg) // Check for Concatenated Message
		{
		 if (pdsc->usrDataFormat == GSM_7BIT)
			{
			uint8_t fillBitsNum = 0;
			uint8_t udhSeptet = 0;

			/* Derive number of octet to be filled */
			fillBitsNum = ((1 + UDH_CONCATENATED_MSG_LEN) * 8) % 7;
			fillBitsNum = 7 - fillBitsNum;

			/* Derive User Data Length in septets */
			udhSeptet = (((1 + UDH_CONCATENATED_MSG_LEN) * 8) + fillBitsNum) / 7;
			obuf[tidx] = gsmLen + udhSeptet; // Updating TP-UDL
			tidx = idx;

			obuf[idx++] = UDH_CONCATENATED_MSG_LEN;
			obuf[idx++] = IE_CONCATENATED_MSG;
			obuf[idx++] = IE_CONCATENATED_MSG_LEN;
			obuf[idx++] = pdsc->concateMsgRefNo;
			obuf[idx++] = pdsc->concateTotalParts;
			obuf[idx++] = pdsc->concateCurntPart;

			/* Copy 7bit text to buffer */
			i_Utf8StrToGsmStr(pdsc->usrData, gsmLen, pdsc->usrData, &gsmLen);
			tidx = tidx + udhSeptet;  // for septet boundary of user data header
			i_Text2Pdu(pdsc->usrData, gsmLen + udhSeptet, &obuf[tidx]);
			}
		else	{
			obuf[idx++] = UDH_CONCATENATED_MSG_LEN;
			obuf[idx++] = IE_CONCATENATED_MSG;
			obuf[idx++] = IE_CONCATENATED_MSG_LEN;
			obuf[idx++] = pdsc->concateMsgRefNo;
			obuf[idx++] = pdsc->concateTotalParts;
			obuf[idx++] = pdsc->concateCurntPart;

			/* Copy 8/16bit text to buffer */
			memcpy(&obuf[idx], pdsc->usrData, pdsc->usrDataLen);
			obuf[idx += pdsc->usrDataLen] = '\0';
			}
		}
	else	{
		if (pdsc->usrDataFormat == GSM_7BIT)
			{
			/* Copy 7bit text to buffer */
			i_Utf8StrToGsmStr(pdsc->usrData, gsmLen, pdsc->usrData, &gsmLen);
			idx += i_Text2Pdu(pdsc->usrData, gsmLen, &obuf[idx]);
			}
		else	{
			/* Copy 8/16bit text to buffer */
			memcpy(&obuf[idx], pdsc->usrData, pdsc->usrDataLen);
			obuf[idx += pdsc->usrDataLen] = '\0';
			}
		}


	*tpdulen = &obuf[idx] - tpdu;					/* Calculate TDPU length */

	return	__bin2hex(obuf, idx, pdu);			/* Convert PDU buffer into the text HEX string,
									** return a result length */
}

//***************************************************************************
// @NAME        : print_decoded_pdu
// @PARAM       : pPduDecodeDesc- Pointer to pdu desc
// @RETURNS     : void
// @DESCRIPTION : This prints pdu descriptor.
//***************************************************************************
void print_decoded_pdu(PDU_DESC *pPduDecodeDesc)
{
	fprintf(stdout, "smscAddrLen      : %d\n", pPduDecodeDesc->smscAddrLen);
	fprintf(stdout, "smscNpi          : %d\n", pPduDecodeDesc->smscNpi);
	fprintf(stdout, "smscTypeOfAddr   : %d\n", pPduDecodeDesc->smscTypeOfAddr);
	fprintf(stdout, "smscAddr         : %s\n", pPduDecodeDesc->smscAddr);
	fprintf(stdout, "firstOct         : %d\n", pPduDecodeDesc->firstOct);
	fprintf(stdout, "isHeaderPrsnt    : %d\n", pPduDecodeDesc->isHeaderPrsnt);
	fprintf(stdout, "msgRefNo         : %d\n", pPduDecodeDesc->msgRefNo);
	fprintf(stdout, "phoneAddrLen     : %d\n", pPduDecodeDesc->phoneAddrLen);
	fprintf(stdout, "phoneAddr        : %s\n", pPduDecodeDesc->phoneAddr);
	fprintf(stdout, "protocolId       : %d\n", pPduDecodeDesc->protocolId);
	fprintf(stdout, "dataCodeScheme   : %d\n", pPduDecodeDesc->dataCodeScheme);
	fprintf(stdout, "msgType          : %d\n", pPduDecodeDesc->msgType);
	fprintf(stdout, "isWapPushMsg     : %d\n", pPduDecodeDesc->isWapPushMsg);
	fprintf(stdout, "isFlashMsg       : %d\n", pPduDecodeDesc->isFlashMsg);
	fprintf(stdout, "isStsReportReq   : %d\n", pPduDecodeDesc->isStsReportReq);
	fprintf(stdout, "isMsgWait        : %d\n", pPduDecodeDesc->isMsgWait);

	fprintf(stdout, "usrDataFormat    : %d\n", pPduDecodeDesc->usrDataFormat);
	fprintf(stdout, "timeStamp        : %s\n", pPduDecodeDesc->timeStamp);
	fprintf(stdout, "dischrgTimeStamp : %s\n", pPduDecodeDesc->dischrgTimeStamp);
	fprintf(stdout, "vldtPrd          : %d\n", pPduDecodeDesc->vldtPrd);
	fprintf(stdout, "vldtPrdFrmt      : %d\n", pPduDecodeDesc->vldtPrdFrmt);

	fprintf(stdout, "usrDataLen       : %d\n", pPduDecodeDesc->usrDataLen);
	fprintf(stdout, "usrData          : %s\n", pPduDecodeDesc->usrData);
	fprintf(stdout, "udhLen           : %d\n", pPduDecodeDesc->udhLen);
	fprintf(stdout, "udhInfoType      : %d\n", pPduDecodeDesc->udhInfoType);

	fprintf(stdout, "udhInfoLen       : %d\n", pPduDecodeDesc->udhInfoLen);
	fprintf(stdout, "concateMsgRefNo  : %d\n", pPduDecodeDesc->concateMsgRefNo);
	fprintf(stdout, "concateTotalParts: %d\n", pPduDecodeDesc->concateTotalParts);
	fprintf(stdout, "concateCurntPart : %d\n", pPduDecodeDesc->concateCurntPart);
	fprintf(stdout, "isConcatenatedMsg: %d\n", pPduDecodeDesc->isConcatenatedMsg);

	fprintf(stdout, "smsSts           : %d\n", pPduDecodeDesc->smsSts);
	fprintf(stdout, "srcPortAddr      : %d\n", pPduDecodeDesc->srcPortAddr);
	fprintf(stdout, "destPortAddr     : %d\n", pPduDecodeDesc->destPortAddr);

	fprintf(stdout, "isDeliveryReq    : %d\n", pPduDecodeDesc->isDeliveryReq);

	fprintf(stdout, "Date             : %02d-%02d-%04d\n", pPduDecodeDesc->date.day, pPduDecodeDesc->date.month, pPduDecodeDesc->date.year);
	fprintf(stdout, "Time             : %02d:%02d:%02d\n", pPduDecodeDesc->time.hour, pPduDecodeDesc->time.minute, pPduDecodeDesc->time.second);

	fflush(stdout);
}
