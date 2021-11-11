#include <stdio.h>
#include <stdlib.h>
int main() {
FILE *inptr;
int i,j,k,l,index=0;
char inp_buf[4096];
inptr = fopen("r-8.bin","rb");
if (!inptr) {
	printf("Unle to open file!");
} 
else {
	fread(inp_buf,sizeof(short int),4096,inptr);
}
	for(j=0;j<64;j++) {
		for(l=0;l<4;l++) {
			//printf("%d\n",l);
			for(i=0;i<16;i++) {
				printf("%d,",inp_buf[index]);
				//printf("%d %d %d\n",i,index,index++);
				index++;
			}
			//index = index + 64;
			printf("\n");
		}
	//printf("read 16 values\n");
	}
return 0;
}
