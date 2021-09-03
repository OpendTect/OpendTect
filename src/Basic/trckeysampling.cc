/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : somewhere around 1999
-*/


#include "trckeyzsampling.h"

#include "cubesampling.h"
#include "fullsubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "odjson.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "uistrings.h"

#include <math.h>


TrcKeySampling::TrcKeySampling( OD::SurvLimitType slt )
{
    init( true, slt );
}


TrcKeySampling::TrcKeySampling( Pos::GeomID gid )
{
    setTo( gid );
}


TrcKeySampling::TrcKeySampling( const HorSubSel& hss )
    : geomsystem_( hss.is2D() ? OD::LineBasedGeom : OD::VolBasedGeom )
{
    if ( hss.is2D() )
    {
	const auto* lhss = hss.asLineHorSubSel();
	const auto trcrg = lhss->trcNrRange();
	const auto lnr = lhss->geomID().lineNr();
	start_ = BinID( lnr, trcrg.start );
	stop_ = BinID( lnr, trcrg.stop );
	step_ = BinID( 1, trcrg.step );
    }
    else
    {
	const auto* chss = hss.asCubeHorSubSel();
	const auto inlrg = chss->inlRange();
	const auto crlrg = chss->crlRange();
	start_ = BinID( inlrg.start, crlrg.start );
	stop_ = BinID( inlrg.stop, crlrg.stop );
	step_ = BinID( inlrg.step, crlrg.step );
    }
}


TrcKeySampling::TrcKeySampling( const LineHorSubSel& lhss )
    : TrcKeySampling((HorSubSel&)lhss)
{
}


TrcKeySampling::TrcKeySampling( const CubeHorSubSel& chss )
    : TrcKeySampling((HorSubSel&)chss)
{
}


TrcKeySampling::TrcKeySampling( const HorSampling& hs )
{
    start_ = hs.start_;
    stop_ = hs.stop_;
    step_ = hs.step_;
}


TrcKeySampling::TrcKeySampling( const Geometry& geom )
{
    setTo( geom );
}


TrcKeySampling::TrcKeySampling( const TrcKeySampling& tks )
{
    *this = tks;
}


TrcKeySampling::TrcKeySampling( const TrcKeyZSampling& tkzs )
{
    *this = tkzs.hsamp_;
}


TrcKeySampling::TrcKeySampling( const TrcKey& tk )
{
    init( false );
    geomsystem_ = tk.geomSystem();
    start_.lineNr() = stop_.lineNr() = tk.lineNr();
    start_.trcNr() = stop_.trcNr() = tk.trcNr();
}


TrcKeySampling::TrcKeySampling( bool inittosi, OD::SurvLimitType slt )
{
    init( inittosi, slt );
}


void TrcKeySampling::setTo( GeomID gid )
{
    setTo( Geometry::get(gid) );
}


void TrcKeySampling::setTo( const Geometry& geom )
{
    setGeomID( geom.geomID() );

    start_.trcNr() = geom.trcNrRange().start;
    stop_.trcNr() = geom.trcNrRange().stop;
    step_.trcNr() = geom.trcNrRange().step;
    if ( geom.is3D() )
    {
	const auto& g3d = *geom.as3D();
	start_.inl() = g3d.inlRange().start;
	stop_.inl() = g3d.inlRange().stop;
	step_.inl() = g3d.inlRange().step;
    }
}


TrcKeySampling& TrcKeySampling::set( const Interval<int>& inlrg,
				      const Interval<int>& crlrg )
{
    setInlRange( inlrg );
    setCrlRange( crlrg );
    return *this;
}


void TrcKeySampling::init( bool initsi, OD::SurvLimitType slt )
{
    if ( initsi )
	SI().getSampling( *this, slt );
    else
    {
	geomsystem_ = OD::VolBasedGeom;
	start_.lineNr() = stop_.lineNr() = mUdf( linenr_type );
	start_.trcNr() = stop_.trcNr() = mUdf( trcnr_type );
	step_.lineNr() = step_.trcNr() = 1;
    }
}


Pos::GeomID TrcKeySampling::getGeomID() const
{
    return is2D() ? GeomID(start_.lineNr()) : GeomID::get3D();
}


void TrcKeySampling::setGeomID( GeomID gid )
{
    geomsystem_ = geomSystemOf( gid );
    if ( is2D() )
    {
	start_.lineNr() = stop_.lineNr() = gid.lineNr();
	step_.lineNr() = 1;
    }
}


BinID TrcKeySampling::atIndex( od_int64 globalidx ) const
{
    const int nrtrcs = nrTrcs();
    if ( !nrtrcs )
	return BinID::udf();

    const int lineidx = (int)(globalidx / nrtrcs);
    const int trcidx = (int)(globalidx % nrtrcs);
    return atIndex( lineidx, trcidx );
}


TrcKey TrcKeySampling::toTrcKey( const Coord& pos, dist_type* distance ) const
{
    const auto& geom = Geometry::get( getGeomID() );
    if ( geom.is2D() )
	return TrcKey( geom.geomID(),
		       geom.as2D()->nearestTracePosition( pos, distance ) );
    return TrcKey( geom.as3D()->nearestTracePosition( pos, distance ) );
}


Coord TrcKeySampling::toCoord( const BinID& bid ) const
{
    const auto& geom = Geometry::get( getGeomID() );
    if ( geom.is2D() )
	return geom.as2D()->getCoord( bid.crl() );
    else
	return geom.as3D()->getCoord( bid );
}


TrcKey TrcKeySampling::center() const
{
    const auto trcnr = trcRange().snappedCenter();
    return is2D() ? TrcKey( getGeomID(), trcnr )
		  : TrcKey( BinID(inlRange().snappedCenter(),trcnr) );
}


void TrcKeySampling::set2DDef()
{
    start_.lineNr() = start_.trcNr() = 0;
    stop_.lineNr() = stop_.trcNr() = mUdf(int);
    step_.lineNr() = step_.trcNr() = 1;
    geomsystem_ = OD::LineBasedGeom;
}


void TrcKeySampling::setLineRange( const Interval<int>& inlrg )
{
    start_.lineNr() = inlrg.start; stop_.lineNr() = inlrg.stop;
    if ( !inlrg.hasStep() )
	return;

    mDynamicCastGet(const StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	step_.lineNr() = inlsrg->step;
}


void TrcKeySampling::setTrcRange( const Interval<int>& crlrg )
{
    start_.trcNr() = crlrg.start; stop_.trcNr() = crlrg.stop;
    if ( !crlrg.hasStep() )
	return;

    mDynamicCastGet(const StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	step_.trcNr() = crlsrg->step;
}


StepInterval<int> TrcKeySampling::lineRange() const
{
    return StepInterval<int>( start_.lineNr(), stop_.lineNr(), step_.lineNr() );
}


StepInterval<int> TrcKeySampling::trcRange() const
{
    return StepInterval<int>( start_.trcNr(), stop_.trcNr(), step_.trcNr() );
}


TrcKeySampling::dist_type TrcKeySampling::lineDistance() const
{
    if ( !is3D() )
	{ pErrMsg("TrcKeySampling !3D, no line dist"); return (dist_type)1; }

    BinID bid( start_ );
    Coord startpos( toCoord( bid ) );
    bid.inl() += step_.inl();
    Coord stoppos( toCoord( bid ) );
    return stoppos.distTo<dist_type>(startpos);
}


TrcKeySampling::dist_type TrcKeySampling::trcDistance() const
{
    BinID bid( start_ );
    Coord startpos( toCoord( bid ) );
    bid.crl() += step_.crl();
    Coord stoppos( toCoord( bid ) );
    return stoppos.distTo<dist_type>(startpos);
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


void TrcKeySampling::includeLine( linenr_type lid )
{
    if ( mIsUdf(start_.lineNr()) || mIsUdf(stop_.lineNr()) || nrLines()<1 )
	start_.lineNr() = stop_.lineNr() = lid;
    else if ( is3D() )
    {
	start_.lineNr() = mMIN( start_.lineNr(), lid );
	stop_.lineNr() = mMAX( stop_.lineNr(), lid );
    }
}


void TrcKeySampling::includeTrc( trcnr_type trcid )
{
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
	include( TrcKey( tks.geomSystem(), tks.start_) );
	include( TrcKey( tks.geomSystem(), tks.stop_ ) );
	return;
    }

    TrcKeySampling temp( *this );
    temp.include( TrcKey( tks.geomSystem(), tks.start_) );
    temp.include( TrcKey( tks.geomSystem(), tks.stop_ ) );

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
    inlrg.start = start_.lineNr(); inlrg.stop = stop_.lineNr();
    mDynamicCastGet(StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	inlsrg->step = step_.lineNr();
    crlrg.start = start_.trcNr(); crlrg.stop = stop_.trcNr();
    mDynamicCastGet(StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	crlsrg->step = step_.trcNr();
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


TrcKeySampling& TrcKeySampling::operator=( const TrcKeySampling& hrg )
{
    geomsystem_ = hrg.geomsystem_;
    start_ = hrg.start_;
    stop_ = hrg.stop_;
    step_ = hrg.step_;
    return *this;
}


bool TrcKeySampling::operator==( const TrcKeySampling& oth ) const
{
    return geomsystem_ == oth.geomsystem_
	&& oth.start_==start_ && oth.stop_==stop_ && oth.step_==step_;
}


bool TrcKeySampling::operator!=( const TrcKeySampling& oth ) const
{ return !(*this==oth); }


od_int64 TrcKeySampling::totalNr() const
{
    return ((od_int64)nrLines()) * nrTrcs();
}


bool TrcKeySampling::isEmpty() const
{
    return nrLines() < 1 || nrTrcs() < 1;
}


void TrcKeySampling::limitTo( const TrcKeySampling& oth, bool ignoresteps )
{
    if ( !overlaps(oth,true) )
	{ init( false ); return; }

    StepInterval<int> inlrg( lineRange() );
    StepInterval<int> crlrg( trcRange() );
    if ( ignoresteps )
    {
	((SampleGate&)inlrg).limitTo( oth.lineRange() );
	((SampleGate&)crlrg).limitTo( oth.trcRange() );
    }
    else
    {
	inlrg.limitTo( oth.lineRange() );
	crlrg.limitTo( oth.trcRange() );
    }

    setLineRange( inlrg );
    setTrcRange( crlrg );
}


# define mAdjustIf(v1,op,v2) \
      if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;

void TrcKeySampling::limitToWithUdf( const TrcKeySampling& h )
{
    TrcKeySampling oth( h ); oth.normalise();
    normalise();

    mAdjustIf(start_.lineNr(),<,oth.start_.lineNr());
    mAdjustIf(start_.trcNr(),<,oth.start_.trcNr());
    mAdjustIf(stop_.lineNr(),>,oth.stop_.lineNr());
    mAdjustIf(stop_.trcNr(),>,oth.stop_.trcNr());
    mAdjustIf(step_.lineNr(),<,oth.step_.lineNr());
    mAdjustIf(step_.trcNr(),<,oth.step_.trcNr());
}


#define mSnapStop( start, stop, step, eps ) \
    stop = start + step * mCast( int, (stop-start+eps)/step );

#define mApproach( diff, var, assignoper, step ) \
    if ( diff>0 ) \
	var assignoper step * mCast( int, (diff)/step );

void TrcKeySampling::shrinkTo( const TrcKeySampling& innertks )
{
    normalise();
    TrcKeySampling tks( innertks );
    tks.normalise();

    mSnapStop( start_.inl(), stop_.inl(), step_.inl(), 0 );
    mSnapStop( start_.crl(), stop_.crl(), step_.crl(), 0 );

    mApproach( tks.start_.inl()-start_.inl(), start_.inl(), +=, step_.inl() );
    mApproach( stop_.inl() - tks.stop_.inl(),  stop_.inl(), -=, step_.inl() );
    mApproach( tks.start_.crl()-start_.crl(), start_.crl(), +=, step_.crl() );
    mApproach( stop_.crl() - tks.stop_.crl(),  stop_.crl(), -=, step_.crl() );
}


void TrcKeySampling::growTo( const TrcKeySampling& outertks )
{
    normalise();
    TrcKeySampling tks( outertks );
    tks.normalise();

    mSnapStop( start_.inl(), stop_.inl(), step_.inl(), 0 );
    mSnapStop( start_.crl(), stop_.crl(), step_.crl(), 0 );

    mApproach( start_.inl()-tks.start_.inl(), start_.inl(), -=, step_.inl() );
    mApproach( tks.stop_.inl() - stop_.inl(),  stop_.inl(), +=, step_.inl() );
    mApproach( start_.crl()-tks.start_.crl(), start_.crl(), -=, step_.crl() );
    mApproach( tks.stop_.crl() - stop_.crl(),  stop_.crl(), +=, step_.crl() );
}


void TrcKeySampling::expand( int nrlines, int nrtrcs )
{
    if ( !mIsUdf(nrlines) && is3D() )
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
    FixedString parval = par[key];
    if ( !parval )
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

    int survid = (int)geomsystem_;
    if ( pars.get(sKey::SurveyID(),survid) && survid > -3 && survid < 1 )
	geomsystem_ = (GeomSystem)survid;

    if ( inlok && crlok )
	return true;

    PtrMan<IOPar> subpars = pars.subselect( IOPar::compKey(sKey::Line(),0) );
    if ( !subpars )
	return false;

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
	GeomID geomid;
	subpars->get( sKey::GeomID(), geomid );
	start_.lineNr() = stop_.lineNr() = geomid.lineNr();
	step_.lineNr() = 1;
	geomsystem_ = OD::LineBasedGeom;
    }

    return trcrgok;
}


bool TrcKeySampling::useJSON( const OD::JSON::Object& obj )
{
    if ( !obj.isPresent(sKey::SurveyID()) )
	return false;

    const int survid = obj.getIntValue( sKey::SurveyID() );
    if ( survid > -3 && survid < 1 )
	geomsystem_ = (GeomSystem)survid;

    if ( is2D() )
    {
	StepInterval<int> trcrg;
	Pos::GeomID gid;
	if ( !obj.get(sKey::TrcRange(),trcrg) ||
	     !obj.getGeomID(sKey::GeomID(),gid) )
	    return false;
	setTrcRange( trcrg );
	setGeomID( gid );
    }
    else
    {
	StepInterval<int> inlrg, crlrg;
	if ( !obj.get(sKey::InlRange(),inlrg) ||
	     !obj.get(sKey::CrlRange(),crlrg) )
	    return false;
	setLineRange( inlrg );
	setTrcRange( crlrg );
    }

    return true;
}


void TrcKeySampling::fillPar( IOPar& pars ) const
{
    if ( is2D() )
    {
	IOPar tmppar;
	tmppar.set( sKey::GeomID(), start_.lineNr() );
	tmppar.set( sKey::FirstTrc(), start_.trcNr() );
	tmppar.set( sKey::LastTrc() , stop_.trcNr() );
	tmppar.set( sKey::StepCrl(), step_.trcNr() );
	tmppar.set( sKey::SurveyID(), (int)geomsystem_ );
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
	pars.set( sKey::SurveyID(), (int)geomsystem_ );
    }
}


void TrcKeySampling::fillJSON( OD::JSON::Object& obj ) const
{
    if ( is2D() )
    {
	obj.set( sKey::GeomID(), getGeomID() );
	obj.set( sKey::TrcRange(), trcRange() );
    }
    else
    {
	obj.set( sKey::InlRange(), inlRange() );
	obj.set( sKey::CrlRange(), crlRange() );
    }

    obj.set( sKey::SurveyID(), (int)geomsystem_ );
}


void TrcKeySampling::removeInfo( IOPar& par )
{
    par.removeWithKey( sKey::FirstInl() );
    par.removeWithKey( sKey::FirstCrl() );
    par.removeWithKey( sKey::LastInl() );
    par.removeWithKey( sKey::LastCrl() );
    par.removeWithKey( sKey::StepInl() );
    par.removeWithKey( sKey::StepCrl() );
    par.removeWithKey( sKey::SurveyID() );
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

inline static bool IsZero( float f, float eps=Eps )
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


namespace Pos
{

void normalise( steprg_type& in, Index_Type defstep )
{
    if ( in.start > in.stop )
	std::swap( in.start, in.stop );
    if ( in.step < 0 )
	in.step *= -1;
    else if ( !in.step )
	in.step = defstep;
}

bool intersect( const steprg_type& rg1, const steprg_type& rg2,
		steprg_type& out )
{
    return ::intersect( rg1.start, rg1.stop, rg1.step,
			rg2.start, rg2.stop, rg2.step,
			out.start, out.stop, out.step );
}

void normaliseZ( ZSampling& zsamp )
{
    if ( zsamp.start > zsamp.stop )
	std::swap(zsamp.start,zsamp.stop);
    if ( zsamp.step < 0 )
	zsamp.step = -zsamp.step;
    else if ( !zsamp.step )
	zsamp.step = SI().zStep();
}

bool intersectF( const ZSampling& zsamp1, const ZSampling& zsamp2,
		 ZSampling& out )
{
    return ::intersectF( zsamp1.start, zsamp1.stop, zsamp1.step,
			 zsamp2.start, zsamp2.stop, zsamp2.step,
			 out.start, out.stop, out.step );
}

} //namespace Pos


bool TrcKeySampling::overlaps( const TrcKeySampling& oth,
			       bool ignoresteps ) const
{
    if ( ignoresteps )
    {
	const StepInterval<int> othlinerg( oth.lineRange() ),
				othtrcrg( oth.trcRange() );
	return othlinerg.overlaps( lineRange() ) &&
	       othtrcrg.overlaps( trcRange() );
    }

    TrcKeySampling intertks;
    return getIntersection( oth, intertks );
}


bool TrcKeySampling::getIntersection( const TrcKeySampling& tks,
				      TrcKeySampling& out ) const
{
    TrcKeySampling tks1( tks ); tks1.normalise();
    TrcKeySampling tks2( *this ); tks2.normalise();

    const Pos::steprg_type linerg1( tks1.lineRange() );
    const Pos::steprg_type linerg2( tks2.lineRange() );
    const Pos::steprg_type trcrg1( tks1.trcRange() );
    const Pos::steprg_type trcrg2( tks2.trcRange() );
    Pos::steprg_type linergout, trcrgout;

    const bool success = Pos::intersect( linerg1, linerg2, linergout ) &&
			 Pos::intersect( linerg2, trcrg2, trcrgout );
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
    BinID relbid( bid.first() - start_.first(),
		  bid.second() - start_.second() );

    BinID ret( 0, 0 );

    if ( is3D() )
    {
	if ( step_.first() )
	    getNearestIdx( relbid.first(), step_.first() );

	ret.first() = start_.first() + relbid.first();

	if ( ret.first() < start_.first() )
	    ret.first() = start_.first();
	else if ( ret.first() > stop_.first() )
	    ret.first() = stop_.first();
    }

    if ( step_.second() )
	getNearestIdx( relbid.second(), step_.second() );

    ret.second() = start_.second() + relbid.second();

    if ( ret.second() < start_.second() )
	ret.second() = start_.second();
    else if ( ret.second() > stop_.second() )
	ret.second() = stop_.second();

    return ret;
}


TrcKey TrcKeySampling::getNearest( const TrcKey& trckey ) const
{
    if ( trckey.isUdf() )
	return trckey;

    if ( trckey.geomSystem() == geomsystem_ )
	return TrcKey( geomsystem_, getNearest(trckey.position()) );

    return getNearest( trckey.getFor(getGeomID()) );
}


void TrcKeySampling::snapToSurvey()
{
    SI().snap( start_, OD::SnapDownward );
    SI().snap( stop_, OD::SnapUpward );
}


void TrcKeySampling::toString( uiPhrase& str ) const
{
    str.appendPhrase(tr("Inline range: %1 - %2 [%3]"))
	.arg( start_.lineNr() ).arg( stop_.lineNr() ).arg( start_.lineNr() );
    str.appendPhrase(tr("Crossline range: %1 - %2 [%3]"))
	.arg( start_.trcNr() ).arg( stop_.trcNr() ).arg( start_.trcNr() );
}


void TrcKeySampling::getRandomSet( int nr, TypeSet<TrcKey>& res ) const
{
    if ( nr > totalNr() )
	nr = (int) totalNr();

    while ( nr )
    {
	const BinID bid( lineRange().start + std::rand() % nrLines(),
			 trcRange().start + std::rand() % nrTrcs() );
	const TrcKey trckey( geomsystem_, bid );
	if ( includes(trckey) && res.addIfNew(trckey) )
	    nr--;
    }
}


BinID TrcKeySampling::atIndex( int i0, int i1 ) const
{
    const trcnr_type trcnr = start_.trcNr() + i1 * step_.trcNr();
    linenr_type linenr = start_.lineNr();
    if ( is3D() )
	linenr += i0 * step_.lineNr();
    return BinID(linenr,trcnr);
}


TrcKey TrcKeySampling::trcKeyAt( int i0, int i1 ) const
{
    const BinID res = atIndex( i0, i1 );
    if ( res.isUdf() )
	return TrcKey::udf();

    return TrcKey( geomsystem_, res );
}


TrcKey TrcKeySampling::trcKeyAt( od_int64 globalidx ) const
{
    const BinID res = atIndex( globalidx );
    if ( res.isUdf() )
	return TrcKey::udf();

    return TrcKey( geomsystem_, res );
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
	nbs += TrcKey( atIndex(idxs[idx]) );
}


bool TrcKeySampling::toNext( BinID& bid ) const
{
    if ( mIsUdf(bid.inl()) || mIsUdf(bid.crl()) )
	return false;

    if ( mIsUdf(bid.inl()) || bid.inl() < start_.inl() )
    {
	bid.inl() = start_.inl();
	bid.crl() = start_.crl();
	return true;
    }

    if ( mIsUdf(bid.crl()) || bid.crl() < start_.crl() )
	bid.crl() = start_.crl() - step_.crl();

    bid.crl() += step_.crl();
    if ( bid.crl() > stop_.crl() )
    {
	bid.crl() = start_.crl();
	bid.inl()++;
	if ( bid.inl() > stop_.inl() )
	    return false;
    }

    return true;
}


void TrcKeySampling::include( const TrcKey& trckey )
{
    if ( trckey.isUdf() )
	return;
    if ( trckey.geomSystem() != geomsystem_ )
	include( trckey.getFor(getGeomID()) );

    includeLine( trckey.lineNr() );
    includeTrc( trckey.trcNr() );
}


od_int64 TrcKeySampling::globalIdx( const TrcKey& tk ) const
{
    if ( tk.geomSystem() != geomsystem_ )
	return globalIdx( tk.getFor(getGeomID()) );

    return globalIdx( tk.position() );
}


od_int64 TrcKeySampling::globalIdx( const BinID& bid ) const
{
    return lineIdx( bid.lineNr() ) * nrTrcs() + trcIdx(bid.trcNr() );
}


bool TrcKeySampling::lineOK( linenr_type lid, bool ignoresteps ) const
{
    const bool linenrok = lid >= start_.lineNr() && lid <= stop_.lineNr();
    return ignoresteps ? linenrok : linenrok && ( step_.lineNr() ?
	!( ( lid-start_.lineNr() ) % step_.lineNr() ) : lid==start_.lineNr() );
}


bool TrcKeySampling::trcOK( trcnr_type tid, bool ignoresteps ) const
{
    const bool trcnrok = tid >= start_.trcNr() && tid <= stop_.trcNr();
    return ignoresteps ? trcnrok : trcnrok && ( step_.crl() ?
	!( ( tid-start_.trcNr() ) % step_.trcNr() ) : tid==start_.trcNr() );
}


bool TrcKeySampling::includes( const TrcKey& tk, bool ignoresteps ) const
{
    if ( tk.geomSystem() != geomsystem_ )
	return includes( tk.getFor(getGeomID()) );

    return lineOK(tk.lineNr(),ignoresteps) && trcOK(tk.trcNr(),ignoresteps);
}


// TrcKeyZSampling


TrcKeyZSampling::TrcKeyZSampling( OD::SurvLimitType slt )
    : hsamp_(slt)
{
    init( true, slt );
}


TrcKeyZSampling::TrcKeyZSampling( GeomID gid )
    : hsamp_(false)
{
    setTo( gid );
}


TrcKeyZSampling::TrcKeyZSampling( const HorSubSel& hss )
    : hsamp_(hss)
    , zsamp_(SurvGeom::get(hss.geomID()).zRange())
{
}


TrcKeyZSampling::TrcKeyZSampling( const GeomSubSel& gss )
    : hsamp_(gss.horSubSel())
    , zsamp_(gss.zRange())
{
}


TrcKeyZSampling::TrcKeyZSampling( const FullSubSel& fss )
    : hsamp_(fss.horSubSel())
    , zsamp_(fss.zRange())
{
}


TrcKeyZSampling::TrcKeyZSampling( const LineSubSel& lss )
    : TrcKeyZSampling((GeomSubSel&)lss)
{
}


TrcKeyZSampling::TrcKeyZSampling( const CubeSubSel& css )
    : TrcKeyZSampling((GeomSubSel&)css)
{
}


TrcKeyZSampling::TrcKeyZSampling( const Geometry& geom )
    : hsamp_( geom )
    , zsamp_( geom.zRange() )
{
}


TrcKeyZSampling::TrcKeyZSampling( const HorSampling& hs )
    : hsamp_(hs)
    , zsamp_( SI().zRange() )
{
}


TrcKeyZSampling::TrcKeyZSampling( const CubeSampling& cs )
    : hsamp_(false)
{
    hsamp_ = TrcKeySampling( cs.hsamp_ );
    zsamp_ = cs.zsamp_;
}


TrcKeyZSampling::TrcKeyZSampling( const TrcKeySampling& tks )
    : hsamp_(tks)
    , zsamp_( SI().zRange() )
{
    const auto gid = tks.getGeomID();
    if ( gid.isValid() && gid.is2D() )
	zsamp_ = Geometry::get2D(gid).zRange();
}


TrcKeyZSampling::TrcKeyZSampling( const TrcKeyZSampling& tkzs )
    : hsamp_(false)
{
    *this = tkzs;
}


TrcKeyZSampling::TrcKeyZSampling( bool settoSI, OD::SurvLimitType slt )
    : hsamp_(false)
{
    init( settoSI, slt );
}


void TrcKeyZSampling::init( bool tosi, OD::SurvLimitType slt )
{
    hsamp_.init( tosi, slt );
    if ( tosi )
	zsamp_ = SI().zRange( slt );
    else
	{ zsamp_.start = zsamp_.stop = 0.f; zsamp_.step = 1.f; }
}


void TrcKeyZSampling::setTo( GeomID gid )
{
    setTo( Geometry::get(gid) );
}


void TrcKeyZSampling::setTo( const Geometry& geom )
{
    hsamp_.setTo( geom );
    zsamp_ = geom.zRange();
}


void TrcKeyZSampling::set2DDef()
{
    hsamp_.set2DDef();
    zsamp_ = SI().zRange();
}




bool TrcKeyZSampling::getIntersection( const TrcKeyZSampling& tkzs,
				       TrcKeyZSampling& out ) const
{
    if ( !hsamp_.getIntersection(tkzs.hsamp_,out.hsamp_) )
	return false;

    ZSampling zsamp1( tkzs.zsamp_ );	Pos::normaliseZ( zsamp1 );
    ZSampling zsamp2( zsamp_ );		Pos::normaliseZ( zsamp2 );
    return Pos::intersectF( zsamp1, zsamp2, out.zsamp_ );
}


bool TrcKeyZSampling::isFlat() const
{
    if ( hsamp_.start_.lineNr()==hsamp_.stop_.lineNr() ||
	 hsamp_.start_.trcNr()==hsamp_.stop_.trcNr() )
	return true;

    return fabs( zsamp_.stop-zsamp_.start ) < fabs( zsamp_.step * 0.5 );
}


OD::SliceType TrcKeyZSampling::defaultDir() const
{
    const int nrinl = nrLines();
    const int nrcrl = nrTrcs();
    const int nrz = nrZ();
    if ( nrz < nrinl && nrz < nrcrl )
	return OD::ZSlice;

    return nrinl<=nrcrl ? OD::InlineSlice : OD::CrosslineSlice;
}


void TrcKeyZSampling::getDefaultNormal( Coord3& ret ) const
{
    if ( defaultDir() == OD::InlineSlice )
	ret = Coord3( SI().binID2Coord().inlDir(), 0 );
    else if ( defaultDir() == OD::CrosslineSlice )
	ret = Coord3( SI().binID2Coord().crlDir(), 0 );
    else
	ret = Coord3( 0, 0, 1 );
}


od_int64 TrcKeyZSampling::totalNr() const
{ return ((od_int64) nrZ()) * ((od_int64) hsamp_.totalNr()); }


int TrcKeyZSampling::lineIdx( int lineid ) const
{ return hsamp_.lineIdx(lineid); }


int TrcKeyZSampling::trcIdx( int trcnr ) const
{ return hsamp_.trcIdx(trcnr); }


int TrcKeyZSampling::zIdx( float z ) const
{ return zsamp_.getIndex(z); }


int TrcKeyZSampling::nrLines() const
{ return hsamp_.nrLines(); }


int TrcKeyZSampling::nrTrcs() const
{ return hsamp_.nrTrcs(); }


int TrcKeyZSampling::nrZ() const
{ return zsamp_.nrSteps() + 1; }


int TrcKeyZSampling::size( SliceType d ) const
{
    return d == OD::InlineSlice		? nrInl()
	: (d == OD::CrosslineSlice	? nrCrl()
					: nrZ());
}


float TrcKeyZSampling::zAtIndex( int idx ) const
{ return zsamp_.atIndex(idx); }


bool TrcKeyZSampling::isEmpty() const
{ return hsamp_.isEmpty(); }


bool TrcKeyZSampling::operator!=( const TrcKeyZSampling& tkzs ) const
{ return !(tkzs==*this); }


TrcKeyZSampling& TrcKeyZSampling::operator=( const TrcKeyZSampling& oth )
{
    hsamp_ = oth.hsamp_;
    zsamp_ = oth.zsamp_;
    return *this;
}


bool TrcKeyZSampling::includes( const TrcKeyZSampling& oth ) const
{
    return hsamp_.includes( oth.hsamp_ ) &&
	   zsamp_.includes( oth.zsamp_.start, false ) &&
	   zsamp_.includes( oth.zsamp_.stop, false );
}


void TrcKeyZSampling::include( const BinID& bid, float z )
{
    hsamp_.include( bid );
    zsamp_.include( z );
}


void TrcKeyZSampling::include( const TrcKeyZSampling& oth )
{
    TrcKeyZSampling tkzs( oth ); tkzs.normalise();
    normalise();

    hsamp_.include( tkzs.hsamp_ );
    if ( tkzs.zsamp_.start < zsamp_.start )
	zsamp_.start = tkzs.zsamp_.start;
    if ( tkzs.zsamp_.stop > zsamp_.stop )
	zsamp_.stop = tkzs.zsamp_.stop;
    if ( tkzs.zsamp_.step < zsamp_.step )
	zsamp_.step = tkzs.zsamp_.step;
}


bool TrcKeyZSampling::isDefined() const
{
    return hsamp_.isDefined()
	&& !mIsUdf(zsamp_.start)
	&& !mIsUdf(zsamp_.stop)
	&& !mIsUdf(zsamp_.step);
}


void TrcKeyZSampling::limitTo( const TrcKeyZSampling& tkzs, bool ignoresteps )
{
    hsamp_.limitTo( tkzs.hsamp_, ignoresteps );
    if ( hsamp_.isEmpty() )
	{ init( false ); return; }

    if ( ignoresteps )
	((ZGate&)zsamp_).limitTo( tkzs.zsamp_ );
    else
	zsamp_.limitTo( tkzs.zsamp_ );
}


void TrcKeyZSampling::limitToWithUdf( const TrcKeyZSampling& c )
{
    TrcKeyZSampling tkzs( c ); tkzs.normalise();
    normalise();
    hsamp_.limitToWithUdf( tkzs.hsamp_ );
    mAdjustIf(zsamp_.start,<,tkzs.zsamp_.start);
    mAdjustIf(zsamp_.stop,>,tkzs.zsamp_.stop);
}


void TrcKeyZSampling::shrinkTo( const TrcKeyZSampling& innertkzs, float releps )
{
    normalise();
    TrcKeyZSampling tkzs( innertkzs );
    tkzs.normalise();

    hsamp_.shrinkTo( tkzs.hsamp_ );

    const float eps = releps * zsamp_.step;
    mSnapStop( zsamp_.start, zsamp_.stop, zsamp_.step, eps );

    mApproach(tkzs.zsamp_.start-zsamp_.start+eps, zsamp_.start,+=, zsamp_.step);
    mApproach(zsamp_.stop - tkzs.zsamp_.stop+eps, zsamp_.stop, -=, zsamp_.step);
}


void TrcKeyZSampling::growTo( const TrcKeyZSampling& outertkzs, float releps )
{
    normalise();
    TrcKeyZSampling tkzs( outertkzs );
    tkzs.normalise();

    hsamp_.growTo( tkzs.hsamp_ );

    const float eps = releps * zsamp_.step;
    mSnapStop( zsamp_.start, zsamp_.stop, zsamp_.step, eps );

    mApproach(zsamp_.start-tkzs.zsamp_.start+eps, zsamp_.start,-=, zsamp_.step);
    mApproach(tkzs.zsamp_.stop - zsamp_.stop+eps, zsamp_.stop, +=, zsamp_.step);
}


bool TrcKeyZSampling::makeCompatibleWith(const TrcKeyZSampling& othertkzs )
{
    TrcKeyZSampling res( othertkzs );
    res.growTo( *this );
    res.expand( 1, 1, 1 );	// "grow to" => "grow over"

    res.limitTo( *this );	// will take care of step-compatibility

    if ( !res.isDefined() || res.isEmpty() )
	return false;

    *this = res;
    return true;
}


bool TrcKeyZSampling::adjustTo( const TrcKeyZSampling& availabletkzs,
				bool falsereturnsdummy )
{
    TrcKeyZSampling compatibletkzs( availabletkzs );
    const bool iscompatible = compatibletkzs.makeCompatibleWith( *this );

    TrcKeyZSampling clippedtkzs( compatibletkzs );
    clippedtkzs.limitTo( *this, true );

    if ( !iscompatible || !clippedtkzs.isDefined() || clippedtkzs.isEmpty() )
    {
	// To create dummy with a single undefined voxel
	if ( !falsereturnsdummy )
	    init( false );
	else
	{
	    *this = compatibletkzs;
	    hsamp_.start_ -= hsamp_.step_;
	    zsamp_.start -= zsamp_.step;
	    hsamp_.stop_ = hsamp_.start_;
	    zsamp_.stop = zsamp_.start;
	}

	return false;
    }

    TrcKeyZSampling adjustedtkzs( compatibletkzs );
    adjustedtkzs.shrinkTo( *this );

    // Only keep adjustments for non-flat dimensions
    if ( nrLines() == 1 )
	adjustedtkzs.hsamp_.setLineRange( hsamp_.lineRange() );
    if ( nrTrcs() == 1 )
	adjustedtkzs.hsamp_.setTrcRange( hsamp_.trcRange() );
    if ( nrZ() == 1 )
	adjustedtkzs.zsamp_ = zsamp_;

    *this = adjustedtkzs;
    return true;
}


void TrcKeyZSampling::expand( int nrlines, int nrtrcs, int nrz )
{
    hsamp_.expand( nrlines, nrtrcs );
    zsamp_.start -= nrz*zsamp_.step;
    zsamp_.stop += nrz*zsamp_.step;
}


void TrcKeyZSampling::snapToSurvey()
{
    hsamp_.snapToSurvey();
    SI().snapZ( zsamp_.start, OD::SnapDownward );
    SI().snapZ( zsamp_.stop, OD::SnapUpward );
}


bool TrcKeyZSampling::operator==( const TrcKeyZSampling& tkzs ) const
{ return isEqual( tkzs ); }


bool TrcKeyZSampling::isEqual( const TrcKeyZSampling& tkzs, float zeps ) const
{
    if ( this == &tkzs ) return true;

    if ( tkzs.hsamp_ == this->hsamp_ )
    {
	if ( mIsUdf(zeps) )
	{
	    const float minzstep = mMIN( fabs(zsamp_.step),
					 fabs(tkzs.zsamp_.step) );
	    zeps = mMAX( 1e-6f, (mIsUdf(minzstep) ? 0.0f : 0.001f*minzstep) );
	}

	float diff = tkzs.zsamp_.start - this->zsamp_.start;
	if ( fabs(diff) > zeps ) return false;

	diff = tkzs.zsamp_.stop - this->zsamp_.stop;
	if ( fabs(diff) > zeps ) return false;

	diff = tkzs.zsamp_.step - this->zsamp_.step;
	if ( fabs(diff) > zeps ) return false;

	return true;
    }

   return false;
}


bool TrcKeyZSampling::usePar( const IOPar& par )
{
    bool isok = hsamp_.usePar( par );
    if ( hsamp_.is2D() )
    {
	PtrMan<IOPar> subpars =
			par.subselect( IOPar::compKey( sKey::Line(), 0 ) );
	if ( !subpars ) return false;
	return isok && subpars->get( sKey::ZRange(), zsamp_ );
    }

    bool zok = par.get( sKey::ZRange(), zsamp_ );
    if ( !zok )
    {
	Survey::FullZSubSel fzss;
	fzss.usePar( par );
	zok = fzss.nrGeomIDs() > 0;
	zsamp_ = fzss.first().zRange();
    }

    return isok && zok;
}


bool TrcKeyZSampling::useJSON( const OD::JSON::Object& obj )
{
    if ( !hsamp_.useJSON(obj) || !obj.isPresent(sKey::ZRange()) )
	return false;

    obj.get( sKey::ZRange(), zsamp_ );

    return true;
}


void TrcKeyZSampling::fillPar( IOPar& par ) const
{
    hsamp_.fillPar( par );
    if ( hsamp_.is2D() )
    {
	IOPar tmppar;
	tmppar.set( sKey::ZRange(), zsamp_.start, zsamp_.stop, zsamp_.step );
	par.mergeComp( tmppar, IOPar::compKey( sKey::Line(), 0 ) );

    }
    else
	par.set( sKey::ZRange(), zsamp_.start, zsamp_.stop, zsamp_.step );
}


void TrcKeyZSampling::fillJSON( OD::JSON::Object& obj ) const
{
    hsamp_.fillJSON( obj );
    obj.set( sKey::ZRange(), zsamp_ );
}


void TrcKeyZSampling::removeInfo( IOPar& par )
{
    TrcKeySampling::removeInfo( par );
    par.removeWithKey( sKey::ZRange() );
}


void TrcKeySampling::normalise()
{
    Pos::steprg_type linerg( inlRange() ), trcrg( trcRange() );
    Pos::normalise( linerg, SI().inlStep() );
    Pos::normalise( trcrg, SI().crlStep() );
    setLineRange( linerg );
    setTrcRange( trcrg );
}


void TrcKeyZSampling::normalise()
{
    hsamp_.normalise();
    Pos::normaliseZ( zsamp_ );
}


static const char* sKeyCurrentPos()	{ return "Current position"; }

TrcKeySamplingIterator::TrcKeySamplingIterator()
    : tks_( true )
    , totalnr_( tks_.totalNr() )
    , curpos_(0)
{}


TrcKeySamplingIterator::TrcKeySamplingIterator( const TrcKeySampling& tks )
{
    setSampling( tks );
}


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
    totalnr_ = tks_.totalNr();
    curpos_ = 0;
}


bool TrcKeySamplingIterator::next() const
{
    curpos_++;

    return curpos_ > -1 && curpos_ < totalnr_;
}


void TrcKeySamplingIterator::reset()
{
    curpos_ = 0;
}


TrcKey TrcKeySamplingIterator::curTrcKey() const
{
    return tks_.trcKeyAt( curpos_ );
}


BinID TrcKeySamplingIterator::curBinID() const
{
    return tks_.atIndex( curpos_ );
}


void TrcKeySamplingIterator::fillPar( IOPar& iop ) const
{
    tks_.fillPar( iop );
    iop.set( sKeyCurrentPos(), curpos_ );
}


void TrcKeySamplingIterator::usePar( const IOPar& iop )
{
    tks_.usePar( iop );
    od_int64 curpos;
    if ( iop.get(sKeyCurrentPos(),curpos) )
	setCurrentPos( curpos );
}
