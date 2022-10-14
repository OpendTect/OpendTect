/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "coltabmapper.h"

#include "arraynd.h"
#include "dataclipper.h"
#include "histequalizer.h"
#include "keystrs.h"
#include "math2.h"
#include "settings.h"
#include "task.h"
#include "uistrings.h"

namespace ColTab
{
static Interval<float> defcliprate_ = Interval<float>(mUdf(float),mUdf(float));
static const char* sKeyDefClipPerc = "dTect.Disp.Default clip perc";
static float defsymmidval_ = mUdf(float);
static bool defautosymm_ = true;
static bool defhisteq_ = false;
static const char* sKeyDefSymmZero = "dTect.Disp.Default symmetry zero";
static const char* sKeyDefAutoSymm = "dTect.Disp.Default auto symmetry";
static const char* sKeyDefHistEq = "dTect.Disp.Default histogram equalisation";
static BufferString defcoltabnm_ = "OD Seismic 1";
static const char* sKeySeisDefName = "dTect.Disp.Default Color Table.Seismics";
static const char* sKeyAttrDefName= "dTect.Disp.Default Color Table.Attributes";
static const char* sKeyDefNameOld = "dTect.Color table.Name";
}


const char* ColTab::defSeqName()
{
    if ( !Settings::common().get(sKeyDefNameOld,defcoltabnm_) )
    {
	// Forward compatibility
	if ( !Settings::common().get(sKeySeisDefName,defcoltabnm_) )
	    Settings::common().get( sKeyAttrDefName, defcoltabnm_ );
    }

    return defcoltabnm_.buf();
}


Interval<float> ColTab::defClipRate()
{
    if ( mIsUdf(defcliprate_.start) || mIsUdf(defcliprate_.stop))
    {
	Interval<float> clipperc( mUdf(float), mUdf(float) );
	Settings::common().get( sKeyDefClipPerc, clipperc );

	if ( mIsUdf(clipperc.start) )
	    clipperc.start = 0.5;
	if ( mIsUdf(clipperc.stop) )
	    clipperc.stop = clipperc.start;
	clipperc.scale( 0.01 );
	defcliprate_ = clipperc;

	float mv = mUdf(float);
	Settings::common().get( sKeyDefSymmZero, mv );
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
    if ( mIsUdf(defcliprate_.start) || mIsUdf(defcliprate_.stop) )
	(void)defClipRate();
    return defsymmidval_;
}


void ColTab::setMapperDefaults( Interval<float> cr, float sm, bool asym,
				bool histeq )
{
    defcliprate_ = cr;
    cr.start *= 100;
    cr.stop *= 100;
    defsymmidval_ = sm;
    defautosymm_ = asym;
    Settings::common().set( sKeyDefClipPerc, cr );
    Settings::common().set( sKeyDefSymmZero, sm );
    Settings::common().setYN( sKeyDefAutoSymm, asym );
    Settings::common().setYN( sKeyDefHistEq, histeq );
    Settings::common().write();
}

namespace ColTab
{
    mDefineEnumUtils( MapperSetup, Type, "Types" )
    { "Fixed", "Auto", "HistEq", 0 };
}

ColTab::MapperSetup::MapperSetup()
    : type_(Auto)
    , cliprate_(defClipRate())
    , symmidval_(defSymMidval())
    , autosym0_(defAutoSymmetry())
    , maxpts_(1000000)
    , nrsegs_(0)
    , range_(Interval<float>::udf())
    , flipseq_( false )
    , rangeChange(this)
    , autoscaleChange(this)
{}


ColTab::MapperSetup::MapperSetup( const ColTab::MapperSetup& oth )
    : rangeChange(this)
    , autoscaleChange(this)
{
    *this = oth;
}


ColTab::MapperSetup::~MapperSetup()
{}


ColTab::MapperSetup&
    ColTab::MapperSetup::operator=( const ColTab::MapperSetup& ms )
{
    if ( &ms == this )
	return *this;

    type_ = ms.type_;
    cliprate_ = ms.cliprate_;
    autosym0_ = ms.autosym0_;
    symmidval_ = ms.symmidval_;

    maxpts_ = ms.maxpts_;
    nrsegs_ = ms.nrsegs_;
    range_ = ms.range_;
    flipseq_ = ms.flipseq_;
    return *this;
}


bool ColTab::MapperSetup::needsReClip( const ColTab::MapperSetup& newmpr ) const
{
    if ( (type_!=newmpr.type_ && newmpr.type_!=ColTab::MapperSetup::Fixed ) ||
	 nrsegs_!=newmpr.nrsegs_ ||
	 maxpts_!=newmpr.maxpts_ || cliprate_!=newmpr.cliprate_ ||
	 autosym0_!=newmpr.autosym0_  || symmidval_!=newmpr.symmidval_ )
	return true;

    return false;
}


bool ColTab::MapperSetup::operator==( const ColTab::MapperSetup& mpr ) const
{
    if ( type_!=mpr.type_ || nrsegs_!=mpr.nrsegs_ || maxpts_!=mpr.maxpts_ ||
	 flipseq_!=mpr.flipseq_ )
	return false;

    if ( type_==Fixed )
    {
	if ( range_!=mpr.range_ )
	    return false;
    }
    else
    {
	if ( !mIsUdf(symmidval_) || !mIsUdf(mpr.symmidval_) )
	{
	    if ( !mIsEqual(symmidval_,mpr.symmidval_, 1e-5 ) )
		return false;
	}

	if ( autosym0_ != mpr.autosym0_ )
	    return false;
    }

    if ( type_==Auto )
    {
	if ( !mIsUdf(cliprate_.start) || !mIsUdf(cliprate_.stop) ||
	     !mIsUdf(mpr.cliprate_.start)  || !mIsUdf(mpr.cliprate_.stop))
	{
	    if ( !mIsEqual(cliprate_.start,mpr.cliprate_.start,1e-5) ||
		 !mIsEqual(cliprate_.stop,mpr.cliprate_.stop,1e-5) )
		return false;
	}
    }

    return true;
}


bool ColTab::MapperSetup::operator!=( const ColTab::MapperSetup& b ) const
{ return !(*this==b); }


void ColTab::MapperSetup::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), TypeNames()[(int)type_] );
    par.set( sKeyClipRate(), cliprate_ );
    par.set( sKeySymMidVal(), symmidval_ );
    par.setYN( sKeyAutoSym(), autosym0_ );
    par.set( sKeyRange(), range_ );
    par.setYN( sKeyFlipSeq(), flipseq_ );
}


bool ColTab::MapperSetup::usePar( const IOPar& par )
{
    const BufferString typestr = par.find( sKey::Type() );
    if ( !TypeDef().parse( typestr, type_ ) )
	return false;

    if ( par.hasKey(sKeyStarWidth()) )
    {
	float start, width;
	par.get( sKeyStarWidth(), start, width );
	range_.start = start;
	range_.stop = start + width;
    }
    else if ( par.hasKey(sKeyRange()) )
	par.get( sKeyRange(), range_ );
    else
	return false;

    cliprate_.start = mUdf(float);
    cliprate_.stop = mUdf(float);
    par.get( sKeyClipRate(), cliprate_ );
    if ( mIsUdf(cliprate_.stop) )
	cliprate_.stop = cliprate_.start;
    else if ( mIsUdf(cliprate_.start) )
	cliprate_ = defClipRate();

    flipseq_ = false;
    par.getYN( sKeyFlipSeq(), flipseq_ );

    return par.get( sKeySymMidVal(), symmidval_ ) &&
	   par.getYN( sKeyAutoSym(), autosym0_ );
}


void ColTab::MapperSetup::setAutoScale( bool yn )
{
    type_ = yn ? Auto : Fixed;
    if ( type_==Auto )
	range_ = Interval<float>::udf();
}


void ColTab::MapperSetup::triggerRangeChange()
{ rangeChange.trigger(); }


void ColTab::MapperSetup::triggerAutoscaleChange()
{ autoscaleChange.trigger(); }



ColTab::Mapper::Mapper()
    : clipper_(new DataClipper)
    , clipperismine_(true)
{
}


ColTab::Mapper::Mapper( const Mapper& oth )
    : Mapper()
{
    *this = oth;
}


ColTab::Mapper::Mapper( const Mapper& oth, bool shareclipper )
    : Mapper()
{
    *this = oth;
    if ( shareclipper && clipperismine_ )
    {
	delete clipper_;
	clipper_ = oth.clipper_;
	clipperismine_ = false;
    }
}


ColTab::Mapper::~Mapper()
{
    if ( clipperismine_ )
	delete clipper_;
}


ColTab::Mapper& ColTab::Mapper::operator =( const Mapper& oth )
{
    if ( &oth != this )
    {
	setup_ = oth.setup_;
	if ( clipperismine_ ) delete clipper_;
	clipper_ = oth.clipperismine_ ? new DataClipper( *oth.clipper_ )
				      : oth.clipper_;
	clipperismine_ = oth.clipperismine_;
	arrnd_ = oth.arrnd_;
	vs_ = oth.vs_;
	dataptr_ = oth.dataptr_;
	datasz_ = oth.datasz_;
    }

    return *this;
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

    const float width = setup_.range_.width( false );

    if ( mIsZero(width,mDefEps) )
	return 0.f;

    float ret = (val-setup_.range_.start) / width;
    if ( setup_.nrsegs_ > 0 )
    {
	ret *= setup_.nrsegs_;
	ret = (0.5f + ((int)ret)) / setup_.nrsegs_;
    }

    if ( ret > 1 ) ret = 1.f;
    else if ( ret < 0 ) ret = 0.f;

    return setup_.flipseq_ ? 1.0f - ret : ret;
}


int ColTab::Mapper::snappedPosition( const ColTab::Mapper* mapper,
				     float v, int nrsteps, int udfval )
{
    if ( mIsUdf(v) )
	return udfval;

    float ret = mapper ? mapper->position( v ) : v;
    ret *= nrsteps;
    if ( ret > nrsteps - 0.9f ) ret = nrsteps - 0.9f;
    else if ( ret < 0 ) ret = 0.f;
    return (int)ret;
}


const Interval<float>& ColTab::Mapper::range() const
{
    return setup_.range_;
}


void ColTab::Mapper::setRange( const Interval<float>& rg )
{
    setup_.range_ = rg;
}


void ColTab::Mapper::setData( const float* dataptr, od_int64 sz,
			      TaskRunner* taskrun )
{
    arrnd_ = nullptr;
    vs_ = nullptr;
    dataptr_ = dataptr;
    datasz_ = sz;
    update( true, taskrun );
}


void ColTab::Mapper::setData( const ValueSeries<float>& vs, TaskRunner* taskrun)
{
    if ( vs.arr() )
    {
	setData( vs.arr(), vs.size(), taskrun );
	return;
    }

    arrnd_ = nullptr;
    dataptr_ = nullptr;

    vs_ = &vs;
    datasz_ = vs.size();
    update( true, taskrun );
}


void ColTab::Mapper::setData( const ArrayND<float>& arrnd, TaskRunner* taskrun )
{
    if ( arrnd.getData() )
    {
	setData( arrnd.getData(), arrnd.totalSize(), taskrun );
	return;
    }
    else if ( arrnd.getStorage() )
    {
	setData( *arrnd.getStorage(), taskrun );
	return;
    }

    vs_ = nullptr;
    dataptr_ = nullptr;

    arrnd_ = &arrnd;
    datasz_ = arrnd.totalSize();
    update( true, taskrun );
}


class SymmetryCalc : public ParallelTask
{
public:

SymmetryCalc( const float* dataptr, const ValueSeries<float>* vs,
	      const ArrayND<float>* arrnd, od_int64 sz )
    : sz_(sz)
    , dataptr_(dataptr)
    , vs_(vs)
    , arrnd_(arrnd)
{}

bool isSymmAroundZero() const
{
    const od_int64 max = mMAX( above0_, below0_ );
    const od_int64 min = mMIN( above0_, below0_ );

    return max==0 || min==0 ? false : max/min - 1 < 0.05;
}

private:

uiString uiNrDoneText() const override
{
    return uiStrings::sDone();
}

od_int64 nrIterations() const override	{ return sz_; }


bool doPrepare( int nrthreads ) override
{
    if ( !dataptr_ && !vs_ && !arrnd_ )
	return false;

    above0vals_.setSize( nrthreads, 0 );
    below0vals_.setSize( nrthreads, 0 );
    above0_ = 0;
    below0_ = 0;
    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx ) override
{
    od_int64 above0 = 0;
    od_int64 below0 = 0;

    PtrMan<ArrayNDIter> iter;
    if ( arrnd_ )
    {
	iter = new ArrayNDIter( arrnd_->info() );
	iter->setGlobalPos( start );
    }

    float val = mUdf(float);
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( dataptr_ )
	    val = dataptr_[idx];
	else if ( vs_ )
	    val = vs_->value( idx );
	else if ( arrnd_ )
	{
	    val = arrnd_->getND( iter->getPos() );
	    iter->next();
	}

	if ( mIsUdf(val) ) continue;
	if ( val < 0 ) below0++;
	else if ( val > 0 ) above0++;
    }

    above0vals_[threadidx] = above0;
    below0vals_[threadidx] = below0;

    return true;
}


bool doFinish( bool success ) override
{
    if ( !success )
	return false;

    while( !above0vals_.isEmpty() )
	above0_ += above0vals_.pop();
    while( !below0vals_.isEmpty() )
	below0_ += below0vals_.pop();

    return true;
}

    const od_int64		sz_;
    const float*		dataptr_;
    const ValueSeries<float>*	vs_;
    const ArrayND<float>*	arrnd_;

    TypeSet<od_int64>		above0vals_;
    TypeSet<od_int64>		below0vals_;

    od_int64			above0_ = 0;
    od_int64			below0_ = 0;

};


void ColTab::Mapper::update( bool full, TaskRunner* taskrun )
{
    if ( setup_.type_ == MapperSetup::Fixed ||
	 (full && (datasz_ < 1 || (!dataptr_ && !vs_ && !arrnd_))) )
	return;

    if ( datasz_ == 1 )
    {
	const TypeSet<int> arrpos( arrnd_ ? arrnd_->nrDims() : 0, 0 );
	const float firstval = dataptr_
			     ? dataptr_[0]
			     : (vs_ ? vs_->value(0)
				    : (arrnd_ ? arrnd_->getND(arrpos.arr())
					      : mUdf(float)));
	setup_.range_.start = mIsUdf(firstval) ? -1.f : firstval-1.f;
	setup_.range_.stop = setup_.range_.start + 2.f;
	return;
    }

    mWarnHistEqNotImpl

    DataClipper& clipper = *clipper_;
    if ( full || clipper.isEmpty() )
    {
	clipper.reset();
	clipper.setApproxNrValues( datasz_, setup_.maxpts_ ) ;
	if ( dataptr_ )
	    clipper.putData( dataptr_, datasz_ );
	else if ( vs_ )
	    clipper.putData( *vs_, datasz_ );
	else if ( arrnd_ )
	    clipper.putData( *arrnd_ );

	clipper.fullSort();

	if ( setup_.autosym0_ )
	{
	    SymmetryCalc symmcalc( dataptr_, vs_, arrnd_, datasz_ );
	    TaskRunner::execute( taskrun, symmcalc );
	    setup_.symmidval_ = symmcalc.isSymmAroundZero() ? 0.f : mUdf(float);
	}
    }

    Interval<float> intv( -1.f, 1.f );
    mIsUdf(setup_.symmidval_)
	    ? clipper.getRange( setup_.cliprate_.start, setup_.cliprate_.stop,
				intv )
	    : clipper.getSymmetricRange( setup_.cliprate_.start,
					 setup_.symmidval_, intv );
    if ( mIsZero(intv.width(),mDefEps) )
	intv += Interval<float>(-1.f,1.f);

    setRange( intv );
}
