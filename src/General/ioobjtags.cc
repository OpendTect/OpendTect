/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "ioobjtags.h"

#include "veldesc.h"
#include "ioobj.h"
#include "ioman.h"
#include "zdomain.h"



bool SetVelocityTag( IOObj& ioobj, const VelocityDesc& desc )
{
    desc.fillPar( ioobj.pars() );
    
    return IOM().commitChanges( ioobj );
}


bool GetVelocityTag( const IOObj& ioobj, VelocityDesc& desc )
{
    return desc.usePar( ioobj.pars() );
}


bool RemoveVelocityTag( IOObj& ioobj )
{
    VelocityDesc::removePars( ioobj.pars() );
    return IOM().commitChanges( ioobj );
}


bool SetVelocityVolumeTag( IOObj& ioobj, const MultiID& velvol )
{
    ioobj.pars().set( VelocityDesc::sKeyVelocityVolume(), velvol );
    return IOM().commitChanges( ioobj );
}


bool GetVelocityVolumeTag( const IOObj& ioobj, MultiID& velvol )
{
    return ioobj.pars().get( VelocityDesc::sKeyVelocityVolume(), velvol );
}


bool RemoveVelocityVolumeTag( IOObj& ioobj )
{
    if ( !ioobj.pars().hasKey( VelocityDesc::sKeyVelocityVolume() ) )
	return true;

    ioobj.pars().remove( VelocityDesc::sKeyVelocityVolume() );
    return IOM().commitChanges( ioobj );
}


bool SetZDomainTag( IOObj& ioobj, const char* zdomain ) 
{
    ioobj.pars().set( ZDomain::sKey(), zdomain );
    return IOM().commitChanges( ioobj );
}


bool SetDepthTag( IOObj& ioobj, const MultiID* velocity )
{
    if ( velocity && !SetVelocityVolumeTag( ioobj, *velocity ) )
	return false;

    return SetZDomainTag( ioobj, ZDomain::sKeyDepth() );
}


bool SetTimeTag( IOObj& ioobj, const MultiID* velocity )
{
    if ( velocity && !SetVelocityVolumeTag( ioobj, *velocity ) )
	return false;

    return SetZDomainTag( ioobj, ZDomain::sKeyDepth() );
}


bool GetZDomainTag( const IOObj& ioobj, BufferString& res )
{
    return ioobj.pars().get( ZDomain::sKey(), res );
}


bool RemoveZDomainTag( IOObj& ioobj, BufferString& res )
{
    if ( !ioobj.pars().hasKey( ZDomain::sKey() ) )
	return true;

    ioobj.pars().remove( ZDomain::sKey() );
    return IOM().commitChanges( ioobj );
}
