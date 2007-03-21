/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:		Feb 2007
 RCS:           $Id: flatviewbitmap.cc,v 1.6 2007-03-21 08:36:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatviewbitmapmgr.h"
#include "flatviewbmp2rgb.h"
#include "array2dbitmapimpl.h"
#include "arrayndimpl.h"
#include "uirgbarray.h"
#include "colortab.h"


FlatView::BitMapMgr::BitMapMgr( const FlatView::Viewer& vwr, bool wva )
    : vwr_(vwr)
    , bmp_(0)
    , pos_(0)
    , data_(0)
    , gen_(0)
    , wva_(wva)
{
    setupChg();
}


void FlatView::BitMapMgr::clearAll()
{
    delete bmp_;	bmp_ = 0;
    delete pos_;	pos_ = 0;
    delete data_;	data_ = 0;
    delete gen_;	gen_ = 0;
}


void FlatView::BitMapMgr::setupChg()
{
    clearAll();

    const Array2D<float>* arr = vwr_.data().arr( wva_ );
    if ( !arr ) return;

    const FlatView::Appearance& app = vwr_.appearance();
    if ( (wva_ && !app.ddpars_.wva_.show_)
      || (!wva_ && !app.ddpars_.vd_.show_) )
	return;

    const FlatPosData& pd = vwr_.data().pos( wva_ );
    pos_ = new A2DBitMapPosSetup( arr->info(), pd.getPositions(true) );
    pos_->setDim1Positions( pd.range(false).start, pd.range(false).stop );
    data_ = new A2DBitMapInpData( *arr );

    if ( !wva_ )
	gen_ = new VDA2DBitMapGenerator( *data_, *pos_ );
    else
    {
	const DataDispPars::WVA& wvapars = app.ddpars_.wva_;
	WVAA2DBitMapGenerator* wvagen
	    		= new WVAA2DBitMapGenerator( *data_, *pos_ );
	wvagen->wvapars().drawwiggles_ = wvapars.wigg_ != Color::NoColor;
	wvagen->wvapars().drawmid_ = wvapars.mid_ != Color::NoColor;
	wvagen->wvapars().fillleft_ = wvapars.left_ != Color::NoColor;
	wvagen->wvapars().fillright_ = wvapars.right_ != Color::NoColor;
	wvagen->wvapars().overlap_ = wvapars.overlap_;
	wvagen->wvapars().midvalue_ = wvapars.midvalue_;
	gen_ = wvagen;
    }
    const DataDispPars::Common* pars = &app.ddpars_.wva_;
    if ( !wva_ ) pars = &app.ddpars_.vd_;
    gen_->pars().clipratio_ = pars->clipperc_ * 0.01;
    gen_->pars().nointerpol_ = pars->blocky_;
    gen_->pars().fliplr_ = app.annot_.x1_.reversed_;
    gen_->pars().fliptb_ = !app.annot_.x2_.reversed_;
    		// in UI pixels, Y is reversed
    gen_->pars().autoscale_ = mIsUdf(pars->rg_.start) ||mIsUdf(pars->rg_.stop);
    if ( !gen_->pars().autoscale_ )
	gen_->pars().scale_ = pars->rg_;
}


bool FlatView::BitMapMgr::generate( const Geom::PosRectangle<double>& wr,
				    const Geom::Size2D<int>& sz )
{
    if ( !gen_ ) return true;

    pos_->setDimRange( 0, Interval<float>( wr.left(), wr.right() ) );
    pos_->setDimRange( 1, Interval<float>( wr.bottom(), wr.top() ) );

    bmp_ = new A2DBitMapImpl( sz.width(), sz.height() );
    if ( !bmp_ || !bmp_->getData() )
	{ delete bmp_; bmp_ = 0; return false; }
    A2DBitMapGenerator::initBitMap( *bmp_ );

    gen_->setBitMap( *bmp_ );
    gen_->fill();
    return true;
}


FlatView::BitMap2RGB::BitMap2RGB( const FlatView::Appearance& a,
				  uiRGBArray& arr )
    : app_(a)
    , arr_(arr)
{
}


void FlatView::BitMap2RGB::draw( const A2DBitMap* wva, const A2DBitMap* vd )
{
    if ( vd )
	drawVD( *vd );
    if ( wva )
	drawWVA( *wva );
}


void FlatView::BitMap2RGB::drawVD( const A2DBitMap& bmp )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(arr_.getSize(true),arr_.getSize(false));
    const FlatView::DataDispPars::VD& pars = app_.ddpars_.vd_;
    ColorTable ctab( pars.ctab_ );
    const int minfill = (int)VDA2DBitMapGenPars::cMinFill;
    const int maxfill = (int)VDA2DBitMapGenPars::cMaxFill;
    ctab.calcList( maxfill - minfill + 1 );

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix, iy );
	    if ( bmpval == A2DBitMapGenPars::cNoFill )
		continue;

	    Color col = ctab.tableColor( (int)bmpval - minfill );
	    if ( col != Color::NoColor )
		arr_.set( ix, iy, col );
	}
    }
}


void FlatView::BitMap2RGB::drawWVA( const A2DBitMap& bmp )
{
    const Geom::Size2D<int> bmpsz(bmp.info().getSize(0),bmp.info().getSize(1));
    const Geom::Size2D<int> arrsz(arr_.getSize(true),arr_.getSize(false));
    const FlatView::DataDispPars::WVA& pars = app_.ddpars_.wva_;

    for ( int ix=0; ix<arrsz.width(); ix++ )
    {
	if ( ix >= bmpsz.width() ) break;
	for ( int iy=0; iy<arrsz.height(); iy++ )
	{
	    if ( iy >= bmpsz.height() ) break;
	    const char bmpval = bmp.get( ix, iy );
	    if ( bmpval == A2DBitMapGenPars::cNoFill )
		continue;

	    Color col( pars.wigg_ );
	    if ( bmpval == WVAA2DBitMapGenPars::cLeftFill )
		col = pars.left_;
	    else if ( bmpval == WVAA2DBitMapGenPars::cRightFill )
		col = pars.right_;
	    else if ( bmpval == WVAA2DBitMapGenPars::cZeroLineFill )
		col = pars.mid_;

	    if ( col != Color::NoColor )
		arr_.set( ix, iy, col );
	}
    }
}

