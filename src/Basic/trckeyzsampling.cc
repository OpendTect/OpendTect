/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "trckeyzsampling.h"

#include "iopar.h"
#include "keystrs.h"
#include "odjson.h"
#include "survinfo.h"
#include "uistrings.h"

#include <math.h>

mStartAllowDeprecatedSection

TrcKeyZSampling::TrcKeyZSampling()
    : hrg(hsamp_),zrg(zsamp_)
{
    init( true );
}


TrcKeyZSampling::TrcKeyZSampling( const TrcKeyZSampling& tkzs )
    : hrg(hsamp_), zrg(zsamp_)
{
    *this = tkzs;
}


TrcKeyZSampling::TrcKeyZSampling( bool settoSI )
    : hrg(hsamp_), zrg(zsamp_)
{
    init( settoSI );
}


TrcKeyZSampling::TrcKeyZSampling( const Pos::GeomID& geomid )
    : hrg(hsamp_), zrg(zsamp_)
{
    hsamp_.setGeomID( geomid );
}

mStopAllowDeprecatedSection


TrcKeyZSampling TrcKeyZSampling::getSynth( const Interval<int>* trcrg,
					   const ZSampling* zrg )
{
    TrcKeyZSampling tkzs;
    tkzs.hsamp_ = TrcKeySampling::getSynth( trcrg );
    tkzs.zsamp_ = zrg ? *zrg : ZSampling::udf();

    return tkzs;
}


TrcKeyZSampling::~TrcKeyZSampling()
{}



mDefineEnumUtils(TrcKeyZSampling,Dir,"Direction")
{ "Inline","Crossline", "ZSlice", nullptr };

template<>
void EnumDefImpl<TrcKeyZSampling::Dir>::init()
 {
     uistrings_ += uiStrings::sInline();
     uistrings_ += uiStrings::sCrossline();
     uistrings_ += uiStrings::sZSlice();
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
	if ( falsereturnsdummy )
	{
	    *this = compatibletkzs;
	    hsamp_.start_ -= hsamp_.step_;
	    zsamp_.start -= zsamp_.step;
	    hsamp_.stop_ = hsamp_.start_;
	    zsamp_.stop = zsamp_.start;
	}
	else
	    init( false );

	return false;
    }

    TrcKeyZSampling adjustedtkzs( compatibletkzs );
    adjustedtkzs.shrinkTo( *this );

    // Only keep adjustments for non-flat dimensions. Preserve step if flat.
    if ( nrLines() == 1 )
	adjustedtkzs.hsamp_.setLineRange( (Interval<int>)hsamp_.lineRange() );
    if ( nrTrcs() == 1 )
	adjustedtkzs.hsamp_.setTrcRange( (Interval<int>)hsamp_.trcRange() );
    if ( nrZ() == 1 )
	adjustedtkzs.zsamp_ = zsamp_;

    *this = adjustedtkzs;
    return true;
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
    return true;
}


static bool intersectFIgnoreSteps( float start_1, float stop_1,
				   float start_2, float stop_2,
				   float& outstart_, float& outstop_ )
{
    if ( stop_1-start_2 < mDefEps || start_1-stop_2 > mDefEps )
	return false;

    outstart_ = start_1 < start_2 ? start_2 : start_1;
    outstop_ = stop_1 > stop_2 ? stop_2 : stop_1;
    return (outstop_ - outstart_) >= -mDefEps;
}


void TrcKeyZSampling::set2DDef()
{
    hsamp_.set2DDef();
    zsamp_ = SI().zRange(false);
}


bool TrcKeyZSampling::init( const Pos::GeomID& geomid )
{
    if ( Survey::isSynthetic(geomid) )
    {
	hsamp_.init( geomid );
	return true;
    }

    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry( geomid );
    if ( !geom )
	return false;

    hsamp_.survid_ = geom->geomSystem();
    (*this) = geom->sampling();
    return true;
}


void TrcKeyZSampling::init( bool tosi )
{
    hsamp_.init( tosi );
    if ( tosi )
	zsamp_ = SI().zRange(false);
    else
	zsamp_.set( 0, 0, 1 );
}


static void normalizeZ( StepInterval<float>& zsamp )
{
    if ( zsamp.start > zsamp.stop )
	Swap(zsamp.start,zsamp.stop);

    if ( zsamp.step < 0 )
	zsamp.step = -zsamp.step;
    else if ( mIsZero(zsamp.step,mDefEpsF) )
	zsamp.step = SI().zStep();
}


bool TrcKeyZSampling::getIntersection( const TrcKeyZSampling& tkzs,
				    TrcKeyZSampling& out,
				    bool ignoresteps ) const
{
    const bool hintersect = hsamp_.getInterSection( tkzs.hsamp_,out.hsamp_,
						    ignoresteps );
    if ( !hintersect )
	return false;

    StepInterval<float> zsamp1( tkzs.zsamp_ );
    normalizeZ( zsamp1 );
    StepInterval<float> zsamp2( zsamp_ );
    normalizeZ( zsamp2 );
    const bool zintersect = ignoresteps ? intersectFIgnoreSteps( zsamp1.start,
								 zsamp1.stop,
								 zsamp2.start,
								 zsamp2.stop,
							     out.zsamp_.start,
							     out.zsamp_.stop )
					: intersectF( zsamp1.start, zsamp1.stop,
						      zsamp1.step, zsamp2.start,
						      zsamp2.stop, zsamp2.step,
						      out.zsamp_.start,
						      out.zsamp_.stop,
						      out.zsamp_.step );
    return (nrZ()==1 && tkzs.nrZ()==1 && zsamp1==zsamp2) || zintersect;
}


bool TrcKeyZSampling::isFlat() const
{
    if ( hsamp_.start_.lineNr()==hsamp_.stop_.lineNr() ||
	 hsamp_.start_.trcNr()==hsamp_.stop_.trcNr() )
	return true;

    return fabs( zsamp_.stop-zsamp_.start ) < fabs( zsamp_.step * 0.5 );
}


TrcKeyZSampling::Dir TrcKeyZSampling::defaultDir() const
{
    const int nrinl = nrLines();
    const int nrcrl = nrTrcs();
    const int nrz = nrZ();
    if ( nrz < nrinl && nrz < nrcrl )
	return Z;

    return nrinl<=nrcrl ? Inl : Crl;
}


void TrcKeyZSampling::getDefaultNormal( Coord3& ret ) const
{
    if ( defaultDir() == Inl )
	ret = Coord3( SI().binID2Coord().inlDir(), 0 );
    else if ( defaultDir() == Crl )
	ret = Coord3( SI().binID2Coord().crlDir(), 0 );
    else
	ret = Coord3( 0, 0, 1 );
}


od_int64 TrcKeyZSampling::totalNr() const
{ return ((od_int64) nrZ()) * ((od_int64) hsamp_.totalNr()); }

int TrcKeyZSampling::lineIdx(int lineid)const
{return hsamp_.lineIdx(lineid);}


int TrcKeyZSampling::trcIdx( int trcnr ) const
{return hsamp_.trcIdx(trcnr); }


int TrcKeyZSampling::zIdx( float z ) const
{ return zsamp_.nearestIndex(z); }


int TrcKeyZSampling::nrLines() const
{ return hsamp_.nrLines(); }


int TrcKeyZSampling::nrTrcs() const
{ return hsamp_.nrTrcs(); }


int TrcKeyZSampling::nrZ() const
{ return zsamp_.nrSteps() + 1; }


int TrcKeyZSampling::size( Dir d ) const
{ return d == Inl
    ? nrInl()
    : (d == Crl
       ? nrCrl()
       : nrZ());
}


float TrcKeyZSampling::zAtIndex( int idx ) const
{ return zsamp_.atIndex(idx); }


bool TrcKeyZSampling::isEmpty() const
{ return hsamp_.isEmpty() || zsamp_.isUdf(); }


bool TrcKeyZSampling::operator!=( const TrcKeyZSampling& tkzs ) const
{ return !(tkzs==*this); }


TrcKeyZSampling& TrcKeyZSampling::operator=(const TrcKeyZSampling& b)
{
    hsamp_ = b.hsamp_;
    zsamp_ = b.zsamp_;
    return *this;
}


bool TrcKeyZSampling::includes( const TrcKeyZSampling& c ) const
{
    return hsamp_.includes( c.hsamp_ ) &&
	   zsamp_.includes( c.zsamp_.start, false ) &&
	   zsamp_.includes( c.zsamp_.stop, false );
}


void TrcKeyZSampling::include( const BinID& bid, float z )
{
    hsamp_.include( bid );
    zsamp_.include( z );
}


void TrcKeyZSampling::include( const TrcKeyZSampling& c )
{
    TrcKeyZSampling tkzs( c ); tkzs.normalize();
    normalize();

    hsamp_.include( tkzs.hsamp_ );
    if ( tkzs.zsamp_.start < zsamp_.start ) zsamp_.start = tkzs.zsamp_.start;
    if ( tkzs.zsamp_.stop > zsamp_.stop ) zsamp_.stop = tkzs.zsamp_.stop;
    if ( tkzs.zsamp_.step < zsamp_.step ) zsamp_.step = tkzs.zsamp_.step;
}


bool TrcKeyZSampling::isDefined() const
{
    return hsamp_.isDefined() && !zsamp_.isUdf();
}


void TrcKeyZSampling::limitTo( const TrcKeyZSampling& tkzs, bool ignoresteps )
{
    hsamp_.limitTo( tkzs.hsamp_, ignoresteps );
    if ( hsamp_.isEmpty() )
    {
	init( false );
	return;
    }

    if ( ignoresteps )
    {
	const ZGate othzsamp_( tkzs.zsamp_ );
	sCast(ZGate&,zsamp_).limitTo( othzsamp_ );
    }
    else
	zsamp_.limitTo( tkzs.zsamp_ );

}


# define mAdjustIf(v1,op,v2) \
      if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;

#define mSnapStop( start, stop, step, eps ) \
    stop = start + step * mCast( int, (stop-start+eps)/step );

#define mApproach( diff, var, assignoper, step ) \
    if ( diff>0 ) \
	var assignoper step * mCast( int, (diff)/step );


void TrcKeyZSampling::limitToWithUdf( const TrcKeyZSampling& c )
{
    TrcKeyZSampling tkzs( c ); tkzs.normalize();
    normalize();
    hsamp_.limitToWithUdf( tkzs.hsamp_ );
    mAdjustIf(zsamp_.start,<,tkzs.zsamp_.start);
    mAdjustIf(zsamp_.stop,>,tkzs.zsamp_.stop);
}


void TrcKeyZSampling::shrinkTo( const TrcKeyZSampling& innertkzs, float releps )
{
    normalize();
    TrcKeyZSampling tkzs( innertkzs );
    tkzs.normalize();

    hsamp_.shrinkTo( tkzs.hsamp_ );

    float eps = Math::Abs( mMAX(zsamp_.start,zsamp_.stop) );
    if ( eps == 0.f )
	eps = zsamp_.step == 0.f ? mDefEpsF : zsamp_.step;
    eps *= releps;
    if ( zsamp_.isEqual(tkzs.zsamp_,eps) )
	return;

    mSnapStop( zsamp_.start, zsamp_.stop, zsamp_.step, eps );
    mApproach(tkzs.zsamp_.start-zsamp_.start+eps, zsamp_.start,+=, zsamp_.step);
    mApproach(zsamp_.stop - tkzs.zsamp_.stop+eps, zsamp_.stop, -=, zsamp_.step);
}


void TrcKeyZSampling::growTo( const TrcKeyZSampling& outertkzs, float releps )
{
    normalize();
    TrcKeyZSampling tkzs( outertkzs );
    tkzs.normalize();

    hsamp_.growTo( tkzs.hsamp_ );

    float eps = Math::Abs( mMAX(zsamp_.start,zsamp_.stop) );
    if ( eps == 0.f )
	eps = zsamp_.step == 0.f ? mDefEpsF : zsamp_.step;
    eps *= releps;
    if ( zsamp_.isEqual(tkzs.zsamp_,eps) )
	return;

    mSnapStop( zsamp_.start, zsamp_.stop, zsamp_.step, eps );
    mApproach(zsamp_.start-tkzs.zsamp_.start+eps, zsamp_.start,-=, zsamp_.step);
    mApproach(tkzs.zsamp_.stop - zsamp_.stop+eps, zsamp_.stop, +=, zsamp_.step);
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
    SI().snapZ( zsamp_.start, -1 );
    SI().snapZ( zsamp_.stop, 1 );
}


bool TrcKeyZSampling::operator==( const TrcKeyZSampling& tkzs ) const
{
    return isEqual( tkzs, (SI().zIsTime() ? 1e-6f : 1e-3f) );
}


bool TrcKeyZSampling::isEqual( const TrcKeyZSampling& tkzs, float zeps ) const
{
    if ( this == &tkzs )
	return true;

    if ( tkzs.hsamp_ == this->hsamp_ )
    {
	if ( mIsUdf(zeps) )
	{
	    const float minzstep = mMIN( fabs(zsamp_.step),
					 fabs(tkzs.zsamp_.step) );
	    zeps = mMAX( 1e-6f, (mIsUdf(minzstep) ? 0.0f : 0.001f*minzstep) );
	}

	float diff = tkzs.zsamp_.start - this->zsamp_.start;
	if ( fabs(diff) > zeps )
	    return false;

	diff = tkzs.zsamp_.stop - this->zsamp_.stop;
	if ( fabs(diff) > zeps )
	    return false;

	diff = tkzs.zsamp_.step - this->zsamp_.step;
	if ( fabs(diff) > zeps )
	    return false;

	return true;
    }

   return false;
}


bool TrcKeyZSampling::usePar( const IOPar& par )
{
    bool isok = hsamp_.usePar( par );
    PtrMan<IOPar> subpars;
    const bool is2d = hsamp_.is2D();
    if ( is2d )
    {
	subpars = par.subselect( IOPar::compKey(sKey::Line(),0) );
	if ( !subpars )
	    return false;
    }

    const IOPar& iop = is2d ? *subpars.ptr() : par;
    if ( !iop.get("Z Subsel.0.Range",zsamp_) )
	isok = isok && iop.get( sKey::ZRange(), zsamp_ );

    return isok;
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


void TrcKeyZSampling::removeInfo( IOPar& par )
{
    TrcKeySampling::removeInfo( par );
    par.removeWithKey( sKey::ZRange() );
}


void TrcKeyZSampling::fillJSON( OD::JSON::Object& obj ) const
{
    hsamp_.fillJSON( obj );
    const BufferString zrgintrvl( sKey::ZRange(), "Interval" );
    obj.set( zrgintrvl, zsamp_ );
}


bool TrcKeyZSampling::useJSON( const OD::JSON::Object& obj )
{
    hsamp_.useJSON( obj );
    const BufferString zrgintrvl( sKey::ZRange(), "Interval" );
    obj.get( zrgintrvl, zsamp_ );
    return true;
}


void TrcKeyZSampling::normalise()
{ normalize(); }

void TrcKeyZSampling::normalize()
{
    hsamp_.normalize();
    normalizeZ( zsamp_ );
}


void TrcKeyZSampling::fillInfoSet( StringPairSet& infoset, float zscale ) const
{
    hsamp_.fillInfoSet( infoset );

    const float zfactor = mIsUdf(zscale) ? (SI().zIsTime() ? 1000 : 1) : zscale;
    uiString str = ::toUiString("%1 - %2 [%3]; Total: %4")
			.arg(zsamp_.start*zfactor)
			.arg(zsamp_.stop*zfactor)
			.arg(zsamp_.step*zfactor)
			.arg(nrZ());
    infoset.add( sKey::ZRange(), str.getFullString() );
}


TrcKeyZSampling::Dir direction( TrcKeyZSampling::Dir slctype , int dimnr )
{
    if ( dimnr == 0 )
	return slctype;
    else if ( dimnr == 1 )
	return slctype == TrcKeyZSampling::Inl ? TrcKeyZSampling::Crl
	: TrcKeyZSampling::Inl;
    else
	return slctype == TrcKeyZSampling::Z
	? TrcKeyZSampling::Crl
	: TrcKeyZSampling::Z;
}


int dimension( TrcKeyZSampling::Dir slctype,
	       TrcKeyZSampling::Dir direction )
{
    if ( slctype == direction )
	return 0;
    else if ( direction == TrcKeyZSampling::Z )
	return 2;
    else if ( direction == TrcKeyZSampling::Inl )
	return 1;

    return slctype == TrcKeyZSampling::Z ? 2 : 1;
}
