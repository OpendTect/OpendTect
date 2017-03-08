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
#include "typeset.h"


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

            plf->devices_ += device.release();
        }

        if ( !plf->devices_.size() )
            continue;

         //Take over ptr
        platforms += plf.release();
    }

    GPU::setPlatforms( platforms );
    GPU::setContextCreatorFunction( Context::createContext );

    return true;
}


GPU::Context* Context::createContext( const TypeSet<int>& deviceids )
{
    const ObjectSet<GPU::Platform>& platforms = GPU::Platform::getPlatforms();

    TypeSet<cl_device_id> devices;

    for ( int idx=0; idx<platforms.size(); idx++ )
    {
        const Platform* platform = (const Platform*) platforms[idx];

        for ( int idy=0; idx<platform->devices_.size(); idy++ )
        {
            const Device* device = (const Device*) platform->devices_[idy];

            if ( deviceids.isPresent( device->id_ ) )
                devices += device->deviceid_;
        }
    }

    if ( !devices.size() )
        return 0;

    return new Context( devices );
}


OpenCL::Context::Context( const TypeSet<cl_device_id>& devices )
{
    cl_int errorcode;
    context_ = clCreateContext( NULL,
                    devices.size(),
                    devices.arr(),
                    pfn_notify,
                    this, &errorcode );

    if ( checkForError( errorcode ) )
    {
        for ( int idx=0; idx<devices.size(); idx++ )
        {
            cl_command_queue queue = clCreateCommandQueue( context_,
                                            devices[idx],
                                            CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                                            &errorcode);
            checkForError( errorcode );
            if ( !checkForError( errorcode ) )
                return;

            if ( !checkForError( clRetainCommandQueue( queue ) ) )
                return;

            queues_ += queue;
        }
    }
}


Context::~Context()
{
    for ( int idx=0; idx<queues_.size(); idx++ )
    {
        clReleaseCommandQueue( queues_[idx] );
    }
}


bool Context::checkForError( cl_int errorcode )
{
    switch ( errorcode )
    {
        case CL_SUCCESS:
            errmsg_.setEmpty();
            break;
        case CL_INVALID_PLATFORM:
            errmsg_ = tr("Invalid platform");
            break;
        case CL_INVALID_VALUE:
            errmsg_ = tr("Invalid value");
            break;
        case CL_INVALID_DEVICE:
            errmsg_ = tr("Invalid device");
            break;
        case CL_DEVICE_NOT_AVAILABLE:
            errmsg_ = tr("Device not available");
            break;
        case CL_OUT_OF_HOST_MEMORY:
            errmsg_ = tr("Out of host memory");
            break;
        case CL_INVALID_CONTEXT:
            errmsg_ = tr("Invalid context");
            break;
        case CL_INVALID_QUEUE_PROPERTIES:
            errmsg_ = tr("Invalid queue properties");
            break;
        default:

            break;

    };

    return isOK();
}

void Context::pfn_notify( const char* errinfo,
                          const void* private_info,
                          size_t cb,
                          void* user_data)
{
    Context* context = (Context*) user_data;
    if ( context )
    {
        Threads::Locker locker( context->contexterrorslock_ );
        context->contexterrors_.add( errinfo );
    }
}

