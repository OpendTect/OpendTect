/*+
-----------------------------------------------------------------------------

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : N. Fredman
 * DATE     : 18-12-2002


-----------------------------------------------------------------------------
*/

static const char* rcsID mUsedVar = "$Id$";


#include "houghtransform.h"

#include "arrayndimpl.h"
#include "arrayndinfo.h"
#include "task.h"
#include "position.h"
#include "sorting.h"
#include "toplist.h"
#include "thread.h"
#include "trigonometry.h"

#include <math.h>

#define mRhoSize 200
#define mThetaSize 360
#define mHalfThetaSize 180

class PlaneFrom3DSpaceHoughTransformTask : public SequentialTask
{
public:	
	PlaneFrom3DSpaceHoughTransformTask(PlaneFrom3DSpaceHoughTransform& ht_)
	    : calcpositions_( ht_.calcpositions_ )
	    , datainfo_( *ht_.datainfo_ )
	    , ht( ht_ )
	    , normals_( *ht_.normals_ )
	    , idx( 0 )
	{ }
		    
protected:			
    int					nextStep();

    TypeSet<unsigned int>&		calcpositions_;
    Array3DInfo&			datainfo_;
    PlaneFrom3DSpaceHoughTransform&	ht;

    int					idx;
    TypeSet<Coord3>&			normals_;
};
				

int PlaneFrom3DSpaceHoughTransformTask::nextStep()
{
    if ( idx>=calcpositions_.size() ) return 0;

    int temppos[3];
    datainfo_.getArrayPos( calcpositions_[idx], temppos );
    const Coord3 pos( temppos[0], temppos[1], temppos[2] );
    static const Coord3 origo( 0,0,0 );

    for ( int normal=0; normal<normals_.size(); normal++ )
    {
	Plane3 plane( normals_[normal], pos, false );
	ht.incParamPos( normal, plane.distanceToPoint(origo) );
    }
   
    idx++;
    return 1;
}

    
PlaneFrom3DSpaceHoughTransform::PlaneFrom3DSpaceHoughTransform()
    : paramspacemutex_( *new Threads::Mutex )
    , paramspace_( 0 )
    , normals_( 0 )
    , datainfo_( 0 )
    , cliprate_( 0.7 )
{}
		
		
PlaneFrom3DSpaceHoughTransform::~PlaneFrom3DSpaceHoughTransform()
{
    delete datainfo_;
    if ( normals_ ) delete normals_;
    delete paramspace_;
}


void PlaneFrom3DSpaceHoughTransform::setResolution( double dangle,
						    int distsize )
{
    if ( normals_ ) delete normals_;
    normals_ = makeSphereVectorSet(dangle);

    if ( paramspace_ ) delete paramspace_;
    paramspace_ = new Array2DImpl<unsigned int>( normals_->size(), distsize );
}


int PlaneFrom3DSpaceHoughTransform::getParamSpaceSize() const
{ return mCast(int, paramspace_->info().getTotalSz()); }


int PlaneFrom3DSpaceHoughTransform::getNrDistVals() const
{ return paramspace_->info().getSize(1); }


void PlaneFrom3DSpaceHoughTransform::setClipRate( float cliprt )
{ cliprate_ = cliprt; }


float PlaneFrom3DSpaceHoughTransform::clipRate() const    
{ return cliprate_; }


void PlaneFrom3DSpaceHoughTransform::setData( const Array3D<float>* data )
{
    const float* dataptr = data->getData();
    const int datasize = mCast(int, data->info().getTotalSz());
    
    ArrPtrMan<float> datacopy = new float[datasize];
    ArrPtrMan<unsigned int> indexes = new unsigned int[datasize];

    int nrsamples = 0;
    for ( int idx=0; idx<datasize; idx++ )
    {
	if ( mIsUdf( dataptr[idx] ) )
	    continue;

	indexes[nrsamples]= idx;
	datacopy[nrsamples++] = dataptr[idx];
    }
    
    
    float* datacopyptr = datacopy.ptr();
    unsigned int* indexesptr = indexes.ptr();
    quickSort( datacopyptr, indexesptr, nrsamples );
    const int savesize = mNINT32( nrsamples*cliprate_ );
   
    for ( int idx=nrsamples-1; idx>=nrsamples-savesize; idx-- )
	calcpositions_ += indexes[idx];

    delete datainfo_;
    datainfo_ = dynamic_cast<Array3DInfo*>( data->info().clone() );

    unsigned int* paramptr = paramspace_->getData();
    memset( paramptr, 0, sizeof(unsigned int)*getParamSpaceSize());

    const double maxx = datainfo_->getSize( 0 );
    const double maxy = datainfo_->getSize( 1 );
    const double maxz = datainfo_->getSize( 2 );

    const double maxdist = Math::Sqrt( maxx*maxx + maxy*maxy + maxz*maxz ); 
    deltadist_ = maxdist / (getNrDistVals()-1);
}


ObjectSet<SequentialTask>* PlaneFrom3DSpaceHoughTransform::createCalculators()
{
    ObjectSet<SequentialTask>* res = new ObjectSet<SequentialTask>;
    (*res) += new PlaneFrom3DSpaceHoughTransformTask( *this );
    return res;
}


TopList<unsigned int, unsigned int>*
PlaneFrom3DSpaceHoughTransform::sortParamSpace(int size) const
{
    if ( !paramspace_ ) return 0;
    
    const unsigned int paramsize = getParamSpaceSize();
    TopList<unsigned int, unsigned int>* res =
		    new TopList<unsigned int, unsigned int>( size );

    unsigned int* paramptr = paramspace_->getData();

    unsigned int lowest = 0;
    for ( unsigned int idx=0; idx<paramsize; idx++ )
    {
	if ( paramptr[idx]>lowest )
	{
	    res->addValue( paramptr[idx], idx );
	    res->getLowestValue( lowest );
	}
    }

    return res;
}


Plane3 PlaneFrom3DSpaceHoughTransform::getPlane( int planenr ) const
{
    int pos[2];
    paramspace_->info().getArrayPos( planenr, pos );
    Coord3 normal = (*normals_)[pos[0]];
    const float dist = (float) ( pos[1] * deltadist_ );

    return Plane3( normal, Coord3(normal.x*dist,normal.y*dist,normal.z*dist),
	    	   false );
}


int PlaneFrom3DSpaceHoughTransform::getNrPointsAfterClip() const
{ return calcpositions_.size(); }


void PlaneFrom3DSpaceHoughTransform::incParamPos( int normalidx, double dist)
{
    const int distid = mNINT32( dist/deltadist_ );
    unsigned int memoffset = mCast( unsigned int, 
			    reinterpret_cast<const Array2DInfo&>
			    (paramspace_->info()).getOffset(normalidx,distid) );
    unsigned int* dataptr = paramspace_->getData();

    paramspacemutex_.lock();
    dataptr[memoffset]++;
    paramspacemutex_.unLock();
}


LineFrom2DSpaceHoughTransform::LineFrom2DSpaceHoughTransform( 
	const Array2D<float>& input )
    : input_(input)
    , origcnt_(0)      
    , hougharr_(0)
    , result_(0)
    , toplistnr_(10)	
    , anglerg_(mUdf(float),mUdf(float))		
    , threshold_(mUdf(float))
    , abovethreshold_(true)		   
{
    const int sz0 = input.info().getSize(0);
    const int sz1 = input.info().getSize(1);
    
    mDeclareAndTryAlloc(Array2DImpl<int>*,arr,Array2DImpl<int>(sz0,sz1));
    if ( !arr )	return;

    mDeclareAndTryAlloc(Array2DImpl<unsigned char>*,result,
	    		Array2DImpl<unsigned char>(sz0,sz1));
    if ( !result )
    {
	delete arr;
	return;
    }
    
    mDeclareAndTryAlloc(Array2DImpl<int>*,harr,
	    		Array2DImpl<int>(mThetaSize,mRhoSize));
    if ( !harr )
    {
	delete arr;
	delete result;
	return;
    }

    origcnt_ =  arr;
    result_ = result;
    hougharr_ = harr;
    hougharr_->setAll(0);
    result_->setAll(0);
}


LineFrom2DSpaceHoughTransform::~LineFrom2DSpaceHoughTransform()
{
    delete origcnt_;
    delete hougharr_; 
    delete result_; 
}


void LineFrom2DSpaceHoughTransform::setLineAngleRange( Interval<float> rg )
{ anglerg_ = Interval<float>( mMAX(rg.start,0), mMIN(rg.stop,(float) M_PI) ); }


void LineFrom2DSpaceHoughTransform::setThreshold( float val, bool above )
{
    threshold_ = val;
    abovethreshold_ = above;
}

bool LineFrom2DSpaceHoughTransform::compute()
{
    if ( !origcnt_ || !hougharr_ || !result_ )
	return false;

    const int rsz = input_.info().getSize(0);
    const int csz = input_.info().getSize(1);
    const float maxrho = (float) sqrt((double)(rsz*rsz+csz*csz));
    
    TypeSet<float> sintable, costable;
    const float factor = 2*M_PI/(float)(mThetaSize);
    for ( int idx=0; idx<mThetaSize; idx++ )
    {
	const float theta = idx*factor;
	sintable += -sin(theta);
	costable += -cos(theta);
    }

    for ( int idx=1; idx<rsz-1; idx++ )
    {
	for ( int idy=1; idy<csz-1; idy++ )
	{
	    const float val = input_.get(idx,idy);
	    if ( mIsUdf(val) ) continue;
	    
	    if ( (abovethreshold_ && val<threshold_) ||
		 (!abovethreshold_ && val>threshold_) )
		continue;

	    result_->set( idx, idy, 1 );
	    int lastidx;		
	    for ( int tidx=0; tidx<mHalfThetaSize; tidx++ )
	    {
		const float radius = (idy-csz/2)*costable[tidx] + 
				     (idx-rsz/2)*sintable[tidx];
		const int ridx = (int)(0.5+(radius/maxrho+0.5)*mRhoSize);
		if ( ridx<0 || ridx>=mRhoSize )
		{
		    lastidx = ridx;
		    continue;
		}

		hougharr_->set(tidx,ridx,hougharr_->get(tidx,ridx)+1);
		hougharr_->set(tidx+mHalfThetaSize,mRhoSize-ridx, 
			hougharr_->get(tidx+mHalfThetaSize,mRhoSize-ridx)+1);
		if ( !tidx )
		{
		    lastidx = ridx;
		    continue;
		}
		
		while ( lastidx-1>ridx && lastidx<mRhoSize )
		{
		    lastidx--;
		    hougharr_->set(tidx,lastidx,hougharr_->get(tidx,lastidx)+1);
		    hougharr_->set(tidx+mHalfThetaSize,mRhoSize-lastidx,
			hougharr_->get(tidx+mHalfThetaSize,mRhoSize-lastidx)+1);
		}
		while ( lastidx+1<ridx && lastidx>0 )
		{
		    lastidx++;
		    hougharr_->set(tidx,lastidx,hougharr_->get(tidx,lastidx)+1);
		    hougharr_->set(tidx+mHalfThetaSize,mRhoSize-lastidx, 
			hougharr_->get(tidx+mHalfThetaSize,mRhoSize-lastidx)+1);
		}		
		lastidx = ridx;
	    }
	}
    }

    for ( int idx=0; idx<rsz; idx++ )
    {
       for ( int idy=0; idy<csz; idy++ )
       {
	   if ( !idx || !idy || idx==rsz-1 || idy==csz-1 )
	       origcnt_->set( idx, idy, 0 );
	   else
	       origcnt_->set( idx, idy, result_->get(idx+1,idy+1) + 
		       result_->get(idx,idy+1) + result_->get(idx-1,idy+1) +
		       result_->get(idx+1,idy) + result_->get(idx,idy) + 
		       result_->get(idx-1,idy) + result_->get(idx+1,idy-1) + 
		       result_->get(idx,idy-1) + result_->get(idx-1,idy-1) );
       }
    }
    result_->setAll(0);

    TypeSet<int> tops, topids;
    TypeSet<int> tis, ris;
    for ( int tidx=0; tidx<mHalfThetaSize; tidx++ )
    {
       for ( int ridx=0; ridx<mRhoSize; ridx++ )
       {
	   bool max = true;
	   for ( int dt =tidx-2; dt<=tidx+2; dt++ )
	   {
	       if ( dt<0 || dt>=mThetaSize )
		   continue;

	       for ( int dr=ridx-2; dr<=ridx+2; dr++ )
	       {   
		   if ( dr<0 || dr>=mRhoSize ||
			hougharr_->get(dt,dr)<=hougharr_->get(tidx,ridx) )
		       continue;
		  
		   max = false;
		   break;
	       }
	       if ( !max ) break;
	   }

	   if ( max )
	   {
	       tops += hougharr_->get(tidx,ridx);
    	       tis += tidx;
    	       ris += ridx;
	   }
       }
    }

    const int localmaxsz = tops.size();
    for ( int idx=0; idx<localmaxsz; idx++ )
       topids += idx;

    sort_coupled( tops.arr(), topids.arr(), localmaxsz );
    
    int nrdone = 0;
    const bool angledefined = !mIsUdf(anglerg_.start);
    for ( int idx=localmaxsz-1; idx>=0 && nrdone<toplistnr_; idx-- )
    {
       int tidx = tis[topids[idx]];
       const float theta = (float) (((float)tidx/(float)(mThetaSize)-0.5)
							*2*M_PI);
       if ( angledefined && !anglerg_.includes(fabs(theta),false) )
	   continue;

       float radius = ((float)ris[topids[idx]]/(float)(mRhoSize)-0.5f)*maxrho;
       if ( setLineFlag(radius,theta) )
	   nrdone++;
    }

    return true;
}


bool LineFrom2DSpaceHoughTransform::setLineFlag(float radius, float theta)
{
    const int rsz = origcnt_->info().getSize(0);
    const int csz = origcnt_->info().getSize(1);

    const bool usec = fabs(theta)>=M_PI_4 && fabs(theta)<=3*M_PI_4;
    const float slop = usec ? cos(theta)/sin(theta) : sin(theta)/cos(theta);
    const float dist = usec ? radius/(float)sin(theta) + (csz/2)*slop + rsz/2 
			    : radius/(float)cos(theta) + (rsz/2)*slop + csz/2;
    int startidx = -1;
    const int sz0 = usec ? csz : rsz;
    const int sz1 = usec ? rsz : csz;
    const int linelength = 5; //sz0/10;

    bool result = false;
    for ( int idx=0; idx<sz0; idx++ )
    {
	const int idy = (int)(0.5+dist-idx*slop);
	if ( idy<0 || idy>=sz1 )
	    continue;

	const int val = usec ? origcnt_->get(idy,idx) : origcnt_->get(idx,idy);
	/*if ( usec )
	    result_->set( idy, idx, val ? 1 :0 );
	else
	    result_->set( idx, idy, val ? 1 : 0 ); result = true; 
	continue;*/

	/*with line length*/
	if ( startidx<0 && val>0 )
	    startidx = idx;
	else if ( !val && startidx>0 )
	{
	    if ( idx-startidx>linelength ) 
	    {
		for ( int tmpidx = startidx; tmpidx<idx; tmpidx++ )
		{
		    int tmpidy = (int)(0.5+dist-tmpidx*slop);
		    if  ( !tmpidx || !tmpidy || tmpidx==sz0-1 || tmpidy==sz1-1 )
			continue;

		    if ( usec )
			result_->set( tmpidy, tmpidx, 1 );
		    else
			result_->set( tmpidx, tmpidy, 1 );
		}

		result = true;
	    }
	    startidx = -1;
	}
    }

    return result;
}


/*This method is more direct use theta between [-PI/2,PI/2]*/
/*bool LineFrom2DSpaceHoughTransform::compute()
{
    if ( !origcnt_ || !hougharr_ || !result_ )
	return false;

    const int rsz = input_.info().getSize(0);
    const int csz = input_.info().getSize(1);
    const float maxrho = sqrt((double)((rsz-1)*(rsz-1)+(csz-1)*(csz-1)));
    
    TypeSet<float> sintable, costable;
    const float factor = M_PI/(float)(mThetaSize-1);
    for ( int idx=0; idx<mThetaSize; idx++ )
    {
	const float theta = -M_PI_2+idx*factor;
	sintable += sin(theta);
	costable += cos(theta);
    }

    for ( int idx=1; idx<rsz; idx++ )
    {
	for ( int idy=1; idy<csz; idy++ )
	{
	    const float val = input_.get(idx,idy);
	    if ( mIsUdf(val) ) continue;
	    
	    if ( (abovethreshold_ && val<threshold_) ||
		 (!abovethreshold_ && val>threshold_) )
		continue;

	    for ( int tidx=0; tidx<mThetaSize; tidx++ )
	    {
		const float rho = idx*costable[tidx] + idy*sintable[tidx];
		int ridx = (int)((rho+maxrho)*mRhoSize/(2*maxrho));
		if ( ridx>=mRhoSize )
		    ridx = mRhoSize-1;

		hougharr_->set( tidx, ridx, hougharr_->get(tidx,ridx)+1 );
	    }
	}
    }

    TypeSet<int> tops, topids;
    TypeSet<int> tis, ris;
    for ( int tidx=0; tidx<mThetaSize; tidx++ )
    {
       for ( int ridx=0; ridx<mRhoSize; ridx++ )
       {
	   bool max = true;
	   for ( int dt =tidx-2; dt<=tidx+2; dt++ )
	   {
	       if ( dt<0 || dt>=mThetaSize )
		   continue;

	       for ( int dr=ridx-2; dr<=ridx+2; dr++ )
	       {   
		   if ( dr>=0 && dr<mRhoSize && 
			(hougharr_->get(dt,dr)>hougharr_->get(tidx,ridx)) )
		   {
		       max = false;
		       break;
		   }
	       }
	   }
	   if ( !max )
	       continue;

	   tops += hougharr_->get(tidx,ridx);
	   tis += tidx;
	   ris += ridx;
       }
    }

    const int localmaxsz = tops.size();
    for ( int idx=0; idx<localmaxsz; idx++ )
       topids += idx;

    result_->setAll(0);
    sort_coupled( tops.arr(), topids.arr(), localmaxsz );
    int nrdone = 0;
    const bool angledefined = !mIsUdf(anglerg_.start);
    for ( int lidx=localmaxsz-1; lidx>=0 && nrdone<toplistnr_; lidx-- )
    {
 	int tidx = tis[topids[lidx]];
 	const float theta = -M_PI_2+(M_PI*tidx)/(float)(mThetaSize-1);
 	if ( angledefined && !anglerg_.includes(fabs(theta),false) )
 	    continue;
 
	float rho = -maxrho+(2*maxrho*ris[topids[lidx]])/(float)(mRhoSize-1);
	if ( mIsZero(sin(theta),1e-5) )
	{
	    for ( int idy=0; idy<csz; idy++ )
	    {
		const int idx = (int)(rho);
		if ( idx<0 || idx>=rsz )
		    continue;
	    
		const float val = input_.get(idx,idy);
		if ( mIsUdf(val) || (abovethreshold_ && val<threshold_) ||
				    (!abovethreshold_ && val>threshold_) )
		    continue;

		result_->set( idx, idy, 1 );
	    }
	}
	else
	{
	    for ( int idx=0; idx<rsz; idx++ )
	    {
		const int idy = (int)((rho-idx*cos(theta))/sin(theta));
		if ( idy<0 || idy>=csz )
		    continue;
	    
		const float val = input_.get(idx,idy);
		if ( mIsUdf(val) || (abovethreshold_ && val<threshold_) ||
				    (!abovethreshold_ && val>threshold_) )
		    continue;

		result_->set( idx, idy, 1 );
	    }
	}

	nrdone++;
    }

    return true;
}*/



