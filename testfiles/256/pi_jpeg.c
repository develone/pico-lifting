
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "lifting.h"

#define DBUG 1
/* First parameter is used to tell the program which sub band to use 
 * 0 Red
 * 1 Green
 * 2 Blue
 * 2nd parameter is used to tell the program to compute the fwd lifting step only or fwd lifting then inv lifting step
 * 0 fwd lifting then inv lifting step
 * 1 fwd lifting step only
 * ./pi_jpeg 0 1 or ./pi_jpeg 0 0
 * ./pi_jpeg 1 1 or ./pi_jpeg 1 0
 * ./pi_jpeg 2 1 or ./pi_jpeg 2 0
 */
struct PTRs {
	/*This is the buffer for inp & output
	2048 x 2048 = 4194304
	256 x 256 = 65536
	
	int inpbuf[4194304*2];
	*/
	int inpbuf[65536*2];
	int *inp_buf;
	int *out_buf;
	int flag;
	int w;
	int h;
	int *fwd_inv;
	int fwd;
	int *red;
} ptrs;

 
int main(int argc, char **argv) {
	
	FILE *inptr,*outptr;
	char *ch;
	int tmp,loop;
	 
	int *red_s_ptr;
	int *wptr;
	int *alt;
	
	int *dummy;
	 
	int *buf_red;
	  
	int i,j;
	/* 
	ptrs.w = 2048;
	ptrs.h = 2048;
	*/  
	ptrs.w = 256;
	ptrs.h = 256;
	
	ptrs.inp_buf = ptrs.inpbuf; 
	//ptrs.out_buf = ptrs.inpbuf + 4194304;
	ptrs.out_buf = ptrs.inpbuf + 65536;
	
	ptrs.fwd_inv =  &ptrs.fwd;
	
	if (DBUG == 1 ) printf("0x%x 0x%x 0x%x \n",buf_red, ptrs.inp_buf, ptrs.out_buf);
	
	/*red_s_ptr = buf_red;*/
	red_s_ptr = ptrs.inpbuf;
	
	
 
	if(ptrs.inp_buf == NULL) return 2;
	
	if(ptrs.fwd_inv == NULL) return 5;
	red_s_ptr = ptrs.inp_buf;
   
     
  if (DBUG == 1 ) printf("ptrs.fwd_inv = 0x%x\n",ptrs.fwd_inv);
  //loop = 4194304;
  loop = 65536;
		ch = argv[1];
		tmp = atoi(ch);
		if (tmp == 0) { 
			if (DBUG == 1 ) printf("reading r.bin\n");
			ptrs.flag = tmp;
			inptr = fopen("r.bin","rb");
			if (!inptr)
			{
				if (DBUG == 1 ) printf("Unle to open file!");
				return 1;
			}
			//else fread(ptrs.inp_buf,sizeof(int),4194304,inptr);
			else fread(ptrs.inpbuf,sizeof(int),65536,inptr);
 
			fclose(inptr);

		}
		else if (tmp == 1) {
			if (DBUG == 1 ) printf("reading g.bin\\n");
			ptrs.flag = tmp;
						inptr = fopen("g.bin","rb");
			if (!inptr)
			{
				if (DBUG == 1 ) printf("Unle to open file!");
				return 1;
			}
			//else fread(ptrs.inpbuf,sizeof(int),4194304,inptr);
			else fread(ptrs.inpbuf,sizeof(int),65536,inptr);
			fclose(inptr);

		}	
		else if (tmp == 2) {
			if (DBUG == 1 ) printf("reading b.bin\\n");
			ptrs.flag = tmp;
						inptr = fopen("b.bin","rb");
			if (!inptr)
			{
				if (DBUG == 1 ) printf("Unle to open file!");
				return 1;
			}
			//else fread(ptrs.inpbuf,sizeof(int),4194304,inptr);
			else fread(ptrs.inpbuf,sizeof(int),65536,inptr);
			fclose(inptr);

		}
		else {
			if (DBUG == 1 ) printf("First parameter can only be 0 1 2 \n");
 			exit (1);
		}
		
		ch = argv[2];
		tmp = atoi(ch); 
 
		if (tmp == 0) { 
			if (DBUG == 1 ) printf("fwd lifting then inv lifting step\n");
			*ptrs.fwd_inv = tmp;
		}
		else if (tmp == 1) {
			if (DBUG == 1 ) printf("fwd lifting step only\n");
			*ptrs.fwd_inv = tmp;
		} else
		{
			if (DBUG == 1 ) printf("2nd parameter can only be 0 1  \n");
			exit (2);
		}
	 
  	
		wptr = ptrs.inp_buf;
		
		alt = ptrs.out_buf;
		if (DBUG == 1 ) printf("w = 0x%x ptrs.inp_buf wptr = 0x%x alt =  0x%x ptrs.fwd_inverse =  0x%x ptrs.fwd_inverse =  0x%x \n",ptrs.w, wptr,alt,ptrs.fwd_inv,*ptrs.fwd_inv);
		if (DBUG == 1 ) printf("starting red dwt\n");
		
		lifting(ptrs.w,wptr,alt,ptrs.fwd_inv);
		if (DBUG == 1 ) printf("finished ted dwt\n");
		 
	
    outptr = fopen("dwt.bin","wb");
    if (!outptr)
	{
 	  if (DBUG == 1 )printf("Unle to open file!");
	return 1;
	}
	//fwrite(ptrs.inp_buf,sizeof( int),4194304,outptr);
	fwrite(ptrs.inp_buf,sizeof( int),65536,outptr);
	fclose(outptr);
 		
	return 0;

}
