/*
 * JTEncode.cpp - JT65/JT9/WSPR/FSQ encoder library for Arduino
 *
 * Copyright (C) 2015-2016 Jason Milldrum <milldrum@gmail.com>
 *
 * Based on the algorithms presented in the WSJT software suite.
 * Thanks to Andy Talbot G4JNT for the white paper on the WSPR encoding
 * process that helped me to understand all of this.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <JTEncode.h> 

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) || defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)
#include <avr/pgmspace.h>
#endif

#include "Arduino.h"

// Define an upper bound on the number of glyphs.  Defining it this ***Que son los glyphs ??***
// way allows adding characters without having to update a hard-coded
// upper bound.
#define NGLYPHS         (sizeof(fsq_code_table)/sizeof(fsq_code_table[0]))

/* Public Class Members */

JTEncode::JTEncode(void)
{
  // Initialize the Reed-Solomon encoder
  rs_inst = (struct rs *)(intptr_t)init_rs_int(6, 0x43, 3, 1, 51, 0); 
}


/*
 * wspr_encode(const char * call, const char * loc, const uint8_t dbm, uint8_t * symbols)
 *
 * Takes an arbitrary message of up to 13 allowable characters and returns
 *
 * call - Callsign (6 characters maximum).
 * loc - Maidenhead grid locator (4 charcters maximum).
 * dbm - Output power in dBm.
 * symbols - Array of channel symbols to transmit retunred by the method.
 *  Ensure that you pass a uint8_t array of size WSPR_SYMBOL_COUNT to the method.
 *
 */
void JTEncode::wspr_encode(const char * call, const char * loc, const uint8_t dbm, uint8_t * symbols)
{
  char call_[7]; // 8 posiciones para el CallSign. 8 positions for CallSign
  char loc_[5]; // 6 posiciones para el locator. 6 positions for Locator
  uint8_t dbm_ = dbm; // del input de la funcion, dbm. Aware that this variable is an unsigned int (8bits)
  int ADC_Value;// escrito por JMB. I wrote this.
  ADC_Value=call_[0,1];// I wrote this. Escrito por JMB. This sentence is wrong, compiler will send syntax error, we have to fit an Int Variable into a Char Array. IOTA FUNTION MIGHT WORK
  strcpy(call_, call); // inicializacion con lo que llega de input. Inicialization of string with string copy function.
  strcpy(loc_, loc);  // inicializacion con lo que llega de input. Same here.
	
  
  // Ensure that the message text conforms to standards. Whats the Dictionary!? ASCII Table because is a char variable or is it something else?
  // --------------------------------------------------
  wspr_message_prep(call_, loc_, dbm_); //se prepara el mensaje

  // Bit packing
  // -----------
  uint8_t c[11];
  wspr_bit_packing(c);

  // Convolutional Encoding
  // ---------------------
  uint8_t s[WSPR_SYMBOL_COUNT]; // vale 162 del #define. It is defined 162 at JTEncode.h
  convolve(c, s, 11, WSPR_BIT_COUNT);

  // Interleaving
  // ------------
  wspr_interleave(s);

  // Merge with sync vector
  // ----------------------
  wspr_merge_sync_vector(s, symbols);
}


uint8_t JTEncode::wspr_code(char c)
{
  // Validate the input then return the proper integer code.
  // Return 255 as an error code if the char is not allowed.
  

  if(isdigit(c))
	{
		return (uint8_t)(c - 48); // That is, subtracting by 48 translates the char values '0'..'9' to the int values 0..9. Unsigned Int is 2bytes (16 bits) long
	}
	else if(c == ' ')
	{
		return 36; // clava un espacio. ASCII Table Space is 32 not 36!!
	}
	else if(c >= 'A' && c <= 'Z')
	{
		return (uint8_t)(c - 55); // un valor entre la "A" y la "Z" Mayusculas. Traduce el valor char en int y lo castea.
	}
	else
	{
		return 255; // mensaje de error.
	}
}

uint8_t JTEncode::gray_code(uint8_t c)
{
  return (c >> 1) ^ c;
}



void JTEncode::wspr_message_prep(char * call, char * loc, uint8_t dbm)
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
    // TO DO: need a better way to handle this
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



void JTEncode::wspr_bit_packing(uint8_t * c)
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





void JTEncode::wspr_interleave(uint8_t * s)
{
  uint8_t d[WSPR_BIT_COUNT];
	uint8_t rev, index_temp, i, j, k;

	i = 0;

	for(j = 0; j < 255; j++)
	{
		// Bit reverse the index
		index_temp = j;
		rev = 0;

		for(k = 0; k < 8; k++)
		{
			if(index_temp & 0x01)
			{
				rev = rev | (1 << (7 - k));
			}
			index_temp = index_temp >> 1;
		}

		if(rev < WSPR_BIT_COUNT)
		{
			d[rev] = s[i];
			i++;
		}

		if(i >= WSPR_BIT_COUNT)
		{
			break;
		}
	}

  memcpy(s, d, WSPR_BIT_COUNT);
}





void JTEncode::wspr_merge_sync_vector(uint8_t * g, uint8_t * symbols)
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




uint8_t JTEncode::crc8(const char * text)
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
