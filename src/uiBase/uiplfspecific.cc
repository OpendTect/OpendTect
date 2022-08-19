/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"


//--- OD_Win_GetSnapShotFile() ---

#ifdef __win__

# include <minmax.h>
# include <stdio.h>
# include <Windows.h>
# include <gdiplus.h>
# include <string>
# include <string.h>
# include <tchar.h>
# include <comdef.h>
# include <algorithm>
# pragma comment (lib,"Gdiplus.lib")
# include "errmsg.h"

using namespace Gdiplus;


static int GetEncoderClsid( const WCHAR* format, CLSID* pClsid )
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize( &num, &size );
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc( size ));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }
   }
   free(pImageCodecInfo);
   return -1;  // Failure
}


static void triggerKeyboardToDoPrScreen()
{
    INPUT ip[2] = { 0 };

    ip[0].type = INPUT_KEYBOARD;
    ip[0].ki.wVk = VK_SNAPSHOT;

    ip[1] = ip[0];
    ip[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(2, ip, sizeof(INPUT));  //trigger PRNT-SCRN

    Sleep( 1000 ); // one second wait to enable clipboard to fill
}


/*
    OD_Win_GetSnapShotFile:
    1) will trigger print screen key press
    2) will capture data from the clipboard (in this case, an image)
    3) will use GDI+ to save file to PNG
    4) will return file name of the print screen saved to.
*/


std::string OD_Win_GetSnapShotFile( const std::string& reqfnm )
{
    triggerKeyboardToDoPrScreen();

    std::string ssfnm;

#define mRetSimp(s) \
    { \
	if ( s && *s ) \
	    { ErrMsg( s ); ssfnm = ""; } \
	return ssfnm;\
    }\


    if ( !OpenClipboard(NULL) )
	mRetSimp( "Cannot open Windows Clipboard" )

#define mRetErrCloseClipBoard(s) { CloseClipboard(); mRetSimp(s); }

    HBITMAP hbm = (HBITMAP)GetClipboardData( CF_BITMAP );
    if ( !hbm )
	mRetErrCloseClipBoard( "Cannot get Windows clipboard data" )

#define mRetErrDelBMAndCloseClipboard(s) \
    { DeleteObject( hbm ); mRetErrCloseClipBoard(s); }

    ULONG_PTR gdiplusToken;
    const Gdiplus::GdiplusStartupInput gdistart;
    if ( Gdiplus::GdiplusStartup(&gdiplusToken,&gdistart,
				    NULL) != Gdiplus::Ok )
	mRetErrDelBMAndCloseClipboard( "Cannot start GDI+" )

#define mRetWithMsg(s) \
	{ Gdiplus::GdiplusShutdown( gdiplusToken ); \
	    mRetErrDelBMAndCloseClipboard(s); }

    CLSID myClsId;
    const int retval = GetEncoderClsid( L"image/png", &myClsId );
    if ( retval < 0 )
	mRetWithMsg( "No PNG encoder found" )

    ssfnm = reqfnm;
    ssfnm += ".png";

    { //limit scope so that image is destroyed before GDI shutdown

	Gdiplus::Bitmap image(hbm, NULL);
	if ( image.Save(_bstr_t(ssfnm.c_str()),&myClsId) != Gdiplus::Ok )
	    mRetWithMsg( "GDI+ image cannot be saved to file" )
    }

    mRetWithMsg( "" ) // no message; still close everything and return ssfnm
}

#endif // __win__


//--- OD::uiInitProcessStatus() ---

#ifdef __mac__

# include "errmsg.h"
# include "uiprocessinit.h"

# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>

extern "C"
{

typedef struct CPSProcessSerNum { UInt32 lo, hi; } CPSProcessSerNum;

OSErr CPSGetCurrentProcess(CPSProcessSerNum*);
OSErr CPSEnableForegroundOperation(CPSProcessSerNum*,
				   UInt32,UInt32,UInt32,UInt32);
OSErr CPSSetFrontProcess(CPSProcessSerNum*);

}

namespace OD
{
    void uiInitProcessStatus()
    {
	CPSProcessSerNum PSN;

	if ( !CPSGetCurrentProcess(&PSN)
	  || !CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103)
	  || !CPSSetFrontProcess(&PSN) )
	    { pFreeFnErrMsg("[MAC ONLY] Could not set front process"); }
    }
}

#else // (not __mac__)

namespace OD
{
    void uiInitProcessStatus() {}
}

#endif // __mac__
