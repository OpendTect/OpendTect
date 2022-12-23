/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gendefs.h"


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
