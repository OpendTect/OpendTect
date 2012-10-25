/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : somewhere around 1999
-*/
 
static const char* rcsID mUsedVar = "$Id$";

#include "cubesampling.h"

#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

#include <math.h>


void HorSampling::init( bool tosi )
{
    if ( tosi )
    {
	start = SI().sampling(false).hrg.start;
	stop = SI().sampling(false).hrg.stop;
	step = SI().sampling(false).hrg.step;
    }
    else
    {
	start.inl = start.crl = stop.inl = stop.crl = mUdf(int);
	step.inl = step.crl = 1;
    }
}


BinID HorSampling::atIndex(  od_int64 globalidx ) const
{
    const int nrcrl = nrCrl();
    if ( !nrcrl )
	return BinID(0,0);

    const int inlidx = (int)(globalidx/nrcrl);
    const int crlidx = (int)(globalidx%nrcrl);
    return atIndex( inlidx, crlidx );
}


void HorSampling::set2DDef()
{
    start.inl = start.crl = 0;
    stop.inl = stop.crl = mUdf(int);
    step.inl = step.crl = 1;
}


HorSampling& HorSampling::set( const Interval<int>& inlrg,
			       const Interval<int>& crlrg )
{
    setInlRange( inlrg );
    setCrlRange( crlrg );
    return *this;
}


void HorSampling::setInlRange( const Interval<int>& inlrg )
{
    start.inl = inlrg.start; stop.inl = inlrg.stop;
    mDynamicCastGet(const StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	step.inl = inlsrg->step;
}


void HorSampling::setCrlRange( const Interval<int>& crlrg )
{
    start.crl = crlrg.start; stop.crl = crlrg.stop;
    mDynamicCastGet(const StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	step.crl = crlsrg->step;
}


StepInterval<int> HorSampling::inlRange() const
{ return StepInterval<int>( start.inl, stop.inl, step.inl ); }


StepInterval<int> HorSampling::crlRange() const
{ return StepInterval<int>( start.crl, stop.crl, step.crl ); }


bool HorSampling::includes( const HorSampling& hs, bool ignoresteps ) const
{
    if ( ignoresteps )
	return hs.start.inl >= start.inl && hs.stop.inl <= stop.inl
	    && hs.start.crl >= start.crl && hs.stop.crl <= stop.crl;

    return includes(hs.start) && includes(hs.stop)
	&& step.inl && !(hs.step.inl % step.inl)
	&& step.crl && !(hs.step.crl % step.crl);
}


void HorSampling::includeInl( int inl )
{
    if ( mIsUdf(start.inl) || mIsUdf(stop.inl) || nrInl()<1 )
	start.inl = stop.inl = inl;
    else
    {
	start.inl = mMIN( start.inl, inl );
	stop.inl = mMAX( stop.inl, inl );
    }
}


void HorSampling::includeCrl( int crl )
{
    if ( mIsUdf(start.crl) || mIsUdf(stop.crl) || nrCrl()<1 )
	start.crl = stop.crl = crl;
    else
    {
	start.crl = mMIN( start.crl, crl );
	stop.crl = mMAX( stop.crl, crl );
    }
}


void HorSampling::include( const HorSampling& hs, bool ignoresteps )
{
    if ( ignoresteps )
    {
	include( hs.start );
	include( hs.stop );
	return;
    }

    HorSampling temp( *this );
    temp.include( hs.start );
    temp.include( hs.stop );

#define mHandleIC( ic ) \
    const int newstart##ic = temp.start.ic; \
    const int newstop##ic = temp.stop.ic; \
    int offset##ic = mIsUdf(start.ic) || mIsUdf(hs.start.ic) ? 0 \
	: ( start.ic != newstart##ic ? start.ic - newstart##ic \
				     : hs.start.ic - newstart##ic ); \
    step.ic = Math::HCFOf( step.ic, hs.step.ic ); \
    if ( offset##ic ) step.ic = Math::HCFOf( step.ic, offset##ic ); \
    start.ic = newstart##ic; stop.ic = newstop##ic

    mHandleIC(inl);
    mHandleIC(crl);
}


void HorSampling::get( Interval<int>& inlrg, Interval<int>& crlrg ) const
{
    inlrg.start = start.inl; inlrg.stop = stop.inl;
    mDynamicCastGet(StepInterval<int>*,inlsrg,&inlrg)
    if ( inlsrg )
	inlsrg->step = step.inl;
    crlrg.start = start.crl; crlrg.stop = stop.crl;
    mDynamicCastGet(StepInterval<int>*,crlsrg,&crlrg)
    if ( crlsrg )
	crlsrg->step = step.crl;
}


bool HorSampling::isDefined() const
{
    return !mIsUdf(start.inl) && !mIsUdf(start.crl) &&
	   !mIsUdf(stop.inl) && !mIsUdf(stop.crl) &&
	   !mIsUdf(step.inl) && !mIsUdf(step.crl);
}


void HorSampling::limitTo( const HorSampling& h )
{
    HorSampling hs( h ); hs.normalise();
    normalise();

    if ( hs.start.inl > stop.inl || hs.stop.inl < start.inl ||
	 hs.start.crl > stop.crl || hs.stop.crl < start.crl )
    {
	init( false );
	return;
    }
    
    if ( hs.start.inl > start.inl ) start.inl = hs.start.inl;
    if ( hs.start.crl > start.crl ) start.crl = hs.start.crl;
    if ( hs.stop.crl < stop.crl ) stop.crl = hs.stop.crl;
    if ( hs.stop.inl < stop.inl ) stop.inl = hs.stop.inl;
    if ( hs.step.inl > step.inl ) step.inl = hs.step.inl;
    if ( hs.step.crl > step.crl ) step.crl = hs.step.crl;
}


# define mAdjustIf(v1,op,v2) \
      if ( !mIsUdf(v1) && !mIsUdf(v2) && v1 op v2 ) v1 = v2;

void HorSampling::limitToWithUdf( const HorSampling& h )
{
    HorSampling hs( h ); hs.normalise();
    normalise();

    mAdjustIf(start.inl,<,hs.start.inl);
    mAdjustIf(start.crl,<,hs.start.crl);
    mAdjustIf(stop.inl,>,hs.stop.inl);
    mAdjustIf(stop.crl,>,hs.stop.crl);
    mAdjustIf(step.inl,<,hs.step.inl);
    mAdjustIf(step.crl,<,hs.step.crl);
}


bool HorSampling::usePar( const IOPar& pars )
{
    bool ret = pars.get( sKey::FirstInl(), start.inl );
    ret = pars.get( sKey::FirstCrl(), start.crl ) || ret;
    ret = pars.get( sKey::LastInl(), stop.inl ) || ret;
    ret = pars.get( sKey::LastCrl(), stop.crl ) || ret;
    pars.get( sKey::StepInl(), step.inl );
    pars.get( sKey::StepCrl(), step.crl );
    return ret;
}


void HorSampling::fillPar( IOPar& pars ) const
{
    pars.set( sKey::FirstInl(), start.inl );
    pars.set( sKey::FirstCrl(), start.crl );
    pars.set( sKey::LastInl(), stop.inl );
    pars.set( sKey::LastCrl(), stop.crl );
    pars.set( sKey::StepInl(), step.inl );
    pars.set( sKey::StepCrl(), step.crl );
}


void HorSampling::removeInfo( IOPar& par )
{
    par.removeWithKey( sKey::FirstInl() );
    par.removeWithKey( sKey::FirstCrl() );
    par.removeWithKey( sKey::LastInl() );
    par.removeWithKey( sKey::LastCrl() );
    par.removeWithKey( sKey::StepInl() );
    par.removeWithKey( sKey::StepCrl() );
}


int HorSampling::nrInl() const
{
    if ( (mIsUdf(start.inl) && mIsUdf(stop.inl)) )
	return 0;
    
    if ( !step.inl )
	return 0;

    if ( start.inl==stop.inl )
	return 1;
    
    int ret = inlIdx( stop.inl );
    return ret < 0 ? 1 - ret : ret + 1;
}


int HorSampling::nrCrl() const
{
    if ( (mIsUdf(start.crl) && mIsUdf(stop.crl)) )
	return 0;
    
    if ( !step.crl )
	return 0;

    if ( start.crl==stop.crl )
	return 1;
    
    int ret = crlIdx( stop.crl );
    return ret < 0 ? 1 - ret : ret + 1;
}


static bool intersect(	int start1, int stop1, int step1,
			int start2, int stop2, int step2,
			int& outstart, int& outstop, int& outstep )
{
    if ( stop1 < start2 || start1 > stop2 )
	return false;

    // Determine step. Only accept reasonable step differences
    outstep = step2 > step1 ? step2 : step1;
    int lostep = step2 > step1 ? step1 : step2;
    if ( !lostep || outstep%lostep ) return false;

    // Snap start
    outstart = start1 < start2 ? start2 : start1;
    while ( (outstart-start1) % step1 )
	outstart += lostep;
    while ( (outstart-start2) % step2 )
	outstart += lostep;

    // Snap stop
    outstop = stop1 > stop2 ? stop2 : stop1;
    int nrsteps = (outstop - outstart) / outstep;
    outstop = outstart + nrsteps * outstep;
    return outstop >= outstart;
}

#define Eps 2e-5

inline bool IsZero( float f, float eps=Eps )
{
    return f > -eps && f < eps;
}

static inline bool inSeries( float v, float start, float step )
{
    float fdiff = (start - v) / step;
    int idiff = mNINT32( fdiff );
    fdiff -= (float)idiff;
    return IsZero( fdiff, 1e-3 );
}


static bool intersectF(	float start1, float stop1, float step1,
			float start2, float stop2, float step2,
			float& outstart, float& outstop, float& outstep )
{
    if ( stop1-start2 < mDefEps || start1-stop2 > mDefEps )
	return false;

    outstep = step2 > step1 ? step2 : step1;
    float lostep = step2 > step1 ? step1 : step2;
    if ( IsZero(lostep) ) return false;

    // See if starts are compatible
    if ( !inSeries(start1,start2,lostep) )
	return false;

    // Only accept reasonable step differences
    int ifac = 1;
    for ( ; ifac<2001; ifac++ )
    {
	float stp = ifac * lostep;
	if ( IsZero(stp-outstep) ) break;
	else if ( ifac == 2000 ) return false;
    }

    outstart = start1 < start2 ? start2 : start1;
    while ( !inSeries(outstart,start1,step1)
	 || !inSeries(outstart,start2,step2) )
	outstart += lostep;

    // Snap stop
    outstop = stop1 > stop2 ? stop2 : stop1;
    int nrsteps = (int)( (outstop - outstart + Eps) / outstep );
    outstop = outstart + nrsteps * outstep;
    return (outstop-outstart) > Eps;
}


bool HorSampling::getInterSection( const HorSampling& hs,
				   HorSampling& out ) const
{
    HorSampling hs1( hs );	hs1.normalise();
    HorSampling hs2( *this );	hs2.normalise();

    return intersect( hs1.start.inl, hs1.stop.inl, hs1.step.inl,
		      hs2.start.inl, hs2.stop.inl, hs2.step.inl,
		      out.start.inl, out.stop.inl, out.step.inl )
	&& intersect( hs1.start.crl, hs1.stop.crl, hs1.step.crl,
		      hs2.start.crl, hs2.stop.crl, hs2.step.crl,
		      out.start.crl, out.stop.crl, out.step.crl );
}


void HorSampling::snapToSurvey()
{
    SI().snap( start, BinID(-1,-1) );
    SI().snap( stop, BinID(1,1) );
}


void HorSampling::toString( BufferString& str ) const
{
    str.add( "Inline range: " ).add( start.inl ).add( " - " ).add( stop.inl )
       .add( " [" ).add( step.inl ).add( "]\n" );
    str.add( "Crossline range: " ).add( start.crl ).add( " - " ).add( stop.crl )
       .add( " [" ).add( step.crl ).add( "]" );
}


void HorSampling::getRandomSet( int nr, TypeSet<BinID>& bidset ) const
{
    if ( nr > totalNr() )
	nr = (int) totalNr();

    while ( nr )
    {
	BinID bid( inlRange().start + std::rand() % nrInl(),
		   crlRange().start + std::rand() % nrCrl() );
	if ( includes(bid) && bidset.addIfNew(bid) )
	    nr--;
    }
}


// CubeSampling
void CubeSampling::set2DDef()
{
    hrg.set2DDef();
    zrg = SI().zRange(false);
}


void CubeSampling::init( bool tosi )
{
    hrg.init( tosi );
    if ( tosi )
	zrg = SI().zRange(false);
    else
	{ zrg.start = zrg.stop = 0; zrg.step = 1; }
}


static void normaliseZ( StepInterval<float>& zrg )
{
    if ( zrg.start > zrg.stop )	Swap(zrg.start,zrg.stop);
    if ( zrg.step < 0 )		zrg.step = -zrg.step;
    else if ( !zrg.step )	zrg.step = SI().zStep();
}


bool CubeSampling::getIntersection( const CubeSampling& cs,
				    CubeSampling& out ) const
{
    if ( !hrg.getInterSection(cs.hrg,out.hrg) )
	return false;

    StepInterval<float> zrg1( cs.zrg );	normaliseZ( zrg1 );
    StepInterval<float> zrg2( zrg );	normaliseZ( zrg2 );
    return intersectF( zrg1.start, zrg1.stop, zrg1.step,
		       zrg2.start, zrg2.stop, zrg2.step,
		       out.zrg.start, out.zrg.stop, out.zrg.step );
}
    

bool CubeSampling::isFlat() const
{
    if ( hrg.start.inl==hrg.stop.inl || hrg.start.crl==hrg.stop.crl )
	return true;

    return fabs( zrg.stop-zrg.start ) < fabs( zrg.step * 0.5 );
}


CubeSampling::Dir CubeSampling::defaultDir() const
{
    const int nrinl = nrInl();
    const int nrcrl = nrCrl();
    const int nrz = nrZ();
    if ( nrz < nrinl && nrz < nrcrl )
	return Z;

    return nrinl < nrcrl ? Inl : Crl;
}


Coord3 CubeSampling::defaultNormal() const
{
    if ( defaultDir() == Inl )
	return Coord3( SI().binID2Coord().rowDir(), 0 );

    if ( defaultDir() == Crl )
	return Coord3( SI().binID2Coord().colDir(), 0 );

    return Coord3( 0, 0, 1 );
} 


od_int64 CubeSampling::totalNr() const
{ return ((od_int64) nrZ()) * ((od_int64) hrg.totalNr()); }


bool CubeSampling::includes( const CubeSampling& c ) const
{
    return hrg.includes( c.hrg ) && 
	   zrg.includes( c.zrg.start, false ) &&
	   zrg.includes( c.zrg.stop, false );
}


void CubeSampling::include( const BinID& bid, float z )
{
    hrg.include( bid );
    zrg.include( z );
}


void CubeSampling::include( const CubeSampling& c )
{
    CubeSampling cs( c ); cs.normalise();
    normalise();

    hrg.include( cs.hrg );
    if ( cs.zrg.start < zrg.start ) zrg.start = cs.zrg.start;
    if ( cs.zrg.stop > zrg.stop ) zrg.stop = cs.zrg.stop;
    if ( cs.zrg.step < zrg.step ) zrg.step = cs.zrg.step;
}


bool CubeSampling::isDefined() const
{
    return hrg.isDefined() &&
	!mIsUdf(zrg.start) && !mIsUdf(zrg.stop) && !mIsUdf(zrg.step);
}



void CubeSampling::limitTo( const CubeSampling& c )
{
    CubeSampling cs( c ); cs.normalise();
    normalise();
    hrg.limitTo( cs.hrg );
    if ( hrg.isEmpty() || cs.zrg.start > zrg.stop || cs.zrg.stop < zrg.start )
    {
	init( false );
	return;
    }

    if ( zrg.start < cs.zrg.start ) zrg.start = cs.zrg.start;
    if ( zrg.stop > cs.zrg.stop ) zrg.stop = cs.zrg.stop;
    if ( zrg.step < cs.zrg.step ) zrg.step = cs.zrg.step;
}


void CubeSampling::limitToWithUdf( const CubeSampling& c )
{
    CubeSampling cs( c ); cs.normalise();
    normalise();
    hrg.limitToWithUdf( cs.hrg );
    mAdjustIf(zrg.start,<,cs.zrg.start);
    mAdjustIf(zrg.stop,>,cs.zrg.stop);
}


void CubeSampling::snapToSurvey()
{
    hrg.snapToSurvey();
    SI().snapZ( zrg.start, -1 );
    SI().snapZ( zrg.stop, 1 );
}


bool CubeSampling::operator==( const CubeSampling& cs ) const
{
    if ( this == &cs ) return true;

   if ( cs.hrg == this->hrg )
   {
       float diff = cs.zrg.start - this->zrg.start;
       const float eps = (float) ( SI().zIsTime() ? 1e-6 : 1e-3 );
       if ( fabs(diff) > eps ) return false;

       diff = cs.zrg.stop - this->zrg.stop;
       if ( fabs(diff) > eps ) return false;

       diff = cs.zrg.step - this->zrg.step;
       if ( fabs(diff) > eps ) return false;

       return true;
   }
   
   return false;
}


bool CubeSampling::usePar( const IOPar& par )
{
    return hrg.usePar( par ) && par.get( sKey::ZRange(), zrg );
}


void CubeSampling::fillPar( IOPar& par ) const
{
    hrg.fillPar( par );
    par.set( sKey::ZRange(), zrg.start, zrg.stop, zrg.step );
}


void CubeSampling::removeInfo( IOPar& par )
{
    HorSampling::removeInfo( par );
    par.removeWithKey( sKey::ZRange() );
}


void HorSampling::normalise()
{
    if ( start.inl > stop.inl )	Swap(start.inl,stop.inl);
    if ( start.crl > stop.crl )	Swap(start.crl,stop.crl);
    if ( step.inl < 0 )		step.inl = -step.inl;
    else if ( !step.inl )	step.inl = SI().inlStep();
    if ( step.crl < 0 )		step.crl = -step.crl;
    else if ( !step.crl )	step.crl = SI().crlStep();
}


void CubeSampling::normalise()
{
    hrg.normalise();
    normaliseZ( zrg );
}


bool HorSamplingIterator::next( BinID& bid )
{
    if ( firstpos_ )
    {
	bid = hrg_.start;
	firstpos_ = false;
	return true;
    }

    bid.crl += hrg_.step.crl;
    if ( bid.crl > hrg_.stop.crl )
    {
	bid.inl += hrg_.step.inl;
	bid.crl = hrg_.start.crl;
    }

    return bid.inl <= hrg_.stop.inl;
}
