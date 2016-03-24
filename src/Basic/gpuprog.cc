/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2016
-*/

#include "gpuprog.h"

#include "manobjectset.h"
#include "ptrman.h"

static ManagedObjectSet<GPU::Platform> platforms;

static GPU::ContextCreateFunction contextCreateFunc = 0;

static Threads::Atomic<int> nextid;

GPU::Device::Device()
    : id_( nextid++ )
    , totalmem_( 0 )
    , maxmemalloc_( 0 )
    , iscpu_( false )
{}


const ObjectSet<GPU::Platform>& GPU::Platform::getPlatforms()
{ return platforms; }


void GPU::setContextCreatorFunction( GPU::ContextCreateFunction func )
{ contextCreateFunc = func; }


void GPU::setPlatforms( ObjectSet<GPU::Platform>& plfs )
{
    platforms.erase();
    platforms = plfs;
}

RefMan<GPU::Context> GPU::Context::createContext(const TypeSet<int>& deviceids)
{
    if ( !contextCreateFunc )
        return 0;
    
    return contextCreateFunc(deviceids);
}
