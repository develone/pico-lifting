#include <stdio.h>
 
const unsigned char CRC7_POLY = 0x91;
unsigned char CRCTable[256];
 
unsigned char getCRCForByte(unsigned char val)
{
  unsigned char j;
 
  for (j = 0; j < 8; j++)
  {
    if (val & 1)
      val ^= CRC7_POLY;
    val >>= 1;
  }
 
  return val;
}
 
void buildCRCTable()
{
  int i;
 
  // fill an array with CRC values of all 256 possible bytes
  for (i = 0; i < 256; i++)
  {
    CRCTable[i] = getCRCForByte(i);
  }
}
 
unsigned char getCRC(unsigned char message[], unsigned char length)
{
  unsigned char i, crc = 0;
 
  for (i = 0; i < length; i++)
    crc = CRCTable[crc ^ message[i]];
  return crc;
}
 
int main()
{
  unsigned char message[3] = {0x83, 0x01, 0x00};
  int i, j;
 
  buildCRCTable();
  message[2] = getCRC(message, 2);
 
  for (i = 0; i < sizeof(message); i++)
  {
    for (j = 0; j < 8; j++)
      printf("%d", (message[i] >> j) % 2);
    printf(" ");
  }
  printf("\n");
}
