/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoKrisSurface.cc,v 1.2 2004-11-16 10:07:23 kristofer Exp $";

#include "SoKrisSurfaceImpl.h"
#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"

#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/threads/SbRWMutex.h>

#ifdef mac
# include "OpenGL/gl.h"
#else
# include "GL/gl.h"
#endif

#define mIsCoordDefined( coord ) (coord[0]<1e29)

MeshSurfacePartPart::MeshSurfacePartPart(SoKrisSurface& m, int s0, int s1 )
    : meshsurface( m )
    , rowstart( s0 )
    , colstart( s1 )
    , cache( 0 )
{ }


MeshSurfacePartPart::~MeshSurfacePartPart()
{
    if ( cache ) cache->unref();
}


void MeshSurfacePartPart::setStart( int row, int col )
{
    rowstart = row; colstart=col; 
}


void MeshSurfacePartPart::touch( int i0, int i1, bool undef )
{
    if ( !isInside(i0,i1) )
	return;

    //cache->invalidize();
    ownvalidation = false;
}


void MeshSurfacePartPart::invalidateCaches()
{ if ( cache ) cache->invalidate(); }


void MeshSurfacePartPart::computeBBox( SoState* state, SbBox3f& box,
				    SbVec3f& center, bool useownvalidation )
{
    if ( cache && ((useownvalidation&&ownvalidation) || cache->isValid(state)))
    {
	box = cache->getBox();
	if ( cache->isCenterSet() )
	    center = cache->getCenter();
	return;
    }

    if ( cache ) cache->unref();
    const bool storedinvalid = SoCacheElement::setInvalid(false);
    state->push();
    cache = new SoBoundingBoxCache(state);
    cache->ref();
    SoCacheElement::set(state, cache);

    box.makeEmpty();
    center = SbVec3f(0,0,0);
    int ncoords = 0;

    const int sidesize = sideSize();
    const int nrrows = meshsurface.nrRows();
    const int nrcols = meshsurface.nrColumns.getValue();
    for ( int rowidx=0; rowidx<sidesize; rowidx++ )
    {
	const int row = rowidx+rowstart;
	if ( row>=nrrows )
	    break;

	int nrcolsonpart = sidesize;
	if ( colstart+sidesize>=meshsurface.nrColumns.getValue() )
	    nrcolsonpart = meshsurface.nrColumns.getValue()-colstart;

	const SbVec3f* coordptr =
	    meshsurface.coordinates.getValues(row*nrcols+colstart);
	for ( int colidx=0; colidx<nrcolsonpart; colidx++ )
	{
	    const SbVec3f& coord = coordptr[colidx];
	    if ( mIsCoordDefined( coord ) )
	    {
		box.extendBy(coordptr[colidx]);
		ncoords++;
	    }
	}
    }

    if ( ncoords ) center /= ncoords;
    cache->set( box, ncoords, center );
    state->pop();
    SoCacheElement::setInvalid(storedinvalid);
}

#define mFindClosestNode( idx0, idx1, idx2 ) \
int closestmatidx = mi##idx0; \
int closestrow = row##idx0; \
int closestcol = col##idx0; \
float shortestdist = (intersection-c##idx0).sqrLength(); \
float dist; \
if ( (dist = (intersection-c##idx1).sqrLength())<shortestdist ) \
{ \
    shortestdist=dist; \
    closestmatidx = mi##idx1; \
    closestrow = row##idx1; \
    closestcol = col##idx1; \
} \
if ( (dist = (intersection-c##idx2).sqrLength())<shortestdist ) \
{ \
    closestmatidx = mi##idx2; \
    closestrow = row##idx2; \
    closestcol = col##idx2; \
} \
\
SoPickedPoint* pickedpoint = action->addIntersection(intersection); \
SoFaceDetail* facedetail = new SoFaceDetail; \
facedetail->setFaceIndex(closestrow); \
facedetail->setPartIndex(closestcol); \
facedetail->setNumPoints(3); \
SoPointDetail pointdetail; \
pointdetail.setMaterialIndex(mi##idx0); \
facedetail->setPoint(0,&pointdetail); \
pointdetail.setMaterialIndex(mi##idx1); \
facedetail->setPoint(1,&pointdetail); \
pointdetail.setMaterialIndex(mi##idx2); \
facedetail->setPoint(2,&pointdetail); \
pickedpoint->setDetail(facedetail, const_cast<SoKrisSurface*>(&meshsurface));

void MeshSurfacePartPart::rayPick( SoRayPickAction* action,
				   bool useownvalidation )
{
    SoState* state = action->getState();

    SbBox3f box;
    SbVec3f dummy;

    computeBBox( state, box, dummy, useownvalidation );
    if ( !action->intersect(box,dummy) )
	return;

    const int nrrows = meshsurface.nrRows();
    const int nrcols = meshsurface.nrColumns.getValue();

    const int sidesize = sideSize();
    for ( int rowidx=0; rowidx<sidesize; rowidx++ )
    {
	const int row = rowidx+rowstart;
	if ( row>= nrrows )
	    return;

	const int currowstart = row*nrcols+colstart;
	const int nxtrowstart = (row+1)*nrcols+colstart;

	const SbVec3f* currowcoordptr =
	    		meshsurface.coordinates.getValues(currowstart);
	const SbVec3f* nxtrowcoordptr =
			meshsurface.coordinates.getValues(nxtrowstart);

	int nci = sidesize;
	if ( colstart+sidesize>=meshsurface.nrColumns.getValue() )
	    nci = meshsurface.nrColumns.getValue()-colstart;

	bool hasmaterial = meshsurface.materialIndex.getNum()>=nxtrowstart+nci;

	const int32_t* currowmatptr = hasmaterial ?
	    		meshsurface.materialIndex.getValues(currowstart) : 0;
	const int32_t* nxtrowmatptr = hasmaterial ?
			meshsurface.materialIndex.getValues(nxtrowstart) : 0;
	SbVec3f c00 = *currowcoordptr++;
	SbVec3f c10 = *nxtrowcoordptr++;
	bool use00 = mIsCoordDefined(c00);
	bool use10 = mIsCoordDefined(c10);
	int mi00 = currowmatptr ? *currowmatptr++ : 1;
	int mi10 = nxtrowmatptr ? *nxtrowmatptr++ : 1;

	const int row00 = row;
	const int row10 = row+1;
	const int row01 = row;
	const int row11 = row+1;
	int col00 = colstart;
	int col10 = colstart;

	for ( int colidx=0; colidx<nci; colidx++ )
	{
	    const SbVec3f c01 = *currowcoordptr++;
	    const SbVec3f c11 = *nxtrowcoordptr++;

	    const bool use01 = mIsCoordDefined(c01);
	    const bool use11 = mIsCoordDefined(c11);

	    int mi01 = currowmatptr ? *currowmatptr++ : 1;
	    int mi11 = nxtrowmatptr ? *nxtrowmatptr++ : 1;

	    const int col01 = col00+1;
	    const int col11 = col10+1;

	    if ( use00+use10+use01+use11>2 )
	    {
		if ( use00 && use10 && use01 && use11 )
		{

		    //TODO own implementation?
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c10, c00, c01, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 10, 00, 01 );
		    }
		    else if ( action->intersect( c01,c11,c10,intersection,
					          barycentric,front))
		    {
			mFindClosestNode( 01, 11, 10 );
		    }
		}
		else if ( !use00 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c01,c11,c10,intersection,
					    barycentric,front))
		    {
			mFindClosestNode( 01, 11, 10 );
		    }
		}
		else if ( !use01 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c11, c10, c00, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 11, 10, 00 );
		    }
		}
		else if ( !use11 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c10, c00, c01, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 10, 00, 01 );
		    }
		}
		else if ( !use10 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c00, c01, c11, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 00, 01, 11 );
		    }
		}
	    }

	    use00=use01; c00=c01; mi00=mi01; col00=col01;
	    use10=use11; c10=c11; mi10=mi11; col10=col11;
	}
    }
}


bool MeshSurfacePartPart::isInside( int i, int j ) const
{
    const int sidesize = sideSize();
    return i>=rowstart&&j>=colstart&&(i-rowstart)>sidesize&&(j-colstart)<sidesize;
}


const int32_t* MeshSurfacePartPart::getMatIndexPtr(int i0,int i1) const
{
    const int idx = meshsurface.getIndex(i0,i1);
    if ( idx==-1 ) return 0;
    return meshsurface.materialIndex.getValues(idx);
}


MeshSurfaceTesselationCache::MeshSurfaceTesselationCache( 
				  const SoKrisSurface& m, bool isstrip_ )
    : meshsurface(m)
    , isstrip( isstrip_ )
    , buildmutex( new SbRWMutex( SbRWMutex::READ_PRECEDENCE) )
    , isvalid( true )
{}


MeshSurfaceTesselationCache::~MeshSurfaceTesselationCache()
{
    delete buildmutex;
}


void MeshSurfaceTesselationCache::reset(bool all)
{
    buildmutex->writeLock();
    setValid(false);
    ni.truncate(0,false);
    ci.truncate(0,false);

    if ( all )
	normals.truncate(0,false);

    buildmutex->writeUnlock();
}



void MeshSurfaceTesselationCache::GLRender(SoGLRenderAction* action)
{
    SoMaterialBundle mb(action);
    const SbVec3f* cptr = meshsurface.coordinates.getValues(0);
    const int32_t* miptr = meshsurface.materialIndex.getValues(0);

    mb.sendFirst();

    buildmutex->readLock();
    const int nrci = ci.getLength();
    bool isopen = false;
    for ( int idx=0; idx<nrci; idx++ )
    {
	const int index = ci[idx];
	if ( index<0 )
	{
	    if ( isopen )
	    {
		glEnd();
		isopen = false;
	    }
	    continue;
	}

	if ( !isopen )
	{
	    mb.sendFirst();
	    glBegin( isstrip ? GL_TRIANGLE_STRIP : GL_TRIANGLE_FAN );
	    isopen = true;
	}

	mb.send(miptr?miptr[index]:0,true);
	glNormal3fv(normals[ni[idx]].getValue());
	glVertex3fv(cptr[index].getValue());
	//Add textureCoordIndex
    }
    
    if ( isopen )
	glEnd();

    buildmutex->readUnlock();
}


MeshSurfacePart::MeshSurfacePart( SoKrisSurface& m, int s0, int s1, int ssz )
    : meshsurface( m )
    , start0( s0 )
    , start1( s1 )
    , sidesize( ssz )
    , resolution( 0 )
    , reshaschanged( true )
    , bboxcache( 0 )
    , bboxvalidation( false )
    , gluecache( 0 )
{
    int spacing = sidesize/2;
    int nrcells = 2;
    while ( true )
    {
	resolutions.push(
	    new MeshSurfacePartResolution(m,s0,s1,nrcells,nrcells,spacing) );
	if ( spacing==1 )
	    break;

	spacing /= 2;
	nrcells *= 2;
    }

    const int nrbboxes = sidesize / MeshSurfacePartPart::sideSize();
    for ( int idx=0; idx<nrbboxes; idx++ )
    {
	const int bboxstart0 = s0+MeshSurfacePartPart::sideSize()*idx;
	for ( int idy=0; idy<nrbboxes; idy++ )
	{
	    bboxes.push( new MeshSurfacePartPart(m,bboxstart0,
			 s1+MeshSurfacePartPart::sideSize()*idy));
	}
    }

    for ( int idx=0; idx<9; idx++ )
	neighbors.append(0);
}


MeshSurfacePart::~MeshSurfacePart()
{
    //Notify neighbors about removal?
    for ( int idx=0; idx<resolutions.getLength(); idx++ )
	delete resolutions[idx];

    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	delete bboxes[idx];

    if ( bboxcache ) bboxcache->unref();
}


void MeshSurfacePart::setStart( int row, int col )
{
    for ( int idx=0; idx<resolutions.getLength(); idx++ )
	resolutions[idx]->setStart( row, col );

    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	bboxes[idx]->setStart( row, col );

}


void MeshSurfacePart::touch( int i0, int i1, bool undef )
{
    const int hsidesize = sidesize/2;
    const int rel0 = i0-start0;
    const int rel1 = i1-start1;

    const int spacing = hsidesize;
    for ( int res=0; res<resolutions.getLength(); res++ )
    {
	if ( rel0<-spacing||rel0>spacing+sidesize||
	     rel1<-spacing||rel1>spacing+sidesize )
	return;

	resolutions[res]->touch( i0, i1, undef);
    }

    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	bboxes[idx]->touch(i0,i1, undef );
}


void MeshSurfacePart::computeBBox(SoState* state, SbBox3f& bbox,
				  SbVec3f& center, bool useownvalidation )
{
    if ( (bboxcache && bboxcache->isValid(state)) ||
	 (useownvalidation && bboxvalidation) )
    {
	bbox = bboxcache->getBox();
	if ( bboxcache->isCenterSet() )
	    center = bboxcache->getCenter();
	return;
    }

    if ( bboxcache )
	bboxcache->unref();

    const bool storedinvalid = SoCacheElement::setInvalid(false);
    state->push();
    bboxcache = new SoBoundingBoxCache(state);
    bboxcache->ref();
    SoCacheElement::set(state, bboxcache);

    center = SbVec3f(0,0,0);
    SbBox3f localbox;
    SbVec3f localcenter;

    int nrboxes = 0;
    for ( int idx=0; idx<bboxes.getLength(); idx++ )
    {
	bboxes[idx]->computeBBox(state,localbox,localcenter,useownvalidation);
	if ( !localbox.isEmpty() )
	{
	    bbox.extendBy(localbox);
	    center += localcenter;
	    nrboxes ++;
	}
    }

    if ( nrboxes ) center /= nrboxes;
    bboxcache->set( bbox, nrboxes, center );
    state->pop();
    SoCacheElement::setInvalid(storedinvalid);
}


void MeshSurfacePart::rayPick( SoRayPickAction* action, bool useownvalidation )
{
    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	bboxes[idx]->rayPick(action,useownvalidation);
}


void MeshSurfacePart::GLRender(SoGLRenderAction* action,bool useownvalidation)
{
    reshaschanged = false;
    if ( resolution<0 ) return;
    resolutions[resolution]->GLRender( action, useownvalidation );
}


int MeshSurfacePart::computeResolution( SoState* state, bool ownv) 
{
    SbBox3f bbox;
    SbVec3f dummy;
    computeBBox( state, bbox, dummy, ownv );

    const int nrrows = nrRows();
    const int nrcols = nrCols();
    const int numres = resolutions.getLength();
    int minres = numres-1;
    const int minsize = nrrows<nrcols?nrrows:nrcols;
    int spacing = sidesize/2;
    for ( ; minres>0; minres-- )
    {
	if ( spacing<=minsize )
	    break;
	spacing /=2;
    }
	
    const int32_t camerainfo = SoCameraInfoElement::get(state);
    int desiredres = minres;
    if ( !(camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE)) )
    {
	SbVec2s screensize;
	SoShape::getScreenSize( state, bbox, screensize );
	const float complexity =
			 SbClamp(SoComplexityElement::get(state), 0.0f, 1.0f);
	const float wantednumcells = complexity*screensize[0]*screensize[1]/32;
	int nrcells = nrrows*nrcols;
	for ( desiredres=numres-1; desiredres>=minres;
	      desiredres-- )
	{
	    const int nextnumcells = nrcells/4;
	    if ( nextnumcells<wantednumcells )
		break;
	    nrcells = nextnumcells;
	}
    }

    return desiredres;
}


bool MeshSurfacePart::setResolution( int desiredres,
				     bool useownvalidation )
{
    for ( int idx=desiredres; idx>0; idx-- )
    {
	bool candisplay = resolutions[idx]->canDisplay(useownvalidation);
	if ( candisplay )
	{
	    if ( idx!=resolution )
	    {
		reshaschanged = true;
		resolution = idx;
	    }

	    return  idx==desiredres &&
		    !resolutions[idx]->needsUpdate(useownvalidation);
	}
    }

    if ( resolution )
    {
	reshaschanged = true;
	resolution = 0;
    }

    return !desiredres;
}



void MeshSurfacePart::setNeighbor( int neighbor, MeshSurfacePart* n, bool cb )
{
    while ( neighbors.getLength() <= neighbor )
	neighbors.push(0);

    neighbors[neighbor] = n;
    if ( n && cb ) n->setNeighbor(8-neighbor, this, false );
}


int MeshSurfacePart::getSpacing( int res ) const
{ return resolutions[res]->getSpacing(); }


#define mAddCoordToIndexes(row_,col_,res_, indexes, normals) \
{ \
    const int index_ = meshsurface.getIndex( row_, col_ ); \
    if ( index_ != -1 ) \
    { \
	indexes.push(index_); \
	normals.push(getNormal(row_,col_,res_,useownvalidation)); \
    } \
}


void MeshSurfacePart::GLRenderGlue( SoGLRenderAction* action,
				    bool useownvalidation )
{
    if ( resolution==-1 )
	return;

    const bool tesselate =
	(!useownvalidation && gluecache
	     ? gluecache->isValid() : gluevalidation ) || reshaschanged ||
	(neighbors[5] && neighbors[5]->hasResChangedSinceLastRender()) ||
	(neighbors[7] && neighbors[7]->hasResChangedSinceLastRender());

    if ( !tesselate )
    {
	gluecache->GLRender(action);
	return;
    }

    if ( !gluecache ) 
	gluecache = new MeshSurfaceTesselationCache(meshsurface,true);

    gluecache->buildmutex->writeLock();

    const int ownspacing = getSpacing(resolution);
    const int res5 = neighbors[5] ? neighbors[5]->getResolution() : resolution;
    const int spacing5 = getSpacing(res5);
    const int res7 = neighbors[7] ? neighbors[7]->getResolution() : resolution;
    const int spacing7 = getSpacing(res7);
    const int res8 = neighbors[8] ? neighbors[8]->getResolution() : res7;

    SbList<SbVec2s> rowgluecells;
    const int rowgluespacing =
	ownspacing>spacing7 ? ownspacing : spacing7;

    int row = start0+sidesize-ownspacing;
    int col = start1;
    for ( int idx=0; idx<sidesize-ownspacing; idx+=rowgluespacing )
    {
	rowgluecells.append( SbVec2s(row,col+idx) );
    }

    SbList<SbVec2s> colgluecells;
    const int colgluespacing =
	ownspacing>spacing5 ? ownspacing : spacing5;

    row = start0;
    col = start1+sidesize-ownspacing;
    for ( int idx=0; idx<sidesize-ownspacing; idx+=colgluespacing )
    {
	colgluecells.append( SbVec2s(row+idx,col) );
    }

    //Make corner.
    if ( res5>=resolution || res7>=resolution )
    {
	if ( res5<resolution )
	{
	    /*
	       2--3
	       |  |
	      -1  |
	       |  |
	      -5--4
	      Nothing needs to be done, since the cell (2) is present
	      in colgluecells and will be made.
	     */
	}
	if ( res7==resolution && res5>resolution )
	{
	    /*
	       |    |
	      -1----2
	       |    |
	       |    3
	       |    |
	      -5----4
	    Add the square (1) to the colcells
	    */
	    row = start0+sidesize-ownspacing;
	    col = start1+sidesize-ownspacing;
	    colgluecells.append( SbVec2s(row,col) );
	}
	else if ( res7<resolution )
	{
	    /*
		  |  |
	       5--1--2
	       |     |
	       4-----3
	       Nothing needs to be done, since the cell (5) is present
	       row rowgluecells and will be made.
	    */
	}
	else if ( res5==resolution && res7>=resolution )
	{
	    /*
		   |   |
		  -1---2
		   |   |
		   |   |
		   |   |
		  -5-4-3
	    */
	    rowgluecells.append(SbVec2s(start0+sidesize-ownspacing,
					start1+sidesize-ownspacing));
	}
	else
	{
	    SbList<int> brickindexes;
	    SbList<SbVec3f> bricknormals;
	    row = start0+sidesize-ownspacing;
	    const int startcol = start1+sidesize-ownspacing;
	    mAddCoordToIndexes( row, col, resolution, brickindexes,
				bricknormals );
	    SbList<int> neighborindexes;
	    SbList<SbVec3f> neighbornormals;

	    col += ownspacing;
	    mAddCoordToIndexes( row, col, res5,
				neighborindexes, neighbornormals );
	    while ( row<=start0+sidesize )
	    {
		const int res = row==start0+sidesize?res8:res5;
		mAddCoordToIndexes( row, col, res,
				    neighborindexes, neighbornormals );
		row+=spacing5;
	    }

	    row = start0+sidesize;

	    while ( col>=startcol )
	    {
		mAddCoordToIndexes( row, col, res7,
				    neighborindexes, neighbornormals );
		col-=spacing7;
	    }

	    addGlueFan( brickindexes, bricknormals,
			neighborindexes, neighbornormals, false );
	}
    }
    else
    {
	/*  The resolution on both neighbors are lower than our, so
	    our corner will look like this:
		     2--3
		     |  |
		  6--1  |
		  |     |
		  5-----4
	*/
	SbList<int> brickindexes;
	SbList<SbVec3f> bricknormals;
	row = start0+sidesize-ownspacing;
	col = start1+sidesize-ownspacing;
	mAddCoordToIndexes( row, col, resolution, brickindexes, bricknormals );

	SbList<int> neighborindexes;
	SbList<SbVec3f> neighbornormals;

	const int minrow = start0+sidesize-spacing5;
	while ( row>minrow )
	{
	    row-=ownspacing;
	    mAddCoordToIndexes( row, col, resolution,
				neighborindexes, neighbornormals );
	    const int cellindex = colgluecells.find(SbVec2s(row,col));
	    if ( cellindex!=-1 ) colgluecells.removeFast(cellindex);
	}

	col+=ownspacing;
	mAddCoordToIndexes( row, col, res5,
			    neighborindexes, neighbornormals );
											row+=spacing5;
	mAddCoordToIndexes( row, col, res8,
			    neighborindexes, neighbornormals );

	col-=spacing7;
	mAddCoordToIndexes( row, col, res7,
											neighborindexes, neighbornormals );
	row-=ownspacing;
	mAddCoordToIndexes( row, col, resolution,
			    neighborindexes, neighbornormals );

	int cellindex = rowgluecells.find(SbVec2s(row,col));
	if ( cellindex!=-1 ) rowgluecells.removeFast(cellindex);

	const int maxcol = start1+sidesize-ownspacing*2;
	while ( col<maxcol )
	{
	    col += ownspacing;
	    mAddCoordToIndexes( row, col, resolution,
				neighborindexes, neighbornormals );
	    cellindex = rowgluecells.find(SbVec2s(row,col));
	    if ( cellindex!=-1 ) rowgluecells.removeFast(cellindex);
	}

	addGlueFan( brickindexes, bricknormals,
		    neighborindexes, neighbornormals, false );
    }

    while ( rowgluecells.getLength() )
    {
	const SbVec2s rc = rowgluecells.pop();
	row = rc[0];
	const int startcol = col = rc[1];

	SbList<int> brickindexes;
	SbList<SbVec3f> bricknormals;
	const int maxcol = col+rowgluespacing;
	const int bordercol = start1+sidesize;
	while ( col<=maxcol )
	{
	    const int res = col==bordercol ? res5 : resolution;
	    mAddCoordToIndexes( row, col, res, brickindexes, bricknormals );
	    col += ownspacing;
	}

	row +=ownspacing;
	col = startcol;
	SbList<int> neighborindexes;
        SbList<SbVec3f> neighbornormals;
        while ( col<=maxcol )
        {
            const int res = col==bordercol ? res8 : res7;
            mAddCoordToIndexes( row, col, res, neighborindexes,
				neighbornormals );
	    col += spacing7;
	}

        if ( ownspacing>=spacing7 )
        {
            addGlueFan( brickindexes, bricknormals,
			neighborindexes, neighbornormals, true );
	}
	else
	{
	    addGlueFan( neighborindexes,
			neighbornormals, brickindexes, bricknormals, false );
	}
    }

    while ( colgluecells.getLength() )
    {
	const SbVec2s rc = colgluecells.pop();
	const int startrow = row = rc[0];
	col = rc[1];

	SbList<int> brickindexes;
	SbList<SbVec3f> bricknormals;
	const int maxrow = row+colgluespacing;
	const int borderrow = start0+sidesize;
	while ( row<=maxrow )
	{
	    const int res = row==borderrow ? res7 : resolution;
	    mAddCoordToIndexes( row, col, res, brickindexes, bricknormals );
	    row += ownspacing;
	}

	row = startrow;
	col += ownspacing;
	SbList<int> neighborindexes;
	SbList<SbVec3f> neighbornormals;
	while ( row<=maxrow )
	{
	    const int res = row==borderrow ? res8 : res5;
	    mAddCoordToIndexes( row, col, res, neighborindexes,
				neighbornormals );
	    row += spacing5;
	}

	if ( ownspacing>=spacing5 )
	{
	    addGlueFan( brickindexes, bricknormals,
			neighborindexes, neighbornormals, false );
	}
	else
	{
	    addGlueFan( neighborindexes,
			neighbornormals, brickindexes, bricknormals, true );
	}
    }

    gluecache->buildmutex->writeUnlock();
}


void MeshSurfacePart::invalidateCaches()
{
    if ( bboxcache ) bboxcache->invalidate();
    if ( gluecache ) gluecache->setValid(false);

    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	bboxes[idx]->invalidateCaches();

    for ( int idx=0; idx<resolutions.getLength(); idx++ )
	resolutions[idx]->invalidateCaches();
}


SbVec3f MeshSurfacePart::getNormal( int row, int col, int res,
				    bool useownvalidation )
{
    const int relrow=row-start0;
    const int relcol=col-start1;

    const int spacing = getSpacing( res );
    SbVec3f norm( 0, 0, 1 );

#define mReturnNormal(rowoff,coloff)\
{\
    MeshSurfacePart* part = 0;\
    if ( relcol+rowoff<0 )\
	part = neighbors[1];\
    else if ( relcol+coloff<0 )\
	part = neighbors[3];\
    else if ( relrow+rowoff>=sidesize )\
	part = relcol+coloff>=sidesize ? neighbors[8] : neighbors[7];\
    else if ( relcol+coloff>=sidesize )\
	part = neighbors[5];\
    else \
	part = this; \
\
    if ( part && part->getNormal(row+rowoff,col+coloff,res, \
	 useownvalidation,norm) )\
	return norm;\
}

    mReturnNormal(0,0)
    mReturnNormal(-spacing,0)
    mReturnNormal(spacing,0)
    mReturnNormal(0,-spacing)
    mReturnNormal(0,spacing)

    return norm;
}


SbBool MeshSurfacePart::getNormal( int row, int col, int res,
       				   bool useownvalidation, SbVec3f& normal)
{
    const int relrow=row-start0;
    const int relcol=col-start1;

    if ( relrow>=0 && relrow<sidesize && relcol>=0 && relcol<sidesize )
    {
	MeshSurfacePartResolution* partres = resolutions[res];
	int spacing = partres->getSpacing();
	if ( partres->getNormal( relcol%spacing, relcol%spacing,
		    		 useownvalidation, normal) )
	    return true;
    }

    return false;
}


#define mAddFanNode( ci_, n_ ) \
    gluecache->ni.push( gluecache->normals.getLength() ); \
    gluecache->normals.push(n_); \
    gluecache->ci.push(ci_)

#define mStopFanStrip \
    gluecache->ni.push(-1); \
    gluecache->ci.push(-1)

void MeshSurfacePart::addGlueFan( const SbList<int>& lowresci,
    const SbList<SbVec3f>& lowresnorm, const SbList<int>& highresci,
    const SbList<SbVec3f>& highresnorm, SbBool forward )
{
    const int nrlowres = lowresci.getLength();
    const int nrhighres = highresci.getLength();


    if ( nrlowres+nrhighres<3 || !nrlowres || !nrhighres )
	return;

    const SbVec3f* cptr = meshsurface.coordinates.getValues(0);

    if ( nrlowres==2 && nrhighres==2 )
    {
	const float d0 = (cptr[lowresci[1]]-
		          cptr[highresci[0]]).length();
	const float d1 = (cptr[highresci[1]]-
			  cptr[lowresci[0]]).length();

	const bool splitlowres = (!forward && d1<d0) || (forward&&d1>d0);
	if ( splitlowres )
	{
	    const int idx = forward?1:0;
	    mAddFanNode( lowresci[idx], lowresnorm[idx] );
	}
	else
	{
	    int idx = forward?0:1;
	    mAddFanNode( lowresci[idx], lowresnorm[idx] );
	    idx = forward?1:0;
	    mAddFanNode( lowresci[idx], lowresnorm[idx] );
	}

	int idx = forward?1:0;
	mAddFanNode( highresci[idx], highresnorm[idx] );
	idx = forward?0:1;
	mAddFanNode( highresci[idx], highresnorm[idx] );

	if ( splitlowres )
	{
	    idx = forward?0:1;
	    mAddFanNode( lowresci[idx], lowresnorm[idx] );
	}
    }
    else if ( nrlowres==1 )
    {
	mAddFanNode( lowresci[0], lowresnorm[0] );
	if ( forward )
	{
	    for ( int idx=nrhighres-1; idx>=0; idx-- )
	    {
		mAddFanNode( highresci[idx], highresnorm[idx] );
	    }
	}
	else
	{
	    for ( int idx=0; idx<nrhighres; idx++ )
	    {
		mAddFanNode( highresci[idx], highresnorm[idx] );
	    }
	}
    }
    else if ( nrhighres==1 )
    {
	if ( forward )
	{
	    mAddFanNode( lowresci[0], lowresnorm[0] );
	    mAddFanNode( lowresci[1], lowresnorm[1] );
	    mAddFanNode( highresci[0], highresnorm[0] );
	}
	else
	{
	    mAddFanNode( lowresci[0], lowresnorm[0] );
	    mAddFanNode( highresci[0], highresnorm[0] );
	    mAddFanNode( lowresci[1], lowresnorm[1] );
	}
    }
    else
    {
	mAddFanNode( lowresci[0], lowresnorm[0] );
	if ( forward )
	{
	    mAddFanNode( lowresci[1], lowresnorm[1] );
	    for ( int idx=nrhighres/2; idx>=0; idx-- )
	    {
		mAddFanNode( highresci[idx], highresnorm[idx] );
	    }
	}
	else
	{
	    for ( int idx=0; idx<=nrhighres/2; idx++ )
	    {
		mAddFanNode( highresci[idx], highresnorm[idx] );
	    }

	    mAddFanNode( lowresci[1], lowresnorm[1] );
	}

	mStopFanStrip;
				    
	mAddFanNode( lowresci[1], lowresnorm[1] );
	if ( forward )
	{
	    for ( int idx=nrhighres-1; idx>=nrhighres/2; idx-- )
	    {
		mAddFanNode( highresci[idx], highresnorm[idx] );
	    }
	}
	else
	{
	    for ( int idx=nrhighres/2; idx<nrhighres; idx++ )
	    {
		mAddFanNode( highresci[idx], highresnorm[idx] );
	    }
	}
    }

    mStopFanStrip;
}



int MeshSurfacePart::nrRows() const
{
    const int totalnrrows = meshsurface.nrRows();
    const int localnrrows = totalnrrows-start0;
    return localnrrows<sidesize?localnrrows:sidesize;
}


int MeshSurfacePart::nrCols() const
{
    const int totalnrcols = meshsurface.nrColumns.getValue();
    const int localnrcols = totalnrcols-start1;
    return localnrcols<sidesize?localnrcols:sidesize;
}


MeshSurfacePartResolution::MeshSurfacePartResolution(SoKrisSurface& m,
	int s0, int s1, int ssz0, int ssz1, int sp )
    : meshsurface(m)
    , start0( s0 )
    , start1( s1 )
    , sidesize0(ssz0)
    , sidesize1(ssz1)
    , cachestatus(2)
    , cache( 0 ) 
    , spacing( sp )
{
}


MeshSurfacePartResolution::~MeshSurfacePartResolution()
{
    delete cache;
}


void MeshSurfacePartResolution::touch( int i0, int i1, bool undef )
{
   if ( !i0%spacing || !i1%spacing ) 
       return;

   int reli0 = (i0-start0)/spacing;
   int reli1 = (i1-start1)/spacing;
   if ( reli0<-1||reli0<-1||reli0>sidesize0||reli1>sidesize1 )
       return;

   if ( undef && reli0>=0 && reli0<sidesize0 && reli1>=0 && reli1<sidesize1 )
       cachestatus = 2;
   else if ( cachestatus<1 ) cachestatus=1;
}


bool MeshSurfacePartResolution::canDisplay( bool ownvalidation) const
{
    if ( ownvalidation ) return cachestatus!=2;
    else return cache && cache->isValid();
}


bool MeshSurfacePartResolution::needsUpdate( bool ownvalidation) const
{
    if ( ownvalidation ) return cachestatus;
    else return cache && !cache->isValid();
}


#define mGetNormalIndex( i, j ) ( (i)*normalsperrow+(j) )


bool MeshSurfacePartResolution::getNormal( int relrow,
			int relcol, bool useownvalidation, SbVec3f& res )
{
    if ( cache && useownvalidation && cachestatus!=2 )
    {
	const int normalsperrow = nrCols();
	const int normalindex = mGetNormalIndex(relrow,relcol);
	res = cache->normals[normalindex];
	return true;
    }

    if ( computeNormal( relrow, relcol, &res ) )
    {
	if ( cache && useownvalidation )
		;
	    //Update normal in cache if it there is a cahce

	return true;
    }

    return false;
}


void MeshSurfacePartResolution::setStart( int row, int col )
{
    start0 = row; start1=col;
}


void MeshSurfacePartResolution::GLRender( SoGLRenderAction* action,
					  bool useownvalidation )
{
    if ( cache && useownvalidation )
    {
	if ( cachestatus==2 )
	    tesselate();
	cache->GLRender(action);
    }
    else
    {
	if ( !cache || !cache->isValid() )
	    tesselate();

	cache->GLRender(action);
    }
}

#define mAppend(ci_,ni_, extra) \
{ \
    cache->ci.append(ci_); \
    cache->ni.append(ni_); \
    extra; \
}


#define mAppendTriangle( idx0, nidx0, idx1, nidx1, idx2, nidx2 ) \
mAppend( idx0, nidx0, ); \
mAppend( idx1, nidx1, ); \
mAppend( idx2, nidx2, lastci = idx2 ); 

#define mAppendSquare( idx0, nidx0, idx1, nidx1, idx2, nidx2, idx3, nidx3 ) \
{ \
    mAppend( idx0, nidx0, ); \
    mAppend( idx1, nidx1, ); \
    mAppend( idx2, nidx2, );  \
    mAppend( idx3, nidx3, lastci = idx3 );  \
}


#define mEndStrip \
{ \
    if ( lastci!=-1 ) \
	mAppend(-1,-1,lastci=-1); \
}

void MeshSurfacePartResolution::invalidateCaches()
{ if ( cache ) cache->setValid(false); }


void MeshSurfacePartResolution::tesselate()
{
    if ( !cache )
	cache = new MeshSurfaceTesselationCache( meshsurface, true );
    else
	cache->reset(true);

    cache->buildmutex->writeLock();

    cachestatus = 0;

    int lastci = -1;
    int idx00,idx10,nidx00,nidx10;
    bool atnextrow = true;

    const int nrrows = nrRows();
    const int nrcols = nrCols();

    for ( int idx0=0; idx0<nrrows-1; idx0++ )
    {
	for ( int idx1=0; idx1<nrcols-1; idx1++ )
	{
	    computeNormal( idx0, idx1 );

	    if ( lastci==-1 )
	    {
		startNewStrip( idx0, idx1,
			       idx00, nidx00, idx10, nidx10, lastci, atnextrow);
		continue;
	    }

	    expandStrip( idx0,idx1,idx00,nidx00,idx10,nidx10,lastci,atnextrow );
	}

	mEndStrip;
	computeNormal( idx0, nrcols-1 );
    }

    for ( int idx1=0; idx1<nrcols; idx1++ )
	computeNormal( nrrows-1, idx1 );

    cache->setValid(true);
    cache->buildmutex->writeUnlock();
}


int MeshSurfacePartResolution::getCoordIndex( int i0, int i1 ) const
{
    const int ci = meshsurface.getIndex( start0+i0*spacing,
					 start1+i1*spacing );
    if ( ci==-1 )
	return -1;

    return mIsCoordDefined(meshsurface.coordinates[ci]) ? ci : -1;
}


int MeshSurfacePartResolution::getFillType( int i0, int i1 ) const
{
    const int ci = meshsurface.getIndex( start0+i0*spacing,
					 start1+i1*spacing );
    return ci>= meshsurface.meshStyle.getNum() ? 0 : meshsurface.meshStyle[ci];
}


void MeshSurfacePartResolution::startNewStrip(
					int idx, int jdx, 
					int& idx00, int& nidx00,
					int& idx10, int& nidx10,
					int& lastci, bool& atnextrow)
{
    const char filltype = getFillType( idx, jdx );
    if ( filltype==7 )
    {
	mEndStrip;
	return;
    }

    const int normalsperrow = nrCols();
    idx00 = getCoordIndex(idx,jdx);
    nidx00 = mGetNormalIndex( idx, jdx );

    idx10 = getCoordIndex(idx+1,jdx);
    nidx10 = mGetNormalIndex( idx+1, jdx );

    const int idx01 = getCoordIndex(idx,jdx+1);
    const int nidx01 = mGetNormalIndex( idx, jdx+1 );

    const int idx11 = getCoordIndex(idx+1,jdx+1);
    const int nidx11 = (idx+1)*normalsperrow+jdx+1;

    if ( (idx00<0)+(idx10<0)+(idx01<0)+(idx11<0) > 1 )
    {
	mEndStrip;
	return;
    }

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0 && filltype<3 )
    {
	bool diagonal00to11 = filltype==0
	    ? getBestDiagonal(idx00,idx10,idx01,idx11,atnextrow)
	    : filltype%2;

	if ( diagonal00to11 )
	    mAppendSquare( idx10, nidx10, idx00, nidx00,
		    	   idx11, nidx11, idx01, nidx01 )
	else
	{
	    mAppendTriangle( idx00, nidx00, idx01, nidx01,
		    	    idx10, nidx10 );
	    mEndStrip;
	    mAppendTriangle( idx10, nidx10, idx01, nidx01,
		    	     idx11, nidx11 );
	}

	atnextrow = !diagonal00to11;

	idx00 = idx01;
	idx10 = idx11;
	nidx00 = nidx01;
	nidx10 = nidx11;

	return;
    }

    if ( idx00<0 && (filltype<3||filltype==4) )
    {
	mAppendTriangle( idx10, nidx10, idx01, nidx01, idx11, nidx11 );
	atnextrow = true;
    }
    else if ( idx10<0 && (filltype<3||filltype==5))
    {
	mAppendTriangle( idx00, nidx00, idx01, nidx01, idx11, nidx11 );
	atnextrow = true;
    }
    else if ( idx01<0 && filltype<4)
    {
	mAppendTriangle( idx00, nidx00, idx10, nidx10, idx11, nidx11 );
	atnextrow = true;
    }
    else if ( idx11<0 && (filltype<3||filltype==6))
    {
	mAppendTriangle( idx00, nidx00, idx10, nidx10, idx01, nidx01 );
	atnextrow = false;
	mEndStrip;
    }

    idx00 = idx01;
    idx10 = idx11;
    nidx00 = nidx01;
    nidx10 = nidx11;
}


void MeshSurfacePartResolution::expandStrip( int idx, int jdx, 
		      int& idx00, int& nidx00,
		      int& idx10, int& nidx10, int& lastci, bool& atnextrow)
{
    const char filltype = getFillType( idx, jdx );
    if ( filltype==7 )
    {
	mEndStrip;
	return;
    }

    const int normalsperrow = nrCols();
    const int idx01 = getCoordIndex(idx,jdx+1);
    const int nidx01 = mGetNormalIndex(idx,jdx+1);

    const int idx11 = getCoordIndex(idx+1,jdx+1);
    const int nidx11 = mGetNormalIndex(idx+1,jdx+1);

    if ( idx01<0 && idx11<0 )
    {
	mEndStrip;
	return;
    }

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0  && filltype<3)
    {
	bool diagonal00to11 = filltype==0
	    ? getBestDiagonal(idx00,idx10,idx01,idx11,atnextrow)
	    : filltype%2;

	if ( atnextrow && !diagonal00to11 )
	{
	    mAppend( idx01, nidx01, );
	    mAppend( idx11, nidx11, lastci=idx11 );
	}
	else if ( !atnextrow && diagonal00to11 )
	{
	    mAppend( idx11, nidx11, );
	    mAppend( idx01, nidx01, lastci=idx01 );
	}
	else
	{
	    mEndStrip;

	    if ( diagonal00to11 )
	    {
		mAppendSquare( idx10, nidx10, idx00, nidx00,
				idx11, nidx11, idx01, nidx01);
	    }
	    else
	    {
		mAppendTriangle( idx00, nidx00, idx01, nidx01,
				 idx10, nidx10 );
		mEndStrip;
		mAppendTriangle( idx10, nidx10, idx01, nidx01,
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

    if ( filltype==4 || filltype==5 )
	mEndStrip;


    if ( idx11>=0 )
    {
	if ( atnextrow )
	{
	    mEndStrip;

	    if ( idx00>=0 && idx10>=0 )
	    {
		mAppendTriangle( idx10, nidx10, idx00, nidx00, idx11, nidx11 );
	    }
	}
	else
	{
	    mAppend( idx11, nidx11, );
	    mEndStrip;
	}
    }
    else if ( idx01>=0 )
    {
	if ( !atnextrow )
	{
	    mEndStrip;

	    if ( idx00>=0 && idx10>=0 )
	    {
		mAppendTriangle(idx01, nidx01, idx00, nidx00, idx10, nidx10 );
	    }
	}
	else
	{
	    mAppend( idx01, nidx01,);
	    mEndStrip;
	}
    }

    idx00 = idx01;
    idx10 = idx11;
    nidx00 = nidx01;
    nidx10 = nidx11;
}


bool MeshSurfacePartResolution::getBestDiagonal( 
    int idx00, int idx10, int idx01, int idx11, bool atnextrow ) const
{
#define mEPS 1e-10
    const SbVec3f* coords = meshsurface.coordinates.getValues(0);
    const SbVec3f p00 = coords[idx00];
    const SbVec3f p10 = coords[idx10];
    const SbVec3f p01 = coords[idx01];
    const SbVec3f p11 = coords[idx11];

    SbVec3f vec0 = p00 - p11;
    SbVec3f vec1 = p10 - p01;
    float len0 = vec0.sqrLength();
    float len1 = vec1.sqrLength();

    if ( atnextrow )
	return ( (len0+mEPS) <= len1 );
    
    return (len0-mEPS) < len1;
}


int MeshSurfacePartResolution::nrRows() const
{
    const int totalnrrows = meshsurface.nrRows();
    const int localnrrows = (totalnrrows-start0)/spacing;
    return localnrrows<sidesize0?localnrrows:sidesize0;
}


int MeshSurfacePartResolution::nrCols() const
{
    const int totalnrcols = meshsurface.nrColumns.getValue();
    const int localnrcols = (totalnrcols-start1)/spacing;
    return localnrcols<sidesize1?localnrcols:sidesize1;
}


/*void SoMeshSurfaceBrick::invalidateNormal( int index )
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

*/


bool MeshSurfacePartResolution::computeNormal(
	int rel0, int rel1, SbVec3f* res )
{
    if ( !cache && !res ) return false;
    const int prevrowindex = getCoordIndex( rel0-1, rel1 );
    const int nextrowindex = getCoordIndex( rel0+1, rel1 );
    const int prevcolindex = getCoordIndex( rel0, rel1-1 );
    const int nextcolindex = getCoordIndex( rel0, rel1+1 );
    const int coordindex = getCoordIndex( rel0, rel1 );

    const SbVec3f* celem = meshsurface.coordinates.getValues(0);

    SbVec3f v0;
    if ( prevcolindex==-1 || nextcolindex==-1 )
    {
	if ( prevcolindex==-1 && nextcolindex==-1 )
	    return false;

	if ( coordindex==-1 )
	    return false;

	if ( prevcolindex==-1 )
	    v0 = celem[nextcolindex]-celem[coordindex];
	else 
	    v0 = celem[coordindex]-celem[prevcolindex];
    }
    else
	v0 = celem[nextcolindex]-celem[prevcolindex];

    if ( !v0.length() )
	return false;

    SbVec3f v1;
    if ( prevrowindex==-1 || nextrowindex==-1 )
    {
	if ( prevrowindex==-1 && nextrowindex==-1 )
	    return false;

	if ( coordindex==-1 )
	    return false;

	if ( prevrowindex==-1 )
	    v1 = celem[nextrowindex]-celem[coordindex];
	else 
	    v1 = celem[coordindex]-celem[prevrowindex];
    }
    else
	v1 = celem[nextrowindex]-celem[prevrowindex];

    if ( !v1.length() )
	return false;

    const SbVec3f normal = v1.cross(v0);
    if ( cache )
    {
	const int normalsperrow = nrCols();
	const int normalindex = mGetNormalIndex(rel0,rel1);
	while ( cache->normals.getLength()<=normalindex )
	    cache->normals.push( SbVec3f(0,0,1) );

	cache->normals[normalindex] = normal;
    }

    if ( res ) *res = normal;

    return true;
}


SO_NODE_SOURCE(SoKrisSurface);
void SoKrisSurface::initClass()
{
    SO_NODE_INIT_CLASS(SoKrisSurface, SoShape, "KrisSurface" );
}


SoKrisSurface::SoKrisSurface()
    : sidesize( 64 )
    , useownvalidation( false )
    , nrcolparts( 0 )
{
    SO_NODE_CONSTRUCTOR(SoKrisSurface);
    SO_NODE_ADD_FIELD( meshStyle, (0) );
    SO_NODE_ADD_FIELD( brickSize, (6) );
    SO_NODE_ADD_FIELD( resolution, (-1) );
    SO_NODE_ADD_FIELD( wireframe, (false) );
    SO_NODE_ADD_FIELD( nrColumns, (1) );
}


void SoKrisSurface::insertColumns(bool before)
{
    const bool oldvalidationflag = useownvalidation;
    useownvalidation = true;
    const int nrrows = nrRows();
    const int nrcols = nrColumns.getValue();

    if ( coordinates.getNum()<nrcols*nrrows )
	coordinates.insertSpace( coordinates.getNum(),
				nrcols*nrrows-coordinates.getNum() );

    if ( materialIndex.getNum()<nrcols*nrrows )
	materialIndex.insertSpace( materialIndex.getNum(),
				   nrcols*nrrows-materialIndex.getNum() );

    if ( meshStyle.getNum()<nrcols*nrrows )
	meshStyle.insertSpace( meshStyle.getNum(),
				nrcols*nrrows-meshStyle.getNum() );


    for ( int idx=nrrows-1; idx>=0; idx-- )
    {
	const int insertpos = (idx+(before?0:1))*nrcols;
	coordinates.insertSpace( insertpos, sidesize );
	materialIndex.insertSpace( insertpos, sidesize );
	meshStyle.insertSpace( insertpos, sidesize );
    }
}


void SoKrisSurface::insertRowsBefore()
{
    coordinates.insertSpace( 0, nrColumns.getValue()*sidesize );
    materialIndex.insertSpace( 0, nrColumns.getValue()*sidesize );
    meshStyle.insertSpace( 0, nrColumns.getValue()*sidesize );

    for ( int idx=0; idx<parts.getLength(); idx++ )
    {
	parts[idx]->setStart( parts[idx]->getRowStart()+sidesize,
			      parts[idx]->getColStart() );
    }
}


void SoKrisSurface::turnOnOwnValidation(bool yn)
{ useownvalidation=yn; }


void SoKrisSurface::computeBBox(SoAction* action, SbBox3f& bbox,
				SbVec3f& center )
{
    adjustNrOfParts();
    SoState* state = action->getState();
    center = SbVec3f(0,0,0);
    SbBox3f localbox;
    SbVec3f localcenter;

    int nrboxes = 0;
    for ( int idx=0; idx<parts.getLength(); idx++ )
    {
	parts[idx]->computeBBox(state,localbox,localcenter,useownvalidation);
	if ( localbox.isEmpty() )
	    continue;

	bbox.extendBy(localbox);
	center += localcenter;
	nrboxes++;
    }

    if ( nrboxes )
	center /= nrboxes;
}


void SoKrisSurface::rayPick(SoRayPickAction* action )
{
    for ( int idx=0; idx<parts.getLength(); idx++ )
	parts[idx]->rayPick(action,useownvalidation);
}


void SoKrisSurface::GLRender(SoGLRenderAction* action)
{
    SoState* state = action->getState();
    const int nrparts = parts.getLength();
    //if ( !(SoCameraInfoElement::get(state)&SoCameraInfo::STEREO) )
    //{
	const int whichres = resolution.getValue();
	for ( int idx=0; idx<nrparts; idx++ )
	{
	    MeshSurfacePart* part = parts[idx];
	    int missingresolution = -1;
	    if ( whichres==-1 )
	    {
		const int desres =
		     	part->computeResolution(state,useownvalidation);
		if ( !part->setResolution(desres,useownvalidation) )
		     missingresolution = desres;
	    }
	    else if ( !part->setResolution(whichres,useownvalidation) )
		missingresolution = whichres;

	    if ( missingresolution!=-1 )
	    {
		part->getResolution(missingresolution)->tesselate();
		part->setResolution(missingresolution,useownvalidation);
		 //SbThreadAutoLock quelock( creatorqueMutex );
		 //SoMeshSurfaceBrick* brick =part->getBrick(missingresolution);
#ifdef __win__
		 //brick->build(true);
#else
		 //const int existingindex = creationquebricks.find( brick );
		 //if ( existingindex!=-1 )
		 //{
		     //if ( existingindex!=creationquebricks.getLength()-1 )
		     //{
			 //creationquebricks[existingindex] = 0;
			 //creationquebricks.push( brick );
		     //}
		 //}
		 //else
		     //creationquebricks.push( brick );

		 //creationcondvar->wakeOne();
#endif
	    }
	}
    //}

    for ( int idx=0; idx<nrparts; idx++ )
	parts[idx]->GLRenderGlue(action, useownvalidation);

    for ( int idx=0; idx<nrparts; idx++ )
	parts[idx]->GLRender(action, useownvalidation);
}


void SoKrisSurface::getBoundingBox(SoGetBoundingBoxAction* action)
{
    SbBox3f box;
    SbVec3f center;
    computeBBox( action, box, center);
    if ( !box.isEmpty() )
    {
	action->extendBy(box);
	action->setCenter(center, true);
    }
}



int SoKrisSurface::getIndex( int i0, int i1 ) const
{
    if ( i1<0 || i1>=nrColumns.getValue() ) return -1;
    const int res = i0*nrColumns.getValue()+i1;
    return res>=0&&res<coordinates.getNum() ? res : -1;
}


int SoKrisSurface::nrRows() const
{
    int totalnrrows = coordinates.getNum()/nrColumns.getValue();
    if ( coordinates.getNum()%nrColumns.getValue() )
	totalnrrows++;
    return totalnrrows;
}


void SoKrisSurface::notify( SoNotList* nl )
{
    SoShape::notify(nl);
    if ( useownvalidation ) 
	return;

    for ( int idx=0; idx<parts.getLength(); idx++ )
	parts[idx]->invalidateCaches();
}


void SoKrisSurface::adjustNrOfParts()
{
    const int nrcolumns = nrColumns.getValue();
    while ( nrcolparts*sidesize<nrcolumns )
    {
	const int nrrowparts = nrRows()/sidesize+1;
	for ( int idx=0; idx<nrrowparts; idx++ )
	{
	    const int insertbefore = (nrcolparts+1)*(idx+1)-1;
	    MeshSurfacePart* newpart = new MeshSurfacePart(*this,
		    		nrcolparts*sidesize, idx*sidesize, sidesize );
	    if ( insertbefore>=parts.getLength() )
		parts.append( newpart );
	    else 
		parts.insert( newpart, insertbefore );

	    //TODO Set neighbors
	}

	nrcolparts++;
    }

    while ( (nrcolparts-1)*sidesize>nrcolumns )
    {
	const int nrrowparts = nrRows()/sidesize+1;
	nrcolparts--;
	for ( int idx=0; idx<nrrowparts; idx++ )
	{
	    const int indextoremove = (idx+1)*nrcolparts;
	    delete parts[indextoremove];
	    parts.remove(indextoremove);

	    //TODO Set neighbors, remove from tesselation queue.
	}
    }

    const int desnrpartrows = nrRows()/sidesize+1;

    while ( parts.getLength()/nrcolparts<desnrpartrows )
    {
	const int newrowidx = parts.getLength()/nrcolparts;
	for ( int idx=0; idx<nrcolparts; idx++ )
	{
	    MeshSurfacePart* newpart = new MeshSurfacePart(*this,
		    		idx*sidesize, newrowidx*sidesize, sidesize );
	    parts.append( newpart );
	    //TODO Set neighbors
	}
    }

    while ( parts.getLength()/nrcolparts>desnrpartrows )
    {
	for ( int idx=0; idx<nrcolparts; idx++ )
	{
	    delete parts[parts.getLength()-1];
	    parts.pop();
	    //TODO Set neighbors, remove from tesselation queue.
	}
    }
}
