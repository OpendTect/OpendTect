/*+
-----------------------------------------------------------------------------

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V. 
 * AUTHOR   : N. Fredman
 * DATE     : 18-12-2002


-----------------------------------------------------------------------------
*/

static const char* rcsID = "$Id: houghtransform.cc,v 1.3 2003-02-24 08:23:54 niclas Exp $";


#include "houghtransform.h"

#include "arrayndimpl.h"
#include "arrayndinfo.h"
#include "basictask.h"
#include "position.h"
#include "sorting.h"
#include "thread.h"
#include "trigonometry.h"

#include <math.h>

class PlaneFrom3DSpaceHoughTransformTask : public BasicTask
{
public:	
	PlaneFrom3DSpaceHoughTransformTask(PlaneFrom3DSpaceHoughTransform& ht_)
	    : calcpositions( ht_.calcpositions )
	    , datainfo( *ht_.datainfo )
	    , ht( ht_ )
	    , nrdipvals( ht_.paramspace->info().getSize( 0 ) )
	    , nrazimuthvals ( ht_.paramspace->info().getSize( 1 ) )
	    , nrdistvals ( ht_.paramspace->info().getSize( 2 ) )
	    , normals( ht_.normals )
	    , idx( 0 )
	    , deltadist( ht_.deltadist )

	{ }
		    
	~PlaneFrom3DSpaceHoughTransformTask()
	{ }
			

protected:			
    int					nextStep();

    TypeSet<unsigned int>&		calcpositions;
    Array3DInfo&			datainfo;
    PlaneFrom3DSpaceHoughTransform&	ht;

    int					idx;
    const float				deltadist;
    const int				nrdipvals;
    const int				nrazimuthvals;    
    const int				nrdistvals;
    Vector3*				normals;
};
				

int PlaneFrom3DSpaceHoughTransformTask::nextStep()
{
    if ( idx>=calcpositions.size() ) return 0;

    int temppos[3];
    datainfo.getArrayPos( calcpositions[idx], temppos );
    const Coord3 pos( temppos[0], temppos[1], temppos[2] );
    static const Coord3 origo( 0,0,0 );
   
    for ( int dip=0; dip<nrdipvals; dip++ )
    {	
	for ( int azimuth=0; azimuth<nrazimuthvals; azimuth++ )
	{
	    if ( !dip && azimuth )
		break;

	    Plane3 plane( normals[dip*nrazimuthvals+azimuth], pos );
    	    float distance = plane.distanceToPoint( origo );
	    int distid = (int) ( distance/deltadist );
	    
	    ht.incParamPos( dip, azimuth, distid );
	}
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
    delete [] normals;
}


void PlaneFrom3DSpaceHoughTransform::setParamSpaceSize( int dipsize,
					int azisize, int distsize )
{
    if ( paramspace ) delete paramspace;
    paramspace = new Array3DImpl<unsigned int>( dipsize, azisize, distsize );

    const float dipstep = M_PI/2/dipsize;
    const float azimuthstep = 2*M_PI/azisize;

    float cosazimuth[azisize];
    float sinazimuth[azisize];

    for ( int idx=0; idx<azisize; idx++ )
    {
	const float azimuth = idx*azimuthstep;
	cosazimuth[idx] = cos( azimuth );
	sinazimuth[idx] = sin( azimuth );
    }

    delete [] normals;
    normals = new Vector3[dipsize*azisize];

    for ( int idx=0; idx<dipsize; idx++ )
    {
	const float dip = idx*dipstep;
	const float sindip = sin( dip );
	const float cosdip = cos( dip );

	for ( int azimuth=0; azimuth<azisize; azimuth++ )
	{
	    const int normalidx = idx*azisize+azimuth;
	    Vector3 ab( cosdip*cosazimuth[azimuth], 
			sindip,
			cosdip*sinazimuth[azimuth] );
	    Vector3 ad( sinazimuth[azimuth], 0, -cosazimuth[azimuth] );
	    normals[normalidx] = ad.cross( ab );
	}
    }
}


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
    memcpy( datacopy, dataptr, datasize*sizeof(float) );
    
    ArrPtrMan<unsigned int> indexes = new unsigned int[datasize];
    
    for ( int idx=0; idx<datasize; idx++ )
	indexes[idx]= idx;

    float* datacopyptr = datacopy.ptr();
    unsigned int* indexesptr = indexes.ptr();
    sort_idxabl_coupled( datacopyptr, indexesptr, datasize );
    const int savesize = mNINT( datasize*cliprate );
   
    for ( int idx=datasize-1; idx>=datasize-savesize; idx-- )
    {
	if (  mIS_ZERO( datacopy[idx] ) ) continue;//Remove when debug is done
	calcpositions += indexes[idx];
    }

    delete datainfo;
    datainfo = dynamic_cast<Array3DInfo*>( data->info().clone() );

    unsigned int* paramptr = paramspace->getData();
    memset( paramptr, sizeof(unsigned int)*paramspace->info().getTotalSz(), 0 );
}


ObjectSet<BasicTask>* PlaneFrom3DSpaceHoughTransform::createCalculators()
{
    const float maxx = datainfo->getSize( 0 );
    const float maxy = datainfo->getSize( 1 );
    const float maxz = datainfo->getSize( 2 );

    const float maxdist = sqrt( maxx*maxx + maxy*maxy + maxz*maxz ); 

    deltadist = maxdist / paramspace->info().getSize(2);
    ObjectSet<BasicTask>* res = new ObjectSet<BasicTask>;
    (*res) += new PlaneFrom3DSpaceHoughTransformTask( *this );
    return res;
}


void PlaneFrom3DSpaceHoughTransform::sortParamSpace( int nrplanes )
{
    if ( !paramspace ) return;
    
    unsigned int paramsize = paramspace->info().getTotalSz();
    
    for ( int idx=0; idx<nrplanes; idx++ )
    {
	houghscores += 0;
	houghpositions += 0;
    }
    
    const unsigned int* paramdata = paramspace->getData();
    
    for ( int idx=0; idx<paramsize; idx++ )
    {
	unsigned int paramval =  paramdata[idx];

	if ( paramval>houghscores[nrplanes-1] )
	{
	    for ( int idy=0; idy<nrplanes; idy++ )
	    {
		if ( paramval>houghscores[idy] )
		{
		    houghscores.remove( nrplanes-1 );
		    houghscores.insert( idy, paramval );
		    houghpositions.remove( nrplanes-1 );
		    houghpositions.insert( idy, idx );
		    break;
		}
	    }
	}
    }
}


Plane3 PlaneFrom3DSpaceHoughTransform::getPlane( int nrplane ) const
{
    int pos[3];
    paramspace->info().getArrayPos( houghpositions[nrplane], pos );

    int nrdipvals = paramspace->info().getSize( 0 );
    int nrazimuthvals = paramspace->info().getSize( 1 );
    int nrdistvals = paramspace->info().getSize( 2 );

    const int normalidx = pos[0]*nrazimuthvals + pos[1];
    Vector3 normal = normals[normalidx];
    normal.normalize();

    const float dist = (pos[2]+0.5) * deltadist;

    return Plane3( normal, Coord3(normal.x*dist,normal.y*dist,normal.z*dist) );
    
}


unsigned int PlaneFrom3DSpaceHoughTransform::getHoughScore( int nrplane ) const
{
    if ( houghscores.size()==0 ) return 0;
    return  houghscores[nrplane];
}


int PlaneFrom3DSpaceHoughTransform::getNrPointsAfterClip() const
{
    return calcpositions.size();
}


void PlaneFrom3DSpaceHoughTransform::incParamPos( int p0, int p1, int p2 )
{
    unsigned int memoffset = reinterpret_cast<const Array3DInfo&>
				(paramspace->info()).getMemPos(p0,p1,p2);
    unsigned int* dataptr = paramspace->getData();

    paramspacemutex.lock();
    dataptr[memoffset]++;
    paramspacemutex.unlock();
}
