/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Jul 2007
-*/

static const char* rcsID = "$Id: coltabmapper.cc,v 1.8 2008-09-22 13:00:45 cvskris Exp $";

#include "coltabmapper.h"
#include "dataclipper.h"
#include "settings.h"
#include "math2.h"
#include "errh.h"

namespace ColTab
{
static float defcliprate_ = mUdf(float);
static const char* sKeyDefClipPerc = "dTect.Disp.Default clip perc";
static float defsymmidval_ = mUdf(float);
static const char* sKeyDefSymmZero = "dTect.Disp.Default symmetry zero";
static BufferString defcoltabnm_ = "Seismics";
static const char* sKeyDefName = "dTect.Disp.Default Color table";
static const char* sKeyDefNameOld = "dTect.Color table.Name";
}


const char* ColTab::defSeqName()
{
    if ( !Settings::common().get(sKeyDefNameOld,defcoltabnm_) )
	Settings::common().get( sKeyDefName, defcoltabnm_ );

    return defcoltabnm_.buf();
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
    defcliprate_ = cr;
    defsymmidval_ = sm;
    Settings::common().set( sKeyDefClipPerc, cr*100 );
    Settings::common().set( sKeyDefSymmZero, sm );
    Settings::common().write();
}


ColTab::MapperSetup::MapperSetup()
    : type_(Auto)
    , cliprate_(defClipRate())
    , symmidval_(defSymMidval())
    , maxpts_(2000)
    , nrsegs_(0)
{}


ColTab::Mapper::Mapper()
    : start_(0), width_(1) , vs_(0)
    , clipper_(*new DataClipper)
{ }


ColTab::Mapper::~Mapper()
{
    delete &clipper_;
}



#define mWarnHistEqNotImpl \
    if ( setup_.type_ == MapperSetup::HistEq ) \
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
    if ( setup_.nrsegs_ > 0 )
    {
	ret *= setup_.nrsegs_;
	ret = (0.5 + ((int)ret)) / setup_.nrsegs_;
    }

    if ( ret > 1 ) ret = 1;
    else if ( ret < 0 ) ret = 0;
    return ret;
}


int ColTab::Mapper::snappedPosition( const ColTab::Mapper* mapper,
	float v, int nrsteps, int udfval )
{
    if ( !Math::IsNormalNumber(v) || mIsUdf(v) ) return udfval;udfval;

    float ret = mapper ? mapper->position( v ) : v;
    ret *= nrsteps;
    if ( ret > nrsteps- 0.9 ) ret = nrsteps- 0.9;
    else if ( ret < 0 ) ret = 0;
    return (int)ret;
}


void ColTab::Mapper::setData( const ValueSeries<float>* vs, int sz )
{
    vs_ = vs; vssz_ = sz;
    update( true );
}


void ColTab::Mapper::update( bool full )
{
    if ( setup_.type_ == MapperSetup::Fixed || !vs_ || vssz_ < 1 )
	return;
    if ( vssz_ == 1 )
	{ start_ = vs_->value(0) - 1; width_ = 2; return; }

    mWarnHistEqNotImpl

    if ( full || clipper_.isEmpty() )
    {
	clipper_.reset();
	clipper_.setApproxNrValues( vssz_, setup_.maxpts_ ) ;
	clipper_.putData( *vs_, vssz_ );
	clipper_.fullSort();
    }

    Interval<float> intv( -1, 1 );
    bool res = mIsUdf(setup_.symmidval_) ?
	       clipper_.getRange( setup_.cliprate_, intv )
	     : clipper_.getSymmetricRange( setup_.cliprate_,
		     			   setup_.symmidval_, intv );
    if ( mIsZero(intv.width(),mDefEps) )
	intv += Interval<float>(-1,1);
    setRange( intv );
}
