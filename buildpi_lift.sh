#/bin/bash
rm -f pi_jpeg lifting.o pi_jpeg.o jpeg.map pi_jpeg.txt
gcc -g -c lifting.c -o lifting.o
gcc -g -c pi_jpeg.c -o pi_jpeg.o
gcc -g -Wl,-Map=pi_jpeg.map pi_jpeg.o  lifting.o -o pi_jpeg
objdump -d pi_jpeg > pi_jpeg.txt
