/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "odsysmem.h"
#include "odmemory.h"

#ifdef __lux__
# include "strmoper.h" 
# include <fstream>
static float swapfree;
#endif

#ifdef __mac__
# include <unistd.h>
# include <mach/mach_init.h>
# include <mach/mach_host.h>
# include <mach/host_info.h>
#endif

#include "iopar.h" 
#include "string2.h" 


void OD::dumpMemInfo( IOPar& res )
{
    od_int64 total, free;
    getSystemMemory( total, free );
    NrBytesToStringCreator converter;
    
    total /= 1024 * 1024; free /= 1024 * 1024;

    res.set( "Total memory (MB)", total );
    res.set( "Free memory (MB)", free );
#ifdef __lux__
    free = swapfree; free /= 1024 * 1024; ifree = mNINT32(free);
    res.set( "Available swap space (MB)", ifree );
#endif
}


#ifdef __lux__
static od_int64 getMemFromStr( char* str, const char* ky )
{
    char* ptr = strstr( str, ky );
    if ( !ptr ) return 0;
    ptr += strlen( ky );
    mSkipBlanks(ptr);
    char* endptr = ptr; mSkipNonBlanks(endptr);
    *endptr = '\0';
    float ret = toFloat( ptr );
    *endptr = '\n';
    ptr = endptr;
    mSkipBlanks(ptr);
    const od_int64 fac = tolower(*ptr) == 'k' ? 1024
		   : (tolower(*ptr) == 'm' ? 1024*1024
		   : (tolower(*ptr) == 'g' ? 1024*1024*1024 : 1) );
    return mNINT64(ret * fac);
}
#endif


#define mErrRet { total = free = 0; return; }

void OD::getSystemMemory( od_int64& total, od_int64& free )
{
#ifdef __lux__

    std::ifstream strm( "/proc/meminfo" );
    BufferString filecont;
    if ( !StrmOper::readFile(strm,filecont) )
	mErrRet

    total = getMemFromStr( filecont.buf(), "MemTotal:" );
    free = getMemFromStr( filecont.buf(), "MemFree:" );
    free += getMemFromStr( filecont.buf(), "Cached:" );
    swapfree = getMemFromStr( filecont.buf(), "SwapFree:" );

#endif
#ifdef __mac__
    vm_statistics_data_t vm_info;
    mach_msg_type_number_t info_count;

    info_count = HOST_VM_INFO_COUNT;
    if ( host_statistics(mach_host_self(),HOST_VM_INFO,
		(host_info_t)&vm_info,&info_count) )
	mErrRet

    total = (vm_info.active_count + vm_info.inactive_count +
	    vm_info.free_count + vm_info.wire_count) * vm_page_size;
    free = vm_info.free_count * vm_page_size;

#endif
#ifdef __win__
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    total = status.ullTotalPhys;
    free = status.ullAvailPhys;
#endif
}
