/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id";

#include "cuda_runtime.h"
#include "cudafft.h"

#include "plugins.h"
#include "errh.h"

extern "C" int GetGPUCalcPluginType()
{
    return PI_AUTO_INIT_EARLY;
}


extern "C" PluginInfo* GetGPUCalcPluginInfo()
{
    static PluginInfo retpii = {
	"Graphics card based calculations", "dGB (Kristofer Tingdahl)", "=dgb",
	"" };
    return &retpii;
}

int cudadevice = -1;

extern "C" const char* InitGPUCalcPlugin( int, char** )
{
    int nrdevices = 0;
    if ( cudaGetDeviceCount(&nrdevices) )
	nrdevices = 0;

    cudaDeviceProp prop;
    for ( int idx=0; idx<nrdevices; idx++ )
    {
	cudaGetDeviceProperties( &prop, idx );
	if ( prop.major>=1 )
	{
	    cudadevice = idx;
	    break;
	}
    }

    //if ( cudadevice==-1 )
	//return 0;

    CudaFFT::initClass();

    return 0;
}
