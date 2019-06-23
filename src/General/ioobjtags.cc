/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2013
-*/


#include "ioobjtags.h"

#include "veldesc.h"
#include "ioobj.h"
#include "zdomain.h"



bool SetVelocityTag( IOObj& ioobj, const VelocityDesc& desc )
{
    desc.fillPar( ioobj.pars() );
    return ioobj.commitChanges().isOK();
}


bool GetVelocityTag( const IOObj& ioobj, VelocityDesc& desc )
{
    return desc.usePar( ioobj.pars() );
}


bool RemoveVelocityTag( IOObj& ioobj )
{
    VelocityDesc::removePars( ioobj.pars() );
    return ioobj.commitChanges().isOK();
}


bool SetVelocityVolumeTag( IOObj& ioobj, const DBKey& velvol )
{
    ioobj.pars().set( VelocityDesc::sKeyVelocityVolume(), velvol );
    return ioobj.commitChanges().isOK();
}


bool GetVelocityVolumeTag( const IOObj& ioobj, DBKey& velvol )
{
    return ioobj.pars().get( VelocityDesc::sKeyVelocityVolume(), velvol );
}


bool RemoveVelocityVolumeTag( IOObj& ioobj )
{
    if ( !ioobj.pars().hasKey( VelocityDesc::sKeyVelocityVolume() ) )
	return true;

    ioobj.pars().removeWithKey( VelocityDesc::sKeyVelocityVolume() );
    return ioobj.commitChanges().isOK();
}


bool SetZDomainTag( IOObj& ioobj, const char* zdomain )
{
    ioobj.pars().set( ZDomain::sKey(), zdomain );
    return ioobj.commitChanges().isOK();
}


bool SetDepthTag( IOObj& ioobj, const DBKey* velocity )
{
    if ( velocity && !SetVelocityVolumeTag( ioobj, *velocity ) )
	return false;

    return SetZDomainTag( ioobj, ZDomain::sKeyDepth() );
}


bool SetTimeTag( IOObj& ioobj, const DBKey* velocity )
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

    ioobj.pars().removeWithKey( ZDomain::sKey() );
    return ioobj.commitChanges().isOK();
}
