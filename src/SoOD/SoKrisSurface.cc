/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoKrisSurface.cc,v 1.1 2004-10-02 12:30:25 kristofer Exp $";

#include "SoKrisSurfaceImpl.h"
#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"

#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/threads/SbRWMutex.h>

#ifdef mac
# include "OpenGL/gl.h"
#else
# include "GL/gl.h"
#endif

MeshSurfacePartPart::MeshSurfacePartPart(SoKrisSurface& m, int s0, int s1 )
    : meshsurface( m )
    , start0( s0 )
    , start1( s1 )
    , cache( 0 )
{}


MeshSurfacePartPart::~MeshSurfacePartPart()
{
    delete cache;
}

void MeshSurfacePartPart::touch( int i0, int i1, bool undef )
{
    if ( !isInside(i0,i1) )
	return;

    //cache->invalidize();
    ownvalidation = false;
}


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

    box.makeEmpty();
    center = SbVec3f(0,0,0);
    int ncoords = 0;

    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int sidesize = sideSize();
    for ( int idx=0; idx<sidesize; idx++ )
    {
	const int i = idx+start0;
	const int32_t* ptr = getCoordIndexPtr(i,start1);
	if ( !ptr ) break;

	int nci = sidesize;
	if ( start1+sidesize>=meshsurface.nrColumns.getValue() )
	    nci = meshsurface.nrColumns.getValue()-start1;

	for ( int jdx=0; jdx<nci; jdx++ )
	{
	    const int ci = *ptr++;
	    if ( ci==-1 ) continue;

	    box.extendBy(celem->get3(ci));
	    ncoords++;
	}
    }

    if ( !cache ) cache = new SoBoundingBoxCache(state);
    if ( ncoords ) center /= ncoords;
    cache->set( box, ncoords, center );
}

#define mFindClosestNode( idx0, idx1, idx2 ) \
int closestcoordidx = ci##idx0; \
int closestmatidx = mi##idx0; \
int closesti = i##idx0; \
int closestj = j##idx0; \
float shortestdist = (intersection-c##idx0).sqrLength(); \
float dist; \
if ( (dist = (intersection-c##idx1).sqrLength())<shortestdist ) \
{ \
    shortestdist=dist; \
    closestcoordidx = ci##idx1; \
    closestmatidx = mi##idx1; \
    closesti = i##idx1; \
    closestj = j##idx1; \
} \
if ( (dist = (intersection-c##idx2).sqrLength())<shortestdist ) \
{ \
    closestcoordidx = ci##idx2; \
    closestmatidx = mi##idx2; \
    closesti = i##idx2; \
    closestj = j##idx2; \
} \
\
SoPickedPoint* pickedpoint = action->addIntersection(intersection); \
SoFaceDetail* facedetail = new SoFaceDetail; \
facedetail->setFaceIndex(closesti); \
facedetail->setPartIndex(closestj); \
facedetail->setNumPoints(3); \
SoPointDetail pointdetail; \
pointdetail.setCoordinateIndex(ci##idx0); \
pointdetail.setMaterialIndex(mi##idx0); \
facedetail->setPoint(0,&pointdetail); \
pointdetail.setCoordinateIndex(ci##idx1); \
pointdetail.setMaterialIndex(mi##idx1); \
facedetail->setPoint(1,&pointdetail); \
pointdetail.setCoordinateIndex(ci##idx2); \
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

    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int sidesize = sideSize();
    for ( int idx=0; idx<sidesize-1; idx++ )
    {
	const int i = idx+start0;
	const int32_t* currowcoordptr = getCoordIndexPtr(i,start1);
	if ( !currowcoordptr ) break;
	const int32_t* nxtrowcoordptr = getCoordIndexPtr(i+1,start1);
	if ( !nxtrowcoordptr ) break;

	const int32_t* currowmatptr = getMatIndexPtr(i,start1);
	const int32_t* nxtrowmatptr = getMatIndexPtr(i+1,start1);

	int nci = sidesize;
	if ( start1+sidesize>=meshsurface.nrColumns.getValue() )
	    nci = meshsurface.nrColumns.getValue()-start1;

	int ci00 = *currowcoordptr++;
	int ci10 = *nxtrowcoordptr++;
	bool useci00 = ci00!=-1;
	bool useci10 = ci10!=-1;
	SbVec3f c00 = celem->get3(ci00);
	SbVec3f c10 = celem->get3(ci10);
	int mi00 = currowmatptr ? *currowmatptr++ : 1;
	int mi10 = nxtrowmatptr ? *nxtrowmatptr++ : 1;

	const int i00 = i;
	const int i10 = i+1;
	const int i01 = i;
	const int i11 = i+1;
	int j00 = start1;
	int j10 = start1;

	for ( int jdx=0; jdx<nci-1; jdx++ )
	{
	    const int ci01 = *currowcoordptr++;
	    const int ci11 = *nxtrowcoordptr++;

	    const SbVec3f c01 = celem->get3(ci01);
	    const SbVec3f c11 = celem->get3(ci11);

	    const bool useci01 = ci01!=-1;
	    const bool useci11 = ci11!=-1;

	    int mi01 = currowmatptr ? *currowmatptr++ : 1;
	    int mi11 = nxtrowmatptr ? *nxtrowmatptr++ : 1;

	    const int j01 = j00+1;
	    const int j11 = j10+1;

	    if ( useci00+useci10+useci01+useci11>2 )
	    {
		if ( useci00 && useci10 && useci01 && useci11 )
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
		else if ( !useci00 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c01,c11,c10,intersection,
					    barycentric,front))
		    {
			mFindClosestNode( 01, 11, 10 );
		    }
		}
		else if ( !useci01 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c11, c10, c00, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 11, 10, 00 );
		    }
		}
		else if ( !useci11 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c10, c00, c01, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 10, 00, 01 );
		    }
		}
		else if ( !useci10 )
		{
		    SbVec3f intersection, barycentric; SbBool front;
		    if ( action->intersect( c00, c01, c11, intersection,
					    barycentric, front ))
		    {
			mFindClosestNode( 00, 01, 11 );
		    }
		}
	    }

	    ci00=ci01; useci00=useci01; c00=c01; mi00=mi01; j00=j01;
	    ci10=ci11; useci10 = useci11; c10=c11; mi10=mi11; j10=j11;
	}
    }
}


bool MeshSurfacePartPart::isInside( int i, int j ) const
{
    const int sidesize = sideSize();
    return i>=start0&&j>=start1&&(i-start0)>sidesize&&(j-start1)<sidesize;
}


const int32_t* MeshSurfacePartPart::getCoordIndexPtr(int i0,int i1) const
{
    const int idx = meshsurface.getCoordIndexIndex(i0,i1);
    if ( idx==-1 ) return 0;
    return meshsurface.coordIndex.getValues(idx);
}


const int32_t* MeshSurfacePartPart::getMatIndexPtr(int i0,int i1) const
{
    const int idx = meshsurface.getCoordIndexIndex(i0,i1);
    if ( idx==-1 ) return 0;
    return meshsurface.materialIndex.getValues(idx);
}


MeshSurfaceTesselationCache::MeshSurfaceTesselationCache( SoState* state,
						      const SoKrisSurface& m)
    : meshsurface(m)
    , buildmutex( new SbRWMutex( SbRWMutex::READ_PRECEDENCE) )
    , SoCache( state )
{
    addElement(SoCoordinateElement::getInstance(state));
}


MeshSurfaceTesselationCache::~MeshSurfaceTesselationCache()
{
    delete buildmutex;
}


void MeshSurfaceTesselationCache::call(SoGLRenderAction* action)
{
    SoMaterialBundle mb(action);
    SoState* state = action->getState();
    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);
    const int32_t* ciptr = meshsurface.coordIndex.getValues(0);
    const int32_t* miptr = meshsurface.materialIndex.getValues(0);

    mb.sendFirst();

    buildmutex->readLock();
    const int nrcii = cii.getLength();
    bool isopen = false;
    for ( int idx=0; idx<nrcii; idx++ )
    {
	const int index = cii[idx];
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
	    glBegin( GL_TRIANGLE_STRIP );
	    isopen = true;
	}

	mb.send(miptr?miptr[index]:0,true);
	glNormal3fv(normals[ni[idx]].getValue());
	glVertex3fv(celem->get3(ciptr[index]).getValue());
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
}


MeshSurfacePart::~MeshSurfacePart()
{
    //Notify neighbors about removal?
    for ( int idx=0; idx<resolutions.getLength(); idx++ )
	delete resolutions[idx];

    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	delete bboxes[idx];
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

    if ( !bboxcache ) bboxcache = new SoBoundingBoxCache(state);
    if ( nrboxes ) center /= nrboxes;
    bboxcache->set( bbox, nrboxes, center );
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


bool MeshSurfacePart::setResolution( SoState* state, int desiredres,
				     bool useownvalidation )
{
    for ( int idx=desiredres; idx>0; idx-- )
    {
	bool candisplay = resolutions[idx]->canDisplay(state,useownvalidation);
	if ( candisplay )
	{
	    if ( idx!=resolution )
	    {
		reshaschanged = true;
		resolution = idx;
	    }

	    return  idx==desiredres &&
		    !resolutions[idx]->needsUpdate(state,useownvalidation);
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
{ delete cache; }


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


bool MeshSurfacePartResolution::canDisplay( SoState* state,
					    bool ownvalidation) const
{
    if ( ownvalidation ) return cachestatus!=2;
    else return cache && cache->isValid(state);
}


bool MeshSurfacePartResolution::needsUpdate( SoState* state,
					     bool ownvalidation) const
{
    if ( ownvalidation ) return cachestatus;
    else return cache && !cache->isValid(state);
}


void MeshSurfacePartResolution::GLRender( SoGLRenderAction* action,
					  bool overridetessel )
{
    SoState* state = action->getState();
    if ( cache && overridetessel )
    {
	if ( cachestatus==2 )
	    tesselate(state);
	cache->call(action);
    }
    else
    {
	if ( !cache || !cache->isValid(state) )
	    tesselate(state);

	cache->call(action);
    }
}

#define mAppend(cii_,ni_, extra) \
{ \
    cache->cii.append(cii_); \
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

void MeshSurfacePartResolution::tesselate(SoState* state)
{
    if ( !cache ) cache = new MeshSurfaceTesselationCache(state,meshsurface);
    cache->buildmutex->writeLock();

    cache->cii.truncate(0, false);
    cache->ni.truncate(0, false);

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
	    computeNormal( state, idx0, idx1 );

	    if ( lastci==-1 )
	    {
		startNewStrip( state, idx0, idx1,
			       idx00, nidx00, idx10, nidx10, lastci, atnextrow);
		continue;
	    }

	    expandStrip( state,
		    	 idx0,idx1,idx00,nidx00,idx10,nidx10,lastci,atnextrow );
	}

	mEndStrip;
	computeNormal( state, idx0, nrcols-1 );
    }

    for ( int idx1=0; idx1<nrcols; idx1++ )
	computeNormal( state, nrrows-1, idx1 );

    cache->cii.fit();
    cache->ni.fit();

    cache->buildmutex->writeUnlock();
}


int MeshSurfacePartResolution::getCoordIndexIndex( int i0, int i1 ) const
{
    const int cii = meshsurface.getCoordIndexIndex( start0+i0*spacing,
	    					    start1+i1*spacing );
    if ( cii==-1 )
	return -1;

    return meshsurface.coordIndex[cii]==-1 ? -1 : cii;
}


int MeshSurfacePartResolution::getFillType( int i0, int i1 ) const
{
    const int cii = meshsurface.getCoordIndexIndex( start0+i0*spacing,
	    					    start1+i1*spacing );
    return cii>= meshsurface.meshStyle.getNum()
	? 0 : meshsurface.meshStyle[cii];
}


#define mGetNormalIndex( i, j ) ( (i)*normalsperrow+(j) )

void MeshSurfacePartResolution::startNewStrip( SoState* state,
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
    idx00 = getCoordIndexIndex(idx,jdx);
    nidx00 = mGetNormalIndex( idx, jdx );

    idx10 = getCoordIndexIndex(idx+1,jdx);
    nidx10 = mGetNormalIndex( idx+1, jdx );

    const int idx01 = getCoordIndexIndex(idx,jdx+1);
    const int nidx01 = mGetNormalIndex( idx, jdx+1 );

    const int idx11 = getCoordIndexIndex(idx+1,jdx+1);
    const int nidx11 = (idx+1)*normalsperrow+jdx+1;

    if ( (idx00<0)+(idx10<0)+(idx01<0)+(idx11<0) > 1 )
    {
	mEndStrip;
	return;
    }

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0 && filltype<3 )
    {
	bool diagonal00to11 = filltype==0
	    ? getBestDiagonal(state,idx00,idx10,idx01,idx11,atnextrow)
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


void MeshSurfacePartResolution::expandStrip( SoState* state, int idx, int jdx, 
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
    const int idx01 = getCoordIndexIndex(idx,jdx+1);
    const int nidx01 = mGetNormalIndex(idx,jdx+1);

    const int idx11 = getCoordIndexIndex(idx+1,jdx+1);
    const int nidx11 = mGetNormalIndex(idx+1,jdx+1);

    if ( idx01<0 && idx11<0 )
    {
	mEndStrip;
	return;
    }

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0  && filltype<3)
    {
	bool diagonal00to11 = filltype==0
	    ? getBestDiagonal(state,idx00,idx10,idx01,idx11,atnextrow)
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


bool MeshSurfacePartResolution::getBestDiagonal( SoState* state,
    int idx00, int idx10, int idx01, int idx11, bool atnextrow ) const
{
#define mEPS 1e-10
    const int32_t* cis = meshsurface.coordIndex.getValues(0);
    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);

    const SbVec3f p00 = celem->get3(cis[idx00]);
    const SbVec3f p10 = celem->get3(cis[idx10]);
    const SbVec3f p01 = celem->get3(cis[idx01]);
    const SbVec3f p11 = celem->get3(cis[idx11]);

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


bool MeshSurfacePartResolution::computeNormal( SoState* state,
	int rel0, int rel1 )
{
    const int prevrowindex = getCoordIndexIndex( rel0-1, rel1 );
    const int nextrowindex = getCoordIndexIndex( rel0+1, rel1 );
    const int prevcolindex = getCoordIndexIndex( rel0, rel1-1 );
    const int nextcolindex = getCoordIndexIndex( rel0, rel1+1 );
    const int coordindex = getCoordIndexIndex( rel0, rel1 );
    const SoCoordinateElement* celem = SoCoordinateElement::getInstance(state);

    SbVec3f v0;
    if ( prevcolindex==-1 || nextcolindex==-1 )
    {
	if ( prevcolindex==-1 && nextcolindex==-1 )
	    return false;

	if ( coordindex==-1 )
	    return false;

	if ( prevcolindex==-1 )
	    v0 = celem->get3(nextcolindex)-celem->get3(coordindex);
	else 
	    v0 = celem->get3(coordindex)-celem->get3(prevcolindex);
    }
    else
	v0 = celem->get3(nextcolindex)-celem->get3(prevcolindex);

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
	    v1 = celem->get3(nextrowindex)-celem->get3(coordindex);
	else 
	    v1 = celem->get3(coordindex)-celem->get3(prevrowindex);
    }
    else
	v1 = celem->get3(nextrowindex)-celem->get3(prevrowindex);

    if ( !v1.length() )
	return false;

    const int normalsperrow = nrCols();
    const int normalindex = mGetNormalIndex(rel0,rel1);
    if ( !cache ) cache = new MeshSurfaceTesselationCache(state,meshsurface);
    while ( cache->normals.getLength()<=normalindex )
	cache->normals.push( SbVec3f(0,0,1) );

    cache->normals[normalindex] = v1.cross(v0);

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
		if ( !part->setResolution(state,desres,useownvalidation) )
		     missingresolution = desres;
	    }
	    else if ( !part->setResolution(state,whichres,useownvalidation) )
		missingresolution = whichres;

	    if ( missingresolution!=-1 )
	    {
		part->getResolution(missingresolution)->tesselate(state);
		part->setResolution(state,missingresolution,useownvalidation);
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
	parts[idx]->updateGlue();

    for ( int idx=0; idx<nrparts; idx++ )
	parts[idx]->GLRender(action, useownvalidation);
}


int SoKrisSurface::getCoordIndexIndex( int i0, int i1 ) const
{
    if ( i1<0 || i1>=nrColumns.getValue() ) return -1;
    const int res = i0*nrColumns.getValue()+i1;
    return res>=0&&res<coordIndex.getNum() ? res : -1;
}


int SoKrisSurface::nrRows() const
{
    int totalnrrows = coordIndex.getNum()/nrColumns.getValue();
    if ( coordIndex.getNum()%nrColumns.getValue() )
	totalnrrows++;
    return totalnrrows;
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
