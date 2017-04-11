/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblockswriter.h"
#include "seisblocksdata.h"
#include "survgeom3d.h"
#include "scaler.h"

static const unsigned short cHdrSz = 128;


Seis::Blocks::Writer::Writer( const Survey::Geometry3D* geom )
    : survgeom_(*(geom ? geom : static_cast<const Survey::Geometry3D*>(
				    &Survey::Geometry::default3D())))
    , scaler_(0)
    , needreset_(true)
{
}


Seis::Blocks::Writer::~Writer()
{
    delete scaler_;
}


uiRetVal Seis::Blocks::Writer::reset()
{
    uiRetVal uirv;
    return uirv;
}


uiRetVal Seis::Blocks::Writer::add( const SeisTrc& trc )
{
    uiRetVal uirv;
    if ( needreset_ )
    {
	uirv = reset();
	if ( uirv.isError() )
	    return uirv;
    }

    //TODO implement
    return uirv;
}


void Seis::Blocks::Writer::setBasePath( const File::Path& fp )
{
    if ( fp != basepath_ )
    {
	basepath_ = fp;
	needreset_ = true;
    }
}
