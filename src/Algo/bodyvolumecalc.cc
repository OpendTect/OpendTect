/*+
----------------------------------------------------------------------------

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR	: Yuancheng Liu
 * DATE		: April 2012


--------------------------------------------------------------------------- 
-*/

static const char* rcsID mUnusedVar = "$Id: bodyvolumecalc.cc,v 1.8 2012-08-07 05:20:49 cvssalil Exp $";


#include "bodyvolumecalc.h"

#include "arraynd.h"
#include "cubesampling.h"
#include "survinfo.h"


BodyVolumeCalculator::BodyVolumeCalculator( const CubeSampling& cs, 
	const Array3D<float>& arr, float threshold, float velocityinmeter )
    : cs_(cs)
    , arr_(arr)
    , threshold_(threshold)
    , volsum_(0)  
{
    const float zfactor = SI().zIsTime() ? velocityinmeter : 
	(SI().zInFeet() ? mFromFeetFactorF : 1);
    const float xyfactor = SI().xyInFeet() ? mFromFeetFactorF : 1;
    unitvol_ = cs_.hrg.step.inl * SI().inlDistance() * xyfactor * 
	       cs_.hrg.step.crl * SI().crlDistance() * xyfactor *
	       cs_.zrg.step * zfactor;
}


od_int64 BodyVolumeCalculator::nrIterations() const
{ return cs_.nrZ()-1; }


bool BodyVolumeCalculator::doWork( od_int64 start, od_int64 stop, int threadid )
{
    if ( !cs_.isDefined() || mIsUdf(threshold_) )
	return 0;

    const int inlsz = arr_.info().getSize(0); 
    const int crlsz = arr_.info().getSize(1); 
    const int zsz = arr_.info().getSize(2);
    if ( cs_.nrInl()!=inlsz || cs_.nrCrl()!=crlsz || cs_.nrZ()!=zsz )
	return 0;

    float nrunits = 0;
    for ( int idx=0; idx<inlsz-1; idx++ )
    {
	for ( int idy=0; idy<crlsz-1; idy++ )
	{
	    for ( int idz=start; idz<stop; idz++ )
	    {
		bool inside[8];
		inside[0] = arr_.get(idx,idy,idz) <= threshold_;
		inside[1] = arr_.get(idx,idy+1,idz) <= threshold_;
		inside[2] = arr_.get(idx+1,idy+1,idz) <= threshold_;
		inside[3] = arr_.get(idx+1,idy,idz) <= threshold_;
		inside[4] = arr_.get(idx,idy,idz+1) <= threshold_;
		inside[5] = arr_.get(idx,idy+1,idz+1) <= threshold_;
		inside[6] = arr_.get(idx+1,idy+1,idz+1) <= threshold_;
		inside[7] = arr_.get(idx+1,idy,idz+1) <= threshold_;
		const char nrdftop = inside[0]+inside[1]+inside[2]+inside[3];
		const char nrdfbas = inside[4]+inside[5]+inside[6]+inside[7];
		if ( !nrdftop || !nrdfbas || nrdftop+nrdfbas<4 )
		    continue;

		if ( (nrdftop==1 && nrdfbas==3) || (nrdftop==3 && nrdfbas==1) )
		{
		    nrunits += 1.0/6.0;
		}
		else if ( nrdftop==2 && nrdfbas==2 )  
		{
		    int ti=-1, tj=-1;
		    int bi=-1, bj=-1;
		    for ( int k=0; k<4; k++ )
		    {
			if ( inside[k] )
			{
			    if ( ti==-1 )
				ti = k;
			    else 
				tj = k;
			}
			if ( inside[k+4] )
			{
			    if ( ti==-1 )
				bi = k+4;
			    else 
				bj = k+4;
			}
		    }

		    if ( (ti==0 && tj==1 && bi==4 && bj==5) ||  
			 (ti==0 && tj==1 && bi==6 && bj==7) ||
			 (ti==1 && tj==2 && bi==5 && bj==6) ||
			 (ti==1 && tj==2 && bi==4 && bj==7) ||
			 (ti==2 && tj==3 && bi==6 && bj==7) ||
			 (ti==2 && tj==3 && bi==4 && bj==5) ||
			 (ti==0 && tj==3 && bi==4 && bj==7) ||
			 (ti==0 && tj==3 && bi==5 && bj==6) ||
			 (ti==1 && tj==3 && bi==5 && bj==7) ||
			 (ti==0 && tj==2 && bi==4 && bj==6) )
			continue;

		    if ( (ti==0 && tj==2 && bi==5 && bj==7) ||
		         (ti==1 && tj==3 && bi==4 && bj==6) )
			nrunits += 1.0/3.0;
		    else
			nrunits += 1.0/6.0;

		}
		else if ( (nrdftop==1 && nrdfbas==4) || 
			  (nrdftop==4 && nrdfbas==1) )
		{
		    nrunits += 1.0/3.0;
		}
		else if ( (nrdftop==2 && nrdfbas==3) || 
			  (nrdftop==3 && nrdfbas==2) )
		{
		    bool ishlf = false;
		    if ( nrdftop==3 )
		    {
			if  ( ((!inside[0] || !inside[2]) && !inside[5] && 
				    !inside[7]) || 
			      ((!inside[1] || !inside[3]) && !inside[4] && 
				    !inside[6]) )
			    ishlf = true;
		    }
		    else
		    {
			if  ( ((!inside[4] || !inside[6]) && !inside[1] && 
				    !inside[3]) || 
			      ((!inside[5] || !inside[7]) && !inside[0] && 
				    !inside[2]) )
			    ishlf = true;
		    }
		    nrunits += (ishlf ? 0.5 : 1.0/3.0);
		}
		else if ( (nrdftop==2 && nrdfbas==4) || 
			  (nrdftop==4 && nrdfbas==2) )
		{
		    const int shift = nrdftop==2  ? 0 : 4;
		    int i=-1, j=-1;
		    for ( int k=0; k<4; k++ )
		    {
			if ( inside[k+shift] )
			{
			    if ( i==-1 )
				i = k+shift;
			    else 
				j = k+shift;
			}
		    }
		    nrunits += (abs(i-j)==2 ? 2.0/3.0 : 0.5);
		}
		else if ( nrdftop==3 && nrdfbas==3 )  
		{
		    if ( (!inside[0] && !inside[4]) || 
			 (!inside[1] && !inside[5]) ||
			 (!inside[2] && !inside[6]) ||
			 (!inside[3] && !inside[7]) )
			nrunits += 0.5;
		    else
			nrunits += 2.0/3.0;
		}
		else if ( (nrdftop==3 && nrdfbas==4) || 
			  (nrdftop==4 && nrdfbas==3) )
		{
		    nrunits += 5.0/6.0;
		}
		else
		{
		    nrunits += 1;
		}
	    }
	}
    }
    
    lock_.lock();
    volsum_ += unitvol_*nrunits;
    lock_.unLock();   
   
    return true;
}


