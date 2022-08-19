/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bitmap2rgb.h"

#include "array2dbitmapimpl.h"
#include "arrayndimpl.h"
#include "coltabindex.h"
#include "coltabsequence.h"
#include "flatposdata.h"
#include "flatview.h"
#include "histequalizer.h"
#include "odimage.h"


BitMap2RGB::BitMap2RGB( const FlatView::Appearance& a, OD::RGBImage& arr )
    : app_(a)
    , array_(arr)
    , histequalizer_(0)
    , clipperdata_(*new TypeSet<float>())
{
}


BitMap2RGB::~BitMap2RGB()
{
    delete histequalizer_;
    delete &clipperdata_;
}


OD::RGBImage& BitMap2RGB::rgbArray()
{ return array_; }

void BitMap2RGB::setRGBArray( const OD::RGBImage& arr )
{ array_ = arr; }

void BitMap2RGB::setClipperData( const TypeSet<float>& clipdata )
{ clipperdata_ = clipdata; }


void BitMap2RGB::draw( const A2DBitMap* wva, const A2DBitMap* vd,
		       const Geom::Point2D<int>& offs, bool clear )
{
    if ( clear )
    {
	OD::Color col( OD::Color::White() );
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
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(array_.getSize(true),array_.getSize(false));
    const FlatView::DataDispPars::VD& pars = app_.ddpars_.vd_;
    ColTab::Sequence ctab( pars.ctab_.buf() );

    const int minfill = (int)VDA2DBitMapGenPars::cMinFill();
    const int maxfill = (int)VDA2DBitMapGenPars::cMaxFill();
    ColTab::IndexedLookUpTable ctindex( ctab, maxfill-minfill+1 );

    if ( (pars.mappersetup_.type_==ColTab::MapperSetup::HistEq) &&
	 !clipperdata_.isEmpty() )
    {
	TypeSet<float> datapts;
	datapts.setCapacity( mCast(int,bmp.info().getTotalSz()), false );
	for ( int idx=0; idx<bmp.info().getSize(0); idx++ )
	    for ( int idy=0; idy<bmp.info().getSize(1); idy++ )
		datapts += (float)bmp.get( idx, idy );

	delete histequalizer_;
	histequalizer_ = new HistEqualizer( maxfill-minfill+1 );
	histequalizer_->setRawData( datapts );
    }

    const int maxcolidx = ctindex.nrCols()-1;
    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix + offs.x, iy + offs.y );
	    if ( bmpval == A2DBitMapGenPars::cNoFill() )
		continue;

	    const int idx = (int)bmpval-minfill;
	    const int colidx = pars.mappersetup_.flipseq_ ? maxcolidx-idx : idx;
	    const OD::Color col = ctindex.colorForIndex( colidx );
	    if ( col.isVisible() )
		array_.set( ix, iy, col );
	}
    }
}


void BitMap2RGB::drawWVA( const A2DBitMap& bmp, const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(array_.getSize(true),array_.getSize(false));
    const FlatView::DataDispPars::WVA& pars = app_.ddpars_.wva_;

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix+offs.x, iy+offs.y );
	    if ( bmpval == A2DBitMapGenPars::cNoFill() )
		continue;

	    OD::Color col( pars.wigg_ );
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
