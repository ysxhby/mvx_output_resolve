#include <linux/module.h>
#include <linux/fb.h>

#include "dbgprint.h"
#include "video.h"
#include "font_inc.c"

#define CHAR_WIDTH        8
#define CHAR_HEIGHT       14
#define CHAR_PIXELS       CHAR_WIDTH * CHAR_HEIGHT
#define CHAR_SIZE         (CHAR_WIDTH * CHAR_HEIGHT / 8)

//#define PRINT_IMAGE 1

PCHAR VideoBuffer_va;
ULONG VideoWidth;
ULONG VideoHeight;
ULONG VideoBitPerPixel;
ULONG VideoPitch;

ULONG StartX = 0;
ULONG StartY = 0;

ULONG GUI_Width = 100;
ULONG GUI_Height = 40;

PUCHAR pVideoBufferBak;
PUCHAR pVideoBufferImage;
PUCHAR pVideoBufferPrint;







ULONG ColorTbl[] = {0x000000,0x000080,0x008000,0x008080,0x800000,0x800080,0x808000,0xC0C0C0,0x808080,0x0000FF,0x00FF00,0x00FFFF,0xFF0000,0xFF00FF,0xFFFF00,0xFFFFFF};

ULONG PrintChar(ULONG x,ULONG y,ULONG ForeColor,ULONG BackColor,ULONG bTransparent,UCHAR CharAscii)
{
    ULONG CharForeColor;
    ULONG CharBackColor;
    register PUCHAR pFontBits;
    ULONG FontMask;
    ULONG x0,y0;
    register ULONG cx,cy;
    register PULONG p;
#ifdef PRINT_IMAGE
    register PULONG p_img;
#endif
    ULONG LinePitch1;

    if(x >= GUI_Width || y >= GUI_Height)
        return 0;

    CharForeColor = ColorTbl[ForeColor];
    CharBackColor = ColorTbl[BackColor];

    pFontBits = &cFontData[CharAscii*CHAR_SIZE];

    x0 = x*CHAR_WIDTH + StartX;
    y0 = y*CHAR_HEIGHT + StartY;

    p = (PULONG)&VideoBuffer_va[(y0 * VideoPitch) + (x0 * 4)];
#ifdef PRINT_IMAGE
    p_img = (PULONG)&pVideoBufferImage[(y0 * VideoPitch) + (x0 * 4)];
#endif
    LinePitch1 = (VideoPitch - (CHAR_WIDTH<<2)) >> 2;
    for(cy = 0; cy < CHAR_HEIGHT; cy++)
    {
        for(cx = 0,FontMask = 0x80; cx < CHAR_WIDTH; cx++)
        {
            if((*pFontBits) & FontMask)
            {
                *p = CharForeColor;
#ifdef PRINT_IMAGE
                *p_img = CharForeColor;
#endif
            }
            else
            {
                if(!bTransparent)
                {
                    *p = CharBackColor;
 #ifdef PRINT_IMAGE
                    *p_img = CharBackColor;
 #endif
                }
            }
            p++;
#ifdef PRINT_IMAGE
            p_img++;
#endif
            FontMask = FontMask >> 1;
        }
        p += LinePitch1;
#ifdef PRINT_IMAGE
        p_img += LinePitch1;
#endif
        pFontBits++;

    }
    return 1;
}

VOID PrintStr(ULONG x,ULONG y,ULONG ForeColor,ULONG BackColor,ULONG bTransparent,PUCHAR String,ULONG FillLine)
{
    ULONG MaxLen = 1024;

    while(*String)
    {
        PrintChar(x,y,ForeColor,BackColor,bTransparent,*String);
        x++;
        if(x >= GUI_Width)
        {
            x = 1;
            y++;
        }
        String++;
        MaxLen--;
        if(!MaxLen)
            break;
    }

    while(FillLine)
    {
        PrintChar(x,y,ForeColor,BackColor,bTransparent,' ');
        x++;
        if(x >= (GUI_Width-1))
            break;
    }
}





VOID VideoInit(void)
{
    struct fb_info *fbi = registered_fb[0];

    VideoBuffer_va = (char *)fbi->screen_base;
    VideoWidth = fbi->var.xres;
    VideoHeight = fbi->var.yres;
    VideoBitPerPixel = fbi->var.bits_per_pixel;
    VideoPitch = fbi->fix.line_length;

    printf("FrameBuffer Virt: 0x%p\r\n",(PVOID)VideoBuffer_va);
    printf("FrameBuffer Phys: 0x%p\r\n",(PVOID)fbi->fix.smem_start);
    printf("Resolution: %d x %d x %d\r\n",VideoWidth,VideoHeight,VideoBitPerPixel);
    printf("Pitch: %d\r\n\r\n",VideoPitch);

    StartX = (VideoWidth - GUI_Width * CHAR_WIDTH) / 2;
    StartY = (VideoHeight - GUI_Height * CHAR_HEIGHT) / 2;


}

VOID VideoRelease(void)
{


}
