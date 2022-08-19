/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bodyvolumecalc.h"

#include "arraynd.h"
#include "trckeyzsampling.h"
#include "survinfo.h"


BodyVolumeCalculator::BodyVolumeCalculator( const TrcKeyZSampling& cs,
	const Array3D<float>& arr, float threshold, float velocityinmeter )
    : arr_(arr)
    , threshold_(threshold)
    , volsum_(0)
    , lock_(true)
{
    const float zfactor = SI().zIsTime() ? velocityinmeter*0.5f :
	(SI().zInFeet() ? mFromFeetFactorF : 1);
    const float xyfactor = SI().xyInFeet() ? mFromFeetFactorF : 1;
    unitvol_ = cs.hsamp_.step_.inl() * SI().inlDistance() * xyfactor *
	       cs.hsamp_.step_.crl() * SI().crlDistance() * xyfactor *
	       cs.zsamp_.step * zfactor;
}


od_int64 BodyVolumeCalculator::nrIterations() const
{ return arr_.info().getSize(2)-1; }


uiString BodyVolumeCalculator::uiMessage() const
{ return tr("Computing volume"); }


bool BodyVolumeCalculator::doWork( od_int64 start, od_int64 stop, int threadid )
{
    if ( mIsUdf(unitvol_) || mIsUdf(threshold_) )
	return 0;

    const int inlsz = arr_.info().getSize(0);
    const int crlsz = arr_.info().getSize(1);

    float nrunits = 0;
    for ( int idx=0; idx<inlsz-1; idx++ )
    {
	for ( int idy=0; idy<crlsz-1; idy++ )
	{
	    for ( int idz=(int) start; idz<=stop; idz++ )
	    {
		bool inside[8];
		inside[0] = arr_.get(idx,idy,idz) >= threshold_;
		inside[1] = arr_.get(idx,idy+1,idz) >= threshold_;
		inside[2] = arr_.get(idx+1,idy+1,idz) >= threshold_;
		inside[3] = arr_.get(idx+1,idy,idz) >= threshold_;
		inside[4] = arr_.get(idx,idy,idz+1) >= threshold_;
		inside[5] = arr_.get(idx,idy+1,idz+1) >= threshold_;
		inside[6] = arr_.get(idx+1,idy+1,idz+1) >= threshold_;
		inside[7] = arr_.get(idx+1,idy,idz+1) >= threshold_;
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
		    if ( (inside[0] && inside[1] && inside[4] && inside[5]) ||
			 (inside[0] && inside[1] && inside[6] && inside[7]) ||
			 (inside[0] && inside[2] && inside[4] && inside[6]) ||
			 (inside[0] && inside[3] && inside[4] && inside[7]) ||
			 (inside[0] && inside[3] && inside[5] && inside[6]) ||
			 (inside[1] && inside[2] && inside[5] && inside[6]) ||
			 (inside[1] && inside[2] && inside[4] && inside[7]) ||
			 (inside[1] && inside[3] && inside[5] && inside[7]) ||
			 (inside[2] && inside[3] && inside[4] && inside[5]) ||
			 (inside[2] && inside[3] && inside[6] && inside[7]) )
			continue;

		    if ( (inside[0] && inside[2] && inside[5] && inside[7]) ||
			 (inside[1] && inside[3] && inside[4] && inside[6]) )
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
		    nrunits += (ishlf ? 0.5f : 1.0f/3.0f);
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
		    nrunits += (abs(i-j)==2 ? 2.0f/3.0f : 0.5f);
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

    Threads::Locker lckr( lock_ );
    volsum_ += unitvol_*nrunits;
    return true;
}
