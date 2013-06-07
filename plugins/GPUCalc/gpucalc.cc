/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gpucalc.cc,v 1.3 2011/02/09 16:54:53 cvskarthika Exp $";

#include "gpucalc.h"

#include "bufstringset.h"
#include "staticstring.h"
#include "varlenarray.h"

#ifdef __APPLE__
#include "OpenCL/cl.h"
#else
#include "CL/cl.h"
#endif

namespace GPU
{

class Context
{
public:
			Context( cl_context context )
			    : context_( context )			{}
			~Context()
			{ clReleaseContext( context_ ); }

    cl_context		context_;
};


class DeviceData
{
public:
    				~DeviceData()
				{
				    clReleaseCommandQueue( queue_ );
				}

    cl_device_id		deviceid_;
    cl_command_queue		queue_;
    cl_context			context_;

    bool			isgpu_;
    bool			enabled_;
};


Device::Device()
    : data_( *new DeviceData )
{}


Device::~Device()
{ delete &data_; }


bool Device::isGPU() const
{ return data_.isgpu_; }


const char* Device::name() const
{
    BufferString& buf = StaticStringManager::STM().getString();
    size_t actualsize;

    cl_int err = clGetDeviceInfo( data_.deviceid_, CL_DEVICE_NAME,
				  buf.minBufSize(), buf.str(), &actualsize );
    if ( err!=CL_SUCCESS )
	return 0;

    if ( actualsize==buf.minBufSize() )
	buf[actualsize-1] = 0;

    return buf;
}


void* Device::getContext() { return data_.context_; }


void* Device::getDevice() { return data_.deviceid_; }



class ProgramData
{
public:
    cl_program		program_;
    cl_kernel		kernel_;
    Device*		device_;
};


Program::Program( Device& d )
    : data_( *new ProgramData )
{ data_.device_ = &d; }


Program::~Program()
{
    clReleaseKernel( data_.kernel_ );
    clReleaseProgram( data_.program_ );
    delete &data_;
}


bool Program::setSource( const BufferStringSet& strs, const char* kernelname )
{
    mAllocVarLenArr( const char*, sources, strs.size() );
    mAllocVarLenArr( size_t, lengths, strs.size() );
    for ( int idx=0; idx<strs.size(); idx++ )
    {
	sources[idx] = strs[idx]->buf();
	lengths[idx] = strs[idx]->size();
    }

    cl_int err;
    data_.program_ = clCreateProgramWithSource (
	(cl_context) data_.device_->getContext(), strs.size(), sources, lengths,
	&err );

    if ( err!=CL_SUCCESS )
	return false;

    cl_device_id device = (cl_device_id) data_.device_->getDevice();
    err = clBuildProgram( data_.program_, 1, &device, NULL, NULL, NULL);
    if ( err!=CL_SUCCESS )
	return false;

    data_.kernel_ = clCreateKernel( data_.program_, kernelname, &err );
    if ( err!=CL_SUCCESS )
	return false;

    return true;
}





#define mGetDevices( isgpu, type ) \
    err = clGetDeviceIDs( platformids[idx], type, 0, NULL, &numdevices ); \
    if ( err!=CL_SUCCESS || !numdevices ) { continue; } \
 \
    { \
	mAllocVarLenArr( cl_device_id, devices, numdevices ); \
	err = clGetDeviceIDs( platformids[idx], type, numdevices, \
				devices, NULL ); \
	if ( err!=CL_SUCCESS ) { continue; } \
	context = clCreateContext( 0, numdevices, devices, NULL, NULL, &err); \
	if ( err!=CL_SUCCESS ) { continue; } \
	contexts_ += new Context( context ); \
     \
	for ( int idy=0; idy<numdevices; idy++ ) \
	{ \
	    cl_command_queue queue; \
	    queue = clCreateCommandQueue( context, devices[idy], 0, &err ); \
	    if ( err!=CL_SUCCESS ) { continue; } \
     \
	    Device* device = new Device; \
	    device->data_.deviceid_ = devices[idx]; \
	    device->data_.context_ = context; \
	    device->data_.queue_ = queue; \
	    device->data_.isgpu_ = isgpu; \
	    devices_ += device; \
	}  \
    }

GPUManager::GPUManager()
{
    cl_platform_id platformids[200];
    cl_uint actualnrplatforms;
    cl_int err = clGetPlatformIDs( 200, platformids, &actualnrplatforms );
    if ( err!=CL_SUCCESS )
	return;

    for ( int idx=0; idx<actualnrplatforms; idx++ )
    {
	cl_uint numdevices;
	cl_context context;
	mGetDevices( false, CL_DEVICE_TYPE_CPU );
	mGetDevices( true, CL_DEVICE_TYPE_GPU );
    }

}


GPUManager::~GPUManager()
{
    for ( int idx=0; idx<devices_.size(); idx++ )
	delete devices_[idx];

    deepErase( contexts_ );
}


GPUManager& manager()
{
    static GPUManager manager;
    return manager;
}

}; //namespace
