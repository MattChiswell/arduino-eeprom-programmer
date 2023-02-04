/*
 | -----------------------------------------
 | EEPROM Read/Write
 | -----------------------------------------
 | Main Program Definition
 | -----------------------------------------
 | Use of this software is at your own risk!
 | License   CC BY-NC-SA 4.0
 |           https://creativecommons.org/licenses/by-nc-sa/4.0/
 | -----------------------------------------
 | @author   Matt Chiswell
 | @email    24735648+MattChiswell@users.noreply.github.com
 | @created  2nd Feb 2023
 | @modified 4th Feb 2023
 | @version  1.0.0
 | -----------------------------------------
*/

/**
  @note   This program was written for the ATMega4809 processors;
          this series completely re-defined the PORT registers,
          therefore, this code will not work at all on previous
          generation chips (like the 328 etc) without significant
          syntax changes
  @note   The code I originaly produced for this purpose was
          based upon the EEPROM Programmer example given by
          Ben Eater (https://github.com/beneater/eeprom-programmer).
          Wanting to play around with the ATMega4809, I refactored 
          the code to use direct registry access.
*/

/**
  Method Definitions
*/
//int main( void );
void asmNOP( uint16_t iterations );
void portSetup( void ) ;
void eepromIOSetup( bool input );
void shiftByte( byte data, bool msbf );
void setAddress( uint16_t address, bool outputEnable );
byte readByte( uint16_t address );
void writeByte( uint16_t address, byte data );
void readEEPROM( uint16_t offset, uint16_t numBytes );
void writeEEPROM( byte data[], uint16_t length, uint16_t offset );
void eraseEEPROM( uint16_t offset, uint16_t length, byte data );
void blinky( int blinks );

/**
  IO Functions / Setup
*/

/**
  @name   asmNOP
  @brief  execute CPU NOP command(s)
  @note   precise timing is pretty difficult here, the loop adds a bit
  @param  iterations  how many times to execute
  @return void
*/
void asmNOP( uint16_t iterations ) {
  for ( int i = 0; i < iterations; i++ ) {
    __asm__("nop");
  }
}

/**
  @name   portSetup
  @brief  initialises the required ports as inputs/outputs
  @note   EEPROM_WE is set high here to ensure write inhibit is active
  @return void
*/
void portSetup(void) {
  // shift registers
  PORTA.DIRSET = PIN0_bm;             // data  [D2]
  PORTF.DIRSET = PIN5_bm;             // clock [D3]
  PORTC.DIRSET = PIN6_bm;             // latch [D4]
  // EEPROM_WE
  PORTD.DIRCLR = PIN3_bm;             // eeprom write enable (active low) [D14]
  PORTD.IN = PIN3_bm;                 // writing to port as input pre-sets the bit in PORTx.OUT
  PORTD.PIN3CTRL = PORT_PULLUPEN_bm;  // eeprom_we pull high
  // take shift register i/o low initially
  PORTA.OUTCLR = PIN0_bm;
  PORTF.OUTCLR = PIN5_bm;
  PORTC.OUTCLR = PIN6_bm;
  // done
  return;
}

/**
  @name   eepromIOSetup
  @brief  initialise the EEPROM I/O pins as inputs/outputs
  @note   defaults to output if [input] is not set true
  @param  input true:input|false:output
  @return void
*/
void eepromIOSetup( bool input=false ) {
  // input
  if ( input ) {
    // ensure EEPROM_WE is high (this method could be called after a write)
    PORTD.PIN3CTRL = PORT_PULLUPEN_bm;
    // setup ports [input]
    PORTB.DIRCLR = PIN2_bm;       // IO 0 [D5]
    PORTF.DIRCLR = PIN4_bm;       // IO 1 [D6]
    PORTA.DIRCLR = PIN1_bm;       // IO 2 [D7]
    PORTE.DIRCLR = PIN3_bm;       // IO 3 [D8]
    PORTB.DIRCLR = PIN0_bm;       // IO 4 [D9]
    PORTB.DIRCLR = PIN1_bm;       // IO 5 [D10]
    PORTE.DIRCLR = PIN0_bm;       // IO 6 [D11]
    PORTE.DIRCLR = PIN1_bm;       // IO 7 [D12]
  // output
  } else {
    // setup ports [output]
    PORTB.DIRSET = PIN2_bm;       // IO 0 [D5]
    PORTF.DIRSET = PIN4_bm;       // IO 1 [D6]
    PORTA.DIRSET = PIN1_bm;       // IO 2 [D7]
    PORTE.DIRSET = PIN3_bm;       // IO 3 [D8]
    PORTB.DIRSET = PIN0_bm;       // IO 4 [D9]
    PORTB.DIRSET = PIN1_bm;       // IO 5 [D10]
    PORTE.DIRSET = PIN0_bm;       // IO 6 [D11]
    PORTE.DIRSET = PIN1_bm;       // IO 7 [D12]
    // now set EEPROM_WE as output
    PORTD.DIRSET = PIN3_bm;
  }
  // done
  return;
}

/**
  @name   shiftByte
  @brief  shifts out one byte, bit-by-bit to shift registers
  @note   clock pulse width of ≈ 250ns in 16MHz mode
          single nop instruction takes ~ 250ns
  @param  data  byte to shift out
  @param  msbf  most significant bit first; default true
  @return void
*/
void shiftByte( byte data, bool msbf=true ) {
  int bit;
  // ensure latch is low
  PORTC.OUTCLR = PIN6_bm;
  // loop through bits
  for ( int i = 0; i < 8; i++ ) {
    // msbf ?
    if ( msbf ) {
      bit = (data & (1 << i));
    } else {
      bit = (data & (1 << (7 - i)));
    }
    // set bit on data pin
    if ( bit ) {
      // logic 1 ~ high
      PORTA.OUTSET = PIN0_bm;
    } else {
      // logic 0 ~ low
      PORTA.OUTCLR = PIN0_bm;
    }
    // pulse clock
    PORTF.OUTSET = PIN5_bm;
    __asm__("nop");
    PORTF.OUTCLR = PIN5_bm;
  }
  // done
  return;
}

/**
  EEPROM Functions
*/

/**
  @name   setAddress
  @brief  set EEPROM address for read/write ops
  @param  address       EEPROM memory address [11-bit]
  @param  outputEnable  EEPROM OE [true|false]
  @return void
*/
void setAddress( uint16_t address, bool outputEnable ) {
  // shift out 16-bits, 1 byte at a time; setting OE as well
  shiftByte( (address >> 8) | (outputEnable ? 0x00 : 0x80), false );
  shiftByte( address, false );
  // latch to outputs
  PORTC.OUTSET = PIN6_bm;
  __asm__("nop");
  PORTC.OUTCLR = PIN6_bm;
  // done
  return;
}

/**
  @name   readByte
  @brief  read byte from given address
  @todo   should we reset OE when done ?
  @param  address EEPROM address [11-bit]
  @return byte
*/
byte readByte( uint16_t address ) {
  // set read address and enable output
  setAddress( address, true );
  // bit by bit
  byte data = 0;
  data = ((~PORTB.IN & PIN2_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTF.IN & PIN4_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTA.IN & PIN1_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTE.IN & PIN3_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTB.IN & PIN0_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTB.IN & PIN1_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTE.IN & PIN0_bm) ? 0 : 1);
  data = (data << 1) + ((~PORTE.IN & PIN1_bm) ? 0 : 1);
  // done
  return data;
}

/**
  @name   writeByte
  @brief  write a byte to given address
  @param  address EEPROM address [11-bit]
  @param  data    byte
  @return void
*/
void writeByte( uint16_t address, byte data ) {
  // set address and disable output
  setAddress( address, false );
  // set bits
  (data & (1 << 7)) ? PORTB.OUTSET = PIN2_bm : PORTB.OUTCLR = PIN2_bm;
  (data & (1 << 6)) ? PORTF.OUTSET = PIN4_bm : PORTF.OUTCLR = PIN4_bm;
  (data & (1 << 5)) ? PORTA.OUTSET = PIN1_bm : PORTA.OUTCLR = PIN1_bm;
  (data & (1 << 4)) ? PORTE.OUTSET = PIN3_bm : PORTE.OUTCLR = PIN3_bm;
  (data & (1 << 3)) ? PORTB.OUTSET = PIN0_bm : PORTB.OUTCLR = PIN0_bm;
  (data & (1 << 2)) ? PORTB.OUTSET = PIN1_bm : PORTB.OUTCLR = PIN1_bm;
  (data & (1 << 1)) ? PORTE.OUTSET = PIN0_bm : PORTE.OUTCLR = PIN0_bm;
  (data & 1) ? PORTE.OUTSET = PIN1_bm : PORTE.OUTCLR = PIN1_bm;
  // setup time ?
  // __asm__("nop");
  // pulse write enable [active low]
  PORTD.OUTCLR = PIN3_bm;
  asmNOP(3);                  // write cycle is very sensitive to this value, datasheet: min 10ns, max 1000ns
  PORTD.OUTSET = PIN3_bm;
  // wait a bit               // this large a value shouldnt be required according to the datasheet, but it
  delay(10);                  // doesnt work correctly without it, data is written but its corrupted
  // done
  return;
}

/**
  @name   readEEPROM
  @brief  reads specified bytes from EEPROM, 16 bytes at a time, starting at offset
          and sends data back via serial
  @param  offset    start reading from this address
  @param  numBytes  number of bytes to read
*/
void readEEPROM( uint16_t offset, uint16_t numBytes ) {
  // alert
  Serial.println("Reading EEPROM");
  // set EEPROM IO to input
  eepromIOSetup(true);
  // read 16 bytes at a time
  int base;
  base += offset;
  for ( base; base < numBytes; base += 16 ) {
    // byte array
    byte data[16];
    // save each byte to its respective index
    for ( int byteIndex = 0; byteIndex < 16; byteIndex++ ) {
      data[byteIndex] = readByte( base + byteIndex );
    }
    // build output string
    char outBuf[56];
    sprintf( outBuf, "%03x:   %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
      base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], 
      data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15] );
    // send it
    Serial.println(outBuf);
  }
  // done
  Serial.println("\nDone");
  return;
}

/**
  @name   writeEEPROM
  @brief  writes data to EEPROM
  @param  data    byte
  @param  length  number of bytes to write
  @param  offset  start at this address
  @return void
*/
void writeEEPROM( byte data[], uint16_t length, uint16_t offset=0 ) {
  // alert
  Serial.println("Programming EEPROM");
  // set EEPROM IO ports as ouputs
  eepromIOSetup();
  // offset ?
  uint16_t address;
  address += offset;
  length += offset;
  // iterate address
  for ( address; address < length; address++ ) {
    // write
    writeByte( address, data[address-offset] );
    // progress
    if ( address % 32 == 0 ) {
      Serial.print(".");
    }
  }
  // done
  Serial.println("\nDone");
  return;
}

/**
  @name   eraseEEPROM
  @brief  write over exisiting data with 0xFF for given address range
  @param  offset  start at this address
  @param  length  how many bytes are we over-writting ?
  @param  data    what is being written ? [optional]
  @return void
*/
void eraseEEPROM( uint16_t offset, uint16_t length, byte data=0xFF ) {
  // alert
  Serial.println("Erasing EEPROM");
  // set EEPROM IO ports as outputs
  eepromIOSetup();
  // offset ?
  uint16_t address;
  address += offset;
  // iterate address
  for( address; address < length; address++ ) {
    // write
    writeByte( address, data );
    // progress
    if ( address % 32 == 0 ) {
      Serial.print(".");
    }
  }
  // done
  Serial.println("\nDone");
  return;
}

/**
  @name   blinky
  @brief  blink the builtin LED
  @param  blinks number of blinks
  @return void
*/
void blinky( int blinks ) {
  // set LED builtin to output and make sure its off
  PORTE.DIRSET = PIN2_bm;
  PORTE.OUTCLR = PIN2_bm;
  // iterate
  for ( int i = 0; i < ((blinks + 1) * 2); i++ ) {
    // toggle
    PORTE.OUTTGL = PIN2_bm;
    // wait
    delay(100);
  }
  // done
  return;
}

/**
  Main
*/

void setup(void) {
  // do stuff once
  // initialise serial
  Serial.begin(57600);
  // initialise ports
  portSetup();
  // wait a bit
  blinky(5);
  // EXAMPLE: read EEPROM
  readEEPROM(0,2048);
  // EXAMPLE: write to EEPROM
  byte data[255];
  uint16_t dataLength = sizeof(data);
  for( int i = 0; i < dataLength; i++ ) {
    data[i] = 0x55;
  }
  writeEEPROM(data, dataLength);
  // EXAMPLE: erase EEPROM
  eraseEEPROM(0,2048);
}

void loop(void) {
  // do stuff forever
}