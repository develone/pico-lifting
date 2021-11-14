#include <stdio.h>
const unsigned char CRC7_POLY = 0x91;
 
unsigned char getCRC(unsigned char message[], unsigned char length)
{
  unsigned char i, j, crc = 0;
 
  for (i = 0; i < length; i++)
  {
    crc ^= message[i];
    for (j = 0; j < 8; j++)
    {
      if (crc & 1)
        crc ^= CRC7_POLY;
      crc >>= 1;
    }
  }
  return crc;
}
 
int main()
{
  // create a message array that has one extra byte to hold the CRC:
  unsigned char message[3] = {0x83, 0x01, 0x00};
  message[2] = getCRC(message, 2);
  // send this message to the Simple Motor Controller
  for(int i=0;i<3;i++) printf("%x ",message[i]);printf("\n");
  message[1] = 0x02;
  
  message[2] = getCRC(message, 2);
  for(int i=0;i<3;i++) printf("%x ",message[i]);printf("\n");
}
