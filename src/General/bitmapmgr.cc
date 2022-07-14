 /*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/

#include "bitmapmgr.h"

#include "arrayndimpl.h"
#include "array2dbitmapimpl.h"
#include "flatposdata.h"
#include "flatview.h"


BitMapMgr::BitMapMgr()
    : appearance_(*new FlatView::Appearance)
    , sz_(mUdf(int),mUdf(int))
    , wr_(mUdf(double),mUdf(double),mUdf(double),mUdf(double))
{
}


BitMapMgr::~BitMapMgr()
{
    delete &appearance_;
    clearAll();
}


void BitMapMgr::init( const FlatDataPack* fdp, const FlatView::Appearance& app,
		      bool wva )
{
    clearAll();

    datapack_ = const_cast<FlatDataPack*>( fdp );
    appearance_ = app;
    wva_ = wva;
    setup();
}


void BitMapMgr::clearAll()
{
    Threads::Locker locker( lock_ );

    deleteAndZeroPtr( bmp_ );
    deleteAndZeroPtr( pos_ );
    deleteAndZeroPtr( data_ );
    deleteAndZeroPtr( gen_ );

    datapack_ = nullptr;
}


void BitMapMgr::setup()
{
    Threads::Locker locker( lock_ );

    if ( !datapack_ )
	return;

    ConstRefMan<FlatDataPack> datapack = datapack_.get();
    Threads::Locker updlckr( datapack->updateLock() );
    const FlatPosData& pd = datapack->posData();
    const Array2D<float>& arr = datapack->data();
    if ( pd.nrPts(true) < arr.info().getSize(0) )
	return;

    pos_ = new A2DBitMapPosSetup( arr.info(), pd.getPositions(true) );
    pos_->setDim1Positions( mCast(float,pd.range(false).start),
			    mCast(float,pd.range(false).stop) );
    data_ = new A2DBitMapInpData( arr );

    if ( !wva_ )
    {
	auto* gen = new VDA2DBitMapGenerator( *data_, *pos_ );
	gen->linearInterpolate( appearance_.ddpars_.vd_.lininterp_ );
	gen_ = gen;
    }
    else
    {
	const FlatView::DataDispPars::WVA& wvapars = appearance_.ddpars_.wva_;
	auto* wvagen = new WVAA2DBitMapGenerator( *data_, *pos_ );
	wvagen->wvapars().drawwiggles_ = wvapars.wigg_.isVisible();
	wvagen->wvapars().drawrefline_ = wvapars.refline_.isVisible();
	wvagen->wvapars().filllow_ = wvapars.lowfill_.isVisible();
	wvagen->wvapars().fillhigh_ = wvapars.highfill_.isVisible();
	wvagen->wvapars().overlap_ = wvapars.overlap_;
	wvagen->wvapars().reflinevalue_ = wvapars.reflinevalue_;
	wvagen->wvapars().x1reversed_ = appearance_.annot_.x1_.reversed_;
	gen_ = wvagen;
    }

    const FlatView::DataDispPars::Common* pars = &appearance_.ddpars_.wva_;
    if ( !wva_ )
	pars = &appearance_.ddpars_.vd_;

    gen_->pars().clipratio_ = pars->mappersetup_.cliprate_;
    gen_->pars().midvalue_ =
	pars->mappersetup_.autosym0_ ? mUdf(float)
				     : pars->mappersetup_.symmidval_;
    gen_->pars().nointerpol_ = pars->blocky_;
    gen_->pars().autoscale_ =
	pars->mappersetup_.type_ == ColTab::MapperSetup::Auto;
    gen_->pars().scale_ = pars->mappersetup_.range_;
}


Geom::Point2D<int> BitMapMgr::dataOffs(
			const Geom::PosRectangle<double>& inpwr,
			const Geom::Size2D<int>& inpsz ) const
{
    Threads::Locker locker( lock_ );

    Geom::Point2D<int> ret( mUdf(int), mUdf(int) );
    if ( mIsUdf(wr_.left()) ) return ret;

    // First see if we have different zooms:
    const Geom::Size2D<double> wrsz = wr_.size();
    const double xratio = wrsz.width() / sz_.width();
    const double yratio = wrsz.height() / sz_.height();
    const Geom::Size2D<double> inpwrsz = inpwr.size();
    const double inpxratio = inpwrsz.width() / inpsz.width();
    const double inpyratio = inpwrsz.height() / inpsz.height();
    if ( !mIsZero(xratio-inpxratio,mDefEps)
      || !mIsZero(yratio-inpyratio,mDefEps) )
	return ret;

    // Now check whether we have a pan outside buffered area:
    const bool xrev = wr_.right() < wr_.left();
    const bool yrev = wr_.top() < wr_.bottom();
    const double xoffs = (xrev ? inpwr.right() - wr_.right()
			       : inpwr.left() - wr_.left()) / xratio;
    const double yoffs = (yrev ? inpwr.top() - wr_.top()
			       : inpwr.bottom() - wr_.bottom()) / yratio;
    if ( xoffs <= -0.5 || yoffs <= -0.5 )
	return ret;
    const double maxxoffs = sz_.width() - inpsz.width() + .5;
    const double maxyoffs = sz_.height() - inpsz.height() + .5;
    if ( xoffs >= maxxoffs || yoffs >= maxyoffs )
	return ret;

    // No, we're cool. Return nearest integers
    ret.x = mNINT32(xoffs); ret.y = mNINT32(yoffs);
    return ret;
}


bool BitMapMgr::generate( const Geom::PosRectangle<double>& wr,
			  const Geom::Size2D<int>& sz,
			  const Geom::Size2D<int>& availsz )
{
    Threads::Locker locker( lock_ );
    if ( !gen_ )
    {
	setup();
	if ( !gen_ )
	    return true;
    }

    if ( !datapack_ )
    {
	pErrMsg("Sanity check");
	return true;
    }

    ConstRefMan<FlatDataPack> datapack = datapack_.get();
    Threads::Locker updlckr( datapack->updateLock() );
    const FlatPosData& pd = datapack->posData();
    pos_->setDimRange( 0, Interval<float>(
				mCast(float,wr.left()-pd.offset(true)),
				mCast(float,wr.right()-pd.offset(true))) );
    pos_->setDimRange( 1,
	Interval<float>(mCast(float,wr.bottom()),mCast(float,wr.top())) );

    bmp_ = new A2DBitMapImpl( sz.width(), sz.height() );
    if ( !bmp_ || !bmp_->isOK() || !bmp_->getData() )
    {
	deleteAndZeroPtr( bmp_ );
	updlckr.unlockNow();
	return false;
    }

    wr_ = wr; sz_ = sz;
    A2DBitMapGenerator::initBitMap( *bmp_ );
    gen_->setBitMap( *bmp_ );
    gen_->setPixSizes( availsz.width(), availsz.height() );

    if ( &datapack->data() != &data_->data() )
    {
	pErrMsg("Sanity check failed");
	return false;
    }

    gen_->fill();

    updlckr.unlockNow();
    return true;
}
