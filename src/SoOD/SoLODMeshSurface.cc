/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoLODMeshSurface.cc,v 1.31 2009-07-22 16:01:35 cvsbert Exp $";

#include "SoLODMeshSurface.h"

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
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/threads/SbRWMutex.h>
#include <Inventor/system/gl.h>

#include <Inventor/errors/SoDebugError.h>

#define mIsCoordDefined( coord ) (coord[0]<1e29)


namespace MeshSurfImpl
{

class MeshSurfacePartResolution;
class MeshSurfaceIndexChanger
{
public:
    			MeshSurfaceIndexChanger( int prevnrcols, int newnrcols,
						 bool changebeforeexisting,
						 int nraddedrows );
    int			convertIndex( int oldindex ) const;

protected:
    int			prevnrcols;
    int			newnrcols;
    bool		changebeforeexisting;

    int			nraddedrows;
};



class MeshSurfacePartPart
{
public:
    				MeshSurfacePartPart(SoLODMeshSurface&,int,int);
    				~MeshSurfacePartPart();
    void			setStart( int row, int col );
    int				getStart( bool row ) const;
    void			touch( int, int, bool undef );
    void			invalidateCaches();
    void			rayPick(SoRayPickAction*, bool );
    void			computeBBox(SoState*, SbBox3f&, SbVec3f&,
	    				    bool useownvalidation );

    static int			sideSize() { return 16; }

protected:
    bool			isInside( int, int ) const;
    const int32_t*		getMatIndexPtr(int,int) const;

    const SoLODMeshSurface&	meshsurface;
    int				rowstart, colstart;

    SoBoundingBoxCache*		cache;
    bool			ownvalidation;
};


class MeshSurfaceTesselationCache
{
public:
    enum 			Primitive { TriangleStrip, TriangleFan };

    				MeshSurfaceTesselationCache(
					const SoLODMeshSurface&, Primitive );
    				~MeshSurfaceTesselationCache();

    void			reset(bool all);
    void			changeCacheIdx(const MeshSurfaceIndexChanger&);

    void			GLRenderSurface(SoGLRenderAction*);
    void			GLRenderLines(SoGLRenderAction*);
    bool			isValid() const { return isvalid; }
    void			setValid(bool yn=true) { isvalid=yn; }

    SbList<int>			triangleci;
    SbList<int>			triangleni;
    SbList<int>			lineci;
    SbList<int>			lineni;

    SbList<SbVec3f>		normals;
    SbRWMutex*			buildmutex;

protected:
    const SoLODMeshSurface&	meshsurface;
    Primitive			primitive;

    bool			isvalid;
};


class MeshSurfacePart
{
public:
    		MeshSurfacePart( SoLODMeshSurface&, int start0, int start1,
				 int sidesize );
    		~MeshSurfacePart();
    void	setStart( int row, int col );
    void	changeCacheIdx(const MeshSurfaceIndexChanger&);
    int		getRowStart() const { return start0; }
    int		getColStart() const { return start1; }
    void	touch( int, int, bool undef );
    void	computeBBox(SoState*, SbBox3f&, SbVec3f&,bool useownvalidation);
    bool	cullTest(SoState*, bool useownvalidation);
    void	rayPick( SoRayPickAction*, bool useownvalidation);
    void	GLRenderSurface(SoGLRenderAction*,bool useownvalidation);
    void	GLRenderWireframe(SoGLRenderAction*,bool useownvalidation);
    void	GLRenderGlue(SoGLRenderAction*,bool useownvalidation);

    void	invalidateCaches();

    int		computeResolution( SoState*, bool useownvalidatoin );
    bool	setResolution( int desiredres, bool useownvalidatoin);
    bool	hasResChangedSinceLastRender() const { return reshaschanged; }
    void	resetChangeResFlag() { reshaschanged=false; }

    void	setNeighbor( int, MeshSurfacePart*, bool callback=false );

    int		nrResolutions() const { return resolutions.getLength(); }
    int		getResolution() const { return resolution; }
    MeshSurfacePartResolution*	getResolution(int i)
    				{ return i>= 0 ? resolutions[i] : 0; }

protected:
    int 	getSpacing( int res ) const;
    int		nrRows() const;
    int		nrCols() const;

    SbVec3f	getNormal( int, int, int, bool );
    SbBool	getNormal( int, int, int, bool, SbVec3f& );
    void	addGlueFan( const SbList<int>&,
			    const SbList<SbVec3f>&,
			    const SbList<int>&,
			    const SbList<SbVec3f>&,
			    SbBool dir );

    int					start0, start1;
    int					sidesize;
    int					resolution;
    bool				reshaschanged;
    SbList<MeshSurfacePartResolution*>	resolutions;
    SbList<MeshSurfacePartPart*>	bboxes;
    SbList<MeshSurfacePart*>		neighbors;
    SoLODMeshSurface&			meshsurface;
    SoBoundingBoxCache*			bboxcache;
    bool				bboxvalidation;

    MeshSurfaceTesselationCache*	gluecache;
    bool				gluevalidation;
};


class MeshSurfacePartResolution 
{
public:
		MeshSurfacePartResolution( SoLODMeshSurface&,
			    int s0, int s1, int ssz0, int ssz1, int spacing);
    		~MeshSurfacePartResolution();
    void	setStart( int row, int col );
    void	changeCacheIdx(const MeshSurfaceIndexChanger&);
    void	GLRenderSurface(SoGLRenderAction*,bool overridetessel);
    void	GLRenderWireframe(SoGLRenderAction*,bool overridetessel);
    void	touch( int, int, bool undef );
    bool	canDisplay(bool ownvalidation) const;
    bool	needsUpdate(bool ownvalidation) const;
    bool	getNormal( int row, int col,
	    		   bool useownvalidation, SbVec3f& );

    int		getSpacing() const { return spacing; }

    void	tesselate();
    void	invalidateCaches();
protected:
    void	startNewStrip( int, int, int&, int&,
					 int&, int&, int&, bool&);
    void	expandStrip( int, int, int&, int&,
	    			       int&, int&, int&, bool&);
    bool	computeNormal( int, int, SbVec3f* =0 );
    bool	getBestDiagonal( int, int, int, int, bool ) const; 
    int		nrCols() const;
    int		nrRows() const;
    int		getCoordIndex( int, int ) const;
    int		getFillType(int,int) const;

    int		start0, start1;
    int		spacing;
    int		sidesize0;
    int		sidesize1;

    int		cachestatus;
    		//0=OK, 1=need retesselation, 2=invalid

    MeshSurfaceTesselationCache*	cache;
    const SoLODMeshSurface&		meshsurface;
};


MeshSurfaceIndexChanger::MeshSurfaceIndexChanger( int oc, int nc, bool bf,int r)
    : prevnrcols( oc )
    , newnrcols( nc )
    , changebeforeexisting( bf )
    , nraddedrows( r )
{}


int MeshSurfaceIndexChanger::convertIndex( int oldindex ) const
{
    if ( oldindex==-1 ) return oldindex;

    int row = oldindex/prevnrcols;
    int col = oldindex%prevnrcols;
    
    if ( changebeforeexisting ) col += (newnrcols-prevnrcols);
    row += nraddedrows;

    return row*newnrcols+col;
}


MeshSurfacePartPart::MeshSurfacePartPart(SoLODMeshSurface& m, int s0, int s1 )
    : meshsurface( m )
    , rowstart( s0 )
    , colstart( s1 )
    , cache( 0 )
    , ownvalidation( false )
{ }


MeshSurfacePartPart::~MeshSurfacePartPart()
{
    if ( cache ) cache->unref();
}


void MeshSurfacePartPart::setStart( int row, int col )
{
    rowstart = row; colstart=col; 
}


int MeshSurfacePartPart::getStart( bool row ) const
{ return row ? rowstart : colstart; }


void MeshSurfacePartPart::touch( int i0, int i1, bool undef )
{
    if ( !isInside(i0,i1) )
	return;

    if ( cache ) cache->invalidate();
    ownvalidation = false;
}


void MeshSurfacePartPart::invalidateCaches()
{ if ( cache ) cache->invalidate(); }


void MeshSurfacePartPart::computeBBox( SoState* state, SbBox3f& box,
				    SbVec3f& center, bool useownvalidation )
{
    if ( cache && (useownvalidation ? ownvalidation : cache->isValid(state)) )
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
    for ( int rowidx=0; rowidx<=sidesize; rowidx++ )
    {
	const int row = rowidx+rowstart;
	if ( row>=nrrows )
	    break;

	int nrcolsonpart = sidesize+1;
	if ( colstart+nrcolsonpart>=nrcols )
	    nrcolsonpart = nrcols-colstart;

	const int colstartidx = row*nrcols+colstart;
	if ( colstartidx+nrcolsonpart>=meshsurface.coordinates.getNum() )
	    nrcolsonpart = meshsurface.coordinates.getNum()-colstartidx;

	if ( nrcolsonpart<0 )
	    continue;

	const SbVec3f* coordptr =
	    meshsurface.coordinates.getValues(colstartidx);
	for ( int colidx=0; colidx<nrcolsonpart; colidx++ )
	{
	    const SbVec3f& coord = coordptr[colidx];
	    if ( mIsCoordDefined( coord ) )
	    {
		box.extendBy(coord);
		ncoords++;
	    }
	}
    }

    if ( ncoords ) center /= ncoords;
    cache->set( box, ncoords, center );
    state->pop();
    SoCacheElement::setInvalid(storedinvalid);
    ownvalidation = true;
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
if ( pickedpoint ) \
{ \
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
    pickedpoint->setDetail(facedetail, const_cast<SoLODMeshSurface*>(&meshsurface)); \
}

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
    const int nc = meshsurface.coordinates.getNum();
    const SbVec3f* lastptr = meshsurface.coordinates.getValues(0) + nc-1;
	    			
    for ( int rowidx=0; rowidx<sidesize; rowidx++ )
    {
	const int row = rowidx+rowstart;
	const int nextrow = row+1;
	if ( nextrow>=nrrows )
	    return;

	const int currowstart = row*nrcols+colstart;
	if ( currowstart>=nc )
	    return;

	const int nxtrowstart = nextrow*nrcols+colstart;
	if ( nxtrowstart>=nc )
	    return;

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
	    if ( nxtrowcoordptr>lastptr )
		break;

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
    return i>=rowstart && j>=colstart &&
	   (i-rowstart)<=sidesize && (j-colstart)<=sidesize;
}


const int32_t* MeshSurfacePartPart::getMatIndexPtr(int i0,int i1) const
{
    const int idx = meshsurface.getIndex(i0,i1);
    if ( idx==-1 ) return 0;
    return meshsurface.materialIndex.getValues(idx);
}


MeshSurfaceTesselationCache::MeshSurfaceTesselationCache( 
	  const SoLODMeshSurface& m,
	  MeshSurfaceTesselationCache::Primitive primitivearg )
    : meshsurface(m)
    , primitive( primitivearg )
    , buildmutex( new SbRWMutex( SbRWMutex::READ_PRECEDENCE) )
    , isvalid( true )
{}


MeshSurfaceTesselationCache::~MeshSurfaceTesselationCache()
{ delete buildmutex; }


void MeshSurfaceTesselationCache::reset(bool all)
{
    buildmutex->writeLock();
    setValid(false);
    triangleni.truncate(0,false);
    triangleci.truncate(0,false);
    lineni.truncate(0,false);
    lineci.truncate(0,false);

    if ( all )
	normals.truncate(0,false);

    buildmutex->writeUnlock();
}


void MeshSurfaceTesselationCache::changeCacheIdx(
	const MeshSurfaceIndexChanger& ch )
{
    buildmutex->writeLock();
    int numci = triangleci.getLength();
    for ( int idx=0; idx<numci; idx++ )
	triangleci[idx] = ch.convertIndex(triangleci[idx]);

    numci = lineci.getLength();
    for ( int idx=0; idx<numci; idx++ )
	lineci[idx] = ch.convertIndex(lineci[idx]);

    buildmutex->writeUnlock();
}



void MeshSurfaceTesselationCache::GLRenderSurface(SoGLRenderAction* action)
{
    GLenum mode;
    if ( primitive==TriangleStrip )
	mode = GL_TRIANGLE_STRIP;
    else if ( primitive==TriangleFan )
	mode = GL_TRIANGLE_FAN; 
    else
	return;

    SoMaterialBundle mb(action);
    const SbVec3f* cptr = meshsurface.coordinates.getValues(0);
    const int nc = meshsurface.coordinates.getNum();
    const int32_t* miptr = meshsurface.materialIndex.getValues(0);
    const int nmi = meshsurface.materialIndex.getNum();

    buildmutex->readLock();
    const int nrci = triangleci.getLength();
    bool isopen = false;
    float texturecoord[2];
    const int nrrows = meshsurface.nrRows();
    const int nrcols = meshsurface.nrColumns.getValue();
    for ( int idx=0; idx<nrci; idx++ )
    {
	const int index = triangleci[idx];
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
	    if ( idx>nrci-3 )
		break;

	    mb.sendFirst();
	    glBegin( mode );
	    isopen = true;
	}

	mb.send(miptr?(index<nmi?miptr[index]:miptr[nmi-1]):0,true);
	glNormal3fv(normals[triangleni[idx]].getValue());
#if __debug__
	if ( index>=nc )
	    SoDebugError::postWarning("MeshSurfaceTesselationCache::GLRender",
				      "Index is too large");
#endif
	const int row = index/nrcols;
	const int col = index%nrcols;
	texturecoord[0] = nrrows>1 ? (float) row/(nrrows-1) : 0;
	texturecoord[1] = nrcols>1 ? (float) col/(nrcols-1) : 0;
	glTexCoord2fv(texturecoord);
	glVertex3fv(cptr[index].getValue());
    }
    
    if ( isopen )
	glEnd();

    buildmutex->readUnlock();
}


void MeshSurfaceTesselationCache::GLRenderLines(SoGLRenderAction* action)
{
    SoState* state = action->getState();
    SoMaterialBundle mb(action);
    const SbVec3f* cptr = meshsurface.coordinates.getValues(0);
    const int nc = meshsurface.coordinates.getNum();
    const int32_t* miptr = meshsurface.materialIndex.getValues(0);
    const int nmi = meshsurface.materialIndex.getNum();

    buildmutex->readLock();
    const int nrci = lineci.getLength();
    bool isopen = false;
    float texturecoord[2];
    const int nrrows = meshsurface.nrRows();
    const int nrcols = meshsurface.nrColumns.getValue();

    glPushAttrib(GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);

    for ( int idx=0; idx<nrci; idx++ )
    {
	const int index = lineci[idx];
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
	    glBegin( GL_LINE_STRIP );
	    isopen = true;
	}

	mb.send(miptr?(index<nmi?miptr[index]:miptr[nmi-1]):0,true);
#if __debug__
	if ( index>=nc )
	    SoDebugError::postWarning("MeshSurfaceTesselationCache::GLRender",
				      "Index is too large");
#endif
	const int row = index/nrcols;
	const int col = index%nrcols;
	texturecoord[0] = nrrows>1 ? (float) row/(nrrows-1) : 0;
	texturecoord[1] = nrcols>1 ? (float) col/(nrcols-1) : 0;
	glTexCoord2fv(texturecoord);
	glVertex3fv(cptr[index].getValue());
    }
    
    if ( isopen )
	glEnd();

    glPopAttrib();

    buildmutex->readUnlock();
}


MeshSurfacePart::MeshSurfacePart( SoLODMeshSurface& m, int s0, int s1, int ssz )
    : meshsurface( m )
    , start0( s0 )
    , start1( s1 )
    , sidesize( ssz )
    , resolution( 0 )
    , reshaschanged( true )
    , bboxcache( 0 )
    , bboxvalidation( false )
    , gluevalidation( false )
    , gluecache( 0 )
{
    int spacing = sidesize/2;
    int nrcells = 2;
    while ( true )
    {
	resolutions.push( new MeshSurfImpl::MeshSurfacePartResolution(
		    m,s0,s1,nrcells,nrcells,spacing) );
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
	    bboxes.push(new MeshSurfImpl::MeshSurfacePartPart(m,bboxstart0,
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
    delete gluecache;
}


void MeshSurfacePart::setStart( int row, int col )
{
    const int diff0 = row-start0, diff1 = col-start1;
    start0 = row; start1 = col;

    for ( int idx=0; idx<resolutions.getLength(); idx++ )
	resolutions[idx]->setStart( row, col );

    for ( int idx=0; idx<bboxes.getLength(); idx++ )
    {
	const int oldrow = bboxes[idx]->getStart(true);
	const int oldcol = bboxes[idx]->getStart(false);
	bboxes[idx]->setStart( oldrow+diff0, oldcol+diff1 );
    }
}


void MeshSurfacePart::changeCacheIdx( const MeshSurfaceIndexChanger& ch )
{
    for ( int idx=0; idx<resolutions.getLength(); idx++ )
	resolutions[idx]->changeCacheIdx( ch );

    if ( gluecache ) gluecache->changeCacheIdx(ch);
}



void MeshSurfacePart::touch( int i0, int i1, bool undef )
{
    const int hsidesize = sidesize/2;
    const int rel0 = i0-start0;
    const int rel1 = i1-start1;

    if ( rel0<-hsidesize || rel1<-hsidesize ||
	 rel0>sidesize+hsidesize || rel1>sidesize+hsidesize )
	return;

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

    if ( bboxcache ) bboxcache->invalidate();

    bboxvalidation = false;
    gluevalidation = false;
}


void MeshSurfacePart::computeBBox(SoState* state, SbBox3f& bbox,
				  SbVec3f& center, bool useownvalidation )
{
    if ( bboxcache &&
	(useownvalidation ? bboxvalidation : bboxcache->isValid(state)) )
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
    bbox.makeEmpty();

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
    bboxvalidation = true;
}


void MeshSurfacePart::rayPick( SoRayPickAction* action, bool useownvalidation )
{
    for ( int idx=0; idx<bboxes.getLength(); idx++ )
	bboxes[idx]->rayPick(action,useownvalidation);
}


void MeshSurfacePart::GLRenderSurface(  SoGLRenderAction* action,
					bool useownvalidation)
{
    if ( resolution<0 ) return;
    resolutions[resolution]->GLRenderSurface( action, useownvalidation );
}


void MeshSurfacePart::GLRenderWireframe(SoGLRenderAction* action,
					bool useownvalidation)
{
    if ( resolution<0 ) return;
    resolutions[resolution]->GLRenderWireframe( action, useownvalidation );
}



bool MeshSurfacePart::cullTest(SoState* state, bool ownv )
{
    SbBox3f bbox;
    SbVec3f dummy;
    computeBBox( state, bbox, dummy, ownv );
    if ( bbox.isEmpty() ) return true;

    return SoCullElement::cullTest(state, bbox, true );
}


int MeshSurfacePart::computeResolution( SoState* state, bool ownv) 
{
    SbBox3f bbox;
    SbVec3f dummy;
    computeBBox( state, bbox, dummy, ownv );
    if ( bbox.isEmpty() ) return -1;

    const int nrrows = neighbors[7] ? sidesize : nrRows()-1;
    const int nrcols = neighbors[5] ? sidesize : nrCols()-1;
    const int numres = resolutions.getLength();
    int minres = numres-1;
    int spacing = 1;
    for ( ; minres>0; minres-- )
    {
	const int nextspacing = spacing*2;
	if ( nrrows%nextspacing || nrcols%nextspacing )
	    break;

	spacing = nextspacing;
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
	for ( desiredres=numres-1; desiredres>minres;
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
    if ( desiredres==-1 )
    {
	resolution = -1;
	return true;
    }

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
{ return res>=0 ? resolutions[res]->getSpacing() : 1; }


#define mAddCoordToIndexes(row_,col_,res_, indexes, normals) \
{ \
    const int index_ = meshsurface.getIndex( row_, col_ ); \
    if ( index_!=-1 && meshsurface.coordinates[index_][2]<1e29 ) \
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
	     ? gluecache->isValid() : !gluevalidation ) || reshaschanged ||
	(neighbors[5] && neighbors[5]->hasResChangedSinceLastRender()) ||
	(neighbors[7] && neighbors[7]->hasResChangedSinceLastRender());

    if ( gluecache && !tesselate )
    {
	gluecache->GLRenderSurface(action);
	return;
    }

    if ( !gluecache ) 
	gluecache = new MeshSurfaceTesselationCache(meshsurface,
				MeshSurfaceTesselationCache::TriangleFan);
    else
	gluecache->reset(true);

    gluecache->buildmutex->writeLock();

    const int ownspacing = getSpacing(resolution);
    const int res5 = neighbors[5] && neighbors[5]->getResolution()!=-1
	? neighbors[5]->getResolution() : resolution;
    const int spacing5 = getSpacing(res5);
    const int res7 = neighbors[7]  && neighbors[7]->getResolution()!=-1
	? neighbors[7]->getResolution() : resolution;
    const int spacing7 = getSpacing(res7);
    const int res8 = neighbors[8] && neighbors[8]->getResolution()!=-1
	? neighbors[8]->getResolution() : res7;

    int row = start0+sidesize-ownspacing;
    int col = start1;

    const int rowgluespacing =
	ownspacing>spacing7 ? ownspacing : spacing7;

    SbList<SbVec2s> rowgluecells;
    for ( int idx=0; idx<sidesize-ownspacing; idx+=rowgluespacing )
    {
	const SbVec2s rc(row,col+idx);
	if ( rc[0]<0 || rc[1]<0 )
	    continue;

	rowgluecells.append( rc );
    }

    const int colgluespacing = ownspacing>spacing5 ? ownspacing : spacing5;

    SbList<SbVec2s> colgluecells;
    row = start0;
    col = start1+sidesize-ownspacing;
    for ( int idx=0; idx<sidesize-ownspacing; idx+=colgluespacing )
    {
	const SbVec2s rc(row+idx,col);
	if ( rc[0]<0 || rc[1]<0 )
	    continue;

	colgluecells.append( rc );
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

    gluevalidation = true;
    gluecache->buildmutex->writeUnlock();
    gluecache->GLRenderSurface(action);
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
	if ( partres->getNormal( relrow/spacing, relcol/spacing,
		    		 useownvalidation, normal) )
	    return true;
    }

    return false;
}


#define mAddFanNode( ci_, n_ ) \
    gluecache->triangleni.push( gluecache->normals.getLength() ); \
    gluecache->normals.push(n_); \
    gluecache->triangleci.push(ci_)

#define mStopFanStrip \
    gluecache->triangleni.push(-1); \
    gluecache->triangleci.push(-1)

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


MeshSurfacePartResolution::MeshSurfacePartResolution(SoLODMeshSurface& m,
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
   int reli0 = i0-start0;
   int reli1 = i1-start1;
   if ( !reli0%spacing || !reli1%spacing ) 
       return;

   reli0/=spacing;
   reli1/=spacing;

   if ( reli0<-1||reli0<-1||reli0>sidesize0||reli1>sidesize1 )
       return;

   if ( undef && reli0>=0 && reli0<sidesize0 && reli1>=0 && reli1<sidesize1 )
       cachestatus = 2;
   else if ( cachestatus<2 )
   {
       cachestatus = cache && !cache->normals.getLength() ? 2 : 1;
   }
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
    const int normalsperrow = nrCols();
    int normalindex = mGetNormalIndex(relrow,relcol);
#if __debug__
    if ( normalindex<0 )
    {
	SoDebugError::postWarning("MeshSurfacePartResolution::getNormal",
				      "normalindex is negative");
	normalindex = 0;
    }
#endif

    if ( cache && useownvalidation && cachestatus!=2 &&
	 normalindex<cache->normals.getLength() )
    {
	if ( normalindex<cache->normals.getLength() )
	{
	    res = cache->normals[normalindex];
	    return true;
	}

	return false;
    }

    return computeNormal( relrow, relcol, &res );
}


void MeshSurfacePartResolution::setStart( int row, int col )
{
    start0 = row; start1=col;
}


void MeshSurfacePartResolution::changeCacheIdx(
			const MeshSurfaceIndexChanger& ch)
{ if ( cache && cachestatus!=2 ) cache->changeCacheIdx(ch); }


void MeshSurfacePartResolution::GLRenderSurface( SoGLRenderAction* action,
						 bool useownvalidation )
{
    bool drawwireframe = meshsurface.wireframe.getValue();
    if ( cache && useownvalidation )
    {
	if ( cachestatus==2 )
	    tesselate();

	cache->GLRenderSurface(action);
    }
    else
    {
	if ( !cache || !cache->isValid() )
	    tesselate();

	cache->GLRenderSurface(action);
    }
}


void MeshSurfacePartResolution::GLRenderWireframe( SoGLRenderAction* action,
						 bool useownvalidation )
{
    if ( cache && useownvalidation )
    {
	if ( cachestatus==2 )
	    tesselate();

	cache->GLRenderLines(action);
    }
    else
    {
	if ( !cache || !cache->isValid() )
	    tesselate();

	cache->GLRenderLines(action);
    }
}

#define mAppend(ci_,ni_, extra) \
{ \
    cache->triangleci.append(ci_); \
    cache->triangleni.append(ni_); \
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
	cache = new MeshSurfaceTesselationCache( meshsurface,
		    	MeshSurfaceTesselationCache::TriangleStrip );
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

    if ( nrrows )
    {
	for ( int idx1=0; idx1<nrcols; idx1++ )
	    computeNormal( nrrows-1, idx1 );
    }

    const int normalsperrow = nrCols();
    const SbVec3f* coordptr = meshsurface.coordinates.getValues(0);
    for ( int idx0=0; idx0<nrrows; idx0++ )
    {
	bool isopen = false;
	for ( int idx1=0; idx1<=nrcols; idx1++ )
	{
	    int ci = getCoordIndex(idx0,idx1);
	    if ( ci==-1 || coordptr[ci][2]>1e29 )
	    {
		if ( isopen )
		{
		    cache->lineci.append(-1);
		    cache->lineni.append(-1);
		    isopen = false;
		}

		continue;
	    }

	    cache->lineci.append(ci);
	    cache->lineni.append(mGetNormalIndex( idx0, idx1 ));
	    isopen = true;
	}

	if ( isopen )
	{
	    cache->lineci.append(-1);
	    cache->lineni.append(-1);
	    isopen = false;
	}
    }


    for ( int idx1=0; idx1<nrcols; idx1++ )
    {
	bool isopen = false;
	for ( int idx0=0; idx0<=nrrows; idx0++ )
	{
	    int ci = getCoordIndex(idx0,idx1);
	    if ( ci==-1 || coordptr[ci][2]>1e29 )
	    {
		if ( isopen )
		{
		    cache->lineci.append(-1);
		    cache->lineni.append(-1);
		    isopen = false;
		}

		continue;
	    }

	    cache->lineci.append(ci);
	    cache->lineni.append(mGetNormalIndex( idx0, idx1 ));
	    isopen = true;
	}

	if ( isopen )
	{
	    cache->lineci.append(-1);
	    cache->lineni.append(-1);
	    isopen = false;
	}
    }


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
	mAppendTriangle( idx10, nidx10, idx00, nidx00, idx11, nidx11 );
	mEndStrip;
    }
    else if ( idx11<0 && (filltype<3||filltype==6))
    {
	mAppendTriangle( idx10, nidx10, idx00, nidx00, idx01, nidx01 );
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

    if ( idx00>=0 && idx10>=0 && idx01>=0 && idx11>=0 && filltype<3)
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
	mAppend( idx11, nidx11, );
	mEndStrip;
    }
    else if ( idx01>=0 )
    {
	mAppend( idx01, nidx01,);
	mEndStrip;
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
	    v0 = (celem[nextcolindex]-celem[coordindex])*2;
	else 
	    v0 = (celem[coordindex]-celem[prevcolindex])*2;
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
	    v1 = (celem[nextrowindex]-celem[coordindex])*2;
	else 
	    v1 = (celem[coordindex]-celem[prevrowindex])*2;
    }
    else
	v1 = celem[nextrowindex]-celem[prevrowindex];

    if ( !v1.length() )
	return false;

    const SbVec3f normal = v1.cross(v0);
    if ( cache )
    {
	const int normalsperrow = nrCols();
	int normalindex = mGetNormalIndex(rel0,rel1);
	if ( normalindex<0 )
	{
	    SoDebugError::postWarning(
		    "MeshSurfacePartResolution::computeNormal",
		    "normalindex is negative");
	    normalindex = 0;
	}

	while ( cache->normals.getLength()<=normalindex )
	    cache->normals.push( SbVec3f(0,0,1) );

	cache->normals[normalindex] = normal;
    }

    if ( res ) *res = normal;

    return true;
}

}; //namespace


SO_NODE_SOURCE(SoLODMeshSurface);
void SoLODMeshSurface::initClass()
{
    SO_NODE_INIT_CLASS(SoLODMeshSurface, SoShape, "KrisSurface" );
}


SoLODMeshSurface::SoLODMeshSurface()
    : sidesize( 64 )
    , useownvalidation( false )
    , nrcolparts( 0 )
    , writemutex(SbRWMutex::READ_PRECEDENCE)
{
    SO_NODE_CONSTRUCTOR(SoLODMeshSurface);
    SO_NODE_ADD_FIELD( coordinates, (1e30,1e30,1e30) );
    SO_NODE_ADD_FIELD( materialIndex, (0) );
    SO_NODE_ADD_FIELD( meshStyle, (0) );
    SO_NODE_ADD_FIELD( resolution, (-1) );
    SO_NODE_ADD_FIELD( wireframe, (false) );
    SO_NODE_ADD_FIELD( wireframeLift, (5) );
    SO_NODE_ADD_FIELD( nrColumns, (64) );

    coordinates.deleteValues(0,-1);
}


SoLODMeshSurface::~SoLODMeshSurface()
{
    for ( int idx=parts.getLength()-1; idx>=0; idx-- )
	delete parts[idx];
}


void SoLODMeshSurface::insertColumns( bool before, int nr )
{
    if ( nr<=0 ) return;
    const bool oldvalidationflag = useownvalidation;
    useownvalidation = true;
    const int nrrows = nrRows();
    const int nrcols = nrColumns.getValue();

    const bool oldcoordnotstatus = coordinates.enableNotify(false);
    const bool oldminotstatus = materialIndex.enableNotify(false);
    const bool oldmsnotstatus = meshStyle.enableNotify(false);
    const bool oldnrcolnotstatus = nrColumns.enableNotify(false);

    for ( int row=before?nrrows-1:nrrows-2; row>=0; row-- )
    {
	const int insertpos = (row+(before?0:1))*nrcols;
	coordinates.insertSpace( insertpos, nr );
	for ( int idy=0; idy<nr; idy++ )
	    coordinates.set1Value( insertpos+idy, SbVec3f(1e30,1e30,1e30) );

	if ( materialIndex.getNum()>insertpos )
	    materialIndex.insertSpace( insertpos, nr );
	if ( meshStyle.getNum()>insertpos )
	    meshStyle.insertSpace( insertpos, nr );
    }

    const MeshSurfImpl::MeshSurfaceIndexChanger
			ic( nrcols, nrcols+nr, before, 0 );
    for ( int idx=0; idx<parts.getLength(); idx++ )
    {
	if ( before )
	{
	    parts[idx]->setStart( parts[idx]->getRowStart(),
		    		  parts[idx]->getColStart()+nr );
	}

	parts[idx]->changeCacheIdx( ic );
    }

    nrColumns.setValue( nrcols+nr );

    coordinates.enableNotify(oldcoordnotstatus);
    materialIndex.enableNotify(oldminotstatus);
    meshStyle.enableNotify(oldmsnotstatus);
    nrColumns.enableNotify(oldnrcolnotstatus);

    useownvalidation = oldvalidationflag;
}


void SoLODMeshSurface::insertRowsBefore(int nr)
{
    if ( nr<=0 ) return;
    const int nrcols = nrColumns.getValue();
    coordinates.insertSpace( 0, nrcols*nr );
    
    for ( int idy=0; idy<nrcols*nr; idy++ )
	coordinates.set1Value( idy, SbVec3f(1e30,1e30,1e30) );

    if ( isMaterialNonDefault() )
	materialIndex.insertSpace( 0, nrcols*nr );
    if ( isMeshStyleNonDefault() )
	meshStyle.insertSpace( 0, nrcols*nr );

    const MeshSurfImpl::MeshSurfaceIndexChanger
			ic( nrcols, nrcols, true, nr );
    for ( int idx=0; idx<parts.getLength(); idx++ )
    {
	parts[idx]->setStart( parts[idx]->getRowStart()+nr,
			      parts[idx]->getColStart() );
	parts[idx]->changeCacheIdx( ic );
    }
}


void SoLODMeshSurface::turnOnOwnValidation(bool yn)
{ useownvalidation=yn; }


void SoLODMeshSurface::touch( int row, int col, bool udf )
{
    for ( int idx=0; idx<parts.getLength(); idx++ )
	parts[idx]->touch( row, col, udf );
}


void SoLODMeshSurface::computeBBox(SoAction* action, SbBox3f& bbox,
				SbVec3f& center )
{

    adjustNrOfParts();

    writemutex.readLock();
    SoState* state = action->getState();
    center = SbVec3f(0,0,0);
    SbBox3f localbox;
    SbVec3f localcenter;

    bbox.makeEmpty();

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

    writemutex.readUnlock();
}


void SoLODMeshSurface::rayPick(SoRayPickAction* action )
{
    writemutex.readLock();

    for ( int idx=0; idx<parts.getLength(); idx++ )
	parts[idx]->rayPick(action,useownvalidation);

    writemutex.readUnlock();
}


void SoLODMeshSurface::GLRender(SoGLRenderAction* action)
{
    if ( !shouldGLRender(action) )
	return;

    SoState* state = action->getState();

    writemutex.readLock();

    const int nrparts = parts.getLength();
    //if ( !(SoCameraInfoElement::get(state)&SoCameraInfo::STEREO) )
    //{
	const int whichres = resolution.getValue();
	for ( int idx=0; idx<nrparts; idx++ )
	{
	    MeshSurfImpl::MeshSurfacePart* part = parts[idx];
	    int missingresolution = -1;
	    if ( whichres==-1 )
	    {
		const int desres =  part->cullTest(state,useownvalidation)
		    ? -1
		    : part->computeResolution(state,useownvalidation);
		if ( !part->setResolution(desres,useownvalidation) )
		     missingresolution = desres;
	    }
	    else
	    {
		const int desres = part->cullTest(state,useownvalidation)
		    ? -1 : whichres;

		if ( !part->setResolution( desres, useownvalidation) )
		    missingresolution = desres;
	    }

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
	parts[idx]->GLRenderSurface(action, useownvalidation);

    for ( int idx=0; idx<nrparts; idx++ )
	parts[idx]->GLRenderGlue(action, useownvalidation);


    if ( wireframe.getValue() )
    {
	for ( int idx=0; idx<nrparts; idx++ )
	    parts[idx]->GLRenderWireframe(action, useownvalidation);
    }

    for ( int idx=0; idx<nrparts; idx++ )
	parts[idx]->resetChangeResFlag();

    writemutex.readUnlock();
}


void SoLODMeshSurface::getBoundingBox(SoGetBoundingBoxAction* action)
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


int SoLODMeshSurface::getIndex( int i0, int i1 ) const
{
    if ( i1<0 || i1>=nrColumns.getValue() ) return -1;
    const int res = i0*nrColumns.getValue()+i1;
    return res>=0 && res<coordinates.getNum() ? res : -1;
}


int SoLODMeshSurface::nrRows() const
{
    const int numcoords = coordinates.getNum();
    if ( !numcoords ) return 0;
    return (numcoords-1)/nrColumns.getValue()+1;
}


void SoLODMeshSurface::notify( SoNotList* nl )
{
    SoShape::notify(nl);
    if ( useownvalidation ) 
	return;

    for ( int idx=0; idx<parts.getLength(); idx++ )
	parts[idx]->invalidateCaches();
}


void SoLODMeshSurface::adjustNrOfParts()
{
    writemutex.writeLock();
    const int nrcols = nrColumns.getValue();
    bool changed = false;

    if ( !coordinates.getNum() )
    {
	while ( parts.getLength() )
	{
	    delete parts[0];
	    parts.remove(0);
	    changed = true; 
	}
    }
    else
    {
	if ( !parts.getLength() && coordinates.getNum() )
	{
	    parts.push(new MeshSurfImpl::MeshSurfacePart(*this,0,0,sidesize) );
	    changed = true; 
	}

	
        while ( parts[0]->getRowStart()>0 )
	{
	    const int newrow = parts[0]->getRowStart()-sidesize;
	    parts.insert( new MeshSurfImpl::MeshSurfacePart(*this, newrow,
			  parts[0]->getColStart(), sidesize), 0 );
	    changed = true; 
	}

	while ( parts.getLength() && parts[0]->getRowStart()<=-sidesize )
	{
	    delete parts[0];
	    parts.remove(0);
	    changed = true; 
	}

        int currentrow = parts[0]->getRowStart();
        int rowstartpartidx = 0;
        while ( currentrow<nrRows() )
        {
	    if ( rowstartpartidx==parts.getLength() )
	    {
	        parts.push(new MeshSurfImpl::MeshSurfacePart(*this, currentrow,
			   parts[0]->getColStart(), sidesize) );
	        changed = true; 
	    }

	    if ( completeRowStart(rowstartpartidx) ) changed = true;
	    if ( completeRowEnd(rowstartpartidx) ) changed = true;

	    while ( rowstartpartidx<parts.getLength() &&
		parts[rowstartpartidx]->getRowStart()==currentrow )
		rowstartpartidx++;

	    currentrow += sidesize;
        }
    }

    if ( changed )
	setPartRelations();

    writemutex.writeUnlock();
}


void SoLODMeshSurface::setPartRelations()
{
    for ( int idx=0; idx<parts.getLength(); idx++ )
    {
	if ( idx<parts.getLength()-1 &&
	     parts[idx]->getColStart()<parts[idx+1]->getColStart() )
	    parts[idx]->setNeighbor( 5, parts[idx+1], true );
	else parts[idx]->setNeighbor( 5, 0 );

	for ( int idy=idx+1; idy<parts.getLength(); idy++ )
	{
	    if ( parts[idy]->getRowStart()==parts[idx]->getRowStart() )
		continue;

	    if ( parts[idy]->getColStart()<parts[idx]->getColStart()-sidesize )
		continue;

	    if ( parts[idy]->getColStart()==parts[idx]->getColStart()-sidesize )
	    {
		parts[idx]->setNeighbor( 6, parts[idy], true );
		if ( ++idy>=parts.getLength() ) continue;
	    }
	    else parts[idx]->setNeighbor( 6, 0, false );

	    if ( parts[idy]->getColStart()==parts[idx]->getColStart() )
	    {
		parts[idx]->setNeighbor( 7, parts[idy], true );
		if ( ++idy>=parts.getLength() ) continue;
	    }
	    else parts[idx]->setNeighbor( 7, 0, false );

	    if ( parts[idy]->getColStart()==parts[idx]->getColStart()+sidesize )
	    {
		parts[idx]->setNeighbor( 8, parts[idy], true );
		if ( ++idy>=parts.getLength() ) continue;
	    }
	    else parts[idx]->setNeighbor( 8, 0, false );

	    break;
	}
    }
}


bool SoLODMeshSurface::completeRowStart(int& rowstartpart)
{
    bool changed = false;
    while ( parts[rowstartpart]->getColStart()>0 )
    {
        parts.insert(
	    new MeshSurfImpl::MeshSurfacePart(*this,
		  parts[rowstartpart]->getRowStart(),
		  parts[rowstartpart]->getColStart()-sidesize,
			     sidesize), rowstartpart );
        changed = true; 
    }

    while ( parts[rowstartpart]->getColStart()<=-sidesize )
    {
	delete parts[rowstartpart];
	parts.remove(rowstartpart);
        changed = true; 
    }

    return changed;
}

bool SoLODMeshSurface::completeRowEnd(int rowstart)
{
    bool changed = false;
    int lastpartonrow=rowstart;
    while ( lastpartonrow<parts.getLength()-1 &&
		parts[lastpartonrow+1]->getRowStart()==
		parts[lastpartonrow]->getRowStart() )
	lastpartonrow++;

    while ( parts[lastpartonrow]->getColStart()+sidesize<nrColumns.getValue() )
    {
	MeshSurfImpl::MeshSurfacePart* newpart =
	    new MeshSurfImpl::MeshSurfacePart(*this,
            parts[lastpartonrow]->getRowStart(),
            parts[lastpartonrow]->getColStart()+sidesize, sidesize);

	if ( lastpartonrow==parts.getLength()-1 )
	    parts.push( newpart );
	else
	    parts.insert( newpart, lastpartonrow+1 );

        changed = true;
	lastpartonrow++;
    }

    while ( parts[lastpartonrow]->getColStart()-sidesize>nrColumns.getValue() )
    {
	delete parts[lastpartonrow];
	parts.remove(lastpartonrow);
	lastpartonrow--;
        changed = true;
    }

    return changed;
}


SbBool SoLODMeshSurface::shouldGLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    const SoShapeStyleElement* shapestyle = SoShapeStyleElement::get(state);
    unsigned int shapestyleflags = shapestyle->getFlags();

    if (shapestyleflags & SoShapeStyleElement::INVISIBLE)
	return false;

    const SbBool transparent = (shapestyleflags &
	    (SoShapeStyleElement::TRANSP_TEXTURE |
	     SoShapeStyleElement::TRANSP_MATERIAL));

    if ( action->handleTransparency(transparent) )
	return false;

    if ( SoComplexityTypeElement::get(state)==
			SoComplexityTypeElement::BOUNDING_BOX )
    {
	GLRenderBoundingBox(action);
	return false;
    }

    return true;
}


#define mNonDefaultCheckImpl( type, field ) \
    const type* vals = field.getValues(0); \
    for ( int idx=field.getNum()-1; idx>=0; idx-- ) \
	if ( vals[idx] ) return true; \
 \
    return false

bool SoLODMeshSurface::isMaterialNonDefault() const
{ mNonDefaultCheckImpl( int32_t, materialIndex ); }


bool SoLODMeshSurface::isMeshStyleNonDefault() const
{ mNonDefaultCheckImpl( int16_t, meshStyle ); }
