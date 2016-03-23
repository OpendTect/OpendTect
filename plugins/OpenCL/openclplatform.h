#ifndef openclplatform_h
#define openclplatform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jul 2007
________________________________________________________________________

-*/

#include "openclmod.h"

#include "gpuprog.h"
#include <OpenCL/cl.h>


namespace OpenCL
{
    mExpClass(OpenCL) Device : public GPU::Device
    {
    public:
        cl_device_id			deviceid_;
    };
    
    
    mExpClass(OpenCL) Platform : public GPU::Platform
    {
    public:
        static bool			initClass();
        
        cl_platform_id 			platformid_;
    };

}  //namespace


#endif
