/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert etc.
 * DATE     : 1996 / Jul 2007
-*/


#include "coltabmapper.h"
#include "dataclipper.h"
#include "histequalizer.h"
#include "keystrs.h"
#include "settings.h"
#include "math2.h"
#include "task.h"
#include "uistrings.h"

static const int cMaxClipperPts = 100000;

namespace ColTab
{
static Interval<float> defcliprate_ = Interval<float>(mUdf(float),mUdf(float));
static const char* sKeyDefClipPerc = "dTect.Disp.Default clip perc";
static float defsymmidval_ = mUdf(float);
static bool defautosymm_ = false;
static bool defhisteq_ = false;
static const char* sKeyDefSymmZero = "dTect.Disp.Default symmetry zero";
static const char* sKeyDefAutoSymm = "dTect.Disp.Default auto symmetry";
static const char* sKeyDefHistEq = "dTect.Disp.Default histogram equalisation";
static const char* sKeyStartWidth = "Start_Width";
}


bool ColTab::isFlipped( SeqUseMode mode )
{
    return mode == FlippedSingle || mode == FlippedCyclic;
}


bool ColTab::isCyclic( SeqUseMode mode )
{
    return mode == UnflippedCyclic || mode == FlippedCyclic;
}


ColTab::SeqUseMode ColTab::getSeqUseMode( bool flipped, bool cyclic )
{
    return flipped ? (cyclic ? FlippedCyclic : FlippedSingle)
		   : (cyclic ? UnflippedCyclic : UnflippedSingle);
}


BufferString ColTab::toString( SeqUseMode mode )
{
    switch ( mode )
    {
    case UnflippedCyclic:	return BufferString( "Cyclic" );
    case FlippedSingle:		return BufferString( "Flipped" );
    case FlippedCyclic:		return BufferString( "ReverseCyclic" );
    default:
    case UnflippedSingle:	return BufferString( "Normal" );
    }
}


void ColTab::toPar( SeqUseMode mode, IOPar& iop )
{
    iop.set( sKeySeqUseMode(), toString(mode) );
}


bool ColTab::fromPar( const IOPar& iop, SeqUseMode& mode )
{
    BufferString modestr;
    if ( !iop.get(sKeySeqUseMode(),modestr) )
	return false;

    const char firstchar = modestr.firstChar();
    mode = UnflippedSingle;
    if ( firstchar == 'C' )
	mode = UnflippedCyclic;
    else if ( firstchar == 'F' )
	mode = FlippedSingle;
    else if ( firstchar == 'R' )
	mode = FlippedCyclic;

    return true;
}


Interval<float> ColTab::defClipRate()
{
    if ( mIsUdf(defcliprate_.start) || mIsUdf(defcliprate_.stop))
    {
	Interval<float> clipperc( mUdf(float), mUdf(float) );
	Settings::common().get( sKeyDefClipPerc, clipperc );

	if ( mIsUdf(clipperc.start) )
	    clipperc.start = 2.5;
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


void ColTab::setMapperDefaults( Interval<float> clipintv, float sm, bool asym,
				bool histeq )
{
    defcliprate_ = clipintv;
    clipintv.scale( 100.f );
    defsymmidval_ = sm;
    defautosymm_ = asym;
    Settings::common().set( sKeyDefClipPerc, clipintv );
    Settings::common().set( sKeyDefSymmZero, sm );
    Settings::common().setYN( sKeyDefAutoSymm, asym );
    Settings::common().setYN( sKeyDefHistEq, histeq );
    Settings::common().write();
}


ColTab::MapperSetup::MapperSetup()
    : isfixed_(false)
    , dohisteq_(defHistEq())
    , range_(Interval<float>::udf())
    , cliprate_(defClipRate())
    , guesssymmetry_(defAutoSymmetry())
    , symmidval_(defSymMidval())
    , nrsegs_(0)
    , sequsemode_(ColTab::UnflippedSingle)
{
}


ColTab::MapperSetup::MapperSetup( Interval<float> rg )
    : isfixed_(true)
    , dohisteq_(defHistEq())
    , range_(rg)
    , cliprate_(defClipRate())
    , guesssymmetry_(defAutoSymmetry())
    , symmidval_(defSymMidval())
    , nrsegs_(0)
    , sequsemode_(ColTab::UnflippedSingle)
{
}


ColTab::MapperSetup::MapperSetup( const MapperSetup& oth )
    : SharedObject(oth)
{
    copyClassData( oth );
}


ColTab::MapperSetup::~MapperSetup()
{
    sendDelNotif();
}


mImplMonitorableAssignment( ColTab::MapperSetup, SharedObject )


void ColTab::MapperSetup::copyClassData( const ColTab::MapperSetup& oth )
{
    isfixed_ = oth.isfixed_;
    dohisteq_ = oth.dohisteq_;
    range_ = oth.range_;
    cliprate_ = oth.cliprate_;
    guesssymmetry_ = oth.guesssymmetry_;
    symmidval_ = oth.symmidval_;
    nrsegs_ = oth.nrsegs_;
    sequsemode_ = oth.sequsemode_;
}


Monitorable::ChangeType ColTab::MapperSetup::compareClassData(
					const MapperSetup& oth ) const
{
    mStartMonitorableCompare();
    mHandleMonitorableCompare( isfixed_, cIsFixedChange() );
    mHandleMonitorableCompare( dohisteq_, cDoHistEqChange() );
    mHandleMonitorableCompare( nrsegs_, cSegChange() );
    mHandleMonitorableCompare( sequsemode_, cUseModeChange() );
    if ( isfixed_ )
    {
	mHandleMonitorableCompare( range_, cRangeChange() );
    }
    else
    {
	mHandleMonitorableCompare( cliprate_, cAutoScaleChange() );
	mHandleMonitorableCompare( guesssymmetry_, cAutoScaleChange() );
	mHandleMonitorableCompare( symmidval_, cAutoScaleChange() );
    }
    mDeliverMonitorableCompare();
}


bool ColTab::MapperSetup::needsReClip( const ColTab::MapperSetup& oth ) const
{
    if ( isFixed() )
	return false;

    return clipRate() != oth.clipRate()
	|| guessSymmetry() != oth.guessSymmetry()
	|| symMidVal() != oth.symMidVal();
}


void ColTab::MapperSetup::fillPar( IOPar& par ) const
{
    mLock4Read();
    par.set( sKey::Type(), dohisteq_ ? (isfixed_ ? "HistEq" : "VarHistEq")
				     : (isfixed_ ? "Fixed" : "Auto") );
    par.set( sKeyRange(), range_ );
    par.set( sKeyClipRate(), cliprate_ );
    par.set( sKeySymMidVal(), symmidval_ );
    par.setYN( sKeyAutoSym(), guesssymmetry_ );
    par.setYN( sKeyFlipSeq(), isFlipped(sequsemode_) );
    par.setYN( sKeyCycleSeq(), isCyclic(sequsemode_) );
    par.removeWithKey( sKeyStartWidth );
    par.removeWithKey( "Max Pts" );
}


void ColTab::MapperSetup::usePar( const IOPar& par )
{
    mLock4Write();
    const char* typestr = par.find( sKey::Type() );
    if ( !typestr && *typestr )
    {
	if ( *typestr == 'F' || *typestr == 'f' )
	    { isfixed_ = true; dohisteq_ = false; }
	else if ( *typestr == 'A' || *typestr == 'a' )
	    { isfixed_ = false; dohisteq_ = false; }
	else if ( *typestr == 'H' || *typestr == 'h' )
	    { isfixed_ = true; dohisteq_ = true; }
	else if ( *typestr == 'V' || *typestr == 'v' )
	    { isfixed_ = false; dohisteq_ = true; }
    }

    if ( par.find(sKeyRange()) )
	par.get( sKeyRange(), range_ );
    else if ( par.find(sKeyStartWidth) ) // Legacy key
    {
	float start, width;
	par.get( sKeyStartWidth, start, width );
	range_.start = start;
	range_.stop = start + width;
    }
    if ( isfixed_ )
    {
	if ( mIsUdf(range_.start) )
	{
	    if ( mIsUdf(range_.stop) )
		range_.stop = 1.0f;
	    range_.start = range_.stop - 1.0f;
	}
	if ( mIsUdf(range_.stop) )
	    range_.stop = range_.start + 1.0f;
    }

    par.get( sKeyClipRate(), cliprate_ );
    par.get( sKeySymMidVal(), symmidval_ );
    par.getYN( sKeyAutoSym(), guesssymmetry_ );
    if ( !isfixed_ )
    {
	if ( mIsUdf(cliprate_.start) )
	{
	    if ( mIsUdf(cliprate_.stop) )
		cliprate_ = defClipRate();
	    else
		cliprate_.start = cliprate_.stop;
	}
	if ( mIsUdf(cliprate_.stop) )
	    cliprate_.stop = cliprate_.start;
    }

    bool flipseq = isFlipped( sequsemode_ );
    bool cycleseq = isCyclic( sequsemode_ );
    par.getYN( sKeyFlipSeq(), flipseq );
    par.getYN( sKeyCycleSeq(), cycleseq );
    sequsemode_ = getSeqUseMode( flipseq, cycleseq );

    mSendEntireObjChgNotif();
}



ColTab::Mapper::Mapper()
    : vs_(0)
    , setup_(new MapperSetup)
    , clipper_(*new DataClipper)
{
}


ColTab::Mapper::Mapper( const Mapper& oth )
    : vs_(0)
    , setup_(new MapperSetup)
    , clipper_(*new DataClipper)
{
    *this = oth;
}


ColTab::Mapper::~Mapper()
{
    delete &clipper_;
}


ColTab::Mapper& ColTab::Mapper::operator =( const Mapper& oth )
{
    if ( this != &oth )
    {
	*setup_ = *oth.setup_;
	vs_ = oth.vs_;
	vssz_ = oth.vssz_;
	clipper_ = oth.clipper_;
    }
    return *this;
}


void ColTab::Mapper::setSetup( const MapperSetup& newsu )
{
    *setup_ = newsu;
}


void ColTab::Mapper::useSetup( MapperSetup& newsu )
{
    replaceMonitoredRef( setup_, newsu );
}


static bool histeq_not_impl_msg_done = false;

float ColTab::Mapper::position( float val ) const
{
    if ( setup_->doHistEq() )
    {
	if ( !histeq_not_impl_msg_done )
	{
	    pErrMsg("TODO: implement HistEq col tab mapping");
	    histeq_not_impl_msg_done = true;
	}
    }

    return getPosition( range(), setup_->seqUseMode(), setup_->nrSegs(),
			val );
}


float ColTab::Mapper::getPosition( const Interval<float>& rg,
				SeqUseMode mode, int nrsegs, float val )
{
    const float width = rg.width( false );
    if ( mIsZero(width,mDefEps) )
	return 0;

    float pos = (val-rg.start) / width;
    if ( nrsegs > 0 )
    {
	pos *= nrsegs;
	pos = (0.5f + ((int)pos)) / nrsegs;
    }

    return seqPos4RelPos( mode, pos );
}


float ColTab::Mapper::seqPos4RelPos( SeqUseMode mode, float pos )
{
    if ( pos > 1.f )
	pos = 1.f;
    else if ( pos < 0.f )
	pos = 0.f;

    if ( isCyclic(mode) )
    {
	pos *= 2;
	if ( pos > 1.0f )
	    pos = 2.0f - pos;
    }

    if ( isFlipped(mode) )
	pos = 1.0f - pos;

    return pos;
}


int ColTab::Mapper::snappedPosition( const ColTab::Mapper* mapper,
	float v, int nrsteps, int udfval )
{
    if ( mIsUdf(v) )
	return udfval;

    float ret = mapper ? mapper->position( v ) : v;
    ret *= nrsteps;
    if ( ret > nrsteps - 0.9f )
	ret = nrsteps - 0.9f;
    else if ( ret < 0 )
	ret = 0;

    return (int)ret;
}


void ColTab::Mapper::setRange( Interval<float> rg )
{
    setup_->setRange( rg );
    setup_->setIsFixed( true );
}


void ColTab::Mapper::setData( const ValueSeries<float>* vs, od_int64 sz,
			      TaskRunner* tskr )
{
    vs_ = vs; vssz_ = sz;
    update( true, tskr );
}


struct SymmetryCalc : public ParallelTask
{
SymmetryCalc( const ValueSeries<float>& vs, od_int64 sz )
    : sz_(sz)
    , vs_(vs)
    , above0_(0)
    , below0_(0)
    , lock_(true)
{
    //TODO should be done calculating the Galton number
    // e.g. https://brownmath.com/stat/shape.htm
}

od_int64 nrIterations() const { return sz_; }

bool doWork( od_int64 start, od_int64 stop, int )
{
    od_int64 above0 = 0;
    od_int64 below0 = 0;

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( mIsUdf(vs_[idx]) ) continue;
	if ( vs_[idx] < 0 ) below0++;
	else if ( vs_[idx] > 0 ) above0++;
    }

    Threads::Locker lock( lock_ );
    above0_ += above0;
    below0_ += below0;

    return true;
}


bool isSymmAroundZero() const
{
    od_int64 max = mMAX( above0_, below0_ );
    od_int64 min = mMIN( above0_, below0_ );
    if ( max==0 || min==0 ) return false;

    return max/min - 1 < 0.05;
}


uiString nrDoneText() const
{
    return uiStrings::sDone();
}

    od_int64			sz_;
    od_int64			above0_;
    od_int64			below0_;
    Threads::Lock		lock_;
    const ValueSeries<float>&	vs_;
};


void ColTab::Mapper::update( bool full, TaskRunner* tskr )
{
    if ( setup_->isFixed() || !vs_ || vssz_ < 1 )
	return;

    // using notify blockers when setting caching data (range, sym mid val)

    if ( vssz_ == 1 )
    {
	const float rgstart = (mIsUdf(vs_->value(0)) ? 0.f : vs_->value(0))-1.f;
	const float rgstop = rgstart + 2.f;
	setup_->setRange( Interval<float>( rgstart, rgstop ) );
	return;
    }

    if ( full || clipper_.isEmpty() )
    {
	clipper_.reset();
	clipper_.setApproxNrValues( vssz_, cMaxClipperPts ) ;
	clipper_.putData( *vs_, vssz_ );
	clipper_.fullSort();

	if ( setup_->guessSymmetry() )
	{
	    SymmetryCalc symmcalc( *vs_, vssz_ );
	    TaskRunner::execute( tskr, symmcalc );
	    setup_->setSymMidVal( symmcalc.isSymmAroundZero()
					? 0.f : mUdf(float) );
	}
    }

    Interval<float> intv( -1, 1 );
    const float symmidval = setup_->symMidVal();
    Interval<float> cliprate( setup_->clipRate() );
    if ( mIsUdf(symmidval) )
	clipper_.getRange( cliprate.start, cliprate.stop, intv );
    else
	clipper_.getSymmetricRange( cliprate.start, symmidval, intv );

    if ( mIsZero(intv.width(),mDefEps) )
	intv.widen( 1.f );

    setup_->setRange( intv );
}
