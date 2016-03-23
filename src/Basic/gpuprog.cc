/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2016
-*/

#include "gpuprog.h"

#include "manobjectset.h"

static ManagedObjectSet<GPU::Platform> platforms;

static GPU::ContextCreateFunction contextCreateFunc = 0;


const ObjectSet<GPU::Platform>& GPU::Platform::getPlatforms()
{ return platforms; }


void GPU::setContextCreatorFunction( GPU::ContextCreateFunction func )
{ contextCreateFunc = func; }


void GPU::setPlatforms( ObjectSet<GPU::Platform>& plfs )
{
    platforms.erase();
    platforms = plfs;
}

