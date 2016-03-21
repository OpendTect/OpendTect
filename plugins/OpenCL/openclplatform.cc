/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/

#include "openclplatform.h"

#include "openclmod.h"
#include "commondefs.h"
#include "bufstring.h"
#include "manobjectset.h"

#include <OpenCL/cl.h>
#include <vector>


using namespace OpenCL;

#define mMaxNrPlatforms 20

static ManagedObjectSet<Platform> platforms;

void Platform::initClass()
{
    cl_platform_id platformids[mMaxNrPlatforms];
    cl_uint nrplatforms;
    if ( clGetPlatformIDs( mMaxNrPlatforms, platformids, &nrplatforms )!=CL_SUCCESS )
        return;
    
    for ( int idx=0; idx<nrplatforms; idx++ )
    {
        BufferString profile, name, vendor;
        if ( clGetPlatformInfo( platformids[idx],
                               CL_PLATFORM_PROFILE,
                               profile.minBufSize(),
                               profile.getCStr(), NULL )!=CL_SUCCESS )
        {
            continue;
        }
        
        if ( profile!="FULL_PROFILE" )
            continue;
        
        if ( clGetPlatformInfo( platformids[idx],
                                CL_PLATFORM_NAME,
                                name.minBufSize(),
                                name.getCStr(), NULL )!=CL_SUCCESS )
        {
            continue;
        }
        
        if ( clGetPlatformInfo( platformids[idx],
                               CL_PLATFORM_VENDOR,
                               vendor.minBufSize(),
                               vendor.getCStr(), NULL )!=CL_SUCCESS )
        {
            continue;
        }
        
        Platform* plf = new Platform;
        
        plf->name_ = name;
        plf->vendor_ = vendor;
        
        platforms += plf;
    }
}
	


