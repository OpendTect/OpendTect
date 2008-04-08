/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Jul 2007
-*/

static const char* rcsID = "$Id: coltabmapper.cc,v 1.3 2008-04-08 03:27:42 cvssatyaki Exp $";

#include "coltabmapper.h"
#include "dataclipper.h"
#include "settings.h"
#include "errh.h"

namespace ColTab
{
static float defcliprate_ = mUdf(float);
static const char* sKeyDefClipPerc = "dTect.Disp.Default clip perc";
static bool defsymmidval_ = mUdf(float);
static const char* sKeyDefSymmZero = "dTect.Disp.Default symmetry zero";
}


float ColTab::defClipRate()
{
    if ( mIsUdf(defcliprate_) )
    {
	float perc = 2.5;
	Settings::common().get( sKeyDefClipPerc, perc );
	float mv = mUdf(float);
	Settings::common().get( sKeyDefSymmZero, mv );
	defcliprate_ = perc * .01;
	defsymmidval_ = mv;
    }
    return defcliprate_;
}


float ColTab::defSymMidval()
{
    if ( mIsUdf(defcliprate_) )
	(void)defClipRate();
    return defsymmidval_;
}


void ColTab::setMapperDefaults( float cr, float sm )
{
    Settings::common().set( sKeyDefClipPerc, cr*100 );
    Settings::common().set( sKeyDefSymmZero, sm );
    Settings::common().write();
}


ColTab::Mapper::Mapper()
    : type_(Auto)
    , clipper_(*new DataClipper)
    , cliprate_(defClipRate())
    , symmidval_(defSymMidval())
    , maxpts_(2000)
    , nrsegs_(0)
    , start_(0), width_(1)
    , vs_(0)
{
}


ColTab::Mapper::~Mapper()
{
    delete &clipper_;
}


#define mWarnHistEqNotImpl \
    if ( type_ == HistEq ) \
    { \
	static bool msgdone = false; \
	if ( !msgdone ) \
	{ \
	    pErrMsg("TODO: implement HistEq col tab scaling"); \
	    msgdone = true; \
	} \
    }

float ColTab::Mapper::position( float val ) const
{
    mWarnHistEqNotImpl
    float ret = (val-start_) / width_;
    if ( nrsegs_ > 0 )
    {
	ret *= nrsegs_;
	ret = (0.5 + ((int)ret)) / nrsegs_;
    }

    if ( ret > 1 ) ret = 1;
    else if ( ret < 0 ) ret = 0;
    return ret;
}


void ColTab::Mapper::setData( const ValueSeries<float>* vs, int sz )
{
    vs_ = vs; vssz_ = sz;
    update( true );
}


void ColTab::Mapper::update( bool full )
{
    if ( type_ == Fixed || !vs_ || vssz_ < 1 )
	return;
    if ( vssz_ == 1 )
	{ start_ = vs_->value(0) - 1; width_ = 2; return; }

    mWarnHistEqNotImpl

    if ( full || clipper_.isEmpty() )
    {
	clipper_.reset();
	clipper_.setApproxNrValues( vssz_, maxpts_ ) ;
	clipper_.putData( *vs_, vssz_ );
	clipper_.fullSort();
    }

    Interval<float> intv;
    bool res = mIsUdf(symmidval_) ?
	       clipper_.getRange( cliprate_, intv )
	     : clipper_.getSymmetricRange( cliprate_, symmidval_, intv );
    if ( res )
	setRange( intv );
}
