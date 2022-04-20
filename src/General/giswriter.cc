/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : March 2021
-*/

#include "giswriter.h"
#include "survinfo.h"

mImplFactory(GISWriter,GISWriter::factory);

GISWriter::GISWriter()
    : coordsys_(SI().getCoordSystem())
    , inpcrs_(SI().getCoordSystem())
{
}


GISWriter::Property::Property()
{
}


GISWriter::Property::~Property()
{
}


GISWriter::Property& GISWriter::Property::operator =( const Property& oth )
{
    if ( &oth == this )
	return *this;

    color_	= oth.color_;
    width_	= oth.width_;
    iconnm_	= oth.iconnm_;
    stlnm_	= oth.stlnm_;
    xpixoffs_	= oth.xpixoffs_;
    objnm_	= oth.objnm_;
    coordysynm_ = oth.coordysynm_;
    nmkeystr_	= oth.nmkeystr_;

    return *this;
}


GISWriter::~GISWriter()
{
    close();
}


bool GISWriter::close()
{
    if ( strm_ )
	strm_->close();

    return true;
}


void GISWriter::coordConverter( TypeSet<Coord3>& crdset )
{
    if ( *coordsys_ == *inpcrs_ )
	return;

    for ( int idx=0; idx<crdset.size(); idx++ )
	coordsys_->convertFrom( crdset[idx].coord(), *inpcrs_ );
}


void GISWriter::coordConverter( TypeSet<Coord>& crdset )
{
    if ( *coordsys_ == *inpcrs_ )
	return;

    for ( int idx=0; idx<crdset.size(); idx++ )
	coordsys_->convertFrom( crdset[idx], *inpcrs_ );
}


uiString GISWriter::successMsg()
{
    return tr("Successfully created %1. Do you want to create more?")
						.arg( factoryDisplayName() );
}


uiString GISWriter::errMsg()
{
    return tr("Failed to create %1").arg( factoryDisplayName() );
}


void GISWriter::setProperties( const GISWriter::Property& props )
{
    properties_ = props;
}
