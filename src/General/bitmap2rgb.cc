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
	Color col( Color::White() );
	col.setTransparency( 255 );
	array_.clear( col );
    }

    if ( vd && app_.ddpars_.vd_.show_ )
	drawVD( *vd, offs );
    if ( wva && app_.ddpars_.wva_.show_ )
	drawWVA( *wva, offs );
}


class VDImageFiller : public ParallelTask
{
public:
VDImageFiller( const A2DBitMap& bmp, const Geom::Point2D<int>& offs,
	     const ColTab::Sequence& ctab, const ColTab::MapperSetup& su,
	     OD::RGBImage& image )
    : ParallelTask()
    , msu_(su)
    , ctindex_(ctab,maxfill_-minfill_+1)
    , bmp_(bmp)
    , offs_(offs)
    , image_(image)
    , imageinfo_(image.getSize(true),image.getSize(false))
{
    bmpsz_ = Geom::Size2D<int>(bmp.info().getSize(0),bmp.info().getSize(1));
    imagesz_ = imageinfo_.getTotalSz();
    maxcolidx_ = ctindex_.nrCols()-1;
}


~VDImageFiller()
{}


protected:
od_int64 nrIterations() const override
{
    return imagesz_;
}


bool doWork( od_int64 start, od_int64 stop, int ) override
{
    int xy[2];
    for ( auto curpos=start; curpos<=stop; curpos++ )
    {
	const bool res = imageinfo_.getArrayPos( curpos, xy );
	if ( !res || xy[0] >= bmpsz_.width() || xy[1] >= bmpsz_.height() )
	    continue;

	const char bmpval = bmp_.get( xy[0]+offs_.x, xy[1]+offs_.y );
	if ( bmpval == A2DBitMapGenPars::cNoFill() )
	    continue;

	const int idx = sCast(int,bmpval) - minfill_;
	const int colidx = msu_.flipseq_ ? maxcolidx_-idx : idx;
	const Color col = ctindex_.colorForIndex( colidx );
	if ( col.isVisible() )
	    image_.set( xy[0], xy[1], col );
    }

    return true;
}


    const int minfill_ = sCast(int,VDA2DBitMapGenPars::cMinFill());
    const int maxfill_ = sCast(int,VDA2DBitMapGenPars::cMaxFill());

    ColTab::MapperSetup		msu_;
    ColTab::IndexedLookUpTable	ctindex_;

    const A2DBitMap&		bmp_;
    Geom::Size2D<int>		bmpsz_;
    const Geom::Point2D<int>&	offs_;
    OD::RGBImage&		image_;
    Array2DInfoImpl		imageinfo_;

    od_int64			imagesz_;
    int				maxcolidx_;
};


void BitMap2RGB::drawVD( const A2DBitMap& bmp, const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(array_.getSize(true),array_.getSize(false));
    const FlatView::DataDispPars::VD& pars = app_.ddpars_.vd_;
    ColTab::Sequence ctab( pars.ctab_.buf() );

    VDImageFiller filler( bmp, offs, ctab, pars.mappersetup_, array_ );
    filler.execute();
}


class WVAImageFiller : public ParallelTask
{
public:
WVAImageFiller( const A2DBitMap& bmp, const Geom::Point2D<int>& offs,
		const FlatView::DataDispPars::WVA& pars, OD::RGBImage& image )
    : ParallelTask()
    , bmp_(bmp)
    , offs_(offs)
    , image_(image)
    , imageinfo_(image.getSize(true),image.getSize(false))
    , pars_(pars)
{
    bmpsz_ = Geom::Size2D<int>(bmp.info().getSize(0),bmp.info().getSize(1));
    imagesz_ = imageinfo_.getTotalSz();
}


~WVAImageFiller()
{}


protected:
od_int64 nrIterations() const override
{
    return imagesz_;
}


bool doWork( od_int64 start, od_int64 stop, int ) override
{
    int xy[2];
    for ( auto curpos=start; curpos<=stop; curpos++ )
    {
	const bool res = imageinfo_.getArrayPos( curpos, xy );
	if ( !res || xy[0] >= bmpsz_.width() || xy[1] >= bmpsz_.height() )
	    continue;

	const char bmpval = bmp_.get( xy[0]+offs_.x, xy[1]+offs_.y );
	if ( bmpval == A2DBitMapGenPars::cNoFill() )
	    continue;

	Color col( pars_.wigg_ );
	if ( bmpval == WVAA2DBitMapGenPars::cLowFill() )
	    col = pars_.lowfill_;
	else if ( bmpval == WVAA2DBitMapGenPars::cHighFill() )
	    col = pars_.highfill_;
	else if ( bmpval == WVAA2DBitMapGenPars::cRefLineFill() )
	    col = pars_.refline_;

	if ( col.isVisible() )
	    image_.set( xy[0], xy[1], col );
    }

    return true;
}

    const A2DBitMap&		bmp_;
    Geom::Size2D<int>		bmpsz_;
    const Geom::Point2D<int>&	offs_;
    OD::RGBImage&		image_;
    Array2DInfoImpl		imageinfo_;
    const FlatView::DataDispPars::WVA& pars_;

    od_int64			imagesz_;
};


void BitMap2RGB::drawWVA( const A2DBitMap& bmp, const Geom::Point2D<int>& offs )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(array_.getSize(true),array_.getSize(false));
    const FlatView::DataDispPars::WVA& pars = app_.ddpars_.wva_;

    WVAImageFiller filler( bmp, offs, pars, array_ );
    filler.execute();
}
