clear
fid = fopen('r.bin','r'); im1 = fread(fid, [2048,inf], 'int32'); fclose(fid);
%fid = fopen('grn.bin','r'); im2 = fread(fid, [256,inf], 'int32'); fclose(fid);
%fid = fopen('blu.bin','r'); im3 = fread(fid, [256,inf], 'int32'); fclose(fid);
fid = fopen('dwt.bin','r'); im4 = fread(fid, [2048,inf], 'int32'); fclose(fid);
figure;
imagesc(im1);
colorbar;
colormap 'gray'
title "r.bin 2048 x 2048 11/06/21"

%figure;
%imagesc(im2);
%colorbar;
%title "grn split from packed rgb catboard 0x0100effc 05/29/19"

%figure;
%imagesc(im3);
%colorbar;
%title "blu split from packed rgb catboard 0x0100effc 05/29/19"

figure;
imagesc(im4);
colorbar;
colormap 'gray'
title "DWT  jpeg.c 4 Lvls 11/06/21 2048 x 2048"

%title "RPi3B fwd lifting step blue sub band ./pi_jpeg 2 1 05/29/19"
%title "FPGA HX8K fwd lifting step green sub band catboard 0x0100f204 05/29/19"
%title "FPGA HX8K fwd lifting step blue sub band catboard 0x0100f204 05/29/19"
%title "simulator fwd lifting step inv red sub band ./jpeg 0 1 05/29/19"
%title "FPGA HX8K fwd lifting step inv lifting step red sub band ./jpeg 0 0 05/29/19"
