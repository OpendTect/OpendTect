/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: gpucalc.cc,v 1.1 2011-02-04 20:23:42 cvskris Exp $";

#include "gpucalc.h"
#include "OpenCL/cl.h"

namespace GPU
{
struct Platform
{
    				~Platform()
				{
				    clReleaseCommandQueue( queue_ );
				    clReleaseContext( context_ );
				}

    cl_platform_id		id_;
    bool			isgpu_;


    TypeSet<cl_device_id>	cpudevices_;
    bool			runoncpu_;
    TypeSet<cl_device_id>	gpudevices_;
    BoolTypeSet			gpudeviceenabled_;

    cl_context			context_;
    cl_command_queue		queue_;
};


class GPUManagerData
{
public:
    				~GPUManagerData()
				{ deepErase( platforms_ ); }
    ObjectSet<Platform>		platforms_;
};


#define mErrRet 	{ delete data_; data_ = 0;  return; }
#define mGetDevices( list, type ) \
    err = clGetDeviceIDs( NULL, type, \
			  0, NULL, &numdevices ); \
    if ( err!=CL_SUCCESS ) mErrRet; \
 \
    data_->list.setSize( numdevices, cl_device_id() ); \
    err = clGetDeviceIDs( NULL, CL_DEVICE_TYPE_CPU, numdevices, \
	    		data_->list.arr(), NULL ); \
    if ( err!=CL_SUCCESS ) mErrRet
GPUManager()
    : data_( new GPMManagerData )
{
    cl_int err = oclGetPlatformID (cl_platform_id *platforms)

    cl_uint numdevices;
    mGetDevices( cpudevices_, CL_DEVICE_TYPE_CPU );
    mGetDevices( gpudevices_, CL_DEVICE_TYPE_GPU );
};


bool GPUManager::isOK() const
{ return data_; }

}

