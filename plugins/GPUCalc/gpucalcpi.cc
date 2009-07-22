/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: gpucalcpi.cc,v 1.3 2009-07-22 16:01:27 cvsbert Exp $";

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
