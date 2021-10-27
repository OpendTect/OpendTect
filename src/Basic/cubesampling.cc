/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : somewhere around 1999
-*/


#include "cubesampling.h"

#include "horsubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "linesubsel.h"
#include "odjson.h"
#include "separstr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "survgeom3d.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

#include <math.h>


HorSampling::HorSampling( bool settosi, OD::SurvLimitType slt )
{
    init( settosi, slt );
}


HorSampling::HorSampling( const CubeHorSubSel& hss )
{
    set( hss.inlRange(), hss.crlRange() );
}


HorSampling::HorSampling( const TrcKeySampling& tks )
{
    set( tks.inlRange(), tks.crlRange() );
}


HorSampling::HorSampling( const TrcKeyZSampling& tkzs )
    : HorSampling(tkzs.hsamp_)
{
}


HorSampling::HorSampling( const BinID& bid )
{
    start_ = stop_ = bid;
    step_ = SI().steps();
}


void HorSampling::init( bool tosi, OD::SurvLimitType slt )
{
    if ( tosi )
    {
	const auto inlrg( SI().inlRange(slt) );
	const auto crlrg( SI().crlRange(slt) );
	start_ = BinID( inlrg.start, crlrg.start );
	stop_ = BinID( inlrg.stop, crlrg.stop );
	step_ = BinID( inlrg.step, crlrg.step );
    }
    else
    {
	start_.inl() = start_.crl() = stop_.inl() = stop_.crl()
	    = mUdf(pos_type);
	step_.inl() = step_.crl() = 1;
    }
}


bool HorSampling::operator==( const HorSampling& oth ) const
{
    return oth.start_==start_ && oth.stop_==stop_ && oth.step_==step_;
}


bool HorSampling::operator!=( const HorSampling& oth ) const
{
    return !(*this==oth);
}


bool HorSampling::isDefined() const
{
    return !mIsUdf( start_.inl() );
}


HorSampling::pos_steprg_type HorSampling::inlRange() const
{
    return pos_steprg_type( start_.inl(), stop_.inl(), step_.inl() );
}


HorSampling::pos_steprg_type HorSampling::crlRange() const
{
    return pos_steprg_type( start_.crl(), stop_.crl(), step_.crl() );
}


void HorSampling::get( pos_rg_type& inlrg, pos_rg_type& crlrg ) const
{
    inlrg.start = start_.inl(); inlrg.stop = stop_.inl();
    mDynamicCastGet(pos_steprg_type*,inlsteprg,&inlrg)
    if ( inlsteprg )
	inlsteprg->step = step_.inl();
    crlrg.start = start_.crl(); crlrg.stop = stop_.crl();
    mDynamicCastGet(pos_steprg_type*,crlsteprg,&crlrg)
    if ( crlsteprg )
	crlsteprg->step = step_.crl();
}


bool HorSampling::includes( const BinID& bid ) const
{
    return includesInl( bid.inl() ) && includesCrl( bid.crl() );
}


bool HorSampling::includesInl( pos_type inl ) const
{
    return !mIsUdf(inl) && inlRange().isPresent( inl );
}


bool HorSampling::includesCrl( pos_type crl ) const
{
    return !mIsUdf(crl) && crlRange().isPresent( crl );
}


void HorSampling::setInlRange( const pos_rg_type& inlrg )
{
    start_.inl() = inlrg.start; stop_.inl() = inlrg.stop;
    mDynamicCastGet(const pos_steprg_type*,steprg,&inlrg)
    if ( steprg )
	step_.inl() = steprg->step;
}


void HorSampling::setCrlRange( const pos_rg_type& crlrg )
{
    start_.crl() = crlrg.start; stop_.crl() = crlrg.stop;
    mDynamicCastGet(const pos_steprg_type*,steprg,&crlrg)
    if ( steprg )
	step_.crl() = steprg->step;
}


void HorSampling::set( const pos_rg_type& inlrg, const pos_rg_type& crlrg)
{
    setInlRange( inlrg );
    setCrlRange( crlrg );
}


void HorSampling::include( const BinID& bid )
{
    includeInl( bid.inl() );
    includeCrl( bid.crl() );
}


void HorSampling::includeInl( pos_type inl )
{
    if ( nrInl() < 1 )
	start_.inl() = stop_.inl() = inl;
    else
    {
	if ( start_.inl() > inl )
	    start_.inl() = inl;
	if ( stop_.inl() < inl )
	    stop_.inl() = inl;
    }
}


void HorSampling::includeCrl( pos_type crl )
{
    if ( nrInl() < 1 )
	start_.crl() = stop_.crl() = crl;
    else
    {
	if ( start_.crl() > crl )
	    start_.crl() = crl;
	if ( stop_.crl() < crl )
	    stop_.crl() = crl;
    }
}


void HorSampling::include( const HorSampling& oth, bool ignoresteps )
{
    if ( ignoresteps )
	{ include( oth.start_ ); include( oth.stop_ ); return; }

    HorSampling temp( *this );
    temp.include( oth.start_ );
    temp.include( oth.stop_ );

#define mHandleIC( ic ) \
    const pos_type newstart_##ic = temp.start_.ic(); \
    const pos_type newstop_##ic = temp.stop_.ic(); \
    pos_type offset##ic = mIsUdf(start_.ic()) || mIsUdf(oth.start_.ic()) ? 0 \
	: ( start_.ic() != newstart_##ic ? start_.ic() - newstart_##ic \
				     : oth.start_.ic() - newstart_##ic ); \
    step_.ic() = Math::HCFOf( step_.ic(), oth.step_.ic() ); \
    if ( offset##ic ) step_.ic() = Math::HCFOf( step_.ic(), offset##ic ); \
    start_.ic() = newstart_##ic; stop_.ic() = newstop_##ic

    mHandleIC(inl);
    mHandleIC(crl);
}


bool HorSampling::isLine() const
{
    return start_.inl() == stop_.inl() || start_.crl() == stop_.crl();
}


HorSampling::size_type HorSampling::nrInl() const
{
    if ( !isDefined() || !step_.inl() )
	return 0;

    const idx_type ret = idx4Inl( stop_.inl() );
    return ret < 0 ? 1 - ret : ret + 1;
}


HorSampling::size_type HorSampling::nrCrl() const
{
    if ( !isDefined() || !step_.crl() )
	return 0;

    const idx_type ret = idx4Crl( stop_.crl() );
    return ret < 0 ? 1 - ret : ret + 1;
}


BinID HorSampling::center() const
{
    return BinID( inlRange().snappedCenter(), crlRange().snappedCenter() );
}


bool HorSampling::toNext( BinID& bid ) const
{
    if ( mIsUdf(bid.inl()) || bid.inl() < start_.inl() )
	{ bid = start_; return true; }

    bid.crl() += step_.crl();
    if ( bid.crl() > stop_.crl() )
    {
	bid.crl() = start_.crl();
	bid.inl() += step_.inl();
	if ( bid.inl() > stop_.inl() )
	    return false;
    }
    return true;
}


void HorSampling::normalise()
{
    if ( start_.inl() > stop_.inl() )
	std::swap(start_.inl(),stop_.inl());
    if ( start_.crl() > stop_.crl() )
	std::swap(start_.crl(),stop_.crl());
    if ( step_.inl() < 0 )
	step_.inl() = -step_.inl();
    else if ( !step_.inl() )
	step_.inl() = SI().inlStep();
    if ( step_.crl() < 0 )
	step_.crl() = -step_.crl();
    else if ( !step_.crl() )
	step_.crl() = SI().crlStep();
}


void HorSampling::limitTo( const HorSampling& oth )
{
    limitTo( CubeHorSubSel(oth) );
}


void HorSampling::limitTo( const CubeHorSubSel& css )
{
    CubeHorSubSel mycss( *this );
    mycss.limitTo( css );
    *this = HorSampling( mycss );
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


void HorSampling::usePar( const IOPar& pars )
{
    const bool inlfound = getRange( pars, sKey::InlRange(),
				    start_.inl(), stop_.inl(), step_.inl() );
    if ( !inlfound )
    {
	pars.get( sKey::FirstInl(), start_.inl() );
	pars.get( sKey::LastInl(), stop_.inl() );
	pars.get( sKey::StepInl(), step_.inl() );
    }

    const bool crlfound = getRange( pars, sKey::CrlRange(),
				    start_.crl(), stop_.crl(), step_.crl() );
    if ( !crlfound )
    {
	pars.get( sKey::FirstCrl(), start_.crl() );
	pars.get( sKey::LastCrl(), stop_.crl() );
	pars.get( sKey::StepCrl(), step_.crl() );
    }
}


void HorSampling::fillPar( IOPar& pars ) const
{
    pars.set( sKey::FirstInl(), start_.inl() );
    pars.set( sKey::FirstCrl(), start_.crl() );
    pars.set( sKey::LastInl(), stop_.inl() );
    pars.set( sKey::LastCrl(), stop_.crl() );
    pars.set( sKey::StepInl(), step_.inl() );
    pars.set( sKey::StepCrl(), step_.crl() );
}


void HorSampling::useJSON( const OD::JSON::Object& obj )
{
    pos_steprg_type inlrg, crlrg;
    if ( obj.get(sKey::InlRange(),inlrg) )
	setInlRange( inlrg );
    if ( obj.get(sKey::CrlRange(),crlrg) )
	setCrlRange( crlrg );
}


void HorSampling::fillJSON( OD::JSON::Object& obj ) const
{
    obj.set( sKey::InlRange(), inlRange() );
    obj.set( sKey::CrlRange(), crlRange() );
}


void HorSampling::removeInfo( IOPar& par )
{
    par.removeWithKey( sKey::FirstInl() );
    par.removeWithKey( sKey::FirstCrl() );
    par.removeWithKey( sKey::LastInl() );
    par.removeWithKey( sKey::LastCrl() );
    par.removeWithKey( sKey::StepInl() );
    par.removeWithKey( sKey::StepCrl() );
    // legacy stuff
    par.removeWithKey( sKey::GeomID() );
    par.removeWithKey( sKey::FirstTrc() );
    par.removeWithKey( sKey::LastTrc() );
    par.removeWithKey( sKey::SurveyID() );
}


void HorSampling::toString( uiPhrase& str ) const
{
    str.appendPhrase(tr("Inline range: %1 - %2 [%3]"))
	.arg( start_.inl() ).arg( stop_.inl() ).arg( start_.inl() );
    str.appendPhrase(tr("Crossline range: %1 - %2 [%3]"))
	.arg( start_.trcNr() ).arg( stop_.trcNr() ).arg( start_.trcNr() );
}


CubeSampling::CubeSampling( bool tosi, OD::SurvLimitType slt )
    : hsamp_(false)
{
    init( tosi, slt );
}


CubeSampling::CubeSampling( const CubeSubSel& css )
    : hsamp_(css.cubeHorSubSel())
    , zsamp_(css.zRange())
{
}


CubeSampling::CubeSampling( const TrcKeySampling& tks )
    : hsamp_(tks)
    , zsamp_(SI().zRange())
{
}


CubeSampling::CubeSampling( const TrcKeyZSampling& tkzs )
    : hsamp_(tkzs.hsamp_)
    , zsamp_(tkzs.zsamp_)
{
}


CubeSampling::CubeSampling( const CubeSampling& oth )
{
    *this = oth;
}


void CubeSampling::init( bool tosi, OD::SurvLimitType slt )
{
    hsamp_.init( tosi, slt );
    if ( tosi )
	zsamp_ = SI().zRange( slt );
    else
	{ zsamp_.start = 0.f; zsamp_.stop = zsamp_.step = 1.f; }
}


bool CubeSampling::operator==( const CubeSampling& oth ) const
{
    return isEqual( oth );
}


bool CubeSampling::operator!=( const CubeSampling& oth ) const
{
    return !isEqual( oth );
}


void CubeSampling::normalise()
{
    hsamp_.normalise();
    if ( zsamp_.start > zsamp_.stop )
	std::swap(zsamp_.start,zsamp_.stop);
    if ( !zsamp_.step )
	zsamp_.step = SI().zStep();
    else if ( zsamp_.step < 0.f )
	zsamp_.step = -zsamp_.step;
}


bool CubeSampling::includes( const BinID& bid, z_type z ) const
{
    return hsamp_.includes( bid )
	&& zsamp_.isPresent( z );
}


bool CubeSampling::includes( const CubeSampling& oth ) const
{
    return hsamp_.includes( oth.hsamp_.start_ )
	&& hsamp_.includes( oth.hsamp_.stop_ )
	&& zsamp_.isCompatible( oth.zsamp_ )
	&& zsamp_.start < oth.zsamp_.start + 1e-6f
	&& zsamp_.stop > oth.zsamp_.stop - 1e-6f;
}


bool CubeSampling::isFlat() const
{
    return hsamp_.isLine()
	|| std::abs( zsamp_.stop-zsamp_.start ) < std::abs( zsamp_.step*0.5f );
}


od_int64 CubeSampling::totalNr() const
{
    return ((od_int64)nrZ()) * hsamp_.totalNr();
}


bool CubeSampling::isEqual( const CubeSampling& oth, z_type zeps ) const
{
    if ( this == &oth )
	return true;
    else if ( oth.hsamp_ != hsamp_ )
	return false;

    if ( mIsUdf(zeps) )
    {
	const float minzstep = mMIN( fabs(zsamp_.step),
				     fabs(oth.zsamp_.step) );
	zeps = mMAX( 1e-6f, (mIsUdf(minzstep) ? 0.0f : 0.001f*minzstep) );
    }

    z_type diff = oth.zsamp_.start - this->zsamp_.start;
    if ( std::abs(diff) > zeps )
	return false;

    diff = oth.zsamp_.stop - this->zsamp_.stop;
    if ( std::abs(diff) > zeps )
	return false;

    diff = oth.zsamp_.step - this->zsamp_.step;
    if ( std::abs(diff) > zeps )
	return false;

    return true;
}


void CubeSampling::include( const BinID& bid, float z )
{
    hsamp_.include( bid );
    zsamp_.include( z );
}


void CubeSampling::include( const CubeSampling& oth )
{
    normalise();
    CubeSampling normoth( oth );
    normoth.normalise();

    hsamp_.include( normoth.hsamp_ );
    if ( normoth.zsamp_.start < zsamp_.start )
	zsamp_.start = normoth.zsamp_.start;
    if ( normoth.zsamp_.stop > zsamp_.stop )
	zsamp_.stop = normoth.zsamp_.stop;
    if ( normoth.zsamp_.step < zsamp_.step )
	zsamp_.step = normoth.zsamp_.step;
}


void CubeSampling::limitTo( const CubeSampling& oth )
{
    hsamp_.limitTo( oth.hsamp_ );
    zsamp_.limitTo( oth.zsamp_ );
}


void CubeSampling::usePar( const IOPar& par )
{
    hsamp_.usePar( par );
    par.get( sKey::ZRange(), zsamp_ );
}


void CubeSampling::fillPar( IOPar& par ) const
{
    hsamp_.fillPar( par );
    par.set( sKey::ZRange(), zsamp_.start, zsamp_.stop, zsamp_.step );
}


void CubeSampling::useJSON( const OD::JSON::Object& obj )
{
    hsamp_.useJSON( obj );
    obj.get( sKey::ZRange(), zsamp_ );
}


void CubeSampling::fillJSON( OD::JSON::Object& obj ) const
{
    hsamp_.fillJSON( obj );
    obj.set( sKey::ZRange(), zsamp_ );
}


void CubeSampling::removeInfo( IOPar& par )
{
    HorSampling::removeInfo( par );
    par.removeWithKey( sKey::ZRange() );
}
