/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "basemapzvalues.h"

#include "iopar.h"

namespace Basemap
{

ZValueMgr& ZValues()
{
    mDefineStaticLocalObject( PtrMan<ZValueMgr>, zvm, (new ZValueMgr) );
    return *zvm;
}


ZValueMgr::ZValueMgr()
    : iopar_(*new IOPar)
{
}


ZValueMgr::~ZValueMgr()
{
    delete &iopar_;
}


void ZValueMgr::set( const char* key, int zval )
{
    const int idx = keys_.indexOf( key );
    if ( zvals_.validIdx(idx) )
	zvals_[idx] = zval;
    else
    {
	keys_.add( key );
	zvals_.add( zval );
    }
}


int ZValueMgr::get( const char* key ) const
{
    const int idx = keys_.indexOf( key );
    return zvals_.validIdx(idx) ? zvals_[idx] : 0;
}


} // namespace Basemap
