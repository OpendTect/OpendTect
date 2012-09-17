/*+
-----------------------------------------------------------------------------

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : February 2012


-----------------------------------------------------------------------------
*/

static const char* rcsID = "$Id: faultextractor.cc,v 1.1 2012/02/14 23:20:31 cvsyuancheng Exp $";


#include "faultextractor.h"

#include "arrayndimpl.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "refcount.h"
#include "sorting.h"
#include "survinfo.h"
#include <math.h>

#define mRhoSize 200
#define mThetaSize 360

    
FaultExtractor::FaultExtractor( const Array3D<float>& input,
       				const CubeSampling& cs )
    : input_(input)
    , harr0_(0)
    , harr1_(0)
    , flag_(0)
    , threshold_(mUdf(float))
    , abovethreshold_(true)		   
    , toplistnr_(10)	
    , anglerg_(mUdf(float),mUdf(float))		
    , cs_( cs )					   
{
    const int sz0 = input.info().getSize(0);
    const int sz1 = input.info().getSize(1);
    const int sz2 = input.info().getSize(2);
    
    mDeclareAndTryAlloc(Array3D<unsigned char>*,flag,
	    		Array3DImpl<unsigned char>(sz0,sz1,sz2));
    if ( !flag ) return;    
    flag_ = flag;
    
    mDeclareAndTryAlloc(Array3D<int>*,ha0,
	    		Array3DImpl<int>(sz0,mRhoSize,mThetaSize));
    if ( !ha0 ) 
    {
	delete flag_; return;
    }
    harr0_ = ha0;

    mDeclareAndTryAlloc(Array3D<int>*,ha1,
	    		Array3DImpl<int>(mThetaSize,mRhoSize,mThetaSize));
    if ( !ha1 )
    {
	delete flag_; delete harr0_; return;
    }
    harr1_ = ha1;

    flag_->setAll(0);
    harr0_->setAll(0);
    harr1_->setAll(0);
}


FaultExtractor::~FaultExtractor()
{
    delete flag_;
    delete harr0_;
    delete harr1_;
    deepUnRef( faults_ );
}


void FaultExtractor::setLineAngleRange( Interval<float> rg )
{ anglerg_ = Interval<float>( mMAX(rg.start,0), mMIN(rg.stop,M_PI) ); }


void FaultExtractor::setThreshold( float val, bool above )
{
    threshold_ = val;
    abovethreshold_ = above;
}

bool FaultExtractor::compute()
{
    if ( !flag_ || !harr0_ || !harr1_ )
	return false;

    const int sz0 = input_.info().getSize(0);
    const int sz1 = input_.info().getSize(1);
    const int sz2 = input_.info().getSize(2);
    const float maxrho = sqrt((double)((sz1-1)*(sz1-1)+(sz2-1)*(sz2-1)));
    
    TypeSet<float> sintable, costable;
    const float factor = M_PI/(float)(mThetaSize-1);
    for ( int idx=0; idx<mThetaSize; idx++ )
    {
	const float theta = -M_PI_2+idx*factor;
	sintable += sin(theta);
	costable += cos(theta);
    }

    for ( int inlidx=0; inlidx<sz0; inlidx++ )
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    for ( int idz=0; idz<sz2; idz++ )
	    {
		const float val = input_.get(inlidx,idy,idz);
		if ( mIsUdf(val) ) continue;
		
		if ( (abovethreshold_ && val<threshold_) ||
		     (!abovethreshold_ && val>threshold_) )
		    continue;

		for ( int tidx=0; tidx<mThetaSize; tidx++ )
		{
		    const float rho = idy*costable[tidx] + idz*sintable[tidx];
		    int ridx = (int)((rho+maxrho)*mRhoSize/(2*maxrho));
		    if ( ridx>=mRhoSize ) ridx = mRhoSize-1;

		    harr0_->set( inlidx, ridx, tidx,
			    harr0_->get(inlidx,ridx,tidx)+1 );
		}
	    }
	}
    }
    
    const int inlsz = harr0_->info().getSize(0);
    const int rhosz = harr0_->info().getSize(1);
    const int thetasz = harr0_->info().getSize(2);
    const float hmaxrho = sqrt((inlsz-1)*(inlsz-1)+(rhosz-1)*(rhosz-1.));
    for ( int thetaidx=0; thetaidx<thetasz; thetaidx++ )
    {
	for ( int rhoidx=0; rhoidx<rhosz; rhoidx++ )
	{
	    for ( int inlidx=0; inlidx<inlsz; inlidx++ )
	    {
		if ( harr0_->get(inlidx,rhoidx,thetaidx)<2 )
		    continue;
		
		for ( int pidx=0; pidx<mThetaSize; pidx++ )
		{
		    float rho = rhoidx*costable[pidx] + inlidx*sintable[pidx];
		    int ridx = (int)((rho+hmaxrho)*mRhoSize/(2*hmaxrho));
		    if ( ridx>=mRhoSize ) ridx = mRhoSize-1;

		    harr1_->set( thetaidx, ridx, pidx,
			    harr1_->get(thetaidx,ridx,pidx)+1 );
		}
	    }
	}
    }

    const int phisz = harr1_->info().getSize(2);
    TypeSet<int> tops, topids;
    TypeSet<int> thetas, rhos, phis;

    for ( int idx=0; idx<thetasz; idx++ )
    {
       for ( int idy=0; idy<rhosz; idy++ )
       {
	   for ( int idz=0; idz<phisz; idz++ )
	   {
	       const int cnt = harr1_->get(idx,idy,idz);
    	       bool max = true;
    	       for ( int dx=idx-1; dx<=idx+1; dx++ )
    	       {
    		   if ( dx<0 || dx>=thetasz )
    		       continue;
		   
		   if ( !max ) break;
    		   for ( int dy=idy-1; dy<=idy+1; dy++ )
    		   {   
		       if ( dy<0 || dy>=rhosz )
			   continue;
		       
		       if ( !max ) break;
		       for ( int dz=idz-1; dz<=idz+1; dz++ )
    		       {   
    			   if ( dz<0 || dz>=phisz )
    			       continue;

			   if ( cnt<harr1_->get(dx,dy,dz) )
    			   {
    			       max = false;
    			       break;
    			   }
    		       }
		   }
    	       }
    	       if ( !max ) continue;
	   
	       tops += cnt;
	       thetas += idx;
	       rhos += idy;
	       phis += idz;
    	   }
       }
    }

    const int maxsz = tops.size();
    for ( int idx=0; idx<maxsz; idx++ )
       topids += idx;

    sort_coupled( tops.arr(), topids.arr(), maxsz );

    const bool angledefined = !mIsUdf(anglerg_.start);    
    int nrdone = 0;
    for ( int li=maxsz-1; li>=0 && nrdone<toplistnr_; li-- )
    {
 	const float phi = -M_PI_2+(M_PI*phis[li])/(mThetaSize-1);
	const float rho = -hmaxrho+(2*hmaxrho*rhos[li])/(mRhoSize-1);
	const float theta = -M_PI_2+(M_PI*thetas[li])/(mThetaSize-1);
	if ( angledefined && !anglerg_.includes(fabs(theta),false) )
	    continue;

	EM::EMObject* ef =EM::EMM().createTempObject(EM::Fault3D::typeStr());
	ef->ref();
	mDynamicCastGet(EM::Fault3D*,f3d,ef);
	if ( !f3d )
	{
	    ef->unRef();
	    continue;
	}

	faults_ += f3d;
	int sticknr = 0;

	if ( mIsZero(sin(phi),1e-5) )
	{
	    const int inlidx = (int)(rho);
	    if ( inlidx<0 || inlidx>=inlsz )
		continue;

	    for ( int rhoidx=0; rhoidx<rhosz; rhoidx++ )
	    {
		if ( harr0_->get(inlidx,rhoidx,thetas[li])<2 )
		    continue;

		makeFaultStick( inlidx, rhoidx, theta, maxrho, *f3d, sticknr );
		sticknr++;
	    }
	}
	else
	{
	    for ( int inlidx=0; inlidx<inlsz; inlidx++ )
	    {
		const int rhoidx = (int)((rho-inlidx*cos(phi))/sin(phi));
		if ( rhoidx<0 || rhoidx>=rhosz || 
		     harr0_->get(inlidx,rhoidx,thetas[li])<2 )
		    continue;

		makeFaultStick( inlidx, rhoidx, theta, maxrho, *f3d, sticknr );
		sticknr++;
	    }
	}

	nrdone++;
    }

    return true;
}


void FaultExtractor::makeFaultStick( int inlidx, int rhoidx, float theta, 
	float maxrho, EM::Fault3D& flt, int sticknr )
{
    const int sz1 = input_.info().getSize(1);
    const int sz2 = input_.info().getSize(2);
    const float rho = -maxrho+(2*maxrho*rhoidx)/(mRhoSize-1);
    int knotidx = 0;
    if ( mIsZero(sin(theta),1e-5) )
    {
	const int idy = (int)(rho);
	if ( idy<0 || idy>=sz1 )
	    return;

	Coord3 pos(SI().transform(cs_.hrg.atIndex(inlidx,idy)),0);
	for ( int idz=0; idz<sz2; idz++ )
	{	
	    const float val = input_.get(inlidx,idy,idz);
	    if ( mIsUdf(val) || (abovethreshold_ && val<threshold_)
		    || (!abovethreshold_ && val>threshold_) )
		continue;

	    pos.z = cs_.zrg.atIndex( idz );
	    flt.geometry().sectionGeometry(0)->insertKnot( 
		    RowCol(sticknr,knotidx++),pos);
	    flag_->set( inlidx, idy, idz, 1 );
	}
    }
    else
    {
	for ( int idy=0; idy<sz1; idy++ )
	{
	    const int idz = (int)((rho-idy*cos(theta))/sin(theta));
	    if ( idz<0 || idz>=sz2 )
		continue;
	    
	    const float val = input_.get(inlidx,idy,idz);
	    if ( mIsUdf(val) || (abovethreshold_ && val<threshold_)
		    || (!abovethreshold_ && val>threshold_) )
		continue;

	    Coord3 pos( SI().transform(cs_.hrg.atIndex(inlidx,idy)),
			cs_.zrg.atIndex(idz) );
	    flt.geometry().sectionGeometry(0)->insertKnot( 
		    RowCol(sticknr,knotidx++),pos);
	    flag_->set( inlidx, idy, idz, 1 );
	}
    }
}


