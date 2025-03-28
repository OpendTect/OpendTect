/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "trckeysampling.h"

#include "iopar.h"
#include "jsonkeystrs.h"
#include "keystrs.h"
#include "odjson.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom.h"

#include <math.h>

mStartAllowDeprecatedSection

TrcKeySampling::TrcKeySampling()
    : survid_( OD::GeomSynth )
    , start( start_ )
    , stop( stop_ )
    , step( step_ )
{
    init( true );
}


TrcKeySampling::TrcKeySampling( const TrcKeySampling& tks )
    : start( start_ )
    , stop( stop_ )
    , step( step_ )
{
    *this = tks;
}


TrcKeySampling::TrcKeySampling( const Pos::GeomID& gid )
    : survid_( OD::GeomSynth )
    , start( start_ )
    , stop( stop_ )
    , step( step_ )
{ init( gid ); }


TrcKeySampling::TrcKeySampling( bool settosi )
    : survid_( OD::GeomSynth )
    , start( start_ )
    , stop( stop_ )
    , step( step_ )
{ init( settosi ); }

mStopAllowDeprecatedSection


TrcKeySampling TrcKeySampling::getSynth( const Interval<int>* trcrg )
{
    TrcKeySampling tks;
    const TrcKey tk = TrcKey::getSynth( -1 );
    tks.setGeomID( tk.geomID() );
    if ( trcrg )
	tks.setTrcRange( *trcrg );
    else
	tks.setTrcRange( StepInterval<int>::udf() );

    return tks;
}


TrcKeySampling::~TrcKeySampling()
{}


bool TrcKeySampling::isUdf() const
{
    return lineRange().isUdf() && trcRange().isUdf();
}


Pos::GeomID TrcKeySampling::getGeomID() const
{
    return Pos::GeomID(is2D() || isSynthetic() ? start_.lineNr() : survid_);
}


void TrcKeySampling::setGeomID( const Pos::GeomID& geomid )
{
    const Pos::GeomID oldid = getGeomID();
    if ( geomid.is2D() || geomid.isSynth() )
    {
	start_.lineNr() = stop_.lineNr() = geomid.asInt();
	step_.lineNr() = 1;
    }

    survid_ = geomid.geomSystem();
    if ( geomid != oldid )
    {
	if ( geomid.is3D() )
	    setLineRange( StepInterval<int>::udf() );

	setTrcRange( StepInterval<int>::udf() );
    }
}


bool TrcKeySampling::init( const Pos::GeomID& gid )
{
    if ( Survey::isSynthetic(gid) )
    {
	setGeomID( gid );
	return true;
    }

    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry( gid );
    if ( !geom )
	return false;

    survid_ = geom->geomSystem();
    (*this) = geom->sampling().hsamp_;
    return true;
}


void TrcKeySampling::init( const TrcKey& tk )
{
    survid_ = tk.geomSystem();
    start_.lineNr() = stop_.lineNr() = tk.lineNr();
    start_.trcNr() = stop_.trcNr() = tk.trcNr();
}


void TrcKeySampling::init( bool tosi )
{
    if ( tosi )
    {
	init( Pos::GeomID(OD::Geom3D) );
    }
    else
    {
	survid_ = OD::GeomSynth;
	start_.lineNr() = start_.trcNr() = stop_.lineNr() =
		stop_.trcNr() = mUdf(int);
	step_.lineNr() = step_.trcNr() = 1;
    }
}


int TrcKeySampling::lineIdxFromGlobal( od_int64 globalidx ) const
{
    const int nrtrcs = nrTrcs();
    if ( !nrtrcs || globalidx>=totalNr() )
	return -1;

    return int(globalidx/nrtrcs);
}


int TrcKeySampling::trcIdxFromGlobal( od_int64 globalidx ) const
{
    const int nrtrcs = nrTrcs();
    if ( !nrtrcs || globalidx>=totalNr() )
	return -1;

    return int(globalidx%nrtrcs);
}


BinID TrcKeySampling::atIndex( od_int64 globalidx ) const
{
    const int nrtrcs = nrTrcs();
    if ( !nrtrcs )
	return BinID::udf();

    const int lineidx = lineIdxFromGlobal( globalidx );
    const int trcidx = trcIdxFromGlobal( globalidx );
    if ( lineidx<0 || trcidx<0 )
	return BinID::udf();

    return atIndex( lineidx, trcidx );
}


TrcKey TrcKeySampling::toTrcKey( const Coord& pos, float* distance ) const
{
    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry( getGeomID());
    return geom ? geom->nearestTrace( pos, distance ) : TrcKey::udf();
}


Coord TrcKeySampling::toCoord( const BinID& bid ) const
{
    const Survey::Geometry& geom = is2D()
		     ? (Survey::Geometry&)Survey::GM().get2D( getGeomID() )
		     : Survey::Geometry::default3D();
    return geom.toCoord( bid );
}


TrcKey TrcKeySampling::center() const
{
    const Pos::IdxPair bid( inlRange().snappedCenter(),
			    crlRange().snappedCenter() );
    return TrcKey( survid_, bid );
}


void TrcKeySampling::set2DDef()
{
    start_.lineNr() = start_.trcNr() = 0;
    stop_.lineNr() = stop_.trcNr() = mUdf(int);
    step_.lineNr() = step_.trcNr() = 1;
    survid_ = OD::Geom2D;
}


TrcKeySampling& TrcKeySampling::set( const Interval<int>& inlrg,
			       const Interval<int>& crlrg )
{
    setInlRange( inlrg );
    setCrlRange( crlrg );
    return *this;
}


TrcKeySampling& TrcKeySampling::set( const Pos::GeomID& geomid,
				     const Interval<int>& trcnrrg )
{
    setGeomID( geomid );
    setTrcRange( trcnrrg );
    return *this;
}


TrcKeySampling& TrcKeySampling::set( const TrcKey& tk )
{
    start_ = stop_ = tk.position();
    setGeomID( tk.geomID() );
    return *this;
}


void TrcKeySampling::setLineRange( const Interval<int>& inlrg )
{
    start_.lineNr() = inlrg.start_; stop_.lineNr() = inlrg.stop_;
    if ( !inlrg.hasStep() )
	return;

    mDynamicCastGet(const StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	step_.lineNr() = inlsrg->step_;
}


void TrcKeySampling::setTrcRange( const Interval<int>& crlrg )
{
    start_.trcNr() = crlrg.start_; stop_.trcNr() = crlrg.stop_;
    if ( !crlrg.hasStep() )
	return;

    mDynamicCastGet(const StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	step_.trcNr() = crlsrg->step_;
}


StepInterval<int> TrcKeySampling::lineRange() const
{ return StepInterval<int>( start_.lineNr(), stop_.lineNr(), step_.lineNr() ); }


StepInterval<int> TrcKeySampling::trcRange() const
{ return StepInterval<int>( start_.trcNr(), stop_.trcNr(), step_.trcNr() ); }


float TrcKeySampling::lineDistance() const
{
    BinID bid( start_ );
    Coord startpos( toCoord( bid ) );
    bid.inl() += step_.inl();
    Coord stoppos( toCoord( bid ) );

    return mCast(float, stoppos.distTo(startpos) );
}


float TrcKeySampling::trcDistance() const
{
    BinID bid( start_ );
    Coord startpos( toCoord( bid ) );
    bid.crl() += step_.crl();
    Coord stoppos( toCoord( bid ) );

    return mCast(float, stoppos.distTo(startpos) );
}


bool TrcKeySampling::includes( const TrcKeySampling& tks,
			       bool ignoresteps ) const
{
    if ( ignoresteps )
	return tks.start_.lineNr() >= start_.lineNr() &&
	       tks.stop_.lineNr() <= stop_.lineNr() &&
	       tks.start_.trcNr() >= start_.trcNr() &&
	       tks.stop_.trcNr() <= stop_.trcNr();

    return includes(tks.start_) && includes(tks.stop_)
	&& step_.lineNr() && !(tks.step_.lineNr() % step_.lineNr())
	&& step_.trcNr() && !(tks.step_.trcNr() % step_.trcNr());
}


void TrcKeySampling::includeLine( Pos::LineID lid )
{
    /*
#ifdef __debug__
    if ( survid_ == OD::GeomSynth )
	pErrMsg("survid_ is not set");
#endif
    */
    if ( mIsUdf(start_.lineNr()) || mIsUdf(stop_.lineNr()) || nrLines()<1 )
	start_.lineNr() = stop_.lineNr() = lid;
    else
    {
	start_.lineNr() = mMIN( start_.lineNr(), lid );
	stop_.lineNr() = mMAX( stop_.lineNr(), lid );
    }
}


void TrcKeySampling::includeTrc( Pos::TraceID trcid )
{
    /*
#ifdef __debug__
    if ( survid_ == OD::GeomSynth )
	pErrMsg("survid_ is not set");
#endif
    */

    if ( mIsUdf(start_.trcNr()) || mIsUdf(stop_.trcNr()) || nrTrcs()<1 )
	start_.trcNr() = stop_.trcNr() = trcid;
    else
    {
	start_.trcNr() = mMIN( start_.trcNr(), trcid );
	stop_.trcNr() = mMAX( stop_.trcNr(), trcid );
    }
}


void TrcKeySampling::include( const TrcKeySampling& tks, bool ignoresteps )
{
    if ( ignoresteps )
    {
	include( TrcKey( tks.survid_, (const Pos::IdxPair&)tks.start_) );
	include( TrcKey( tks.survid_, (const Pos::IdxPair&)tks.stop_ ) );
	return;
    }

    TrcKeySampling temp( *this );
    temp.include( TrcKey( tks.survid_, (const Pos::IdxPair&)tks.start_) );
    temp.include( TrcKey( tks.survid_, (const Pos::IdxPair&)tks.stop_ ) );

#define mHandleIC( ic ) \
    const int newstart_##ic = temp.start_.ic(); \
    const int newstop_##ic = temp.stop_.ic(); \
    int offset##ic = mIsUdf(start_.ic()) || mIsUdf(tks.start_.ic()) ? 0 \
	: ( start_.ic() != newstart_##ic ? start_.ic() - newstart_##ic \
				     : tks.start_.ic() - newstart_##ic ); \
    step_.ic() = Math::HCFOf( step_.ic(), tks.step_.ic() ); \
    if ( offset##ic ) step_.ic() = Math::HCFOf( step_.ic(), offset##ic ); \
    start_.ic() = newstart_##ic; stop_.ic() = newstop_##ic

    mHandleIC(inl);
    mHandleIC(crl);
}


void TrcKeySampling::get( Interval<int>& inlrg, Interval<int>& crlrg ) const
{
    inlrg.start_ = start_.lineNr();
    inlrg.stop_ = stop_.lineNr();
    mDynamicCastGet(StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	inlsrg->step_ = step_.lineNr();
    crlrg.start_ = start_.trcNr(); crlrg.stop_ = stop_.trcNr();
    mDynamicCastGet(StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	crlsrg->step_ = step_.trcNr();
}


TrcKeySampling TrcKeySampling::getLineChunk( int nrchunks, int chunknr ) const
{
    TrcKeySampling ret( *this );
    if ( nrchunks < 1 )
	return ret;

    int nrlines = (stop_.lineNr() - start_.lineNr()) / step_.lineNr();
    float fnrlinesperchunk = ((float)nrlines) / nrchunks;
    if ( chunknr > 0 )
    {
	const float fnrsteps = fnrlinesperchunk * chunknr;
	ret.start_.lineNr() += mNINT32(fnrsteps) * step_.lineNr();
    }
    if ( chunknr < nrchunks-1 )
    {
	// return one step before the next start
	const float fnrsteps = fnrlinesperchunk * (chunknr + 1);
	ret.stop_.lineNr() = start_.lineNr()
			   + mNINT32(fnrsteps-1) * step_.lineNr();
	if ( ret.stop_.lineNr() > stop_.lineNr() )
	    ret.stop_.lineNr() = stop_.lineNr();
    }

    return ret;
}


bool TrcKeySampling::isDefined() const
{
    return !mIsUdf(start_.lineNr()) && !mIsUdf(start_.trcNr()) &&
	   !mIsUdf(stop_.lineNr()) && !mIsUdf(stop_.trcNr()) &&
	   !mIsUdf(step_.lineNr()) && !mIsUdf(step_.trcNr());
}


TrcKeySampling& TrcKeySampling::operator=(const TrcKeySampling& hrg)
{
    survid_ = hrg.survid_;
    start_ = hrg.start_;
    stop_ = hrg.stop_;
    step_ = hrg.step_;

    return *this;
}


bool TrcKeySampling::operator==( const TrcKeySampling& tks ) const
{
    return tks.start_==start_ && tks.stop_==stop_ &&
	   tks.step_==step_ && tks.survid_==survid_;
}


bool TrcKeySampling::operator!=( const TrcKeySampling& tks ) const
{ return !(*this==tks); }


od_int64 TrcKeySampling::totalNr() const
{ return ((od_int64) nrLines()) * nrTrcs(); }


bool TrcKeySampling::isEmpty() const
{ return nrLines() < 1 || nrTrcs() < 1; }


void TrcKeySampling::limitTo( const TrcKeySampling& tks, bool ignoresteps )
{
    if ( !overlaps(tks,true) )
    {
	init( false );
	return;
    }

    StepInterval<int> inlrg( lineRange() );
    StepInterval<int> crlrg( trcRange() );
    if ( ignoresteps )
    {
	((SampleGate&)inlrg).limitTo( tks.lineRange() );
	((SampleGate&)crlrg).limitTo( tks.trcRange() );
    }
    else
    {
	inlrg.limitTo( tks.lineRange() );
	crlrg.limitTo( tks.trcRange() );
    }

    setLineRange( inlrg );
    setTrcRange( crlrg );
}


# define mAdjustIf(v1,op,v2) \
      if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;

void TrcKeySampling::limitToWithUdf( const TrcKeySampling& h )
{
    TrcKeySampling tks( h ); tks.normalize();
    normalize();

    mAdjustIf(start_.lineNr(),<,tks.start_.lineNr());
    mAdjustIf(start_.trcNr(),<,tks.start_.trcNr());
    mAdjustIf(stop_.lineNr(),>,tks.stop_.lineNr());
    mAdjustIf(stop_.trcNr(),>,tks.stop_.trcNr());
    mAdjustIf(step_.lineNr(),<,tks.step_.lineNr());
    mAdjustIf(step_.trcNr(),<,tks.step_.trcNr());
}


#define mSnapStop( start, stop, step, eps ) \
    stop = start + step * mCast( int, (stop-start+eps)/step );

#define mApproach( diff, var, assignoper, step ) \
    if ( diff>0 ) \
	var assignoper step * mCast( int, (diff)/step );

void TrcKeySampling::shrinkTo( const TrcKeySampling& innertks )
{
    normalize();
    TrcKeySampling tks( innertks );
    tks.normalize();

    mSnapStop( start_.inl(), stop_.inl(), step_.inl(), 0 );
    mSnapStop( start_.crl(), stop_.crl(), step_.crl(), 0 );

    mApproach( tks.start_.inl()-start_.inl(), start_.inl(), +=, step_.inl() );
    mApproach( stop_.inl() - tks.stop_.inl(),  stop_.inl(), -=, step_.inl() );
    mApproach( tks.start_.crl()-start_.crl(), start_.crl(), +=, step_.crl() );
    mApproach( stop_.crl() - tks.stop_.crl(),  stop_.crl(), -=, step_.crl() );
}


void TrcKeySampling::growTo( const TrcKeySampling& outertks )
{
    normalize();
    TrcKeySampling tks( outertks );
    tks.normalize();

    mSnapStop( start_.inl(), stop_.inl(), step_.inl(), 0 );
    mSnapStop( start_.crl(), stop_.crl(), step_.crl(), 0 );

    mApproach( start_.inl()-tks.start_.inl(), start_.inl(), -=, step_.inl() );
    mApproach( tks.stop_.inl() - stop_.inl(),  stop_.inl(), +=, step_.inl() );
    mApproach( start_.crl()-tks.start_.crl(), start_.crl(), -=, step_.crl() );
    mApproach( tks.stop_.crl() - stop_.crl(),  stop_.crl(), +=, step_.crl() );
}


void TrcKeySampling::expand( int nrlines, int nrtrcs )
{
    if ( !mIsUdf(nrlines) && !is2D() )
    {
	start_.lineNr() -= nrlines*step_.lineNr();
	stop_.lineNr() += nrlines*step_.lineNr();
    }

    if ( !mIsUdf(nrtrcs) )
    {
	start_.trcNr() -= nrtrcs*step_.trcNr();
	stop_.trcNr() += nrtrcs*step_.trcNr();
    }
}


static bool getRange( const IOPar& par, const char* key, int& start_,
		      int& stop_, int& step_ )
{
    const BufferString parval = par[key];
    if ( parval.isEmpty() )
	return false;

    FileMultiString fms( parval );
    if ( fms.size() > 0 )
	start_ = fms.getIValue( 0 );

    if ( fms.size() > 1 )
	stop_ = fms.getIValue( 1 );

    if ( fms.size() > 2 )
	step_ = fms.getIValue( 2 );

    return true;
}


bool TrcKeySampling::usePar( const IOPar& pars )
{
    bool inlok = getRange( pars, sKey::InlRange(),
			   start_.lineNr(), stop_.lineNr(), step_.lineNr() );
    if ( !inlok )
    {
	inlok = pars.get( sKey::FirstInl(), start_.lineNr() );
	inlok = pars.get( sKey::LastInl(), stop_.lineNr() ) || inlok;
	pars.get( sKey::StepInl(), step_.lineNr() );
    }

    bool crlok = getRange( pars, sKey::CrlRange(),
			   start_.trcNr(), stop_.trcNr(), step_.trcNr() );
    if ( !crlok )
    {
	crlok = pars.get( sKey::FirstCrl(), start_.trcNr() );
	crlok = pars.get( sKey::LastCrl(), stop_.trcNr() ) || crlok;
	pars.get( sKey::StepCrl(), step_.trcNr() );
    }

    if ( !pars.get(sKey::SurveyID(),survid_) )
	survid_ = OD::Geom3D;

    if ( inlok && crlok )
	return true;

    PtrMan<IOPar> subpars = pars.subselect( IOPar::compKey(sKey::Line(),0) );
    if ( !subpars ) return false;

    bool trcrgok = getRange( *subpars, sKey::TrcRange(),
			     start_.trcNr(), stop_.trcNr(), step_.trcNr() );
    if ( !trcrgok )
    {
	trcrgok = subpars->get( sKey::FirstTrc(), start_.trcNr() );
	trcrgok = subpars->get( sKey::LastTrc(), stop_.trcNr() ) || trcrgok;
	subpars->get( sKey::StepCrl(), step_.trcNr() );
    }

    if ( trcrgok )
    {
	Pos::GeomID geomid;
	subpars->get( sKey::GeomID(), geomid );
	start_.lineNr() = stop_.lineNr() = geomid.asInt();
	step_.lineNr() = 1;
	survid_ = OD::Geom2D;
    }

    return trcrgok;
}


void TrcKeySampling::fillPar( IOPar& pars ) const
{
    const Pos::GeomID geomid = getGeomID();
    if ( is2D() )
    {
	IOPar tmppar;
	tmppar.set( sKey::GeomID(), geomid );
	tmppar.set( sKey::FirstTrc(), start_.trcNr() );
	tmppar.set( sKey::LastTrc() , stop_.trcNr() );
	tmppar.set( sKey::StepCrl(), step_.trcNr() );
	tmppar.set( sKey::SurveyID(), survid_ );
	pars.mergeComp( tmppar, IOPar::compKey( sKey::Line(), 0 ) );
    }
    else
    {
	pars.set( sKey::FirstInl(), start_.lineNr() );
	pars.set( sKey::FirstCrl(), start_.trcNr() );
	pars.set( sKey::LastInl(), stop_.lineNr() );
	pars.set( sKey::LastCrl(), stop_.trcNr() );
	pars.set( sKey::StepInl(), step_.lineNr() );
	pars.set( sKey::StepCrl(), step_.trcNr() );
	pars.set( sKey::GeomID(), geomid );
	pars.set( sKey::SurveyID(), survid_ );
    }
}


void TrcKeySampling::removeInfo( IOPar& par )
{
    par.removeWithKey( sKey::FirstInl() );
    par.removeWithKey( sKey::FirstCrl() );
    par.removeWithKey( sKey::LastInl() );
    par.removeWithKey( sKey::LastCrl() );
    par.removeWithKey( sKey::StepInl() );
    par.removeWithKey( sKey::StepCrl() );
}


void TrcKeySampling::fillJSON( OD::JSON::Object& obj ) const
{
    if ( is2D() )
    {
	OD::JSON::Object localobj;
	const StepInterval<Index_Type> trcrng(
	    start_.trcNr(), stop_.trcNr(), step_.trcNr() );
	obj.set( sJSONKey::TrcRange(), trcrng );
	obj.set( sJSONKey::GeomID(), start_.lineNr() );
    }
    else
    {
	const StepInterval<Index_Type> inlrng( start_.lineNr(), stop_.lineNr(),
	    step_.lineNr() );
	obj.set( sJSONKey::InlRange(), inlrng );
	const StepInterval<Index_Type> crlrng( start_.trcNr(), stop_.trcNr(),
	    step_.trcNr() );
	obj.set( sJSONKey::CrlRange(), crlrng );
    }

    obj.set( sJSONKey::SurveyID(), survid_ );
}


bool TrcKeySampling::useJSON( const OD::JSON::Object& obj )
{
    StepInterval<Index_Type> rng;
    if ( is2D() )
    {
	obj.get( sJSONKey::TrcRange(), rng );
	start_.trcNr() = rng.start_;
	stop_.trcNr() = rng.stop_;
	step_.trcNr() = rng.step_;

	start_.lineNr() = obj.getIntValue( sJSONKey::GeomID() );
    }
    else
    {
	obj.get( sJSONKey::InlRange(), rng );
	start_.lineNr() = rng.start_;
	stop_.lineNr() = rng.stop_;
	step_.lineNr() = rng.step_;

	obj.get( sJSONKey::CrlRange(), rng );
	start_.trcNr() = rng.start_;
	stop_.trcNr() = rng.stop_;
	step_.trcNr() = rng.step_;
    }

    const int survid = obj.getIntValue( sJSONKey::SurveyID() );
    if ( mIsUdf(survid) )
	survid_ = OD::Geom3D; //Legacy
    else if ( survid >= OD::GeomSynth && survid <= OD::Geom2D )
	survid_ = OD::GeomSystem(survid);
    else
	survid_ = OD::Geom3D;

    return true;
}


int TrcKeySampling::nrLines() const
{
    if ( (mIsUdf(start_.lineNr()) && mIsUdf(stop_.lineNr())) )
	return 0;

    if ( !step_.lineNr() )
	return 0;

    if ( start_.lineNr()==stop_.lineNr() )
	return 1;

    int ret = inlIdx( stop_.lineNr() );
    return ret < 0 ? 1 - ret : ret + 1;
}


int TrcKeySampling::nrTrcs() const
{
    if ( (mIsUdf(start_.trcNr()) && mIsUdf(stop_.trcNr())) )
	return 0;

    if ( !step_.trcNr() )
	return 0;

    if ( start_.trcNr()==stop_.trcNr() )
	return 1;

    int ret = crlIdx( stop_.trcNr() );
    return ret < 0 ? 1 - ret : ret + 1;
}


static bool intersect(	int start_1, int stop_1, int step_1,
			int start_2, int stop_2, int step_2,
			int& outstart_, int& outstop_, int& outstep_ )
{
    if ( stop_1 < start_2 || start_1 > stop_2 )
	return false;

    // Determine step_. Only accept reasonable step_ differences
    outstep_ = step_2 > step_1 ? step_2 : step_1;
    int lostep_ = step_2 > step_1 ? step_1 : step_2;
    if ( !lostep_ || outstep_%lostep_ ) return false;

    // Snap start_
    outstart_ = start_1 < start_2 ? start_2 : start_1;
    while ( (outstart_-start_1) % step_1 )
	outstart_ += lostep_;
    while ( (outstart_-start_2) % step_2 )
	outstart_ += lostep_;

    // Snap stop_
    outstop_ = stop_1 > stop_2 ? stop_2 : stop_1;
    int nrstep_s = (outstop_ - outstart_) / outstep_;
    outstop_ = outstart_ + nrstep_s * outstep_;
    return outstop_ >= outstart_;
}

#define Eps 2e-5

inline bool IsZero( float f, float eps=Eps )
{
    return f > -eps && f < eps;
}


static inline bool inSeries( float v, float start_, float step_ )
{
    float fdiff = (start_ - v) / step_;
    int idiff = mNINT32( fdiff );
    fdiff -= (float)idiff;
    return IsZero( fdiff, 1e-3 );
}


static bool intersectF( float start_1, float stop_1, float step_1,
			float start_2, float stop_2, float step_2,
			float& outstart_, float& outstop_, float& outstep_ )
{
    if ( stop_1-start_2 < mDefEps || start_1-stop_2 > mDefEps )
	return false;

    outstep_ = step_2 > step_1 ? step_2 : step_1;
    float lostep_ = step_2 > step_1 ? step_1 : step_2;
    if ( IsZero(lostep_) ) return false;

    // See if start_s are compatible
    if ( !inSeries(start_1,start_2,lostep_) )
	return false;

    // Only accept reasonable step_ differences
    int ifac = 1;
    for ( ; ifac<2001; ifac++ )
    {
	float stp = ifac * lostep_;
	if ( IsZero(stp-outstep_) ) break;
	else if ( ifac == 2000 ) return false;
    }

    outstart_ = start_1 < start_2 ? start_2 : start_1;
    while ( !inSeries(outstart_,start_1,step_1)
	 || !inSeries(outstart_,start_2,step_2) )
	outstart_ += lostep_;

    // Snap stop_
    outstop_ = stop_1 > stop_2 ? stop_2 : stop_1;
    int nrstep_s = (int)( (outstop_ - outstart_ + Eps) / outstep_ );
    outstop_ = outstart_ + nrstep_s * outstep_;
    return (outstop_-outstart_) > Eps;
}


static bool intersectIgnoreSteps( int start_1, int stop_1,
				  int start_2, int stop_2,
				  int& outstart_, int& outstop_ )
{
    if ( stop_1 < start_2 || start_1 > stop_2 )
	return false;

    outstart_ = start_1 < start_2 ? start_2 : start_1;
    outstop_ = stop_1 > stop_2 ? stop_2 : stop_1;
    return outstop_ >= outstart_;
}


bool TrcKeySampling::overlaps( const TrcKeySampling& oth,
			       bool ignoresteps ) const
{
    TrcKeySampling intertks;
    return getInterSection( oth, intertks, ignoresteps );
}


bool TrcKeySampling::getInterSection( const TrcKeySampling& tks,
				   TrcKeySampling& out,
				   bool ignoresteps ) const
{
    TrcKeySampling tks1( tks ); tks1.normalize();
    TrcKeySampling tks2( *this ); tks2.normalize();

    if ( ignoresteps )
    {
	const bool success =
	    intersectIgnoreSteps( tks1.start_.inl(), tks1.stop_.inl(),
				  tks2.start_.inl(), tks2.stop_.inl(),
				  out.start_.inl(), out.stop_.inl() ) &&
	    intersectIgnoreSteps( tks1.start_.crl(), tks1.stop_.crl(),
				  tks2.start_.crl(), tks2.stop_.crl(),
				  out.start_.crl(), out.stop_.crl() );
	return success;
    }

    const StepInterval<int> linerg1( tks1.lineRange() );
    const StepInterval<int> linerg2( tks2.lineRange() );
    const StepInterval<int> trcrg1( tks1.trcRange() );
    const StepInterval<int> trcrg2( tks2.trcRange() );
    StepInterval<int> linergout, trcrgout;

    const bool success = Pos::intersect( linerg1, linerg2, linergout ) &&
			 Pos::intersect( trcrg1, trcrg2, trcrgout );
    if ( success )
    {
	out.setLineRange( linergout );
	out.setTrcRange( trcrgout );
    }

    return success;
}


static void getNearestIdx( Pos::Index_Type& diridx, Pos::Index_Type step_ )
{
    const Pos::Index_Type rest = diridx % step_;
    if ( !rest )
	return;

    if ( rest > step_/2 )
	diridx += step_ - rest;
    else
	diridx -= rest;
}


BinID TrcKeySampling::getNearest( const BinID& bid ) const
{
    BinID relbid( bid.first - start_.first,
		  bid.second - start_.second );

    BinID ret( 0, 0 );

    if ( is3D(survid_) )
    {
	if ( step_.first )
	    getNearestIdx( relbid.first, step_.first );

	ret.first = start_.first + relbid.first;

	if ( ret.first < start_.first )
	    ret.first = start_.first;
	else if ( ret.first > stop_.first )
	    ret.first = stop_.first;
    }

    if ( step_.second )
	getNearestIdx( relbid.second, step_.second );

    ret.second = start_.second + relbid.second;

    if ( ret.second < start_.second )
	ret.second = start_.second;
    else if ( ret.second > stop_.second )
	ret.second = stop_.second;

    return ret;
}


TrcKey TrcKeySampling::getNearest( const TrcKey& trckey ) const
{
    if ( trckey.geomSystem() != survid_ )
	return TrcKey::udf();

    const BinID bid = getNearest(trckey.position());
    return TrcKey( survid_, (const Pos::IdxPair&)bid );
}


void TrcKeySampling::snapToSurvey()
{
    SI().snap( start_, BinID(-1,-1) );
    SI().snap( stop_, BinID(1,1) );
}


void TrcKeySampling::toString( BufferString& str ) const
{
    str.add( "In-line range: " ).add( start_.lineNr() ).add( " - " )
	.add( stop_.lineNr() ).add( " [" ).add( step_.lineNr() ).add( "]\n" );
    str.add( "Cross-line range: " ).add( start_.trcNr() ).add( " - " )
	.add( stop_.trcNr() ).add( " [" ).add( step_.trcNr() ).add( "]" );
}


void TrcKeySampling::fillInfoSet( StringPairSet& infoset ) const
{
    if ( !is2D() )
    {
	uiString str = toUiString("%1 - %2 [%3]; Total: %4")
			.arg(start_.inl())
			.arg(stop_.inl())
			.arg(step_.inl())
			.arg(nrInl());
	infoset.add( sKey::InlRange(), str.getFullString() );
    }

    uiString str = toUiString("%1 - %2 [%3]; Total: %4")
			.arg(start_.crl())
			.arg(stop_.crl())
			.arg(step_.crl())
			.arg(nrCrl());
    infoset.add( is2D() ? sKey::TrcRange() : sKey::CrlRange(),
		 str.getFullString() );
}


void TrcKeySampling::getRandomSet( int nr, TypeSet<TrcKey>& res ) const
{
    if ( nr > totalNr() )
	nr = (int) totalNr();

    while ( nr )
    {
	//TODO Only compatible with 3D data, adapt for the other types
	const Pos::IdxPair bid( lineRange().start_ + std::rand() % nrLines(),
				trcRange().start_ + std::rand() % nrTrcs() );
	const TrcKey trckey( survid_, bid );
	if ( includes(trckey) && res.addIfNew(trckey) )
	    nr--;
    }
}


BinID TrcKeySampling::atIndex( int i0, int i1 ) const
{
#ifdef __debug__
    if ( survid_ == OD::GeomSynth )
	pErrMsg("survid_ is not set");
#endif
    const Pos::LineID linenr = is3D( survid_ )
			     ? start_.lineNr() + i0*step_.lineNr()
			     : start_.lineNr();
    const Pos::TraceID trcnr = start_.trcNr() + i1*step_.trcNr();

    return BinID(linenr,trcnr);
}


TrcKey TrcKeySampling::trcKeyAt( int i0, int i1 ) const
{
    const BinID res = atIndex( i0, i1 );
    if ( res.isUdf() )
	return TrcKey::udf();

    return TrcKey( survid_, (const Pos::IdxPair&)res );
}


TrcKey TrcKeySampling::trcKeyAt( od_int64 globalidx ) const
{
    const BinID res = atIndex( globalidx );
    if ( res.isUdf() )
	return TrcKey::udf();

    return TrcKey( survid_, (const Pos::IdxPair&)res );
}


void TrcKeySampling::neighbors( od_int64 globalidx,
				TypeSet<od_int64>& nbs ) const
{
    nbs.erase();
    const int nrtrcs = nrTrcs(); const int nrlines = nrLines();
    if ( globalidx > nrtrcs )
	nbs += globalidx-nrtrcs;
    if ( globalidx < nrtrcs*(nrlines-1) )
	nbs += globalidx+nrtrcs;
    if ( globalidx%nrtrcs != 0 )
	nbs += globalidx-1;
    if ( (globalidx+1)%nrtrcs != 0 )
	nbs += globalidx+1;
}


void TrcKeySampling::neighbors( const TrcKey& tk, TypeSet<TrcKey>& nbs ) const
{
    TypeSet<od_int64> idxs; neighbors( globalIdx(tk), idxs );
    for ( int idx=0; idx<idxs.size(); idx++ )
	nbs += trcKeyAt( idxs[idx] );
}


void TrcKeySampling::include( const TrcKey& trckey )
{
    if ( survid_ == OD::GeomSynth )
	survid_ = trckey.geomSystem();
#ifdef __debug__
    else if ( survid_ != trckey.geomSystem() )
	pErrMsg("OD::GeomSystem should be the same");
#endif
    includeLine( trckey.lineNr() );
    includeTrc( trckey.trcNr() );
}


od_int64 TrcKeySampling::globalIdx( const TrcKey& trk ) const
{
    if ( trk.geomSystem() != survid_ )
	return -1;

    return globalIdx( trk.position() );
}


od_int64 TrcKeySampling::globalIdx( const BinID& bid ) const
{
    return lineIdx( bid.lineNr() ) * nrTrcs() + trcIdx(bid.trcNr() );
}


bool TrcKeySampling::lineOK( Pos::LineID lid ) const
{
    return lineOK( lid, false );
}


bool TrcKeySampling::trcOK( Pos::TraceID tid ) const
{
    return trcOK( tid, false );
}


bool TrcKeySampling::lineOK( Pos::LineID lid, bool ignoresteps ) const
{
    const bool linenrok = lid >= start_.lineNr() && lid <= stop_.lineNr();
    return ignoresteps ? linenrok : linenrok && ( step_.lineNr() ?
	!( ( lid-start_.lineNr() ) % step_.lineNr() ) : lid==start_.lineNr() );

}


bool TrcKeySampling::trcOK( Pos::TraceID tid, bool ignoresteps ) const
{
    const bool trcnrok = tid >= start_.trcNr() && tid <= stop_.trcNr();
    return ignoresteps ? trcnrok : trcnrok && ( step_.crl() ?
	!( ( tid-start_.trcNr() ) % step_.trcNr() ) : tid==start_.trcNr() );
}


bool TrcKeySampling::includes( const TrcKey& tk ) const
{
    return includes( tk, false );
}


bool TrcKeySampling::includes( const TrcKey& tk, bool ignoresteps ) const
{
    return survid_==tk.geomSystem() && lineOK(tk.lineNr(),ignoresteps)
				    && trcOK(tk.trcNr(),ignoresteps);
}


BinID TrcKeySampling::corner( int idx ) const
{
    if ( idx==1 )
	return BinID( stop_.inl(), start_.crl() );
    if ( idx==2 )
	return BinID( start_.inl(), stop_.crl() );

    return idx==3 ? stop_ : start_;
}


void TrcKeySampling::normalise()
{ normalize(); }

void TrcKeySampling::normalize()
{
    if ( start_.lineNr() > stop_.lineNr() )
	Swap(start_.lineNr(),stop_.lineNr());
    if ( start_.trcNr() > stop_.trcNr() )
	Swap(start_.trcNr(),stop_.trcNr());
    if ( step_.lineNr() < 0 )
	step_.lineNr() = -step_.lineNr();
    else if ( !step_.lineNr() )
	step_.lineNr() = SI().inlStep();
    if ( step_.trcNr() < 0 )
	step_.trcNr() = -step_.trcNr();
    else if ( !step_.trcNr() )
	step_.trcNr() = SI().crlStep();
}



// TrcKeySamplingIterator
TrcKeySamplingIterator::TrcKeySamplingIterator()
    : tks_( true )
{
    reset();
}


TrcKeySamplingIterator::TrcKeySamplingIterator( const TrcKeySampling& hs )
{
    setSampling( hs );
}


TrcKeySamplingIterator::~TrcKeySamplingIterator()
{}


void TrcKeySamplingIterator::setSampling( const TrcKeySampling& tks )
{
#ifdef __debug__
    if ( !tks.isDefined() )
    {
	pErrMsg("Initializing iterator with undefined TrcKeySampling");
	DBG::forceCrash(true);
    }
#endif
    tks_ = tks;
    reset();
}


void TrcKeySamplingIterator::reset()
{
    curpos_ = 0;
    totalnr_ = tks_.totalNr();
}


bool TrcKeySamplingIterator::next( TrcKey& tk ) const
{
    const od_int64 mypos = curpos_++;
    if ( mypos<0 || mypos>=totalnr_ )
	return false;

    tk = tks_.trcKeyAt( mypos );
    return true;
}


bool TrcKeySamplingIterator::next( BinID& res ) const
{
    const od_int64 mypos = curpos_++;
    if ( mypos<0 || mypos>=totalnr_ )
	return false;

    res = tks_.atIndex( mypos );
    return true;
}



namespace Pos
{
bool intersect( const StepInterval<int>& rg1, const StepInterval<int>& rg2,
		StepInterval<int>& out )
{
    return ::intersect( rg1.start_, rg1.stop_, rg1.step_,
			rg2.start_, rg2.stop_, rg2.step_,
			out.start_, out.stop_, out.step_ );
}


bool intersectF( const StepInterval<float>& zsamp1,
		 const StepInterval<float>& zsamp2,
		 StepInterval<float>& out )
{
    return ::intersectF( zsamp1.start_, zsamp1.stop_, zsamp1.step_,
			 zsamp2.start_, zsamp2.stop_, zsamp2.step_,
			 out.start_, out.stop_, out.step_ );
}

}
