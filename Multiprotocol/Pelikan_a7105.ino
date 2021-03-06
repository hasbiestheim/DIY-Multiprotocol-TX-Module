/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */
// Compatible with CADET PRO V4 TX

#if defined(PELIKAN_A7105_INO)

#include "iface_a7105.h"

#define PELIKAN_FORCE_ID

#define PELIKAN_BIND_COUNT		400
#define PELIKAN_BIND_RF			0x3C
#define PELIKAN_NUM_RF_CHAN 	0x1D
#define PELIKAN_PAQUET_PERIOD	7980

static void __attribute__((unused)) pelikan_build_packet()
{
	static boolean upper=false;
	packet[0] = 0x15;
    if(IS_BIND_IN_PROGRESS)
	{
		packet[1] = 0x04;			//version??
		packet[2] = rx_tx_addr[0];
		packet[3] = rx_tx_addr[1];
		packet[4] = rx_tx_addr[2];
		packet[5] = rx_tx_addr[3];
		packet[6] = 0x05;			//??
		packet[7] = 0x00;			//??
		packet[8] = 0x55;			//??
		packet_length = 10;
	}
	else
	{
		//ID
		packet[1]  = rx_tx_addr[0];
		packet[7]  = rx_tx_addr[1];
		packet[12] = rx_tx_addr[2];
		packet[13] = rx_tx_addr[3];
		//Channels
		uint8_t offset=upper?4:0;
		uint16_t channel=convert_channel_16b_nolimit(CH_AETR[offset++], 153, 871);
		uint8_t top=(channel>>2) & 0xC0;
		packet[2]  = channel;
		channel=convert_channel_16b_nolimit(CH_AETR[offset++], 153, 871);
		top|=(channel>>4) & 0x30;
		packet[3]  = channel;
		channel=convert_channel_16b_nolimit(CH_AETR[offset++], 153, 871);
		top|=(channel>>6) & 0x0C;
		packet[4]  = channel;
		channel=convert_channel_16b_nolimit(CH_AETR[offset], 153, 871);
		top|=(channel>>8) & 0x03;
		packet[5]  = channel;
		packet[6]  = top;
		//Check
		crc8=0x15;
		for(uint8_t i=1;i<8;i++)
			crc8+=packet[i];
		packet[8]=crc8;
		//Low/Up channel flag
		packet[9]=upper?0xAA:0x00;
		upper=!upper;
		//Hopping counters
		if(++packet_count>4)
		{
			packet_count=0;
			if(++hopping_frequency_no>=PELIKAN_NUM_RF_CHAN)
				hopping_frequency_no=0;
		}
		packet[10]=hopping_frequency_no;
		packet[11]=packet_count;

		packet_length = 15;
	}

	//Check
	crc8=0x15;
	for(uint8_t i=1; i<packet_length-1 ;i++)
		crc8+=packet[i];
	packet[packet_length-1]=crc8;

	//Send
	#ifdef DEBUG_SERIAL
		if(packet[9]==0x00)
		{
			debug("C: %02X P(%d):",IS_BIND_IN_PROGRESS?PELIKAN_BIND_RF:hopping_frequency[hopping_frequency_no],packet_length);
			for(uint8_t i=0;i<packet_length;i++)
				debug(" %02X",packet[i]);
			debugln("");
		}
	#endif
	A7105_WriteData(packet_length, IS_BIND_IN_PROGRESS?PELIKAN_BIND_RF:hopping_frequency[hopping_frequency_no]);
	A7105_SetPower();
}

uint16_t ReadPelikan()
{
	#ifndef FORCE_PELIKAN_TUNING
		A7105_AdjustLOBaseFreq(1);
	#endif
	if(IS_BIND_IN_PROGRESS)
	{
		bind_counter--;
		if (bind_counter==0)
		{
			BIND_DONE;
			A7105_Strobe(A7105_STANDBY);
			A7105_WriteReg(A7105_03_FIFOI,0x28);
		}
	}
	#ifdef MULTI_SYNC
		telemetry_set_input_sync(PELIKAN_PAQUET_PERIOD);
	#endif
	pelikan_build_packet();
	return PELIKAN_PAQUET_PERIOD;
}

const uint8_t PROGMEM pelikan_hopp[][PELIKAN_NUM_RF_CHAN] = {
	{ 0x5A,0x46,0x32,0x6E,0x6C,0x58,0x44,0x42,0x40,0x6A,0x56,0x54,0x52,0x3E,0x68,0x66,0x64,0x50,0x3C,0x3A,0x38,0x62,0x4E,0x4C,0x5E,0x4A,0x36,0x5C,0x34 }
};

uint16_t initPelikan()
{
	A7105_Init();
	if(IS_BIND_IN_PROGRESS)
		A7105_WriteReg(A7105_03_FIFOI,0x10);
	
	//ID
	#ifdef PELIKAN_FORCE_ID
		rx_tx_addr[0]=0x0D;
        rx_tx_addr[1]=0xF4;
        rx_tx_addr[2]=0x50;
        rx_tx_addr[3]=0x18;
	#endif

	// Fill frequency table
	for(uint8_t i=0;i<PELIKAN_NUM_RF_CHAN;i++)
		hopping_frequency[i]=pgm_read_byte_near(&pelikan_hopp[0][i]);

	hopping_frequency_no=PELIKAN_NUM_RF_CHAN;
	packet_count=5;
	return 2400;
}
#endif
