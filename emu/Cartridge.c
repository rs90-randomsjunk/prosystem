// ----------------------------------------------------------------------------
//   ___  ___  ___  ___       ___  ____  ___  _  _
//  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /
// /    / \  /__/ ___/ ___/ ___/   /   /__  /    /  emulator
//
// ----------------------------------------------------------------------------
// Copyright 2005 Greg Stanton
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// ----------------------------------------------------------------------------
// Cartridge.cpp
// ----------------------------------------------------------------------------
#include "Cartridge.h"
#define CARTRIDGE_SOURCE "Cartridge.cpp"

char cartridge_title[128];
char cartridge_description[128];
char cartridge_year[128];
char cartridge_maker[128];
char cartridge_digest[128];
char cartridge_filename[512];
byte cartridge_type;
byte cartridge_region;
bool cartridge_pokey;
byte cartridge_controller[2];
byte cartridge_bank;
uint cartridge_flags;

static byte* cartridge_buffer = NULL;
static uint cartridge_size = 0;

extern unsigned long crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

// ----------------------------------------------------------------------------
// HasHeader
// ----------------------------------------------------------------------------
static bool cartridge_HasHeader(const byte* header) {
  uint index;
  
  const char HEADER_ID[ ] = {"ATARI7800"};
  for(index = 0; index < 9; index++) {
    if(HEADER_ID[index] != header[index + 1]) {
      return false;
    }
  }
  return true;
}

// ----------------------------------------------------------------------------
// GetBankOffset
// ----------------------------------------------------------------------------
static uint cartridge_GetBankOffset(byte bank) {
  return bank * 16384;
}

// ----------------------------------------------------------------------------
// WriteBank
// ----------------------------------------------------------------------------
static void cartridge_WriteBank(word address, byte bank) {
  uint offset = cartridge_GetBankOffset(bank);
  if(offset < cartridge_size) {
    memory_WriteROM(address, 16384, cartridge_buffer + offset);
    cartridge_bank = bank;
  }
}

// ----------------------------------------------------------------------------
// ReadHeader
// ----------------------------------------------------------------------------
static void cartridge_ReadHeader(const byte* header) {
  uint index;
  char temp[33] = {0};
  for(index = 0; index < 32; index++) {
    temp[index] = header[index + 17];  
  }
  strcpy(cartridge_title,temp);
  
  //cartridge_size  = header[49] << 32; // why 32?
  cartridge_size  = header[49] << 24;
  cartridge_size |= header[50] << 16;
  cartridge_size |= header[51] << 8;
  cartridge_size |= header[52];

  if(header[53] == 0) {
    if(cartridge_size > 131072) {
      cartridge_type = CARTRIDGE_TYPE_SUPERCART_LARGE;
    }
    else if(header[54] == 2 || header[54] == 3) {
      cartridge_type = CARTRIDGE_TYPE_SUPERCART;
    }
    else if(header[54] == 4 || header[54] == 5 || header[54] == 6 || header[54] == 7) {
      cartridge_type = CARTRIDGE_TYPE_SUPERCART_RAM;
    }
    else if(header[54] == 8 || header[54] == 9 || header[54] == 10 || header[54] == 11) {
      cartridge_type = CARTRIDGE_TYPE_SUPERCART_ROM;
    }
    else {
      cartridge_type = CARTRIDGE_TYPE_NORMAL;
    }
  }
  else {
    if(header[53] == 1) {
      cartridge_type = CARTRIDGE_TYPE_ABSOLUTE;
    }
    else if(header[53] == 2) {
      cartridge_type = CARTRIDGE_TYPE_ACTIVISION;
    }
    else {
      cartridge_type = CARTRIDGE_TYPE_NORMAL;
    }
  }
  
  cartridge_pokey = (header[54] & 1)? true: false;
  cartridge_controller[0] = header[55];
  cartridge_controller[1] = header[56];
  cartridge_region = header[57];
  cartridge_flags = 0;
}

// ----------------------------------------------------------------------------
// Load
// ----------------------------------------------------------------------------
static bool _cartridge_Load(const byte* data, uint size) {
  uint index;
  if(size <= 128) {
    fprintf(stderr,"Cartridge data is invalid.");
    //logger_LogError("Cartridge data is invalid.", CARTRIDGE_SOURCE);
    return false;
  }

  cartridge_Release( );
  
  byte header[128] = {0};
  for(index = 0; index < 128; index++) {
    header[index] = data[index];
  }

  uint offset = 0;
  if(cartridge_HasHeader(header)) {
    cartridge_ReadHeader(header);
    size -= 128;
    offset = 128;
  }
  else {
    cartridge_size = size;
  }
  
  cartridge_buffer = (char *) malloc(cartridge_size);
  for(index = 0; index < cartridge_size; index++) {
    cartridge_buffer[index] = data[index + offset];
  }
  
//strcpy(cartridge_digest,hash_Compute(cartridge_buffer, cartridge_size));
  hash_Compute(cartridge_buffer, cartridge_size,cartridge_digest);
  return true;
}

// ----------------------------------------------------------------------------
// Load
// ----------------------------------------------------------------------------
bool cartridge_Load(char *filename) 
{
	FILE* file;
  if(strlen(filename) == 0) {
	fprintf(stderr,"Cartridge filename is invalid.");
    //logger_LogError("Cartridge filename is invalid.", CARTRIDGE_SOURCE);
    return false;
  }
  
  cartridge_Release();
  //logger_LogInfo("Opening cartridge file " + filename + ".");
  
  byte* data = NULL;
  uint size = archive_GetUncompressedFileSize(filename);
  if(size == 0) {
	file = fopen(filename, "rb");
    if (file < 0) {
      fprintf(stderr,"Failed to open the cartridge file %s for reading", filename);
      return false;  
    }

	struct stat fileStat;
    if (stat(filename,&fileStat) < 0)  {
      fprintf(stderr,"Failed to find the size of the cartridge file.");
      return false;
    }
	
	size = fileStat.st_size;
    data = (char *) malloc(size);
    if(fread((char *) data, sizeof(uint8_t), size, file) < 0) {
      fclose(file);
	  fprintf(stderr,"Failed to read the cartridge data.");
      cartridge_Release( );
      free(data);
      return false;
    }    

    fclose(file);    
  }
  else {
    data = (char *) malloc(size);
    archive_Uncompress(filename, data, size);
  }

  if(!_cartridge_Load(data, size)) {
	  fprintf(stderr,"Failed to load the cartridge data into memory.\n");
    free(data);
    return false;
  }
  if(data != NULL) {
	gameCRC = crc32(0, data, size);
	free(data);
  }
  strcpy(cartridge_filename, filename);
  return true;
}

bool cartridge_Load_buffer(char* rom_buffer, int rom_size) {
  cartridge_Release();
  byte* data = (byte *)rom_buffer;
  uint size = rom_size;

  if(!_cartridge_Load(data, size)) {
    return false;
  }
  strcpy(cartridge_filename ,"");
  return true;
}

// ----------------------------------------------------------------------------
// Store
// ----------------------------------------------------------------------------
void cartridge_Store( ) {
  switch(cartridge_type) {
    case CARTRIDGE_TYPE_NORMAL:
      memory_WriteROM(65536 - cartridge_size, cartridge_size, cartridge_buffer);
      break;
    case CARTRIDGE_TYPE_SUPERCART:
      if(cartridge_GetBankOffset(7) < cartridge_size) {
        memory_WriteROM(49152, 16384, cartridge_buffer + cartridge_GetBankOffset(7));
      }
      break;
    case CARTRIDGE_TYPE_SUPERCART_LARGE:
      if(cartridge_GetBankOffset(8) < cartridge_size) {
        memory_WriteROM(49152, 16384, cartridge_buffer + cartridge_GetBankOffset(8));
        memory_WriteROM(16384, 16384, cartridge_buffer + cartridge_GetBankOffset(0));
      }
      break;
    case CARTRIDGE_TYPE_SUPERCART_RAM:
      if(cartridge_GetBankOffset(7) < cartridge_size) {
        memory_WriteROM(49152, 16384, cartridge_buffer + cartridge_GetBankOffset(7));
        memory_ClearROM(16384, 16384);
      }
      break;
    case CARTRIDGE_TYPE_SUPERCART_ROM:
      if(cartridge_GetBankOffset(7) < cartridge_size && cartridge_GetBankOffset(6) < cartridge_size) {
        memory_WriteROM(49152, 16384, cartridge_buffer + cartridge_GetBankOffset(7));
        memory_WriteROM(16384, 16384, cartridge_buffer + cartridge_GetBankOffset(6));
      }
      break;
    case CARTRIDGE_TYPE_ABSOLUTE:
      memory_WriteROM(16384, 16384, cartridge_buffer);
      memory_WriteROM(32768, 32768, cartridge_buffer + cartridge_GetBankOffset(2));
      break;
    case CARTRIDGE_TYPE_ACTIVISION:
      if(122880 < cartridge_size) {
        memory_WriteROM(40960, 16384, cartridge_buffer);
        memory_WriteROM(16384, 8192, cartridge_buffer + 106496);
        memory_WriteROM(24576, 8192, cartridge_buffer + 98304);
        memory_WriteROM(32768, 8192, cartridge_buffer + 122880);
        memory_WriteROM(57344, 8192, cartridge_buffer + 114688);
      }
      break;
  }
}

// ----------------------------------------------------------------------------
// Write
// ----------------------------------------------------------------------------
void cartridge_Write(word address, byte data) {
  switch(cartridge_type) {
    case CARTRIDGE_TYPE_SUPERCART:
    case CARTRIDGE_TYPE_SUPERCART_RAM:
    case CARTRIDGE_TYPE_SUPERCART_ROM:
      if(address >= 32768 && address < 49152 && data < 9) {
        cartridge_StoreBank(data);
      }
      break;
    case CARTRIDGE_TYPE_SUPERCART_LARGE:
      if(address >= 32768 && address < 49152 && data < 9) {
        cartridge_StoreBank(data + 1);
      }
      break;
    case CARTRIDGE_TYPE_ABSOLUTE:
      if(address == 32768 && (data == 1 || data == 2)) {
        cartridge_StoreBank(data - 1);
      }
      break;
    case CARTRIDGE_TYPE_ACTIVISION:
      if(address >= 65408) {
        cartridge_StoreBank(address & 7);
      }
      break;
  }

  if(cartridge_pokey && address >= 0x4000 && address < 0x4009) {
    switch(address) {
      case POKEY_AUDF1:
        pokey_SetRegister(POKEY_AUDF1, data);
        break;
      case POKEY_AUDC1:
        pokey_SetRegister(POKEY_AUDC1, data);
        break;
      case POKEY_AUDF2:
        pokey_SetRegister(POKEY_AUDF2, data);
        break;
      case POKEY_AUDC2:
        pokey_SetRegister(POKEY_AUDC2, data);
        break;
      case POKEY_AUDF3:
        pokey_SetRegister(POKEY_AUDF3, data);
        break;
      case POKEY_AUDC3:
        pokey_SetRegister(POKEY_AUDC3, data);
        break;
      case POKEY_AUDF4:
        pokey_SetRegister(POKEY_AUDF4, data);
        break;
      case POKEY_AUDC4:
        pokey_SetRegister(POKEY_AUDC4, data);
        break;
      case POKEY_AUDCTL:
        pokey_SetRegister(POKEY_AUDCTL, data);
        break;
    }
  }
}

// ----------------------------------------------------------------------------
// StoreBank
// ----------------------------------------------------------------------------
void cartridge_StoreBank(byte bank) {
  switch(cartridge_type) {
    case CARTRIDGE_TYPE_SUPERCART:
      cartridge_WriteBank(32768, bank);
      break;
    case CARTRIDGE_TYPE_SUPERCART_RAM:
      cartridge_WriteBank(32768, bank);
      break;
    case CARTRIDGE_TYPE_SUPERCART_ROM:
      cartridge_WriteBank(32768, bank);
      break;
    case CARTRIDGE_TYPE_SUPERCART_LARGE:
      cartridge_WriteBank(32768, bank);        
      break;
    case CARTRIDGE_TYPE_ABSOLUTE:
      cartridge_WriteBank(16384, bank);
      break;
    case CARTRIDGE_TYPE_ACTIVISION:
      cartridge_WriteBank(40960, bank);
      break;
  }  
}

// ----------------------------------------------------------------------------
// IsLoaded
// ----------------------------------------------------------------------------
bool cartridge_IsLoaded( ) {
  return (cartridge_buffer != NULL)? true: false;
}

// ----------------------------------------------------------------------------
// Release
// ----------------------------------------------------------------------------
void cartridge_Release( ) {
  if(cartridge_buffer != NULL) {
    free(cartridge_buffer);
    cartridge_size = 0;
    cartridge_buffer = NULL;

    // These values need to be reset so that moving between carts works
    // consistently. This seems to be a ProSystem emulator bug.
    //
    cartridge_type = 0;
    cartridge_region = 0;
    cartridge_pokey = 0;
    memset( cartridge_controller, 0, sizeof( cartridge_controller ) );
    cartridge_bank = 0;
    cartridge_flags = 0;
  }
}
