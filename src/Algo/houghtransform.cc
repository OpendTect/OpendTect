/*+
-----------------------------------------------------------------------------

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : N. Fredman
 * DATE     : 18-12-2002


-----------------------------------------------------------------------------
*/

static const char* rcsID = "$Id: houghtransform.cc,v 1.12 2009-07-22 16:01:29 cvsbert Exp $";


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
	    : calcpositions( ht_.calcpositions )
	    , datainfo( *ht_.datainfo )
	    , ht( ht_ )
	    , normals( *ht_.normals )
	    , idx( 0 )
	{ }
		    
protected:			
    int					nextStep();

    TypeSet<unsigned int>&		calcpositions;
    Array3DInfo&			datainfo;
    PlaneFrom3DSpaceHoughTransform&	ht;

    int					idx;
    TypeSet<Coord3>&			normals;
};
				

int PlaneFrom3DSpaceHoughTransformTask::nextStep()
{
    if ( idx>=calcpositions.size() ) return 0;

    int temppos[3];
    datainfo.getArrayPos( calcpositions[idx], temppos );
    const Coord3 pos( temppos[0], temppos[1], temppos[2] );
    static const Coord3 origo( 0,0,0 );

    for ( int normal=0; normal<normals.size(); normal++ )
    {
	Plane3 plane( normals[normal], pos, false );
	float distance = plane.distanceToPoint( origo );
	ht.incParamPos( normal, plane.distanceToPoint( origo ));
    }
   
    idx++;
    return 1;
}

    
PlaneFrom3DSpaceHoughTransform::PlaneFrom3DSpaceHoughTransform()
    : paramspacemutex( *new Threads::Mutex )
    , paramspace( 0 )
    , normals( 0 )
    , datainfo( 0 )
    , cliprate( 0.7 )
{}
		
		
PlaneFrom3DSpaceHoughTransform::~PlaneFrom3DSpaceHoughTransform()
{
    delete datainfo;
    if ( normals ) delete normals;
    delete paramspace;
}


void PlaneFrom3DSpaceHoughTransform::setResolution( double dangle,
						    int distsize )
{
    if ( normals ) delete normals;
    normals = makeSphereVectorSet(dangle);

    if ( paramspace ) delete paramspace;
    paramspace = new Array2DImpl<unsigned int>( normals->size(), distsize );
}


int PlaneFrom3DSpaceHoughTransform::getParamSpaceSize() const
{ return paramspace->info().getTotalSz(); }


int PlaneFrom3DSpaceHoughTransform::getNrDistVals() const
{ return paramspace->info().getSize(1); }


void PlaneFrom3DSpaceHoughTransform::setClipRate( float cliprt )
{
    cliprate = cliprt;
}


float PlaneFrom3DSpaceHoughTransform::clipRate() const    
{
    return cliprate;
}


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
    const int savesize = mNINT( nrsamples*cliprate );
   
    for ( int idx=nrsamples-1; idx>=nrsamples-savesize; idx-- )
	calcpositions += indexes[idx];

    delete datainfo;
    datainfo = dynamic_cast<Array3DInfo*>( data->info().clone() );

    unsigned int* paramptr = paramspace->getData();
    memset( paramptr, 0, sizeof(unsigned int)*getParamSpaceSize());

    const float maxx = datainfo->getSize( 0 );
    const float maxy = datainfo->getSize( 1 );
    const float maxz = datainfo->getSize( 2 );

    const float maxdist = Math::Sqrt( maxx*maxx + maxy*maxy + maxz*maxz ); 
    deltadist = maxdist / (getNrDistVals()-1);
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
    if ( !paramspace ) return 0;
    
    const unsigned int paramsize = getParamSpaceSize();
    TopList<unsigned int, unsigned int>* res =
		    new TopList<unsigned int, unsigned int>( size, 0, true );

    unsigned int* paramptr = paramspace->getData();

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
    paramspace->info().getArrayPos( planenr, pos );
    Coord3 normal = (*normals)[pos[0]];
    const float dist = pos[1] * deltadist;

    return Plane3( normal, Coord3(normal.x*dist,normal.y*dist,normal.z*dist),
	    	   false );
}


int PlaneFrom3DSpaceHoughTransform::getNrPointsAfterClip() const
{
    return calcpositions.size();
}


void PlaneFrom3DSpaceHoughTransform::incParamPos( int normalidx, double dist)
{
    const int distid = mNINT( dist/deltadist );
    unsigned int memoffset = reinterpret_cast<const Array2DInfo&>
			    (paramspace->info()).getOffset(normalidx,distid);
    unsigned int* dataptr = paramspace->getData();

    paramspacemutex.lock();
    dataptr[memoffset]++;
    paramspacemutex.unLock();
}
