/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoMeshSurfaceBrick.cc,v 1.7 2004-07-26 13:04:37 kristofer Exp $";


#include "SoMeshSurfaceBrick.h"

#include <Inventor/SoPrimitiveVertex.h>

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/bundles/SoTextureCoordinateBundle.h>

#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include <Inventor/elements/SoNormalElement.h>

#include "Inventor/threads/SbMutex.h"

SO_NODE_SOURCE(SoMeshSurfaceBrick);

void SoMeshSurfaceBrick::initClass(void)
{
    SO_NODE_INIT_CLASS( SoMeshSurfaceBrick, SoIndexedTriangleStripSet,
	    		"IndexedTriangleStripSet" );
}


SoMeshSurfaceBrick::SoMeshSurfaceBrick()
    : coords( 0 )
    , validstate( 2 )
    , buildmutex( new SbMutex )
{
    SO_NODE_CONSTRUCTOR(SoMeshSurfaceBrick);

    SO_NODE_ADD_FIELD(normals,(0,0,0));
    SO_NODE_ADD_FIELD(sideSize,(32));
    SO_NODE_ADD_FIELD(spacing,(1));

    //textureCoordIndex.connectFrom( &coordIndex );
}


SoMeshSurfaceBrick::~SoMeshSurfaceBrick()
{
    delete buildmutex;
}


void SoMeshSurfaceBrick::setCoordPtr( const SbVec3f* crds )
{
    coords = crds;
}


#define mEndStrip() \
{ \
    coordIndex.set1Value( nrcrds, -1 ); \
    normalIndex.set1Value( nrcrds, -1 ); \
    nrcrds++; \
}

void SoMeshSurfaceBrick::build(bool lock)
{
    if ( lock ) buildmutex->lock();
    if ( !validstate )
    {
	if ( lock ) buildmutex->unlock();
	return;
    }

    validstate = 0;

    coordIndex.enableNotify(false);
    normalIndex.enableNotify(false);
    coordIndex.deleteValues(0);
    normalIndex.deleteValues(0);
    int nrcrds = 0;
    int idx00, idx10,nidx00,nidx10;
    bool atnextrow = true;

    const int nrcells = sideSize.getValue();
    for ( int rowidx=0; rowidx<nrcells; rowidx++ )
    {
	for ( int colidx=0; colidx<nrcells; colidx++ )
	{
	    if ( nrcrds < 2 || coordIndex[nrcrds-1] == -1 )
	    {
		startNewStrip( rowidx, colidx, nrcrds,
			       idx00, nidx00, idx10, nidx10, atnextrow);
		continue;
	    }

	    expandStrip( rowidx, colidx, nrcrds,
		         idx00, nidx00, idx10, nidx10, atnextrow );
	}
    
	if ( nrcrds && coordIndex[nrcrds-1] != -1 )
	    mEndStrip()
    }

    const int nrnormals = (nrcells+2)*(nrcells+1);
    for ( int idx=0; idx<nrnormals; idx++ )
	computeNormal(idx);

    if ( lock ) buildmutex->unlock();

    coordIndex.enableNotify(true);
    normalIndex.enableNotify(true);
    coordIndex.touch();
    normalIndex.touch();
}


void SoMeshSurfaceBrick::startNewStrip( int rowidx, int colidx, int& nrcrds,
					int& idx00, int& nidx00,
					int& idx10, int& nidx10,
					bool& atnextrow)
{
    const int normalsperrow = getNormalsPerRow();
    idx00 = getValidIndex( getCoordIndex(rowidx,colidx) );
    nidx00 = rowidx*normalsperrow+colidx;

    idx10 = getValidIndex( getCoordIndex(rowidx+1,colidx) );
    nidx10 = (rowidx+1)*normalsperrow+colidx;

    const int idx01 = getValidIndex( getCoordIndex(rowidx,colidx+1) );
    const int nidx01 = rowidx*normalsperrow+colidx+1;

    const int idx11 = getValidIndex( getCoordIndex(rowidx+1,colidx+1) );
    const int nidx11 = (rowidx+1)*normalsperrow+colidx+1;

    if ( (idx00<0)+(idx10<0)+(idx01<0)+(idx11<0) > 1 )
    {
	if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
	    mEndStrip()

	return;
    }

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0  )
    {
	bool diagonal00to11 = 
		getBestDiagonal(idx00,idx10,idx01,idx11,atnextrow);

	if ( diagonal00to11 )
	    createTriangle( nrcrds, idx10, nidx10, idx00, nidx00,
		    		    idx11, nidx11, idx01, nidx01 );
	else
	{
	    createTriangle( nrcrds, idx00, nidx00, idx01, nidx01,
		    		    idx10, nidx10, -1, -1 );
	    createTriangle( nrcrds, idx10, nidx10, idx01, nidx01,
		    		    idx11, nidx11 );
	}

	atnextrow = !diagonal00to11;

	idx00 = idx01;
	idx10 = idx11;
	nidx00 = nidx01;
	nidx10 = nidx11;

	return;
    }

    if ( idx00<0 )
    {
	createTriangle( nrcrds, idx10, nidx10, idx01, nidx01, idx11, nidx11 );
	atnextrow = true;
    }
    else if ( idx10<0 )
    {
	createTriangle( nrcrds, idx00, nidx00, idx01, nidx01, idx11, nidx11 );
	atnextrow = true;
    }
    else if ( idx01<0 )
    {
	createTriangle( nrcrds, idx00, nidx00, idx10, nidx10, idx11, nidx11 );
	atnextrow = true;
    }
    else if ( idx11<0 )
    {
	createTriangle( nrcrds, idx00, nidx00, idx10, nidx10, idx01, nidx01 );
	atnextrow = false;
	mEndStrip()
    }

    idx00 = idx01;
    idx10 = idx11;
    nidx00 = nidx01;
    nidx10 = nidx11;
}


void SoMeshSurfaceBrick::expandStrip( int rowidx, int colidx, int& nrcrds,
	                              int& idx00, int& nidx00,
				      int& idx10, int& nidx10, bool& atnextrow)
{
    const int normalsperrow = getNormalsPerRow();
    const int idx01 = getValidIndex( getCoordIndex(rowidx,colidx+1) );
    const int nidx01 = rowidx*normalsperrow+colidx+1;

    const int idx11 = getValidIndex( getCoordIndex(rowidx+1,colidx+1) );
    const int nidx11 = (rowidx+1)*normalsperrow+colidx+1;

    if ( idx01<0 && idx11<0 )
    {
	if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
	    mEndStrip()
	return;
    }

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0  )
    {
	bool diagonal00to11 = getBestDiagonal(idx00,idx10,idx01,idx11,
					     atnextrow);

	if ( atnextrow && !diagonal00to11 )
	    createTriangle( nrcrds, idx01, nidx01, idx11, nidx11 );
	else if ( !atnextrow && diagonal00to11 )
	    createTriangle( nrcrds, idx11, nidx11, idx01, nidx01 );
	else
	{
	    if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
		mEndStrip()

	    if ( diagonal00to11 )
		createTriangle( nrcrds, idx10, nidx10, idx00, nidx00,
					idx11, nidx11, idx01, nidx01);
	    else
	    {
		createTriangle( nrcrds, idx00, nidx00, idx01, nidx01,
					idx10, nidx10, -1, -1 );
		createTriangle( nrcrds, idx10, nidx10, idx01, nidx01,
					idx11, nidx11 );
	    }

	    atnextrow = !diagonal00to11;
	}

	idx00 = idx01;
	idx10 = idx11;
	nidx00 = nidx01;
	nidx10 = nidx11;
	return;
    }

    if ( idx11>=0 )
    {
	if ( atnextrow )
	{
	    if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
		mEndStrip();

	    if ( idx00>=0 && idx10>=0 )
	    {
		createTriangle( nrcrds, idx10, nidx10,
					idx00, nidx00, idx11, nidx11 );
	    }
	}
	else
	{
	    createTriangle( nrcrds, idx11, nidx11, -1, -1 );
	}
    }
    else if ( idx01>=0 )
    {
	if ( !atnextrow )
	{
	    if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
		mEndStrip();

	    if ( idx00>=0 && idx10>=0 )
	    {
		createTriangle( nrcrds, idx01, nidx01,
					idx00, nidx00, idx10, nidx10 );
	    }
	}
	else
	{
	    createTriangle( nrcrds, idx01, nidx01, -1, -1 );
	}
    }

    idx00 = idx01;
    idx10 = idx11;
    nidx00 = nidx01;
    nidx10 = nidx11;
}


void SoMeshSurfaceBrick::createTriangle( int& nrcrds, int idx0, int nidx0,
					int idx1, int nidx1,
					 int idx2, int nidx2,
					 int idx3, int nidx3 )
{
    coordIndex.set1Value( nrcrds, idx0 );
    normalIndex.set1Value( nrcrds, nidx0 );
    nrcrds++;

    coordIndex.set1Value( nrcrds, idx1 );
    normalIndex.set1Value( nrcrds, nidx1 );
    nrcrds++;
    if ( idx2 >= -1 )
    {
	coordIndex.set1Value( nrcrds, idx2 );
	normalIndex.set1Value( nrcrds, nidx2 );
	nrcrds++;
    }
    if ( idx3 >= -1 )
    {
	coordIndex.set1Value( nrcrds, idx3 );
	normalIndex.set1Value( nrcrds, nidx3 );
	nrcrds++;
    }
}


bool SoMeshSurfaceBrick::getBestDiagonal( int idx00, int idx10, int idx01, 
					  int idx11, bool atnextrow ) const
{
#define mEPS 1e-10

    const SbVec3f p00 = coords[idx00];
    const SbVec3f p10 = coords[idx10];
    const SbVec3f p01 = coords[idx01];
    const SbVec3f p11 = coords[idx11];

    if ( isUndefined(p00) || isUndefined(p10) || 
	 isUndefined(p01) || isUndefined(p11) )
	return atnextrow;

    SbVec3f vec0 = p00 - p11;
    SbVec3f vec1 = p10 - p01;
    float len0 = vec0.sqrLength();
    float len1 = vec1.sqrLength();

    if ( atnextrow )
	return ( (len0+mEPS) <= len1 );
    
    return (len0-mEPS) < len1;
}


int SoMeshSurfaceBrick::getValidIndex( int crdidx ) const
{
    const SbVec3f& crd = coords[crdidx];
    if ( !isUndefined(crd[2]) )
	return crdidx;

    return -1;
}


void SoMeshSurfaceBrick::invalidate()
{
    validstate = 2;
}


void SoMeshSurfaceBrick::doUpdate()
{
    if ( !validstate )
	validstate = 1;
}


int SoMeshSurfaceBrick::getValidState() const
{
    return validstate;
}


void SoMeshSurfaceBrick::invalidateNormal( int index )
{
    if ( invalidNormals.find(index)!=-1 )
	invalidNormals.push(index);
}


SbBool SoMeshSurfaceBrick::getNormal( int index, SbVec3f& res )
{
    buildmutex->lock();
    if ( (validstate!=2 && invalidNormals.find(index)==-1 &&
	   		           normals[index]!=SbVec3f(0,0,0)) ||
         computeNormal(index) )
    {
	res = normals[index];
        buildmutex->unlock();
	return true;
    }

    buildmutex->unlock();
    return false;
}


void SoMeshSurfaceBrick::GLRender(SoGLRenderAction* action)
{
    buildmutex->lock();
    if ( validstate==2 )
	build(false);
    else if ( !validstate )
    {
	if ( invalidNormals.getLength() )
	{
	    for ( int idx=invalidNormals.getLength()-1; idx>=0; idx-- )
	    {
		if ( computeNormal( invalidNormals[idx] ) )
		    invalidNormals.remove(idx);
	    }
	}
    }

    SoState* state = action->getState();
    SoNormalElement::set(action->getState(),
	    		 this, normals.getNum(), normals.getValues(0));

    inherited::GLRender(action);
    buildmutex->unlock();
}


SbBool SoMeshSurfaceBrick::computeNormal( int index )
{
    if ( !coords ) return false;
    const int nrnormalsperrow = getNormalsPerRow();
    const int relcoordrow = index/nrnormalsperrow;
    const int relcoordcol = index%nrnormalsperrow;
    const int coordindex = getCoordIndex(relcoordrow,relcoordcol);

    normals.set1Value( index, SbVec3f(0,0,0) );

    SbVec3f prevcolcoord;
    SbBool prevcolundef;
    if ( relcoordcol )
    {
	prevcolcoord = coords[getCoordIndex(relcoordrow,relcoordcol-1l)];
	prevcolundef = isUndefined( prevcolcoord );
    }
    else
	prevcolundef = true;

    const SbVec3f nextcolcoord =
		  coords[getCoordIndex(relcoordrow,relcoordcol+1)];
    const SbBool nextcolundef = isUndefined( nextcolcoord );

    SbVec3f v0;
    if ( prevcolundef || nextcolundef )
    {
	if ( prevcolundef && nextcolundef )
	    return false;

	if ( isUndefined( coords[coordindex] ) )
	    return false;

	if ( prevcolundef )
	    v0 = nextcolcoord-coords[coordindex];
	else 
	    v0 = coords[coordindex]-prevcolcoord;
    }
    else
	v0 = nextcolcoord-prevcolcoord;

    if ( !v0.length() )
	return false;

    SbVec3f prevrowcoord;
    SbBool prevrowundef;
    if ( relcoordrow )
    {
	prevrowcoord = coords[getCoordIndex(relcoordrow-1,relcoordcol)];
	prevrowundef = isUndefined( prevrowcoord );
    }
    else
	prevrowundef = true;

    const SbVec3f nextrowcoord =
		  coords[getCoordIndex(relcoordrow+1,relcoordcol)];
    const SbBool nextrowundef = isUndefined( nextrowcoord );

    SbVec3f v1;
    if ( prevrowundef || nextrowundef )
    {
	if ( prevrowundef && nextrowundef )
	    return false;

	if ( isUndefined( coords[coordindex] ) )
	    return false;

	if ( prevrowundef )
	    v1 = nextrowcoord-coords[coordindex];
	else 
	    v1 = coords[coordindex]-prevrowcoord;
    }
    else
	v1 = nextrowcoord-prevrowcoord;

    if ( !v1.length() )
	return false;

    normals.set1Value( index, v1.cross( v0 ) );

    return true;
}
