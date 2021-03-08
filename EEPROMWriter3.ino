// Hardware is Arduino Nano ATmega328P (Old Bootloader)
// Currently made to only use 16KiB of a 32KiB EEPROM
//////////////////////// Pins
#define SHIFT_SER 2
#define SHIFT_RCLK 3
#define SHIFT_SRCLK 4

#define WRITE_ENABLE 6
#define OUTPUT_ENABLE 7

#define DATA_BUS_7 8
#define DATA_BUS_6 5
#define DATA_BUS_5 19
#define DATA_BUS_4 18
#define DATA_BUS_3 17
#define DATA_BUS_2 16
#define DATA_BUS_1 15
#define DATA_BUS_0 14

//////////////////////// Other stuff
#define WRITE_MODE true
#define READ_MODE false

uint8_t receiveBuffer[256];

void setup()
{
  pinMode(SHIFT_SER, OUTPUT);
  pinMode(SHIFT_RCLK, OUTPUT);
  pinMode(SHIFT_SRCLK, OUTPUT);
  digitalWrite(SHIFT_RCLK, LOW);
  digitalWrite(SHIFT_SRCLK, LOW);

  digitalWrite(WRITE_ENABLE, HIGH); //do this before pinMode so writeEnable is never low
  digitalWrite(OUTPUT_ENABLE, HIGH);
  pinMode(OUTPUT_ENABLE, OUTPUT);
  pinMode(WRITE_ENABLE, OUTPUT);

  Serial.begin(38400);
}

void setAddr(uint16_t addr)
{
  addr &= 0x7FFF; // ROM is 32K, so address bus is 15 bit
  shiftOut(SHIFT_SER, SHIFT_SRCLK, LSBFIRST, addr & 0xFF);
  shiftOut(SHIFT_SER, SHIFT_SRCLK, LSBFIRST, addr >> 8);

  digitalWrite(SHIFT_RCLK, HIGH);
  digitalWrite(SHIFT_RCLK, LOW);
}

uint8_t readByte(uint16_t addr)
{
  setMode(READ_MODE);
  setAddr(addr);
  digitalWrite(WRITE_ENABLE, HIGH);
  digitalWrite(OUTPUT_ENABLE, LOW);
  //delayMicroseconds(1);
  delay(2);
  return digitalRead(DATA_BUS_0) |
         digitalRead(DATA_BUS_1) << 1 |
         digitalRead(DATA_BUS_2) << 2 |
         digitalRead(DATA_BUS_3) << 3 |
         digitalRead(DATA_BUS_4) << 4 |
         digitalRead(DATA_BUS_5) << 5 |
         digitalRead(DATA_BUS_6) << 6 |
         digitalRead(DATA_BUS_7) << 7;
}

void writeByte(uint16_t addr, uint8_t data)
{
  digitalWrite(OUTPUT_ENABLE, HIGH);
  digitalWrite(WRITE_ENABLE, HIGH);
  setAddr(addr);
  setMode(WRITE_MODE);
  digitalWrite(DATA_BUS_7, data & 0b10000000);
  digitalWrite(DATA_BUS_6, data & 0b01000000);
  digitalWrite(DATA_BUS_5, data & 0b00100000);
  digitalWrite(DATA_BUS_4, data & 0b00010000);
  digitalWrite(DATA_BUS_3, data & 0b00001000);
  digitalWrite(DATA_BUS_2, data & 0b00000100);
  digitalWrite(DATA_BUS_1, data & 0b00000010);
  digitalWrite(DATA_BUS_0, data & 0b00000001);
  digitalWrite(WRITE_ENABLE, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_ENABLE, HIGH);
  delayMicroseconds(2);
  //setMode(READ_MODE);
  //digitalWrite(OUTPUT_ENABLE, LOW);
  //Wait for write cycle to finish
  delay(8);
  //while (((bool)digitalRead(DATA_BUS_7)) != ((bool)(data & 0x80))) { }
}

void setMode(bool mode) //false = read, true = write
{
  uint8_t pinState = mode ? OUTPUT : INPUT;
  pinMode(DATA_BUS_7, pinState);
  pinMode(DATA_BUS_6, pinState);
  pinMode(DATA_BUS_5, pinState);
  pinMode(DATA_BUS_4, pinState);
  pinMode(DATA_BUS_3, pinState);
  pinMode(DATA_BUS_2, pinState);
  pinMode(DATA_BUS_1, pinState);
  pinMode(DATA_BUS_0, pinState);
}

void loop()
{
  if (Serial.available() > 0)
  {
    uint8_t receivedChar = Serial.read();
    if (receivedChar == 'P') // "Ping"
    {
      Serial.write('P'); // "Pong"
    }
    else if (receivedChar == 'R') // Read Whole EEPROM
    {
      digitalWrite(WRITE_ENABLE, HIGH);
      digitalWrite(OUTPUT_ENABLE, LOW);
      delay(10);
      for (uint16_t readPtr = 0; readPtr < 0x4000; readPtr++)
      {
        Serial.write(readByte(readPtr));
      }
    }
    else if (receivedChar == 'W') // Write Whole EEPROM
    {
      for (uint16_t addressHigh = 0; addressHigh < 0x4000; addressHigh += 0x0100)
      {
        uint16_t receivedByteNum = 0;
        while (receivedByteNum < 256)
        {
          if (Serial.available() > 0)
          {
            receiveBuffer[receivedByteNum] = Serial.read();
            receivedByteNum++;
          }
        }
        for (uint16_t wroteByteNum = 0; wroteByteNum < 256; wroteByteNum++)
        {
          uint8_t dataToWrite = receiveBuffer[wroteByteNum];
          uint16_t addressToWrite = addressHigh + wroteByteNum;
          if(readByte(addressToWrite) != dataToWrite)
          {
            writeByte(addressToWrite, dataToWrite);
          }
        }
        Serial.write('K'); // Send message to indicate ready for new block
      }
    }
  }
}
