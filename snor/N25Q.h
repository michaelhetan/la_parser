#ifndef __N25Q_H__
#define __N25Q_H__

#define OPCODE_RDID1		0x9F
#define	OPCODE_RDID2		0x9E
#define OPCODE_MULTIIORDID	0xAF
#define OPCODE_RDPARAM		0x5A

#define OPCODE_READ			0x03
#define OPCODE_FASTRD		0x0B
#define OPCODE_DORD			0x3B
#define OPCODE_DIORD		0xBB
#define OPCODE_QORD			0x6B
#define OPCODE_QIORD		0xEB
#define OPCODE_DTRRD		0x0D
#define OPCODE_DTRDORD		0x3D
#define OPCODE_DTRDIORD		0xBD
#define OPCODE_DTRQORD		0x6D
#define OPCODE_DTRQIORD		0xED
#define OPCODE_READ_4B		0x13
#define OPCODE_FASTRD_4B	0x0C
#define OPCODE_DORD_4B		0x3C
#define OPCODE_DIORD_4B		0xBC
#define OPCODE_QORD_4B		0x6C
#define OPCODE_QIORD_4B		0xEC

#define OPCODE_WREN			0x06
#define OPCODE_WRDIS		0x04

#define OPCODE_RDSR			0x05
#define OPCODE_WRSR			0x01
#define OPCODE_RDLR			0xE8
#define OPCODE_WRLR			0xE5
#define OPCODE_RDFSR		0x70
#define OPCODE_CLRFSR		0x50
#define OPCODE_RDNVCR		0xB5
#define OPCODE_WRNVCR		0xB1
#define OPCODE_RDVCR		0X85
#define OPCODE_WRVCR		0x81
#define OPCODE_RDVECR		0x65
#define OPCODE_WRVECR		0x61
#define OPCODE_RDEAR		0xC8
#define OPCODE_WREAR		0xC5

#define OPCODE_PP			0x02
#define OPCODE_PP_4B		0x12
#define OPCODE_DIP			0xA2
#define OPCODE_EDIP			0xD2
#define OPCODE_QIP			0x32
#define OPCODE_QIP_4B		0x34
//#define OPCODE_EQIP1		0x12
#define OPCODE_EQIP		0x38

#define OPCODE_SSE			0x20
#define OPCODE_SSE_4B		0x21
#define OPCODE_SE			0xD8
#define OPCODE_SE_4B		0xDC
#define OPCODE_BE			0xC7
#define OPCODE_RESUME		0x7A
#define OPCODE_SUSPEND		0x75

#define OPCODE_RDOTP		0x4B
#define OPCODE_WROTP		0x42

#define OPCODE_EN4B			0xB7
#define OPCODE_EX4B			0xE9

#define OPCODE_ENQUAD		0x35
#define OPCODE_EXQUAD		0xF5



#endif