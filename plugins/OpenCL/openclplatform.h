#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jul 2007
________________________________________________________________________

-*/

#include "openclmod.h"

#include "gpuprog.h"
#include "bufstringset.h"
#include "typeset.h"

#define CL_USE_DEPRECATED_OPENCL_2_0_APIS

#ifdef __mac__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


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

        cl_platform_id			platformid_;
    };

    mExpClass(OpenCL) Context : public GPU::Context
    { mODTextTranslationClass(Context);
    public:
        static GPU::Context*		createContext(const TypeSet<int>& deviceids);
				Context(const TypeSet<cl_device_id>&);
                                        ~Context();

        cl_context			context_;
        TypeSet<cl_command_queue>	queues_;

        BufferStringSet			contexterrors_;
        Threads::Lock			contexterrorslock_;

        bool				checkForError(cl_int);
				//!<Sets error message

        static void			pfn_notify( const char* errinfo,
                                            const void* private_info,
                                            size_t cb,
                                            void* user_data);
				//!<Callback on errors in context
    };

}  //namespace
