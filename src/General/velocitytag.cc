/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2013
-*/

static const char* rcsID mUsedVar = "$Id: ibmformat.cc 30517 2013-07-02 13:07:14Z bert.bril@dgbes.com $";

#include "velocitytag.h"

#include "veldesc.h"
#include "ioobj.h"
#include "ioman.h"



bool Vel::SetStorageTag( IOObj& ioobj, const VelocityDesc& desc )
{
    desc.fillPar( ioobj.pars() );
    
    return IOM().commitChanges( ioobj );
}


bool Vel::GetStorageTag( const IOObj& ioobj, VelocityDesc& desc )
{
    return desc.usePar( ioobj.pars() );
}


bool Vel::RemoveStorageTag( IOObj& ioobj )
{
    VelocityDesc::removePars( ioobj.pars() );
    return IOM().commitChanges( ioobj );
}
