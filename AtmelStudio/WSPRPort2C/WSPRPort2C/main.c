/*
3 Marzo 2018 - Erick Vazquez Gonzalez erick@ahorratec.org
Este algoritmo es un port de C++ a C de la codification WSPR
*/
#include <atmel_start.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

//Extracto de JTEncode.h (Omitiendo todos los protocolos que no son WSPR)

#define WSPR_SYMBOL_COUNT                   162
#define WSPR_BIT_COUNT                      162
#define NGLYPHS         (sizeof(fsq_code_table)/sizeof(fsq_code_table[0]))
 
typedef struct fsq_varicode		//Define la estructura de la tabla de caracteres (2 elementos)
{	uint8_t ch;
	uint8_t var[2];				//2 elementos, podemos ajustar el arreglo y amplificar el numero de bytes en el CRC 
} Varicode;

// The FSQ varicode table, based on the FSQ Varicode V3.0--- Revisar el documento
// document provided by Murray Greenman, ZL1BPU -- Buscar a Murray GreenMan

const Varicode fsq_code_table[] PROGMEM =	//Por alguna razon no se utilizado 2^8 caracteres; porque? 
{
	{' ', {00, 00}}, // space
	{'!', {11, 30}},
	{'"', {12, 30}},
	{'#', {13, 30}},
	{'$', {14, 30}},
	{'%', {15, 30}},
	{'&', {16, 30}},
	{'\'', {17, 30}},
	{'(', {18, 30}},
	{')', {19, 30}},
	{'*', {20, 30}},
	{'+', {21, 30}},
	{',', {27, 29}},
	{'-', {22, 30}},
	{'.', {27, 00}},
	{'/', {23, 30}},
	{'0', {10, 30}},
	{'1', {01, 30}},
	{'2', {02, 30}},
	{'3', {03, 30}},
	{'4', {04, 30}},
	{'5', {05, 30}},
	{'6', {06, 30}},
	{'7', {07, 30}},
	{'8', {8, 30}},
	{'9', {9, 30}},
	{':', {24, 30}},
	{';', {25, 30}},
	{'<', {26, 30}},
	{'=', {00, 31}},
	{'>', {27, 30}},
	{'?', {28, 29}},
	{'@', {00, 29}},
	{'A', {01, 29}},
	{'B', {02, 29}},
	{'C', {03, 29}},
	{'D', {04, 29}},
	{'E', {05, 29}},
	{'F', {06, 29}},
	{'G', {07, 29}},
	{'H', {8, 29}},
	{'I', {9, 29}},
	{'J', {10, 29}},
	{'K', {11, 29}},
	{'L', {12, 29}},
	{'M', {13, 29}},
	{'N', {14, 29}},
	{'O', {15, 29}},
	{'P', {16, 29}},
	{'Q', {17, 29}},
	{'R', {18, 29}},
	{'S', {19, 29}},
	{'T', {20, 29}},
	{'U', {21, 29}},
	{'V', {22, 29}},
	{'W', {23, 29}},
	{'X', {24, 29}},
	{'Y', {25, 29}},
	{'Z', {26, 29}},
	{'[', {01, 31}},
	{'\\', {02, 31}},
	{']', {03, 31}},
	{'^', {04, 31}},
	{'_', {05, 31}},
	{'`', {9, 31}},
	{'a', {01, 00}},
	{'b', {02, 00}},
	{'c', {03, 00}},
	{'d', {04, 00}},
	{'e', {05, 00}},
	{'f', {06, 00}},
	{'g', {07, 00}},
	{'h', {8, 00}},
	{'i', {9, 00}},
	{'j', {10, 00}},
	{'k', {11, 00}},
	{'l', {12, 00}},
	{'m', {13, 00}},
	{'n', {14, 00}},
	{'o', {15, 00}},
	{'p', {16, 00}},
	{'q', {17, 00}},
	{'r', {18, 00}},
	{'s', {19, 00}},
	{'t', {20, 00}},
	{'u', {21, 00}},
	{'v', {22, 00}},
	{'w', {23, 00}},
	{'x', {24, 00}},
	{'y', {25, 00}},
	{'z', {26, 00}},
	{'{', {06, 31}},
	{'|', {07, 31}},
	{'}', {8, 31}},
	{'~', {00, 30}},
	{127, {28, 31}}, // DEL
	{13,  {28, 00}}, // CR
	{10,  {28, 00}}, // LF
	{0,   {28, 30}}, // IDLE
	{241, {10, 31}}, // plus/minus
	{246, {11, 31}}, // division sign
	{248, {12, 31}}, // degrees sign
	{158, {13, 31}}, // multiply sign
	{156, {14, 31}}, // pound sterling sign
	{8,   {27, 31}}  // BS
};

const uint8_t crc8_table[] PROGMEM = {				//Redudancia ciclica, de un solo Byte (8 bits)
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31,
	0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
	0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9,
	0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
	0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1,
	0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
	0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe,
	0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
	0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16,
	0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
	0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
	0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
	0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8,
	0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
	0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10,
	0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
	0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f,
	0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
	0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7,
	0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
	0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
	0xfa, 0xfd, 0xf4, 0xf3
};


int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	/* Replace with your application code */
	while (1) {
		
		void wspr_encode(const char * call, const char * loc, const uint8_t dbm, uint8_t * symbols)
		{
			char call_[7];
			char loc_[5];
			uint8_t dbm_ = dbm;
			strcpy(call_, call);
			strcpy(loc_, loc);

			// Ensure that the message text conforms to standards
			// --------------------------------------------------
			wspr_message_prep(call_, loc_, dbm_);

			// Bit packing
			// -----------
			uint8_t c[11];
			wspr_bit_packing(c);

			// Convolutional Encoding
			// ---------------------
			uint8_t s[WSPR_SYMBOL_COUNT];
			convolve(c, s, 11, WSPR_BIT_COUNT);

			// Interleaving
			// ------------
			wspr_interleave(s);

			// Merge with sync vector
			// ----------------------
			wspr_merge_sync_vector(s, symbols);
		}
		
		int wspr_code(char c)
		{
			// Validate the input then return the proper integer code.
			// Return 255 as an error code if the char is not allowed.

			if(isdigit(c))
			{
				return (uint8_t)(c - 48);
			}
			else if(c == ' ')
			{
				return 36;
			}
			else if(c >= 'A' && c <= 'Z')
			{
				return (uint8_t)(c - 55);
			}
			else
			{
				return 255;
			}
		}
		
		int wspr_message_prep(char * call, char * loc, uint8_t dbm)
		{
			// Callsign validation and padding
			// -------------------------------

			// If only the 2nd character is a digit, then pad with a space.
			// If this happens, then the callsign will be truncated if it is
			// longer than 5 characters.
			if((call[1] >= '0' && call[1] <= '9') && (call[2] < '0' || call[2] > '9'))
			{
				memmove(call + 1, call, 5);
				call[0] = ' ';
			}

			// Now the 3rd charcter in the callsign must be a digit
			if(call[2] < '0' || call[2] > '9')
			{
				// TODO: need a better way to handle this
				call[2] = '0';
			}

			// Ensure that the only allowed characters are digits and
			// uppercase letters
			uint8_t i;
			for(i = 0; i < 6; i++)
			{
				call[i] = toupper(call[i]);
				if(!(isdigit(call[i]) || isupper(call[i])))
				{
					call[i] = ' ';
				}
			}

			memcpy(callsign, call, 6);

			// Grid locator validation
			for(i = 0; i < 4; i++)
			{
				loc[i] = toupper(loc[i]);
				if(!(isdigit(loc[i]) || (loc[i] >= 'A' && loc[i] <= 'R')))
				{
					memcpy(loc, "AA00", 5);
					//loc = "AA00";
				}
			}

			memcpy(locator, loc, 4);

			// Power level validation
			// Only certain increments are allowed
			if(dbm > 60)
			{
				dbm = 60;
			}
			const uint8_t valid_dbm[19] =
			{0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40,
			43, 47, 50, 53, 57, 60};
			for(i = 0; i < 19; i++)
			{
				if(dbm == valid_dbm[i])
				{
					power = dbm;
				}
			}
			// If we got this far, we have an invalid power level, so we'll round down
			for(i = 1; i < 19; i++)
			{
				if(dbm < valid_dbm[i] && dbm >= valid_dbm[i - 1])
				{
					power = valid_dbm[i - 1];
				}
			}
		}
		
		int wspr_bit_packing(uint8_t * c)
		{
			uint32_t n, m;

			n = wspr_code(callsign[0]);
			n = n * 36 + wspr_code(callsign[1]);
			n = n * 10 + wspr_code(callsign[2]);
			n = n * 27 + (wspr_code(callsign[3]) - 10);
			n = n * 27 + (wspr_code(callsign[4]) - 10);
			n = n * 27 + (wspr_code(callsign[5]) - 10);

			m = ((179 - 10 * (locator[0] - 'A') - (locator[2] - '0')) * 180) +
			(10 * (locator[1] - 'A')) + (locator[3] - '0');
			m = (m * 128) + power + 64;

			// Callsign is 28 bits, locator/power is 22 bits.
			// A little less work to start with the least-significant bits
			c[3] = (uint8_t)((n & 0x0f) << 4);
			n = n >> 4;
			c[2] = (uint8_t)(n & 0xff);
			n = n >> 8;
			c[1] = (uint8_t)(n & 0xff);
			n = n >> 8;
			c[0] = (uint8_t)(n & 0xff);

			c[6] = (uint8_t)((m & 0x03) << 6);
			m = m >> 2;
			c[5] = (uint8_t)(m & 0xff);
			m = m >> 8;
			c[4] = (uint8_t)(m & 0xff);
			m = m >> 8;
			c[3] |= (uint8_t)(m & 0x0f);
			c[7] = 0;
			c[8] = 0;
			c[9] = 0;
			c[10] = 0;
		}
		
		
		
		void wspr_merge_sync_vector(uint8_t * g, uint8_t * symbols)
		{
			uint8_t i;
			const uint8_t sync_vector[WSPR_SYMBOL_COUNT] =
			{1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0,
				1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
				0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
				0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0,
				1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1,
				0, 0, 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
				1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
			1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0};

			for(i = 0; i < WSPR_SYMBOL_COUNT; i++)
			{
				symbols[i] = sync_vector[i] + (2 * g[i]);
			}
		}
		
		
		
		void crc8(const char * text)
		{
			uint8_t crc = '\0';
			uint8_t ch;

			int i;
			for(i = 0; i < strlen(text); i++)
			{
				ch = text[i];
				//#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
				#if defined(__arm__)
				crc = crc8_table[(crc) ^ ch];
				#else
				crc = pgm_read_byte(&(crc8_table[(crc) ^ ch]));
				#endif
				crc &= 0xFF;
			}

			return crc;
		}


	}
}
