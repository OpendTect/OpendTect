/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "emregion.h"

namespace EM
{

Region::Region( Pos::GeomID geomid )
    : geomid_(geomid)
{
    tkzs_.init( false );
}


Region::~Region()
{}


int Region::id() const
{ return id_; }

const TrcKeyZSampling& Region::getBoundingBox() const
{ return tkzs_; }


bool Region::isInside( const TrcKey& tk, float z, bool ) const
{ return tkzs_.hsamp_.includes( tk ) && tkzs_.zsamp_.includes( z, false ); }


void Region::fillPar( IOPar& par ) const
{
}


bool Region::usePar( const IOPar& par )
{
    return true;
}

} // namespace EM
