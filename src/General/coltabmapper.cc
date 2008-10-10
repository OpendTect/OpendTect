/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Jul 2007
-*/

static const char* rcsID = "$Id: coltabmapper.cc,v 1.12 2008-10-10 21:13:05 cvskris Exp $";

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


DefineEnumNames( ColTab::MapperSetup, Type, 1, "Types" )
{ "Fixed", "Auto", "HistEq", 0 };


ColTab::MapperSetup::MapperSetup()
    : type_(Auto)
    , cliprate_(defClipRate())
    , symmidval_(defSymMidval())
    , maxpts_(2000)
    , nrsegs_(0)
    , start_(0), width_(1)
{}


bool ColTab::MapperSetup::operator==( const ColTab::MapperSetup& b ) const
{
    if ( type_!=b.type_ || nrsegs_!=b.nrsegs_ || maxpts_!=b.maxpts_ )
	return false;

    if ( type_==Fixed )
    {
	if ( start_!=b.start_ || width_!=b.width_ )
	    return false;
    }
    else 
    {
	if ( !mIsUdf(symmidval_) || !mIsUdf(b.symmidval_) )
	{
	    if ( !mIsEqual(symmidval_,b.symmidval_, 1e-5 ) )
		return false;
	}
    }

    if ( type_==Auto )
    {
	if ( !mIsUdf(cliprate_) || !mIsUdf(b.cliprate_) )
	{
	    if ( !mIsEqual(cliprate_,b.cliprate_, 1e-5 ) )
		return false;
	}
    }

    return true;
}


bool ColTab::MapperSetup::operator!=( const ColTab::MapperSetup& b ) const
{ return !(*this==b); }


void ColTab::MapperSetup::fillPar( IOPar& par ) const
{
    par.set( sKeyType(), TypeNames[(int) type_] );
    par.set( sKeyClipRate(), cliprate_ );
    par.set( sKeySymMidVal(), symmidval_ );
    par.set( sKeyMaxPts(), maxpts_ );
    par.set( sKeyRange(), start_, width_ );
}


bool ColTab::MapperSetup::usePar( const IOPar& par )
{
    const char* typestr = par.find( sKeyType() );
    const int typeidx = TypeDef().convert( typestr );
    if ( typeidx==-1 )
	return false;

    type_ = (Type) typeidx;

    return par.get( sKeyClipRate(), cliprate_ ) &&
	   par.get( sKeySymMidVal(), symmidval_ ) &&
	   par.get( sKeyMaxPts(), maxpts_ ) &&
	   par.get( sKeyRange(), start_, width_ );
}


ColTab::Mapper::Mapper()
    : vs_(0)
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
    float ret = (val-setup_.start_) / setup_.width_;
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
    if ( !Math::IsNormalNumber(v) || mIsUdf(v) ) return udfval;

    float ret = mapper ? mapper->position( v ) : v;
    ret *= nrsteps;
    if ( ret > nrsteps- 0.9 ) ret = nrsteps- 0.9;
    else if ( ret < 0 ) ret = 0;
    return (int)ret;
}


Interval<float> ColTab::Mapper::range() const
{
    return Interval<float>( setup_.start_, setup_.start_ + setup_.width_ );
}


void ColTab::Mapper::setRange( const Interval<float>& rg )
{
    setup_.start_ = rg.start;
    setup_.width_ = rg.width();
}


void ColTab::Mapper::setData( const ValueSeries<float>* vs, od_int64 sz )
{
    vs_ = vs; vssz_ = sz;
    update( true );
}


void ColTab::Mapper::update( bool full )
{
    if ( setup_.type_ == MapperSetup::Fixed || !vs_ || vssz_ < 1 )
	return;
    if ( vssz_ == 1 )
	{ setup_.start_ = vs_->value(0) - 1; setup_.width_ = 2; return; }

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
