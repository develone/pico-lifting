clear;
close all;
A = imread('lena_rgb_64.pgm');
fid = fopen('r.bin','w');
for i=1:64
	for j=1:64
		fwrite(fid,A(j,i),"uint32");
	end
end
fclose(fid)

