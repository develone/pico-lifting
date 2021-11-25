/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"

#define DBUG 0
#define DBUG1 0
#define DBUG2 0
#define DBUG3 0
#define DBUG4 1

#define imgsize 4096
//#define imgsize 512
struct PTRs {
	/*This is the buffer for inp & output
	2048 x 2048 = 4194304
	256 x 256 = 65536
	64 x 64 = 4096
	*/
	short int inpbuf[imgsize*2];
	short int *inp_buf;
	short int *out_buf;
	short int flag;
	short int w;
	short int h;
	short int *fwd_inv;
	short int fwd;
	short int *red;
	char *head;
	char *tail;
	char *topofbuf;
	char *endofbuf;
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
	//printf("ip = 0x%x tp = 0x%x \n",ip,tp);
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
	//printf("testing test_fwd \n");
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


unsigned char tt[128];
const char src[] = "Hello, world! ";
const short int a[] = {161,157,156,157,159,162,162,166,172,165,148,117,93,94,94,94,
100,104,103,102,100,102,103,107,109,110,110,108,107,106,105,105,
102,101,105,102,94,82,80,84,85,82,77,66,60,57,53,52,
52,84,131,127,82,51,42,39,40,40,39,77,91,72,57,52,
159,157,158,160,161,163,167,166,153,130,99,92,95,95,93,98,
103,103,102,101,99,100,102,103,106,107,106,106,106,103,103,106,
101,98,100,93,86,79,77,78,80,81,76,71,67,60,54,50,
47,59,108,145,152,149,140,134,122,112,107,110,124,105,83,72,
157,156,158,159,165,166,158,137,104,87,90,92,93,94,91,96,
101,103,104,102,100,106,108,109,108,104,102,100,100,99,100,100,
95,89,88,82,76,71,66,63,69,74,72,72,75,76,91,85,
72,63,78,113,143,153,156,160,161,161,170,176,175,168,180,191,
155,155,157,164,168,159,144,132,127,127,128,127,125,126,123,122,
124,128,126,126,128,130,134,133,132,127,122,120,120,118,118,116,
112,109,106,101,98,96,93,90,101,117,119,117,120,115,119,121,
112,91,72,70,70,62,63,71,78,89,101,117,130,134,162,181,
160,163,166,166,165,160,159,162,163,164,163,161,159,158,158,159,
159,159,162,163,163,164,165,165,163,164,162,161,162,161,160,161,
160,162,161,160,159,158,157,157,157,160,161,162,163,159,155,156,
157,152,148,149,151,150,150,149,146,142,138,136,135,133,133,134,
170,170,165,160,159,161,163,165,166,167,168,167,166,165,164,167,
168,170,174,173,171,173,174,173,172,173,173,173,173,174,175,175,
177,177,179,179,178,176,173,175,173,170,171,174,177,178,176,175,
173,171,170,174,177,178,178,176,176,175,173,174,174,174,175,174,
168,164,159,161,160,160,162,163,161,161,160,161,160,160,161,161,
165,167,171,169,170,170,171,171,170,171,174,172,174,173,174,175,
175,177,180,181,181,178,175,177,177,178,178,179,179,180,178,178,
176,173,173,175,177,178,180,175,178,178,178,179,181,182,183,181,
134,131,132,131,131,130,132,132,131,130,128,127,127,128,129,127,
130,133,133,134,135,137,138,138,139,141,141,142,142,142,142,144,
144,144,147,149,148,148,147,149,150,152,153,153,152,154,153,153,
148,163,152,158,157,158,158,156,159,157,161,156,153,159,150,146,
97,95,95,94,93,92,91,88,83,83,85,85,85,85,83,80,
85,85,85,86,86,84,83,84,84,82,82,83,85,86,86,86,
86,88,94,93,91,91,86,88,96,91,90,93,115,84,86,84,
127,110,125,149,116,111,98,98,104,112,99,99,97,71,60,57,
106,104,104,104,103,103,101,97,94,96,96,97,97,95,93,91,
95,96,96,97,97,96,94,96,94,93,91,93,94,96,94,95,
93,99,104,103,103,102,115,111,100,98,105,110,106,118,108,92,
115,94,82,50,44,48,55,49,49,50,57,50,49,50,51,52,
109,108,110,109,108,107,107,103,102,103,102,103,103,102,100,102,
104,105,103,103,103,102,102,102,101,100,100,102,104,104,105,105,
104,106,114,114,124,114,128,117,110,121,120,116,111,89,78,61,
62,48,43,52,58,56,49,50,52,58,48,54,52,56,55,62,
109,108,107,108,106,107,107,105,103,103,104,104,103,104,101,104,
106,105,103,104,102,102,104,104,103,102,102,105,107,107,106,106,
104,105,98,131,130,110,97,85,82,70,77,69,76,93,72,78,
66,71,92,61,54,49,47,59,63,49,58,53,54,54,60,67,
109,108,107,108,107,107,109,108,106,105,105,105,106,104,104,105,
107,105,103,103,103,103,104,104,103,102,103,103,105,107,106,105,
101,96,113,129,115,94,107,93,96,80,88,69,52,62,64,64,
62,64,78,71,52,66,94,67,56,59,57,53,51,62,64,67,
117,116,116,115,115,115,117,115,112,111,111,111,111,111,110,110,
112,111,108,107,104,103,104,103,105,106,107,109,115,114,113,111,
133,137,145,82,85,85,79,75,88,78,63,64,75,71,55,53,
47,70,92,66,85,86,71,66,58,61,57,59,61,71,78,90,
124,123,123,124,122,123,124,122,119,118,117,118,119,118,115,116,
115,108,103,106,115,125,113,94,100,108,114,117,123,120,122,122,
154,168,155,60,69,75,86,89,74,77,71,91,114,126,121,108,
122,95,65,73,52,60,59,58,53,57,60,61,63,65,58,69,
129,128,128,127,125,125,126,125,121,120,121,121,121,118,124,159,
176,182,197,198,192,200,204,200,175,124,105,111,118,120,133,146,
152,133,76,70,100,105,115,117,104,102,84,87,85,108,120,80,
87,107,99,83,68,62,50,49,55,53,49,49,60,90,104,85,
132,130,131,129,128,127,129,127,125,123,125,125,127,145,157,142,
130,126,138,152,159,169,182,188,195,202,184,179,148,122,129,120,
133,82,84,107,99,68,85,82,122,117,93,81,95,88,104,119,
95,112,101,66,81,66,52,55,60,65,56,52,65,69,69,92,
133,132,132,131,130,128,129,127,127,126,126,126,119,107,108,106,
105,107,113,130,140,152,142,137,156,176,188,175,161,139,120,101,
84,70,71,77,50,52,56,53,53,83,114,106,86,116,106,121,
111,106,87,79,81,89,71,70,74,77,78,56,64,66,80,87,
132,133,133,131,129,129,132,130,128,131,126,113,116,114,115,115,
110,108,110,109,112,126,128,136,129,125,128,116,118,130,133,112,
75,67,76,67,65,51,46,49,69,111,88,98,129,131,108,122,
137,107,115,107,96,89,97,81,92,78,118,95,68,77,81,64,
133,133,133,131,130,131,131,131,137,127,113,115,116,116,116,117,
120,114,115,114,114,129,121,113,130,128,117,120,126,126,103,76,
90,69,87,97,89,60,79,86,50,55,79,157,167,158,166,96,
125,104,124,133,118,120,118,127,113,83,87,123,99,109,83,66,
135,134,133,133,131,130,131,135,117,114,118,119,118,119,121,126,
123,122,123,119,138,130,120,134,127,123,124,125,117,82,74,74,
93,104,113,103,101,56,78,112,93,88,139,180,138,157,86,56,
80,134,104,111,136,126,130,101,106,78,90,101,88,81,81,87,
135,135,134,133,131,132,131,138,121,119,122,117,120,124,130,126,
125,124,124,138,134,127,137,130,124,126,126,126,74,68,79,87,
57,49,71,83,102,116,140,77,51,135,158,119,132,61,52,54,
51,73,99,116,98,90,134,107,91,123,64,49,48,47,46,76,
135,134,134,132,132,132,131,128,122,125,122,119,127,133,136,126,
130,124,143,141,128,140,132,129,126,126,120,84,57,51,49,55,
74,54,48,50,56,89,100,48,97,120,120,101,52,57,58,63,
71,71,62,83,118,130,107,96,64,59,51,47,47,47,53,79,
134,135,133,133,132,131,133,133,131,127,124,128,133,139,134,131,
122,146,145,134,143,136,130,131,129,124,92,69,69,66,70,60,
46,68,53,50,48,42,66,122,175,152,78,64,52,55,55,57,
57,61,63,62,57,60,61,91,60,52,49,45,47,53,85,105,
136,134,134,135,133,132,142,136,130,133,132,136,138,137,134,121,
151,149,139,148,140,133,133,136,126,88,69,50,59,63,51,62,
82,50,50,47,77,112,140,158,134,50,54,52,55,56,54,55,
52,53,54,57,60,67,62,83,67,50,48,58,82,71,111,120,
135,133,133,130,129,130,144,144,135,134,134,139,140,138,121,156,
159,147,151,147,144,142,140,124,111,78,80,68,54,59,61,56,
57,50,61,109,142,179,182,92,46,61,64,66,63,62,57,53,
54,54,54,55,51,53,57,58,54,55,74,98,106,84,120,126,
134,133,133,127,126,132,154,149,141,139,137,142,136,128,145,173,
163,162,157,149,148,148,130,124,75,63,56,68,59,58,68,72,
47,92,143,166,189,139,48,51,55,57,53,57,62,62,62,59,
53,53,57,61,62,57,55,61,77,97,115,121,84,113,128,130,
133,132,133,129,127,136,165,160,154,148,144,144,141,139,181,168,
172,173,165,158,155,135,125,100,68,80,78,67,85,73,60,84,
136,159,178,184,106,73,78,87,96,104,102,93,82,76,62,60,
62,59,58,63,59,62,86,108,119,124,121,88,93,127,131,131,
133,132,132,129,126,141,176,168,170,163,162,158,152,177,179,175,
179,169,168,165,156,128,107,82,54,51,65,94,88,72,105,152,
174,185,182,114,118,122,119,117,119,122,120,111,99,81,68,64,
62,60,69,89,107,109,125,129,127,109,78,103,129,131,133,134,
133,132,131,129,125,138,175,176,178,178,181,172,172,191,182,177,
179,177,166,167,159,127,90,66,85,70,53,52,61,123,156,181,
194,184,128,134,135,135,134,133,132,132,132,132,124,110,88,65,
74,70,65,65,75,82,89,87,85,103,126,137,137,136,135,135,
136,135,136,131,129,126,180,182,182,185,185,173,195,190,182,188,
181,170,172,167,161,141,138,101,75,63,64,73,134,158,168,171,
167,111,138,151,152,147,145,142,140,138,138,136,130,121,110,75,
85,114,111,113,108,113,117,123,132,137,138,140,141,139,139,139,
134,133,134,132,131,126,169,187,188,189,182,195,200,188,193,186,
177,175,173,169,161,135,106,88,94,73,78,121,170,179,177,176,
83,71,132,156,166,161,156,150,148,145,143,142,137,132,129,110,
106,129,125,124,134,134,136,137,141,142,143,141,142,141,140,139,
134,132,133,133,133,129,144,193,191,193,189,206,199,193,194,189,
183,179,172,166,159,153,92,88,73,101,104,143,156,164,179,134,
65,90,124,153,173,174,164,155,152,148,143,144,147,139,137,128,
120,131,130,136,137,139,140,142,143,146,144,143,143,143,143,142,
132,132,131,129,130,129,123,174,195,193,201,203,196,202,197,190,
190,180,178,165,162,153,90,70,88,121,154,180,176,178,132,135,
61,91,144,153,173,175,168,160,156,155,149,125,138,142,141,139,
129,138,139,140,141,140,139,140,141,143,144,145,145,145,144,143,
131,130,131,130,130,131,129,126,189,194,208,202,202,203,199,196,
194,186,181,170,165,153,66,55,138,179,180,182,174,159,138,127,
88,195,167,160,171,168,164,157,156,158,151,129,128,135,145,150,
143,143,141,142,142,143,141,139,139,140,142,143,146,148,146,147,
131,129,129,129,129,129,130,123,141,203,209,206,203,203,200,199,
195,187,179,177,166,143,63,137,176,189,176,162,163,159,157,95,
91,115,148,154,155,152,146,136,139,151,148,133,132,133,150,163,
154,145,142,145,147,148,146,142,141,141,140,141,144,145,148,147,
130,130,131,129,128,129,129,125,113,181,217,210,206,205,201,199,
196,180,184,180,166,150,140,170,183,179,173,173,161,170,175,129,
131,119,128,137,142,143,135,134,128,142,152,132,147,132,162,166,
159,146,152,158,155,152,149,147,146,145,142,141,142,144,147,148,
129,129,130,130,129,129,128,125,117,109,182,220,209,205,205,200,
188,187,188,177,166,159,175,186,181,178,173,166,177,178,176,166,
147,138,135,133,133,133,129,134,116,156,189,139,160,138,158,163,
157,167,173,167,162,158,157,154,152,150,148,143,141,143,146,150,
122,124,125,125,126,125,124,120,117,111,95,149,213,210,207,205,
198,194,184,177,161,179,192,192,181,174,174,187,192,193,186,179,
175,180,188,180,171,162,151,145,127,172,190,146,155,145,149,138,
151,192,188,178,171,168,163,160,158,154,151,149,147,144,147,149,
111,115,118,120,119,119,118,115,113,108,103,89,162,218,212,205,
202,194,180,169,175,190,193,189,183,182,188,197,199,198,192,189,
198,181,177,197,203,207,210,209,180,178,178,134,142,129,85,55,
86,198,194,186,180,177,174,169,166,161,158,156,151,150,150,152,
124,111,109,111,113,114,112,109,108,106,101,95,81,130,204,215,
201,194,182,172,185,195,194,190,188,186,193,198,203,203,197,186,
136,83,133,152,163,160,156,154,149,147,142,133,98,53,48,54,
55,168,201,195,191,187,185,183,176,172,167,162,160,156,154,156,
149,140,127,125,123,121,120,120,119,117,115,114,112,105,95,147,
204,192,175,184,194,189,192,191,193,176,194,201,209,209,205,151,
80,102,137,134,141,145,143,141,137,126,95,54,52,52,56,61,
59,101,205,204,200,199,196,193,190,185,180,174,167,165,162,162,
159,157,153,150,144,141,142,144,146,149,148,148,148,146,145,143,
160,190,189,191,195,193,196,195,164,87,129,155,180,197,195,111,
68,125,99,113,129,132,128,115,85,52,51,53,50,57,59,57,
55,47,154,211,208,207,206,203,199,197,192,188,180,177,171,168,
153,158,162,160,155,149,145,146,151,156,155,155,155,155,154,156,
155,154,191,199,192,197,198,196,110,103,85,66,82,106,106,97,
64,54,69,84,92,84,66,50,51,55,59,64,62,67,72,77,
77,75,63,166,216,214,211,209,206,206,203,199,194,188,182,178,
153,159,161,160,158,154,150,148,146,149,152,152,154,154,154,154,
149,179,196,197,195,201,203,168,119,120,109,83,57,47,46,53,
53,50,54,55,60,59,65,67,69,78,78,80,82,82,85,88,
90,88,81,62,131,200,218,214,211,210,210,207,204,200,194,190,
155,159,162,159,158,156,154,150,143,128,121,126,126,127,127,123,
129,195,202,196,200,202,197,101,154,155,147,146,144,135,123,109,
101,94,83,75,66,62,59,56,59,62,63,66,72,76,77,81,
84,81,76,72,60,47,106,159,190,213,214,212,210,208,204,199,
154,156,159,159,157,155,153,151,147,129,90,60,58,61,60,52,
178,199,200,199,201,205,165,103,167,177,85,60,64,72,93,109,
120,127,136,145,152,153,144,136,129,115,95,80,63,63,63,67,
73,80,92,103,109,118,121,126,132,131,149,183,207,213,209,207,
155,156,156,157,157,155,153,147,146,148,137,110,74,49,46,151,
195,205,198,202,206,193,113,135,181,109,48,51,57,58,63,64,
65,70,72,64,55,59,80,99,106,112,124,136,149,153,159,160,
155,149,146,143,141,144,146,146,142,99,88,100,94,125,159,186,
156,155,154,156,157,155,152,146,144,147,148,146,130,94,122,192,
208,201,202,196,170,137,104,178,126,49,56,58,61,62,65,60,
55,53,52,53,52,56,59,84,111,132,141,145,150,152,150,146,
118,128,143,146,145,146,144,142,142,115,84,103,105,97,91,90,
157,154,152,154,156,153,149,146,146,148,148,147,149,139,199,210,
204,202,194,166,158,122,176,120,50,55,60,62,60,56,53,54,
54,53,59,70,98,119,134,144,149,152,153,154,155,154,155,151,
125,123,138,142,143,143,141,139,141,131,81,89,99,105,106,98,
180,156,151,152,153,149,145,144,144,144,145,145,144,147,208,214,
204,209,206,182,175,187,130,51,56,57,53,50,52,54,53,58,
76,107,125,133,140,152,156,156,154,154,152,150,152,152,152,148,
132,115,133,140,140,139,138,138,140,137,83,68,87,101,115,115,
216,206,174,149,149,146,143,142,143,146,146,146,147,148,207,223,
220,220,212,170,115,45,50,53,53,51,52,52,53,77,110,127,
136,142,149,154,156,155,154,152,150,150,148,147,150,153,151,148,
138,108,124,135,137,133,132,133,133,137,98,47,75,107,118,127,
195,216,216,201,163,144,143,143,145,147,146,149,151,114,58,120,
127,89,48,50,51,52,50,53,55,56,65,97,125,137,137,146,
155,156,154,153,152,152,152,149,149,147,146,147,147,150,150,148,
143,110,111,128,132,129,125,125,131,146,139,139,137,124,115,122,
107,145,202,220,217,195,156,142,146,149,152,138,79,48,52,51,
54,54,53,53,51,56,53,55,75,107,124,133,146,158,160,159,
155,154,151,150,150,149,149,149,147,146,144,145,145,145,145,142,
140,115,99,119,126,125,148,163,150,142,158,187,202,143,105,131,
120,113,114,166,216,223,215,183,150,153,113,54,47,55,57,56,
54,52,50,50,54,58,88,126,144,139,146,151,156,161,159,156,
154,152,150,151,149,146,145,147,145,145,144,143,144,141,139,136,
133,117,115,128,137,178,214,212,192,162,186,201,157,85,109,114,
122,121,117,114,127,185,222,225,203,88,46,52,59,57,57,52,
51,51,54,60,95,133,145,148,157,165,166,159,150,159,159,155,
154,152,153,149,146,145,143,144,142,141,139,138,135,133,128,125,
119,111,97,133,168,204,211,213,214,217,207,169,78,81,91,91,
123,122,122,122,117,110,148,192,105,47,52,57,54,53,55,51,
55,57,82,125,148,148,155,162,163,163,163,163,149,157,162,155,
154,152,151,148,144,142,139,136,134,136,142,146,153,157,160,175,
195,200,197,209,215,216,216,206,188,166,121,72,70,82,95,83,
124,122,123,123,123,119,96,46,46,53,55,51,54,54,55,56,
56,111,144,147,153,158,159,160,162,162,162,161,152,152,159,156,
154,152,150,145,140,135,134,157,178,189,195,200,209,216,218,219,
215,207,209,214,212,202,169,134,118,105,80,78,83,102,80,85,
125,124,125,124,129,99,47,49,54,55,50,55,52,54,51,93,
140,148,150,159,159,157,159,159,160,162,161,161,154,146,153,153,
153,150,145,139,133,150,183,196,197,199,205,211,211,213,215,212,
211,209,207,191,155,113,100,106,102,97,89,90,99,80,76,95,
125,126,129,131,100,49,51,56,56,51,55,51,56,61,118,146,
148,157,159,159,156,155,155,157,158,158,160,161,159,146,148,153,
151,147,140,134,167,193,195,196,202,208,209,209,211,215,214,214,
210,204,157,86,88,97,103,107,99,87,96,97,79,78,103,123,
127,128,135,101,49,52,56,54,51,54,53,61,79,133,142,150,
158,158,157,158,157,155,154,155,155,157,159,158,158,147,145,154,
151,143,136,166,196,194,196,204,206,206,208,210,212,214,212,212,
201,158,74,91,98,111,112,103,93,100,94,85,92,108,113,94,
126,130,102,51,56,58,54,53,52,54,68,93,136,140,154,158,
157,156,156,157,156,155,153,154,154,156,157,157,156,148,142,148,
145,136,145,190,195,196,204,207,208,209,208,211,213,209,212,206,
159,70,79,95,109,101,95,99,102,94,94,102,104,100,84,58,
122,104,51,50,55,53,51,51,54,59,90,141,147,157,159,158,
156,154,155,157,156,155,153,153,153,154,154,156,154,152,141,142,
141,131,170,191,191,200,208,207,209,210,212,213,212,208,209,198,
85,66,89,104,98,99,104,99,94,96,96,87,82,69,61,61,
142,53,51,55,54,50,50,52,48,103,149,146,161,162,157,157,
155,153,154,156,156,153,152,152,152,152,152,152,151,151,143,133,
136,140,183,190,194,204,204,208,207,209,211,213,210,207,209,172,
55,85,89,94,102,102,96,88,94,82,64,62,67,63,59,91};

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

char * bump_head(char * head, char * endofbuf,char * topofbuf) {
 
	if(head == endofbuf) {

		
			//printf("head == endofbuf\n");
			head = topofbuf;
	}
	else {
		//printf("head < endofbuf\n");
		head = head + 1;
	}
 
	
	return((char *)head);
}
char * bump_tail(char * tail,char * endofbuf,char * topofbuf) {
	
	if(tail == endofbuf) {

		
			//printf("tail == endofbuf\n");
			tail = topofbuf;
	}
	else {
		//printf("tail < endofbuf\n");
		tail = tail + 1;
	}
 
	
	return((char *)tail);
}
char * dec_head(char * head,char * endofbuf,char * topofbuf) {
	if(head == topofbuf) {
			//printf("head == topofbuf\n");
			//head = topofbuf;
	}
	else {
		//printf("head < topofbuf\n");
		head = head - 1;
	}

	return((char *)head);
}
char * dec_tail(char * tail,char * endofbuf,char * topofbuf) {
	if(tail == topofbuf) {
			printf("tail == topofbuf\n");
			//head = topofbuf;
	}
	else {
		printf("tail < topofbuf\n");
		tail = tail - 1;
	}

	return((char *)tail); 
}
int read_tt(char * head, char * endofbuf,char * topofbuf) {

	int i,numtoread = 64;
	unsigned char CRC;
	 
	//printf("0x%x 0x%x 0x%x \n",ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
	for(i=0;i<numtoread;i++) {
		
		*ptrs.head = getchar();
	 	ptrs.head = (char *)bump_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
	}
	
	CRC = getCRC(tt,numtoread);
	printf("0x%x\n",CRC);
	//for(i=0;i<numtoread;i++) bump_tail(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
	//for(i=0;i<numtoread;i++) printf("%c",tt[i]);
	
	
	//printf("\n");

	 
	//printf("0x%x 0x%x 0x%x \n",ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
	//printf("CRC = 0x%x\n",CRC);
	
	return(1);
}
unsigned char userInput;

int main() {
	unsigned char recCRC;
	unsigned char message[3] = {0xd3, 0x01, 0x00};
	int flag = 0,numofchars,error=0;
    stdio_init_all();
    int i,j,l,index;
    ptrs.w = 64;
    ptrs.h = 64;
    
    
    ptrs.inp_buf = ptrs.inpbuf;   
    ptrs.head = &tt[0];
	ptrs.tail = &tt[0];
	ptrs.topofbuf = &tt[0];
	
	ptrs.out_buf = ptrs.inpbuf + imgsize;
	ptrs.endofbuf = &tt[128];
	sleep_ms(2000);
	printf("setting pointers\n");
	printf("ptrs.inp_buf = 0x%x ptrs.out_buf = 0x%x\n",ptrs.inpbuf, ptrs.out_buf);
	
	printf("head 0x%x tsil 0x%x end 0x%x top 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
	
	ptrs.fwd_inv =  &ptrs.fwd;
    *ptrs.fwd_inv = 1;
    
    buildCRCTable();
	message[2] = getCRC(message, 2);
	const uint SERIAL_BAUD = 1000000;
    while (true) {
        if (DBUG == 1 ) {
            printf("Hello, world!\n");
            printf("Now copmpiles with lifting code as part of hello_usb.c\n"); 
            printf("structure PTRS added to hello_usb.c\n");
            printf("ptrs.w = %d ptrs.h = %d \n", ptrs.w, ptrs.h);
            printf("These are the variables needed for lifting\n");
            printf("ptrs.inp_buf = 0x%x ptrs.out_buf = 0x%x\n",ptrs.inp_buf, ptrs.out_buf);
            
            printf("w = %d ptrs.fwd_inv = 0x%x ptrs.fwd_inv = %d\n",ptrs.w,ptrs.fwd_inv, *ptrs.fwd_inv);
            printf("head = 0x%x tail = 0x%x end = 0x%x top = 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf); 
            //for(int i=0;i<25;i++) printf("%d ",a[i]);
            //printf("\n");
            printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf); 
            ptrs.head = (char *)bump_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.tail = (char *)bump_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.head = (char *)bump_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.tail = (char *)bump_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.head = (char *)bump_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.tail = (char *)bump_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
			printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf); 
            ptrs.head = (char *)dec_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.tail = (char *)dec_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.head = (char *)dec_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.tail = (char *)dec_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.head = (char *)dec_head(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);
            ptrs.tail = (char *)dec_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
            
            printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf); 

        } 
        if (DBUG1 == 1) {
			printf("Command (1 = Send or 0 = Wait):\n");
			userInput = getchar();
			
			for(i = 0; i < imgsize;i++) ptrs.inp_buf[i] = a[i];
			 
			lifting(ptrs.w,ptrs.inp_buf,ptrs.out_buf,ptrs.fwd_inv);
			
			if(userInput == '1'){
				//for(i=0;i<imgsize;i++) printf("%d ",ptrs.inp_buf[i]);
				index = 0;
				for(j=0;j<64;j++) {
					//for(l=0;l<4;l++) {
					//printf("%d\n",l);
					for(i=0;i<64;i++) {
						printf("%d,",ptrs.inp_buf[index]);
						//printf("%d %d %d\n",i,index,index++);
						index++;
					}
					//index = index + 64;
					printf("\n");
					//}
				}
			}
			
		}
		if (DBUG2 == 1) {
			for (i = 0; i < sizeof(message); i++)
			{
				for (j = 0; j < 8; j++)
				printf("%d", (message[i] >> j) % 2);
				printf(" ");
			}
			printf("\n");
			printf("0x%x 0x%x 0x%x 0x%x\n",*ptrs.head,*ptrs.tail,*ptrs.endofbuf,*ptrs.topofbuf);	
		}
	//printf("read 16 values\n");
	

			//printf("\n");

	
		if (DBUG3 == 1) {
			//printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf); 
			read_tt(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);	
			//printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
			for(i=0;i<32;i++) printf("%d ",tt[i]);
			printf("\n");
			for(i=32;i<64;i++) printf("%d ",tt[i]);
			printf("\n");
			
			//numofchars = ptrs.head -ptrs.tail;
			//printf("%d ", numofchars);
			//printf("0x%x 0x%x 0x%x 0x%x 0x%x \n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf,ptrs.inp_buf);
			for(i=0;i<64;i++) {
				*ptrs.inp_buf = (unsigned short int)*ptrs.tail;
				ptrs.inp_buf++;
				ptrs.tail = (char *)bump_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
			}
			recCRC = getchar();
			printf("recCRC 0x%x ",recCRC);
			
			printf("0x%x 0x%x 0x%x 0x%x 0x%x \n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf,ptrs.inp_buf);
			
		}
		
		if (DBUG4 == 1) {
			/* Reads 4096 values*/
			
			while (ptrs.inp_buf < ptrs.out_buf) {
				//printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf); 
				read_tt(ptrs.head,ptrs.endofbuf,ptrs.topofbuf);	
				//printf("head = 0x%x tail = 0x%x 0x%x 0x%x\n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
				for(i=0;i<32;i++) printf("%d ",tt[i]);
				printf("\n");
				for(i=32;i<64;i++) printf("%d ",tt[i]);
				printf("\n");
			
				//numofchars = ptrs.head -ptrs.tail;
				//printf("%d ", numofchars);
				//printf("0x%x 0x%x 0x%x 0x%x 0x%x \n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf,ptrs.inp_buf);
				for(i=0;i<64;i++) {
					*ptrs.inp_buf = (unsigned short int)*ptrs.tail;
					ptrs.inp_buf++;
					ptrs.tail = (char *)bump_tail(ptrs.tail,ptrs.endofbuf,ptrs.topofbuf);
				}
				recCRC = getchar();
				printf("recCRC 0x%x ",recCRC);
			
				printf("0x%x 0x%x 0x%x 0x%x 0x%x \n",ptrs.head,ptrs.tail,ptrs.endofbuf,ptrs.topofbuf,ptrs.inp_buf);
			}
			ptrs.inp_buf = ptrs.inpbuf;
			for(i = 0; i < imgsize;i++) {
				if (ptrs.inp_buf[i] == a[i]) {
					//printf("matched %d \n",i);
					error = 0;
				} else {
					//printf("error %d\n",i);
					error = 1;
				}
				
			}
			printf("errors %d \n",error); 
			printf("Command (1 = Send or 0 = Wait):\n");
			userInput = getchar();
			 
			lifting(ptrs.w,ptrs.inp_buf,ptrs.out_buf,ptrs.fwd_inv);
			
			if(userInput == '1'){
				//for(i=0;i<imgsize;i++) printf("%d ",ptrs.inp_buf[i]);
				index = 0;
				for(j=0;j<64;j++) {
					//for(l=0;l<4;l++) {
					//printf("%d\n",l);
					for(i=0;i<64;i++) {
						printf("%d,",ptrs.inp_buf[index]);
						//printf("%d %d %d\n",i,index,index++);
						index++;
					}
					//index = index + 64;
					printf("\n");
					//}
				}
			} 
		}
        //sleep_ms(8000);
        sleep_ms(50);
    }
    return 0;
}
