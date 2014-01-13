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
#include "nrbytes2string.h"
#include "thread.h"

#ifdef __lux__
# include "od_istream.h"
# include "strmoper.h"
# include <fstream>
static od_int64 swapfree;
#endif

#ifdef __mac__
# include <unistd.h>
# include <mach/mach_init.h>
# include <mach/mach_host.h>
# include <mach/host_info.h>
#endif

#include "iopar.h"
#include "string2.h"
#include <string.h>


void OD::sysMemCopy( void* dest, const void* org, od_int64 sz )
{
    memcpy( dest, org, (size_t)sz );
}

#define mExecNonParallel(sz) (sz < 1000000 || Threads::getNrProcessors() < 4)


void OD::memCopy( void* dest, const void* org, od_int64 sz )
{
    if ( sz <= 0 )
	return;
    else if ( !dest )
	{ pFreeFnErrMsg("dest null","OD::memCopy"); return; }
    else if ( !org )
	{ pFreeFnErrMsg("org null","OD::memCopy"); return; }

    if ( mExecNonParallel(sz) )
	memcpy( dest, org, (size_t)sz );
    else
    {
	if ( sz % 8 == 0 )
	{
	    MemCopier<od_int64> mcp( (od_int64*)dest, (const od_int64*)org,
		    			(size_t)(sz/8) );
	    mcp.execute();
	}
	else if ( sz % 4 == 0 )
	{
	    MemCopier<int> mcp( (int*)dest, (const int*)org, (size_t)(sz/4) );
	    mcp.execute();
	}
	else
	{
	    MemCopier<char> mcp( (char*)dest, (const char*)org, (size_t)sz );
	    mcp.execute();
	}
    }
}


void OD::memSet( void* data, char setto, od_int64 sz )
{
    if ( sz <= 0 )
	return;
    else if ( !data )
	{ pFreeFnErrMsg("data null","OD::memSet"); return; }

    if ( mExecNonParallel(sz) )
	memset( data, (int)setto, (size_t)sz );
    else
    {
	MemSetter<char> msetter( (char*)data, setto, (size_t)sz );
	msetter.execute();
    }
}


void OD::memZero( void* data, od_int64 sz )
{
    OD::memSet( data, '\0', sz );
}


void OD::dumpMemInfo( IOPar& res )
{
    od_int64 total, free;
    getSystemMemory( total, free );
    NrBytesToStringCreator converter;

    converter.setUnitFrom( total );

    res.set( "Total memory", converter.getString(total) );
    res.set( "Free memory", converter.getString( free ) );
#ifdef __lux__
    res.set( "Available swap space", converter.getString(swapfree) );
#endif
}


#ifdef __lux__
static od_int64 getMemFromStr( char* str, const char* ky )
{
    char* ptr = firstOcc( str, ky );
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

    od_istream strm( "/proc/meminfo" );
    BufferString filecont;

    if ( !strm.getAll(filecont) )
	mErrRet

    total = getMemFromStr( filecont.getCStr(), "MemTotal:" );
    free = getMemFromStr( filecont.getCStr(), "MemFree:" );
    free += getMemFromStr( filecont.getCStr(), "Cached:" );
    swapfree = getMemFromStr( filecont.getCStr(), "SwapFree:" );

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
