/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odsysmem.h"

#include "bufstringset.h"
#include "envvars.h"
#include "odmemory.h"
#include "nrbytes2string.h"
#include "thread.h"

#if defined( __lux__ )
# include "file.h"
#elif defined( __mac__ )
# include <mach/mach.h>
# include <mach/mach_init.h>
# include <mach/mach_host.h>
# include <mach/host_info.h>
# include <sys/types.h>
# include <unistd.h>
#endif

#include "iopar.h"
#include "string2.h"
#include <string.h>

#ifdef __mac__
# include <sys/sysctl.h>
#endif


void OD::sysMemCopy( void* dest, const void* org, od_int64 sz )
{
    memcpy( dest, org, (size_t)sz );
}


static inline bool executeNonParallel( od_int64 sz )
{
    // This first check is needed because we cannot use
    // Threads::getNrProcessors() at program startup
    if ( sz < 2*mODMemMinThreadSize )
	return true;

    const int nrproc = Threads::getNrProcessors();
    return nrproc < 4 || sz < (nrproc*mODMemMinThreadSize);
}


void OD::memCopy( void* dest, const void* org, od_int64 sz )
{
    if ( sz <= 0 )
	return;
    else if ( !dest )
	{ pFreeFnErrMsg("dest null"); return; }
    else if ( !org )
	{ pFreeFnErrMsg("org null"); return; }

    if ( executeNonParallel(sz) )
	sysMemCopy( dest, org, (size_t)sz );
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


void OD::memMove( void* dest, const void* org, od_int64 sz )
{
    if ( sz <= 0 )
	return;
    else if ( !dest )
	{ pFreeFnErrMsg("dest null"); return; }
    else if ( !org )
	{ pFreeFnErrMsg("org null"); return; }

    memmove( dest, org, (size_t)sz );
}


void OD::sysMemSet( void* data, int setto, size_t sz )
{
    memset( data, setto, sz );
}


void OD::memSet( void* data, char setto, od_int64 sz )
{
    if ( sz <= 0 )
	return;
    else if ( !data )
	{ pFreeFnErrMsg("data null"); return; }

    if ( executeNonParallel(sz) )
	sysMemSet( data, (int)setto, (size_t)sz );
    else
    {
	MemSetter<char> memsetter( (char*)data, setto, (size_t)sz );
	memsetter.execute();
    }
}


void OD::sysMemZero( void* data, size_t sz )
{
    sysMemSet( data, 0, sz );
}


void OD::memZero( void* data, od_int64 sz )
{
    OD::memSet( data, '\0', sz );
}


void OD::dumpMemInfo( StringPairSet& res )
{
    od_int64 total, free;
    getSystemMemory( total, free );
    NrBytesToStringCreator converter;
    converter.setUnitFrom( total );

    res.add( "Total memory", converter.getString( total ) );
    res.add( "Free memory", converter.getString( free ) );
}


void OD::dumpMemInfo( IOPar& res )
{
    StringPairSet meminfo;
    dumpMemInfo( meminfo );
    for ( const auto* entry : meminfo )
	res.add( entry->first(), entry->second() );
}


#ifdef __lux__
static od_int64 getMemFromStr( char* str, const char* ky )
{
    char* ptr = firstOcc( str, ky );
    if ( !ptr )
	return 0;

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
#if defined(__win__)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    total = status.ullTotalPhys;
    free = status.ullAvailPhys;
#elif defined(__lux__)
    BufferString filecont;
    if ( !File::getContent("/proc/meminfo",filecont) )
	mErrRet

    total = getMemFromStr( filecont.getCStr(), "MemTotal:" );
    free = getMemFromStr( filecont.getCStr(), "MemAvailable:" );
    // Available <= free + cached
    if ( free < 1 )
    {
	free = getMemFromStr( filecont.getCStr(), "MemFree:" ) +
	       getMemFromStr( filecont.getCStr(), "Cached:" );
    }
#elif defined( __mac__ )
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    size_t len = sizeof(total);
    sysctl(mib, 2, &total, &len, nullptr, 0);

    vm_statistics64_data_t vm_info;
    mach_msg_type_number_t info_count;
    info_count = HOST_VM_INFO64_COUNT;
    if ( host_statistics64(mach_host_self(),HOST_VM_INFO64,
			   (host_info64_t)&vm_info,&info_count) )
        mErrRet

    free = (vm_info.free_count + vm_info.inactive_count) * vm_page_size;
    // Available (free + inactive)
#endif
}
