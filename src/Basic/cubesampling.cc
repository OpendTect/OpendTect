/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : somewhere around 1999
-*/
 
static const char* rcsID = "$Id: cubesampling.cc,v 1.5 2004-04-01 13:39:50 bert Exp $";

#include "cubesampling.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "iopar.h"
#include <math.h>

const char* HorSampling::startstr = "Start";
const char* HorSampling::stopstr = "Stop";
const char* HorSampling::stepstr = "Step";

const char* CubeSampling::zrangestr = "Z range";

void HorSampling::init()
{
    start = SI().range(false).start;
    stop = SI().range(false).stop;
    step.inl = SI().inlStep();
    step.crl = SI().crlStep();
}


void CubeSampling::init()
{
    hrg.init();
    assign( zrg, SI().zRange(false) );
}


HorSampling::HorSampling( const BinIDRange& bidr )
{ set( bidr ); }
HorSampling::HorSampling( const BinIDSampler& bs )
{ set( bs ); }
HorSampling& HorSampling::set( const BinIDSampler& bs )
{ set( (const BinIDRange&)bs ); return *this; }


HorSampling& HorSampling::set( const BinIDRange& bidr )
{
    start = bidr.start;
    stop = bidr.stop;
    if ( bidr.type() == 1 )
	step = ((const BinIDSampler&)bidr).step;
    return *this;
}


bool HorSampling::usePar( const IOPar& par )
{
    return
	par.get( startstr, start.inl, start.crl ) &&
	par.get( stopstr, stop.inl, stop.crl ) &&
	par.get( stepstr, step.inl, step.crl );
}


void HorSampling::fillPar( IOPar& par ) const
{
    par.set( startstr, start.inl, start.crl );
    par.set( stopstr, stop.inl, stop.crl );
    par.set( stepstr, step.inl, step.crl );
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

inline bool IsZero( float f )
{
    return f > -Eps && f < Eps;
}

static inline bool inSeries( float v, float start, float step )
{
    float fdiff = (start - v) / step;
    int idiff = mNINT( fdiff );
    fdiff -= idiff;
    return IsZero( fdiff );
}


static bool intersectF(	float start1, float stop1, float step1,
			float start2, float stop2, float step2,
			float& outstart, float& outstop, float& outstep )
{
    if ( stop1-start2 < mEPSILON || start1-stop2 > mEPSILON )
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
    int nrsteps = (int)((outstop - outstart) / outstep + Eps);
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


void HorSampling::snapToSurvey(bool work)
{
    SI().snap( start, BinID(-1,-1), work );
    SI().snap( stop, BinID(1,1), work );
}



static void normaliseZ( StepInterval<float>& zrg )
{
    if ( zrg.start > zrg.stop )	Swap(zrg.start,zrg.stop);
    if ( zrg.step < 0 )		zrg.step = -zrg.step;
    else if ( !zrg.step )	zrg.step = SI().zRange(false).step;
}


bool CubeSampling::getInterSection( const CubeSampling& cs,
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


void CubeSampling::snapToSurvey(bool work)
{
    hrg.snapToSurvey(work);
    zrg.start = SI().zRange(work).snap( zrg.start-SI().zRange(work).step/2 );
    zrg.stop = SI().zRange(work).snap( zrg.stop+SI().zRange(work).step/2 );
}


bool CubeSampling::operator==( const CubeSampling& cs ) const
{
   if ( cs.hrg==this->hrg )
   {
       float diff = fabs(cs.zrg.start-this->zrg.start);
       const float eps = SI().zIsTime() ? 1e-6 : 1e-3;
       if ( diff>eps ) return false;

       diff = fabs(cs.zrg.stop-this->zrg.stop);
       if ( diff>eps ) return false;

       diff = fabs(cs.zrg.step-this->zrg.step);
       if ( diff>eps ) return false;

       return true;
   }
   
   return false;
}


bool CubeSampling::usePar( const IOPar& par )
{
    return hrg.usePar( par ) &&
	   par.get( zrangestr, zrg.start, zrg.stop, zrg.step );
}


void CubeSampling::fillPar( IOPar& par ) const
{
    hrg.fillPar( par );
    par.set( zrangestr, zrg.start, zrg.stop, zrg.step );
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
