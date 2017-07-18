#pragma once

#define InitRGBQUAD(rgbquad) \
    rgbquad.rgbBlue = 0; \
    rgbquad.rgbGreen = 0; \
    rgbquad.rgbRed = 0; \
    rgbquad.rgbReserved = 0;
    
#define SplitRGB(clr, red, green, blue) \
    blue = clr & 0x000000FF; \
    green = (clr & 0x0000FF00) >> 8; \
    red = (clr & 0x00FF0000) >> 16;

#define CombineRGB(red, green, blue) \
    (((red) << 16) | ((green) << 8) | (blue))

#define IsRGBSimilar(clr1, clr2) \
    ( (((clr1) & 0x00F80000) == ((clr2) & 0x00F80000)) && (((clr1) & 0x0000F800) == ((clr2) & 0x0000F800)) && (((clr1) & 0x000000F8) == ((clr2) & 0x000000F8)))
