/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : December 2018
-*/

#include "giswriter.h"
#include "survinfo.h"

mImplClassFactory(GISWriter, factory);

GISWriter::GISWriter()
    : inpcrs_(SI().getCoordSystem())
    , coordsys_(SI().getCoordSystem())
{}


void GISWriter::setProperties(const Property& properties)
{
    properties_ = properties;
    ispropset_ = true;
}

#define isCoordSysSame \
    *coordsys_ == *inpcrs_ \


void GISWriter::coordConverter( TypeSet<Coord3d>& crdset )
{
    if ( isCoordSysSame )
	return;
    for ( int idx=0; idx<crdset.size(); idx++ )
	coordsys_->convertFrom( crdset[idx].getXY(), *inpcrs_ );
}


void GISWriter::coordConverter( TypeSet<Coord>& crdset )
{
    if ( isCoordSysSame )
	return;
    for (int idx = 0; idx < crdset.size(); idx++)
	coordsys_->convertFrom( crdset[idx], *inpcrs_ );
}
