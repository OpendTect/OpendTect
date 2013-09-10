/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimacinit.h"

#include <QMacStyle>
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>

extern "C"
{

typedef struct CPSProcessSerNum
{
        UInt32          lo;
        UInt32          hi;
} CPSProcessSerNum;

extern OSErr    CPSGetCurrentProcess(CPSProcessSerNum *);
extern OSErr    CPSEnableForegroundOperation( CPSProcessSerNum *,
                                              UInt32, UInt32, UInt32, UInt32);
extern OSErr    CPSSetFrontProcess(CPSProcessSerNum *);
extern OSErr    NativePathNameToFSSpec(char *, FSSpec *, unsigned long);

}

void uiInitMac()
{
    ProcessSerialNumber psn;
    CPSProcessSerNum PSN;

    GetCurrentProcess(&psn);
    if (!CPSGetCurrentProcess(&PSN))
    {
	if (!CPSEnableForegroundOperation(&PSN, 0x03, 0x3C, 0x2C, 0x1103))
	{
	    if (!CPSSetFrontProcess(&PSN))
	    {
		GetCurrentProcess(&psn);
	    }
	}
    }
}

