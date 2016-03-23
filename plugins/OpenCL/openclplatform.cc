/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/

#include "openclplatform.h"

#include "manobjectset.h"
#include "ptrman.h"

#include <OpenCL/cl.h>
#include <vector>


using namespace OpenCL;

#define mMaxNrPlatforms 20
#define mMaxNrDevices 20

#define mGetCLPropertyString( func, id, prop, var ) \
BufferString var; \
if ( func( id, prop, var.minBufSize(), var.getCStr(), NULL ) != CL_SUCCESS ) continue

#define mGetCLProperty( func, id, prop, type, var, failureaction ) \
type var; \
if ( func( id, prop, sizeof(var), &var, NULL )!=CL_SUCCESS ) \
    failureaction

#define mGetCLDeviceProperty( prop, type, var ) \
mGetCLProperty( clGetDeviceInfo, deviceids[idy], prop, type, var, continue )


bool Platform::initClass()
{
    ObjectSet<GPU::Platform> platforms;
    cl_platform_id platformids[mMaxNrPlatforms];
    cl_uint nrplatforms;
    if ( clGetPlatformIDs( mMaxNrPlatforms, platformids, &nrplatforms )!=CL_SUCCESS )
        return true;
    
    for ( int idx=0; idx<nrplatforms; idx++ )
    {
        mGetCLPropertyString( clGetPlatformInfo, platformids[idx], CL_PLATFORM_PROFILE, profile );
        if ( profile!="FULL_PROFILE" )
            continue;
        
        mGetCLPropertyString( clGetPlatformInfo, platformids[idx], CL_PLATFORM_NAME, platformname );
        mGetCLPropertyString( clGetPlatformInfo, platformids[idx], CL_PLATFORM_VENDOR, vendor );
        
        PtrMan<Platform> plf = new OpenCL::Platform;
        
        plf->name_ = platformname;
        plf->vendor_ = vendor;
        plf->platformid_ = platformids[idx];
        
        cl_uint nrdevices;
        cl_device_id deviceids[mMaxNrDevices];
        if ( clGetDeviceIDs( platformids[idx],
                             CL_DEVICE_TYPE_ALL,
                             mMaxNrDevices,
                             deviceids,
                             &nrdevices)!=CL_SUCCESS )
        {
            continue;
        }
        
        for ( int idy=0; idy<nrdevices; idy++ )
        {
            mGetCLDeviceProperty( CL_DEVICE_AVAILABLE, cl_bool, available );
            if ( !available )
                continue;
            
            mGetCLDeviceProperty( CL_DEVICE_COMPILER_AVAILABLE, cl_bool, hascompiler );
            if ( !hascompiler )
                continue;
            
            
            mGetCLDeviceProperty( CL_DEVICE_IMAGE_SUPPORT, cl_bool, hasimagesupport );
            if ( !hasimagesupport )
                continue;
            
            mGetCLDeviceProperty( CL_DEVICE_GLOBAL_MEM_SIZE, cl_ulong, globalmem );
            mGetCLDeviceProperty( CL_DEVICE_MAX_MEM_ALLOC_SIZE, cl_ulong, maxallocmem );
            
            mGetCLDeviceProperty( CL_DEVICE_IMAGE2D_MAX_HEIGHT , size_t, max2dheight );
            mGetCLDeviceProperty( CL_DEVICE_IMAGE2D_MAX_WIDTH, size_t, max2dwidth );
            mGetCLDeviceProperty( CL_DEVICE_IMAGE3D_MAX_DEPTH, size_t, max3ddepth );
            mGetCLDeviceProperty( CL_DEVICE_IMAGE3D_MAX_HEIGHT, size_t, max3dheight );
            mGetCLDeviceProperty( CL_DEVICE_IMAGE3D_MAX_WIDTH, size_t, max3dwidth );
            
            mGetCLDeviceProperty( CL_DEVICE_TYPE, cl_device_type, devicetype );
            
            mGetCLPropertyString( clGetDeviceInfo, deviceids[idy],  CL_DEVICE_NAME, devicename );
            mGetCLPropertyString( clGetDeviceInfo, deviceids[idy],  CL_DEVICE_VENDOR, devicevendor );
            
            PtrMan<OpenCL::Device> device = new OpenCL::Device;
            device->deviceid_ = deviceids[idy];
            device->totalmem_ = globalmem;
            device->maxmemalloc_ = maxallocmem;
            device->max2Dsize_[0] = max2dheight;
            device->max2Dsize_[1] = max2dwidth;
            
            device->max3Dsize_[0] = max3ddepth;
            device->max3Dsize_[1] = max3dheight;
            device->max3Dsize_[2] = max3dwidth;
            device->name_ = BufferString( devicevendor, " ", devicename );
            device->iscpu_ = devicetype==CL_DEVICE_TYPE_CPU;
            
            plf->devices_ += device.set( 0, false );
        }
        
        if ( !plf->devices_.size() )
            continue;
        
         //Take over ptr
        platforms += plf.set( 0, false );
    }
    
    GPU::setPlatforms( platforms );
    
    return true;
}
