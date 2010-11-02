/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Jul 2007
-*/

static const char* rcsID = "$Id: coltabmapper.cc,v 1.25 2010-11-02 15:02:00 cvskris Exp $";

#include "coltabmapper.h"
#include "dataclipper.h"
#include "histequalizer.h"
#include "keystrs.h"
#include "settings.h"
#include "math2.h"
#include "errh.h"
#include "task.h"

namespace ColTab
{
static float defcliprate_ = mUdf(float);
static const char* sKeyDefClipPerc = "dTect.Disp.Default clip perc";
static float defsymmidval_ = mUdf(float);
static bool defautosymm_ = false;
static bool defhisteq_ = false;
static const char* sKeyDefSymmZero = "dTect.Disp.Default symmetry zero";
static const char* sKeyDefAutoSymm = "dTect.Disp.Default auto symmetry";
static const char* sKeyDefHistEq = "dTect.Disp.Default histogram equalisation";
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


bool ColTab::defAutoSymmetry()
{
    Settings::common().getYN( sKeyDefAutoSymm, defautosymm_ );
    return defautosymm_;
}


bool ColTab::defHistEq()
{
    return defhisteq_;
}


float ColTab::defSymMidval()
{
    if ( mIsUdf(defcliprate_) )
	(void)defClipRate();
    return defsymmidval_;
}


void ColTab::setMapperDefaults( float cr, float sm, bool asym, bool histeq )
{
    defcliprate_ = cr;
    defsymmidval_ = sm;
    defautosymm_ = asym;
    Settings::common().set( sKeyDefClipPerc, cr*100 );
    Settings::common().set( sKeyDefSymmZero, sm );
    Settings::common().setYN( sKeyDefAutoSymm, asym );
    Settings::common().setYN( sKeyDefHistEq, histeq );
    Settings::common().write();
}

namespace ColTab
{
    DefineEnumNames( MapperSetup, Type, 1, "Types" )
    { "Fixed", "Auto", "HistEq", 0 };
}

ColTab::MapperSetup::MapperSetup()
    : type_(Auto)
    , cliprate_(defClipRate())
    , symmidval_(defSymMidval())
    , autosym0_(defAutoSymmetry())
    , maxpts_(2560)
    , nrsegs_(0)
    , start_(0), width_(1)
    , rangeChange(this)
    , autoscaleChange(this)
{}


ColTab::MapperSetup&
    ColTab::MapperSetup::operator=( const ColTab::MapperSetup& ms )
{
    type_ = ms.type_;
    cliprate_ = ms.cliprate_;
    autosym0_ = ms.autosym0_;
    symmidval_ = ms.symmidval_;

    maxpts_ = ms.maxpts_;
    nrsegs_ = ms.nrsegs_;
    start_ = ms.start_;
    width_ = ms.width_;
    return *this;
}


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

	if ( autosym0_ != b.autosym0_ )
	    return false;
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
    par.set( sKey::Type, TypeNames()[(int)type_] );
    par.set( sKeyClipRate(), cliprate_ );
    par.set( sKeySymMidVal(), symmidval_ );
    par.setYN( sKeyAutoSym(), autosym0_ );
    par.set( sKeyMaxPts(), maxpts_ );
    par.set( sKeyRange(), start_, width_ );
}


bool ColTab::MapperSetup::usePar( const IOPar& par )
{
    const char* typestr = par.find( sKey::Type );
    if ( !TypeParse( typestr, type_ ) )
	return false;

    return par.get( sKeyClipRate(), cliprate_ ) &&
	   par.get( sKeySymMidVal(), symmidval_ ) &&
	   par.getYN( sKeyAutoSym(), autosym0_ ) &&
	   par.get( sKeyMaxPts(), maxpts_ ) &&
	   par.get( sKeyRange(), start_, width_ );
}


void ColTab::MapperSetup::triggerRangeChange()
{ rangeChange.trigger(); }

void ColTab::MapperSetup::triggerAutoscaleChange()
{ autoscaleChange.trigger(); }



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

    if ( mIsZero(setup_.width_,mDefEps) )
	return 0;

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
    setup_.width_ = rg.width(false);
}


void ColTab::Mapper::setData( const ValueSeries<float>* vs, od_int64 sz,
			      TaskRunner* tr )
{
    vs_ = vs; vssz_ = sz;
    update( true, tr );
}


struct SymmetryCalc : public ParallelTask
{
SymmetryCalc( const ValueSeries<float>& vs, od_int64 sz )
    : sz_(sz)
    , vs_(vs)
    , above0_(0)
    , below0_(0)
{}

od_int64 nrIterations() const { return sz_; }

bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( mIsUdf(vs_[idx]) ) continue;
	if ( vs_[idx] < 0 ) below0_++;
	else if ( vs_[idx] > 0 ) above0_++;
    }
    return true;
}


bool isSymmAroundZero() const
{
    od_int64 max = mMAX( above0_, below0_ );
    od_int64 min = mMIN( above0_, below0_ );
    if ( max==0 || min==0 ) return false;

    return max/min - 1 < 0.05;
}

    od_int64	sz_;
    od_int64	above0_;
    od_int64	below0_;
    const ValueSeries<float>& vs_;
};


void ColTab::Mapper::update( bool full, TaskRunner* tr )
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

	if ( setup_.autosym0_ )
	{
	    SymmetryCalc symmcalc( *vs_, vssz_ );
	    if ( tr )
		tr->execute( symmcalc );
	    else
		symmcalc.execute();

	    setup_.symmidval_ = symmcalc.isSymmAroundZero() ? 0 : mUdf(float);
	}
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
