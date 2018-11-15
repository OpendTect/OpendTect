 /*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2007
________________________________________________________________________

-*/

#include "bitmap2rgb.h"

#include "array2dbitmapimpl.h"
#include "arrayndimpl.h"
#include "coltabseqmgr.h"
#include "flatposdata.h"
#include "flatview.h"
#include "odimage.h"


BitMap2RGB::BitMap2RGB( const FlatView::Appearance& a, OD::RGBImage& arr )
    : app_(a)
    , array_(arr)
{
}


BitMap2RGB::~BitMap2RGB()
{
}


OD::RGBImage& BitMap2RGB::rgbArray()
{ return array_; }

void BitMap2RGB::setRGBArray( const OD::RGBImage& arr )
{ array_ = arr; }


void BitMap2RGB::draw( const A2DBitMap* wva, const A2DBitMap* vd,
		       const Geom::Point2D<int>& offs, bool clear )
{
    if ( clear )
    {
	Color col = Color::White();
	col.setTransparency( 255 );
	array_.clear( col );
    }

    if ( vd && app_.ddpars_.vd_.show_ )
	drawVD( *vd, offs );
    if ( wva && app_.ddpars_.wva_.show_ )
	drawWVA( *wva, offs );
}


void BitMap2RGB::drawVD( const A2DBitMap& bmp, const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.getSize(0),bmp.getSize(1));
    const Geom::Size2D<int> arrsz(array_.getSize(true),array_.getSize(false));
    const FlatView::DataDispPars::VD& pars = app_.ddpars_.vd_;
    ConstRefMan<ColTab::Sequence> colseq = ColTab::SeqMGR()
					    .getAny( pars.colseqname_ );
    const int minfill = (int)VDA2DBitMapGenPars::cMinFill();
    const int maxfill = (int)VDA2DBitMapGenPars::cMaxFill();
    const ColTab::Table ctab( *colseq, maxfill-minfill+1, *pars.mapper_ );

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix + offs.x_, iy + offs.y_ );
	    if ( bmpval == A2DBitMapGenPars::cNoFill() )
		continue;

	    const int colidx = (int)bmpval-minfill;
	    const Color col = ctab.color( colidx );
	    if ( col.isVisible() )
		array_.set( ix, iy, col );
	}
    }
}


void BitMap2RGB::drawWVA( const A2DBitMap& bmp, const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.getSize(0),bmp.getSize(1));
    const Geom::Size2D<int> arrsz(array_.getSize(true),array_.getSize(false));
    const FlatView::DataDispPars::WVA& pars = app_.ddpars_.wva_;

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix+offs.x_, iy+offs.y_ );
	    if ( bmpval == A2DBitMapGenPars::cNoFill() )
		continue;

	    Color col( pars.wigg_ );
	    if ( bmpval == WVAA2DBitMapGenPars::cLowFill() )
		col = pars.lowfill_;
	    else if ( bmpval == WVAA2DBitMapGenPars::cHighFill() )
		col = pars.highfill_;
	    else if ( bmpval == WVAA2DBitMapGenPars::cRefLineFill() )
		col = pars.refline_;

	    if ( col.isVisible() )
		array_.set( ix, iy, col );
	}
    }
}
