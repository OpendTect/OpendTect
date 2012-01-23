/*+
-----------------------------------------------------------------------------

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : N. Fredman
 * DATE     : 18-12-2002


-----------------------------------------------------------------------------
*/

static const char* rcsID = "$Id: houghtransform.cc,v 1.13 2012-01-23 15:39:12 cvsyuancheng Exp $";


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
{ return paramspace_->info().getTotalSz(); }


int PlaneFrom3DSpaceHoughTransform::getNrDistVals() const
{ return paramspace_->info().getSize(1); }


void PlaneFrom3DSpaceHoughTransform::setClipRate( float cliprt )
{ cliprate_ = cliprt; }


float PlaneFrom3DSpaceHoughTransform::clipRate() const    
{ return cliprate_; }


void PlaneFrom3DSpaceHoughTransform::setData( const Array3D<float>* data )
{
    const float* dataptr = data->getData();
    const int datasize = data->info().getTotalSz();
    
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
    const int savesize = mNINT( nrsamples*cliprate_ );
   
    for ( int idx=nrsamples-1; idx>=nrsamples-savesize; idx-- )
	calcpositions_ += indexes[idx];

    delete datainfo_;
    datainfo_ = dynamic_cast<Array3DInfo*>( data->info().clone() );

    unsigned int* paramptr = paramspace_->getData();
    memset( paramptr, 0, sizeof(unsigned int)*getParamSpaceSize());

    const float maxx = datainfo_->getSize( 0 );
    const float maxy = datainfo_->getSize( 1 );
    const float maxz = datainfo_->getSize( 2 );

    const float maxdist = Math::Sqrt( maxx*maxx + maxy*maxy + maxz*maxz ); 
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
		    new TopList<unsigned int, unsigned int>( size, 0, true );

    unsigned int* paramptr = paramspace_->getData();

    unsigned int lowest = res->getBottomValue();
    for ( int idx=0; idx<paramsize; idx++ )
    {
	if ( paramptr[idx]>lowest )
	{
	    res->addValue( paramptr[idx], idx );
	    lowest = res->getBottomValue();
	}
    }

    return res;
}


Plane3 PlaneFrom3DSpaceHoughTransform::getPlane( int planenr ) const
{
    int pos[2];
    paramspace_->info().getArrayPos( planenr, pos );
    Coord3 normal = (*normals_)[pos[0]];
    const float dist = pos[1] * deltadist_;

    return Plane3( normal, Coord3(normal.x*dist,normal.y*dist,normal.z*dist),
	    	   false );
}


int PlaneFrom3DSpaceHoughTransform::getNrPointsAfterClip() const
{ return calcpositions_.size(); }


void PlaneFrom3DSpaceHoughTransform::incParamPos( int normalidx, double dist)
{
    const int distid = mNINT( dist/deltadist_ );
    unsigned int memoffset = reinterpret_cast<const Array2DInfo&>
			    (paramspace_->info()).getOffset(normalidx,distid);
    unsigned int* dataptr = paramspace_->getData();

    paramspacemutex_.lock();
    dataptr[memoffset]++;
    paramspacemutex_.unLock();
}


LineFrom2DSpaceHoughTransform::LineFrom2DSpaceHoughTransform( 
	const Array2D<float>& input )
    : input_(0)
    , hougharr_(0)
    , result_(0)
    , rhosz_(200)
    , thetasz_(360)	
    , maxrho_(0)
    , toplistnr_(10)	
    , anglerg_(mUdf(float),mUdf(float))		
    , threshold_(0)
    , abovethreshold_(true)		   
{
    const int sz0 = input.info().getSize(0);
    const int sz1 = input.info().getSize(1);
    maxrho_ = sqrt((double)(sz0*sz0+sz1*sz1));
    
    mDeclareAndTryAlloc( Array2DImpl<float>*, arr,
	    Array2DImpl<float>(sz0,sz1) );
    if ( !arr )	return;

    mDeclareAndTryAlloc( Array2DImpl<unsigned char>*, result,
	    Array2DImpl<unsigned char>(sz0,sz1) );
    if ( !result )
    {
	delete arr;
	return;
    }
    
    mDeclareAndTryAlloc( Array2DImpl<int>*, harr,
	    Array2DImpl<int>(thetasz_,rhosz_) );
    if ( !harr )
    {
	delete arr;
	delete result;
	return;
    }

    arr->copyFrom( input );
    input_ =  arr;
    
    result_ = result;
    result_->setAll(0);
    
    hougharr_ = harr;
    hougharr_->setAll(0);
    
    const float factor = 2*M_PI/(float)thetasz_;
    for ( int idx=0; idx<thetasz_; idx++ )
    {
	const float theta = idx*factor-M_PI;
	sintable_ += sin(theta);
	costable_ += cos(theta);
    }
}


LineFrom2DSpaceHoughTransform::~LineFrom2DSpaceHoughTransform()
{
    delete input_;
    delete hougharr_; 
    delete result_; 
}


void LineFrom2DSpaceHoughTransform::setLineAngleRange( Interval<float> rg )
{ anglerg_ = Interval<float>( mMAX(rg.start,0), mMIN(rg.stop,M_PI) ); }


void LineFrom2DSpaceHoughTransform::setThreshold( float val, bool above )
{
    threshold_ = val;
    abovethreshold_ = above;
}


bool LineFrom2DSpaceHoughTransform::compute()
{
    if ( !input_ || !hougharr_ || !result_ || !sintable_.size() )
	return false;

    const int rsz = input_->info().getSize(0);
    const int csz = input_->info().getSize(1);

    for ( int idx=1; idx<rsz-1; idx++ )
    {
	for ( int idy=1; idy<csz-1; idy++ )
	{
	    if ( (abovethreshold_ && input_->get(idx,idy)<threshold_) ||
	         (!abovethreshold_ && input_->get(idx,idy)>threshold_) )
		continue;

	    result_->set( idx, idy, 1 );
	    int lastidx;		
	    for ( int tidx=0; tidx<thetasz_/2; tidx++ )
	    {
		const float radius = (idy-csz/2)*costable_[tidx] + 
				     (idx-rsz/2)*sintable_[tidx];
		const int ridx = (int)(0.5+(radius/maxrho_+0.5)*rhosz_);
		if ( ridx<0 || ridx>=rhosz_ )
		{
		    lastidx = ridx;
		    continue;
		}

		hougharr_->set( tidx, ridx, hougharr_->get(tidx,ridx)+1 );
		hougharr_->set( tidx+thetasz_/2, rhosz_-ridx, 
			hougharr_->get(tidx+thetasz_/2,rhosz_-ridx)+1 );
		if ( !tidx )
		{
		    lastidx = ridx;
		    continue;
		}
		
		while ( lastidx-1>ridx && lastidx<rhosz_ )
		{
		    lastidx--;
		    hougharr_->set(tidx,lastidx, 
			    hougharr_->get(tidx,lastidx)+1 );
		    hougharr_->set( tidx+thetasz_/2, rhosz_-lastidx, 
			hougharr_->get(tidx+thetasz_/2,rhosz_-lastidx)+1 );
		}
		while ( lastidx+1<ridx && lastidx>0 )
		{
		    lastidx++;
		    hougharr_->set(tidx,lastidx, 
			    hougharr_->get(tidx,lastidx)+1 );
		    hougharr_->set(tidx+thetasz_/2, rhosz_-lastidx, 
			hougharr_->get(tidx+thetasz_/2,rhosz_-lastidx)+1 );
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
	       input_->set( idx, idy, 0 );
	   else
	       input_->set( idx, idy, result_->get(idx+1,idy+1) + 
		       result_->get(idx,idy+1) + result_->get(idx-1,idy+1) +
		       result_->get(idx+1,idy) + result_->get(idx,idy) + 
		       result_->get(idx-1,idy) + result_->get(idx+1,idy-1) + 
		       result_->get(idx,idy-1) + result_->get(idx-1,idy-1) );
       }
    }
    result_->setAll(0);

    TypeSet<int> tops, topids;
    TypeSet<int> tis, ris;
    for ( int tidx=0; tidx<thetasz_/2; tidx++ )
    {
       for ( int ridx=0; ridx<rhosz_; ridx++ )
       {
	   bool max = true;
	   for ( int dt =tidx-2; dt<=tidx+2; dt++ )
	   {
	       if ( dt<0 || dt>=thetasz_ )
		   continue;

	       for ( int dr=ridx-2; dr<=ridx+2; dr++ )
	       {   
		   if ( dr>=0 && dr<rhosz_ && 
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

    sort_coupled( tops.arr(), topids.arr(), localmaxsz );
    int localmin = localmaxsz>toplistnr_ ? localmaxsz-toplistnr_ : 0;
    int nrdone = 0;
    const bool angledefined = !mIsUdf(anglerg_.start);
    for ( int idx=localmaxsz-1; idx>=0 && nrdone<toplistnr_; idx-- )
    {
       int tidx = tis[topids[idx]];
       const float theta = ((float)tidx/(float)thetasz_-0.5)*2*M_PI;
       if ( angledefined && !anglerg_.includes(fabs(theta),false) )
	   continue;

       nrdone++;
       const float radius = ((float)ris[topids[idx]]/(float)rhosz_-0.5)*maxrho_;
       setLineFlag(radius, theta);
    }

    return true;

    /*Method 2*//*
    for ( int idx=1; idx<rsz-1; idx++ )
    {
	for ( int idy=1; idy<csz-1; idy++ )
	{
	    if ( input_->get(idx,idy)>0.8 )
		continue;

	    for ( int tidx=0; tidx<thetasz_; tidx++ )
	    {
		double theta = (double)tidx*M_PI/(double) thetasz_;
		double tantheta = tan(theta);
		double rho;
		if ( tantheta>263 )
		    rho = (double)idy;
		else
		{
		    double denom = tantheta * tantheta + 1.0;
		    double y1 = ((double)idx-(double)idy*tantheta)/denom;
		    double x1 = ((double)idy*tantheta*tantheta-
			    (double)idx*tantheta)/denom;
		    rho = sqrt (x1 * x1 + y1 * y1);
		}

		int ridx = (long)(rho*maxrho_+0.5);
		int val = hougharr_->get(tidx,ridx);
		if ( val<255 )
		    hougharr_->set(tidx,ridx, val+1);
	    }
	}
    }*/
}


void LineFrom2DSpaceHoughTransform::setLineFlag(float radius, float theta)
{
    const int rsz = input_->info().getSize(0);
    const int csz = input_->info().getSize(1);

    const bool usec = fabs(theta)>=M_PI_4 && fabs(theta)<=3*M_PI_4;
    const float slop = usec ? cos(theta)/sin(theta) : sin(theta)/cos(theta);
    const float dist = usec ? radius/(float)sin(theta) + (csz/2)*slop + rsz/2 
			    : radius/(float)cos(theta) + (rsz/2)*slop + csz/2;
    int startidx = -1;
    const int sz0 = usec ? csz : rsz;
    const int sz1 = usec ? rsz : csz;
    for ( int idx=0; idx<sz0; idx++ )
    {
	const int idy = (int)(0.5+dist-idx*slop);
	if ( idy<0 || idy>=sz1 )
	    continue;

	const float val = usec ? input_->get(idy,idx) : input_->get(idx,idy);
	if ( startidx<0 && val>0 )
	    startidx = idx;
	else if ( mIsZero(val,1e-8) && startidx>0 )
	{
	    if ( idx-startidx>30 ) //length threshold
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
	    }
	    startidx = -1;
	}
    }
}




