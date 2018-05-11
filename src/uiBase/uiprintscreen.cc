/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Asif
 Date:          11/05/2018
________________________________________________________________________

-*/
#include <stdio.h>
#include <Windows.h>
#include <gdiplus.h>
#include <string>
#include <string.h>
#include <tchar.h>
#include <comdef.h>
#include <algorithm>
#pragma comment (lib,"Gdiplus.lib")

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

/*
    OD_Win_GetSnapShotFile:
    1) will trigger print screen key press
    2) will capture data from the clipboard (in this case, an image)
    3) will use GDI+ to save file to PNG
    4) will return file name of the print screen saved to.
*/

std::string OD_Win_GetSnapShotFile( const std::string& reqfnm )
{
    INPUT ip[2] = { 0 };

    ip[0].type = INPUT_KEYBOARD;
    ip[0].ki.wVk = VK_SNAPSHOT;

    ip[1] = ip[0];
    ip[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(2, ip, sizeof(INPUT));  //trigger PRNT-SCRN

    Sleep( 1000 ); // one second wait

    std::string ssfnm;

    if ( !OpenClipboard(NULL) )
	{ ErrMsg( "Cannot open Windows Clipboard" ); return ssfnm; }

    HBITMAP hbm = (HBITMAP)GetClipboardData( CF_BITMAP );

    ULONG_PTR gdiplusToken;
    const Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );

    CLSID myClsId;
    const int clssid = GetEncoderClsid( L"image/png", &myClsId );
    if ( clssid < 0 )
	{ ErrMsg( "No PNG encoder found" ); return ssfnm; }

    ssfnm = reqfnm;
    ssfnm += ".png";

    { //limit scope so that image is destroyed before GDI shutdown

	Gdiplus::Bitmap image(hbm, NULL);
	if ( image.Save(_bstr_t(ssfnm.c_str()),&myClsId) != Gdiplus::Ok )
	{
	    ErrMsg( "GDI+ image cannot be saved" );
	    ssfnm = ""; return ssfnm;
	}
    }

    CloseClipboard();
    DeleteObject( hbm );
    Gdiplus::GdiplusShutdown( gdiplusToken );

    return ssfnm;
}
