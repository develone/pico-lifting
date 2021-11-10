/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"

#define DBUG 1

struct PTRs {
	/*This is the buffer for inp & output
	2048 x 2048 = 4194304
	256 x 256 = 65536
	*/
	short int inpbuf[4096*2];
	short int *inp_buf;
	short int *out_buf;
	short int flag;
	short int w;
	short int h;
	short int *fwd_inv;
	short int fwd;
	short int *red;
} ptrs;

void	singlelift(short int rb, short int w, short int * const ibuf, short int * const obuf) {
	short int	col, row;
	//printf("in singlelift \n");
	for(row=0; row<w; row++) {
		register short int	*ip, *op, *opb;
		register short int	ap,b,cp,d;

		//
		// Ibuf walks down rows (here), but reads across columns (below)
		// We might manage to get rid of the multiply by doing something
		// like: 
		//	ip = ip + (rb-w);
		// but we'll keep it this way for now.
		//
		//setting to beginning of each row
		ip = ibuf+row*rb;

		//
		// Obuf walks across columns (here), writing down rows (below)
		//
		// Here again, we might be able to get rid of the multiply,
		// but let's get some confidence as written first.
		//
		op = obuf+row;
		opb = op + w*rb/2;
		//printf("ip = 0x%x op = 0x%x opb = 0x%x\n",ip,op,opb);
		//
		// Pre-charge our pipeline
		//

		// a,b,c,d,e ...
		// Evens get updated first, via the highpass filter
		ap = ip[0];
		b  = ip[1];
		cp = ip[2];
		d  = ip[3]; ip += 4;
		//printf("ap = %d b = %d cp = %d d = %d\n",ap,b,cp,d);
		//
		ap = ap-b; // img[0]-(img[1]+img[-1])>>1)
		cp = cp- ((b+d)>>1);
		 
		op[0] = ap;
		opb[0]  = b+((ap+cp+2)>>2);

		for(col=1; col<w/2-1; col++) {
			op +=rb; // = obuf+row+rb*col = obuf[col][row]
			opb+=rb;// = obuf+row+rb*(col+w/2) = obuf[col+w/2][row]
			ap = cp;
			b  = d;
			cp = ip[0];	// = ip[row][2*col+1]
			d  = ip[1];	// = ip[row][2*col+2]

			//HP filter in fwd dwt			
			cp  = (cp-((b+d)>>1)); //op[0] is obuf[col][row]
			*op = ap; //op[0] is obuf[col][row]
			 
			//LP filter in fwd dwt
			*opb = b+((ap+cp+2)>>2);
			ip+=2;	// = ibuf + (row*rb)+2*col
		}

		op += rb; opb += rb;
		*op  = cp;
		*opb = d+((cp+1)>>3);
	}
}

void	ilift(short int rb, short int w,  short int * const ibuf,  short int * const obuf) {
	short int	col, row;

	for(row=0; row<w; row++) {
		register short int	*ip, *ipb, *op;
		register short int	b,c,d,e;

		//
		// Ibuf walks down rows (here), but reads across columns (below)
		// We might manage to get rid of the multiply by doing something
		// like: 
		//	ip = ip + (rb-w);
		// but we'll keep it this way for now.
		//
		//setting to beginning of each row
		op = obuf+row*rb;

		//
		// Obuf walks across columns (here), writing down rows (below)
		//
		// Here again, we might be able to get rid of the multiply,
		// but let's get some confidence as written first.
		//
		ip  = ibuf+row;
		ipb = ip + w*rb/2;
		//printf("ip = 0x%x op = 0x%x ipb = 0x%x\n",ip,op,ipb);
		//
		// Pre-charge our pipeline
		//

		// a,b,c,d,e ...
		// Evens get updated first, via the highpass filter
		c = ip[0]; // would've been called 'a'
		ip += rb;
		e = ip[0];	// Would've been called 'c'
		d  = ipb[0] -((c+e+2)>>2);

		op[0] = c+d;	// Here's the mirror, left-side
		op[1] = d;
		//printf("c = %d e = %d d = %d c+d = %d\n",c,e,c,c+d);
		for(col=1; col<w/2-1; col++) {
			op += 2;
			ip += rb; ipb += rb;

			c = e; b = d;
			e = ip[0];
			d = ipb[0] - ((c+e+2)>>2);
			c = c + ((b+d)>>1);

			op[0] = c;
			op[1] = d;
		}

		ipb += rb;
		d = ipb[0] - ((e+1)>>3);

		c = e + ((b+d)>>1);
		op[2] = c;
		op[3] = d;	// Mirror
	}
}

void	lifting(short int w, short int *ibuf, short int *tmpbuf, short int *fwd) {
	const	short int	rb = w;
	short int	lvl;

	short int	*ip = ibuf, *tp = tmpbuf, *test_fwd = fwd;
	printf("ip = 0x%x tp = 0x%x \n",ip,tp);
	short int	ov[3];

	const short int	LVLS = 3;

/*
	for(lvl=0; lvl<w*w; lvl++)
		ibuf[lvl] = 0;
	for(lvl=0; lvl<w*w; lvl++)
		tmpbuf[lvl] = 5000;

	for(lvl=0; lvl<w; lvl++)
		ibuf[lvl*(rb+1)] = 20;

	singlelift(rb,w,ip,tp);
	for(lvl=0; lvl<w*w; lvl++)
		ibuf[lvl] = tmpbuf[lvl];

	return;
*/

	for(lvl=0; lvl<LVLS; lvl++) {
		// Process columns -- leave result in tmpbuf
		//printf("in lifting \n");
		singlelift(rb, w, ip, tp);
		// Process columns, what used to be the rows from the last
		// round, pulling the data from tmpbuf and moving it back
		// to ibuf.
		//printf("w = 0x%x ip = 0x%x tp = 0x%x \n",w,ip,tp);
		singlelift(rb, w, tp, ip);
		//printf("back from singlelift\n");
		// lower_upper
		//
		// For this, we just adjust our pointer(s) so that the "image"
		// we are processing, as referenced by our two pointers, now
		// references the bottom right part of the image.
		//
		// Of course, we haven't really changed the dimensions of the
		// image.  It started out rb * rb in size, or the initial w*w,
		// we're just changing where our pointer into the image is.
		// Rows remain rb long.  We pretend (above) that this new image
		// is w*w, or should I say (w/2)*(w/2), but really we're just
		// picking a new starting coordinate and it remains rb*rb.
		//
		// Still, this makes a subimage, within our image, containing
		// the low order results of our processing.
		short int	offset = w*rb/2+w/2;
		ip = &ip[offset];
		tp = &tp[offset];
		ov[lvl] = offset + ((lvl)?(ov[lvl-1]):0);

		// Move to the corner, and repeat
		w>>=1;
	}
	printf("testing test_fwd \n");
	if (test_fwd[0]==0) {
	for(lvl=(LVLS-1); lvl>=0; lvl--) {
		short int	offset;

		w <<= 1;

		if (lvl)
			offset = ov[lvl-1];
		else
			offset = 0;
		ip = &ibuf[offset];
		tp = &tmpbuf[offset];
		//printf("ip = 0x%x tp = 0x%x \n",ip,tp);

		ilift(rb, w, ip, tp);
		//printf("back from ilift\n");
		ilift(rb, w, tp, ip);
		//printf("back from ilift\n");
	}
	}
}

const char src[] = "Hello, world! ";
const short int a[] = {
161,157,156,157,159,162,162,166,172,165,148,117,93,94,94,94,
159,157,158,160,161,163,167,166,153,130,99,92,95,95,93,98,
157,156,158,159,165,166,158,137,104,87,90,92,93,94,91,96,
155,155,157,164,168,159,144,132,127,127,128,127,125,126,123,122,
160,163,166,166,165,160,159,162,163,164,163,161,159,158,158,159,
170,170,165,160,159,161,163,165,166,167,168,167,166,165,164,167,
168,164,159,161,160,160,162,163,161,161,160,161,160,160,161,161,
134,131,132,131,131,130,132,132,131,130,128,127,127,128,129,127,
97,95,95,94,93,92,91,88,83,83,85,85,85,85,83,80,
106,104,104,104,103,103,101,97,94,96,96,97,97,95,93,91,
109,108,110,109,108,107,107,103,102,103,102,103,103,102,100,102,
109,108,107,108,106,107,107,105,103,103,104,104,103,104,101,104,
109,108,107,108,107,107,109,108,106,105,105,105,106,104,104,105,
117,116,116,115,115,115,117,115,112,111,111,111,111,111,110,110,
124,123,123,124,122,123,124,122,119,118,117,118,119,118,115,116,
129,128,128,127,125,125,126,125,121,120,121,121,121,118,124,159,
132,130,131,129,128,127,129,127,125,123,125,125,127,145,157,142,
133,132,132,131,130,128,129,127,127,126,126,126,119,107,108,106,
132,133,133,131,129,129,132,130,128,131,126,113,116,114,115,115,
133,133,133,131,130,131,131,131,137,127,113,115,116,116,116,117,
135,134,133,133,131,130,131,135,117,114,118,119,118,119,121,126,
135,135,134,133,131,132,131,138,121,119,122,117,120,124,130,126,
135,134,134,132,132,132,131,128,122,125,122,119,127,133,136,126,
134,135,133,133,132,131,133,133,131,127,124,128,133,139,134,131,
136,134,134,135,133,132,142,136,130,133,132,136,138,137,134,121,
135,133,133,130,129,130,144,144,135,134,134,139,140,138,121,156,
134,133,133,127,126,132,154,149,141,139,137,142,136,128,145,173,
133,132,133,129,127,136,165,160,154,148,144,144,141,139,181,168,
133,132,132,129,126,141,176,168,170,163,162,158,152,177,179,175,
133,132,131,129,125,138,175,176,178,178,181,172,172,191,182,177,
136,135,136,131,129,126,180,182,182,185,185,173,195,190,182,188,
134,133,134,132,131,126,169,187,188,189,182,195,200,188,193,186,
134,132,133,133,133,129,144,193,191,193,189,206,199,193,194,189,
132,132,131,129,130,129,123,174,195,193,201,203,196,202,197,190,
131,130,131,130,130,131,129,126,189,194,208,202,202,203,199,196,
131,129,129,129,129,129,130,123,141,203,209,206,203,203,200,199,
130,130,131,129,128,129,129,125,113,181,217,210,206,205,201,199,
129,129,130,130,129,129,128,125,117,109,182,220,209,205,205,200,
122,124,125,125,126,125,124,120,117,111,95,149,213,210,207,205,
111,115,118,120,119,119,118,115,113,108,103,89,162,218,212,205,
124,111,109,111,113,114,112,109,108,106,101,95,81,130,204,215,
149,140,127,125,123,121,120,120,119,117,115,114,112,105,95,147,
159,157,153,150,144,141,142,144,146,149,148,148,148,146,145,143,
153,158,162,160,155,149,145,146,151,156,155,155,155,155,154,156,
153,159,161,160,158,154,150,148,146,149,152,152,154,154,154,154,
155,159,162,159,158,156,154,150,143,128,121,126,126,127,127,123,
154,156,159,159,157,155,153,151,147,129,90,60,58,61,60,52,
155,156,156,157,157,155,153,147,146,148,137,110,74,49,46,151,
156,155,154,156,157,155,152,146,144,147,148,146,130,94,122,192,
157,154,152,154,156,153,149,146,146,148,148,147,149,139,199,210,
180,156,151,152,153,149,145,144,144,144,145,145,144,147,208,214,
216,206,174,149,149,146,143,142,143,146,146,146,147,148,207,223,
195,216,216,201,163,144,143,143,145,147,146,149,151,114,58,120,
107,145,202,220,217,195,156,142,146,149,152,138,79,48,52,51,
120,113,114,166,216,223,215,183,150,153,113,54,47,55,57,56,
122,121,117,114,127,185,222,225,203,88,46,52,59,57,57,52,
123,122,122,122,117,110,148,192,105,47,52,57,54,53,55,51,
124,122,123,123,123,119,96,46,46,53,55,51,54,54,55,56,
125,124,125,124,129,99,47,49,54,55,50,55,52,54,51,93,
125,126,129,131,100,49,51,56,56,51,55,51,56,61,118,146,
127,128,135,101,49,52,56,54,51,54,53,61,79,133,142,150,
126,130,102,51,56,58,54,53,52,54,68,93,136,140,154,158,
122,104,51,50,55,53,51,51,54,59,90,141,147,157,159,158,
142,53,51,55,54,50,50,52,48,103,149,146,161,162,157,157,
80,145,106,0,0,16,0,0,8,0,0,0,4,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,80,250,182,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
255,255,255,255,56,210,231,190,124,119,223,182,24,37,250,182,
0,80,250,182,0,0,0,0,128,58,248,182,64,40,250,182,
108,155,248,182,0,0,0,0,0,0,0,0,172,3,1,0,
180,212,231,190,205,212,231,190,226,212,231,190,245,212,231,190,
123,214,231,190,158,214,231,190,192,214,231,190,241,214,231,190,
228,221,231,190,10,222,231,190,21,222,231,190,38,222,231,190,
16,0,0,0,214,176,63,0,6,0,0,0,0,16,0,0,
9,0,0,0,172,3,1,0,11,0,0,0,233,3,0,0,
31,0,0,0,241,223,231,190,15,0,0,0,46,212,231,190,
0,83,72,69,76,76,61,47,98,105,110,47,98,97,115,104,
71,95,83,69,83,83,73,79,78,95,80,65,84,72,61,47,
95,80,82,69,70,73,88,61,108,120,100,101,45,112,105,45,
72,95,83,79,67,75,61,47,116,109,112,47,115,115,104,45,
108,111,99,97,108,47,111,112,101,110,111,99,100,47,98,105,
83,83,73,79,78,61,76,88,68,69,45,112,105,0,83,83,
68,61,47,104,111,109,101,47,100,101,118,101,108,47,112,105,
116,100,109,45,120,115,101,115,115,105,111,110,0,76,79,71,
78,95,84,89,80,69,61,120,49,49,0,71,80,71,95,65,
65,85,84,72,79,82,73,84,89,61,47,104,111,109,101,47,
114,97,115,0,88,68,71,95,71,82,69,69,84,69,82,95,
84,72,61,47,111,112,116,47,112,105,99,111,45,115,100,107,
48,58,100,105,61,48,49,59,51,52,58,108,110,61,48,49,
58,99,100,61,52,48,59,51,51,59,48,49,58,111,114,61,
52,50,58,111,119,61,51,52,59,52,50,58,115,116,61,51,
49,58,42,46,97,114,106,61,48,49,59,51,49,58,42,46,
108,122,109,97,61,48,49,59,51,49,58,42,46,116,108,122,
61,48,49,59,51,49,58,42,46,122,61,48,49,59,51,49,
108,122,111,61,48,49,59,51,49,58,42,46,120,122,61,48,
49,59,51,49,58,42,46,116,98,122,61,48,49,59,51,49,
58,42,46,106,97,114,61,48,49,59,51,49,58,42,46,119,
108,122,61,48,49,59,51,49,58,42,46,97,99,101,61,48,
59,51,49,58,42,46,99,97,98,61,48,49,59,51,49,58,
42,46,106,112,103,61,48,49,59,51,53,58,42,46,106,112,
42,46,98,109,112,61,48,49,59,51,53,58,42,46,112,98,
109,61,48,49,59,51,53,58,42,46,120,112,109,61,48,49,
49,59,51,53,58,42,46,115,118,103,122,61,48,49,59,51,
53,58,42,46,109,112,101,103,61,48,49,59,51,53,58,42,
42,46,109,112,52,61,48,49,59,51,53,58,42,46,109,52,
118,61,48,49,59,51,53,58,42,46,119,109,118,61,48,49,
59,51,53,58,42,46,97,118,105,61,48,49,59,51,53,58,
120,99,102,61,48,49,59,51,53,58,42,46,120,119,100,61,
48,49,59,51,53,58,42,46,111,103,120,61,48,49,59,51,
54,58,42,46,109,105,100,61,48,48,59,51,54,58,42,46,
46,111,103,103,61,48,48,59,51,54,58,42,46,114,97,61,
61,48,48,59,51,54,58,42,46,120,115,112,102,61,48,48,
68,71,95,83,69,65,84,95,80,65,84,72,61,47,111,114,
78,68,95,80,65,84,72,61,47,111,112,116,47,112,105,99,
53,54,99,111,108,111,114,0,85,83,69,82,61,100,101,118,
112,105,99,111,45,101,120,97,109,112,108,101,115,0,88,68,
115,101,114,47,49,48,48,49,0,76,67,95,65,76,76,61,
111,99,97,108,47,115,104,97,114,101,58,47,117,115,114,47,
100,109,58,47,118,97,114,47,108,105,98,47,109,101,110,117,
58,47,117,115,114,47,108,111,99,97,108,47,98,105,110,58,
117,115,114,47,103,97,109,101,115,0,71,68,77,83,69,83,
83,95,83,69,83,83,73,79,78,95,66,85,83,95,65,68,
110,117,120,45,80,65,77,0,79,76,68,80,87,68,61,47    
    };

int main() {
    stdio_init_all();
    ptrs.w = 64;
    ptrs.h = 64;
    
    ptrs.inp_buf = ptrs.inpbuf; 
	ptrs.out_buf = ptrs.inpbuf + 4096;
	
	ptrs.fwd_inv =  &ptrs.fwd;
    *ptrs.fwd_inv = 1;
    
    for(int i = 0; i < 4096;i++)
        {
            *ptrs.inp_buf = a[i];
            //printf("%d \n",*ptrs.inp_buf);
            ptrs.inp_buf++;
            
        }
    while (true) {
        if (DBUG == 1 ) {
            printf("Hello, world!\n");
            printf("Now copmpiles with lifting code as part of hello_usb.c\n"); 
            printf("structure PTRS added to hello_usb.c\n");
            printf("ptrs.w = %d ptrs.h = %d \n", ptrs.w, ptrs.h);
            printf("These are the variables needed for lifting\n");
            printf("ptrs.inp_buf = 0x%x ptrs.out_buf = 0x%x\n",ptrs.inp_buf, ptrs.out_buf);
            
            printf("w = %d ptrs.fwd_inv = 0x%x ptrs.fwd_inv = %d\n",ptrs.w,ptrs.fwd_inv, *ptrs.fwd_inv); 
            //for(int i=0;i<25;i++) printf("%d ",a[i]);
            //printf("\n");
        } 
        
        printf("Calling lifting!\n");
        lifting(ptrs.w,ptrs.inp_buf,ptrs.out_buf,ptrs.fwd_inv);
        printf("Back in main!\n");
        for(int i=0;i<25;i++) {
            printf("%d ",*ptrs.out_buf);
            ptrs.out_buf++;
        }
        ptrs.out_buf = ptrs.inpbuf + 4096;
        sleep_ms(1000);
    }
    return 0;
}
