/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoMeshSurfaceSquare.cc,v 1.6 2003-10-08 09:55:44 kristofer Exp $";


#include "SoMeshSurfaceSquare.h"

#include "SoMeshSurface.h"
#include "SoMeshSurfaceBrick.h"
#include "SoMeshSurfaceBrickWire.h"
#include "SoForegroundTranslation.h"

#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"

#include <Inventor/SoPickedPoint.h>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>

#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCullElement.h>

#include <Inventor/events/SoMouseButtonEvent.h>

#include <Inventor/lists/SoCallbackList.h>

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>

#include "Inventor/sensors/SoFieldSensor.h"


SO_KIT_SOURCE(SoMeshSurfaceSquare);

void SoMeshSurfaceSquare::initClass()
{
    SO_KIT_INIT_CLASS( SoMeshSurfaceSquare, SoBaseKit, "BaseKit");
    SO_ENABLE( SoGLRenderAction, SoCameraInfoElement );
}


SoMeshSurfaceSquare::SoMeshSurfaceSquare()
    : sizePowerSensor(
	    new SoFieldSensor( SoMeshSurfaceSquare::sizePowerCB, this ))
    , bboxcache( 0 )
    , showwire( true )
    , showtri( true )
    , pickcallbacks( 0 )
    , updateglue( true )
{
    SO_KIT_CONSTRUCTOR(SoMeshSurfaceSquare);
    isBuiltIn = true;

    SO_KIT_ADD_CATALOG_ENTRY(topSeparator,SoSeparator,false,this, ,false);

    SO_KIT_ADD_CATALOG_ENTRY(eventCatcher,SoEventCallback,false,topSeparator,
	    		     coords,false);
    SO_KIT_ADD_CATALOG_ENTRY(coords,SoCoordinate3,false,topSeparator,
	    		     texturecoords,false);
    SO_KIT_ADD_CATALOG_ENTRY(texturecoords,SoTextureCoordinate2,
	    		false,topSeparator, wireSeparator,false);
    SO_KIT_ADD_CATALOG_ENTRY(wireSeparator,SoSeparator,false,topSeparator,
	    		     triResSwitch,false);

    SO_KIT_ADD_CATALOG_ENTRY(wireTranslation,SoForegroundTranslation,false,
	    		     wireSeparator, wireResSwitch,false);
    SO_KIT_ADD_CATALOG_ENTRY(wireResSwitch,SoSwitch,false,wireSeparator,,false);

    SO_KIT_ADD_CATALOG_ENTRY(triResSwitch,SoSwitch,false,topSeparator,
	    		     glueSwitch,false);
    SO_KIT_ADD_CATALOG_ENTRY(glueSwitch,SoSwitch,false,topSeparator, ,false);

    SO_KIT_ADD_CATALOG_ENTRY(glueGroup,SoSeparator,false,glueSwitch, ,false);
    SO_KIT_ADD_CATALOG_ENTRY(glueNormals,SoNormal,false,glueGroup,glue,false);
    SO_KIT_ADD_CATALOG_ENTRY(glue,SoIndexedFaceSet,false,glueGroup, ,false);

    SO_KIT_INIT_INSTANCE();

    triswitchptr = (SoSwitch*) getAnyPart("triResSwitch",true);
    wireswitchptr = (SoSwitch*) getAnyPart("wireResSwitch",true);
    glueswitchptr = (SoSwitch*) getAnyPart("glueSwitch",true);
    glueptr = (SoIndexedFaceSet*) getAnyPart("glue",true);
    coordptr = (SoCoordinate3*) getAnyPart("coords",true);
    gluenormalptr = (SoNormal*) getAnyPart("glueNormals",true);
    texturecoordptr = (SoTextureCoordinate2*) getAnyPart("texturecoords",true);

    for ( int idx=0; idx<9; idx++ )
	neigbors.append(0);

    SO_KIT_ADD_FIELD( origo, (0) );
    SO_KIT_ADD_FIELD( sizepower, (6) );
    SO_KIT_ADD_FIELD( end, (0) );

    origo.set1Value(1,0);

    end.set1Value(0,0);
    end.set1Value(1,0);

    sizePowerSensor->attach( &sizepower );
    sizePowerSensor->trigger();
}


SoMeshSurfaceSquare::~SoMeshSurfaceSquare()
{
    sizePowerSensor->detach();
    delete sizePowerSensor;
    delete bboxcache;
    delete pickcallbacks;
}


void SoMeshSurfaceSquare::updateTextureCoords()
{
    SbVec2s rowinfo( origo[0], end[0]-origo[0]+1 );
    SbVec2s colinfo( origo[1], end[1]-origo[1]+1 );
    texturecoordptr->point.deleteValues(0);
    for ( int row=origo[0]; row<=end[0]; row++ )
	for ( int col=origo[1]; col<=end[1]; col++ )
	    setTextureCoord( row, col, rowinfo, colinfo );
}


void SoMeshSurfaceSquare::setTextureCoord( int row, int col,
    				const SbVec2s& rowinfo, const SbVec2s& colinfo )
{
    int index = getCoordIndex( row, col );
    if ( index==-1 ) return;
    const float relrow = (float)(row-rowinfo[0]) / (rowinfo[1] - 1);
    const float relcol = (float)(col-colinfo[0]) / (colinfo[1] - 1);
    texturecoordptr->point.set1Value( index, SbVec2f(relcol,relrow) );
    // swapped row,col for correct texture mapping.
}


void SoMeshSurfaceSquare::setPos( int row, int col, const SbVec3f& np,
       				 const SbVec2s& rowinfo, const SbVec2s& colinfo)
{
    const int index = getCoordIndex( row, col );
    if ( index==-1 ) return;

    if ( bboxcache )
    {
	if ( SoMeshSurface::isUndefined(np) )
	{
	    delete bboxcache;
	    bboxcache = 0;
	    updateglue = true;
	}
	else
	{
	    bboxcache->extendBy(np);
	}
    }

    if ( SoMeshSurface::isUndefined( coordptr->point[index]) )
	updateglue = true;

    coordptr->point.set1Value(index,np);

    for ( int idx=0; idx<9; idx++ )
    {
	if ( neigbors[idx] )
	    neigbors[idx]->touch( row, col );
    }

    if ( row>end[0] ) end.set1Value(0,row);
    if ( col>end[1] ) end.set1Value(1,col);

    setTextureCoord( row, col, rowinfo, colinfo );
    touch( row, col );
}


void SoMeshSurfaceSquare::removePos( int row, int col )
{
    const int index = getCoordIndex( row, col );
    if ( index<0 ) return;

    coordptr->point.set1Value( index, SbVec3f(SoMeshSurface::undefVal(),
					      SoMeshSurface::undefVal(),
					      SoMeshSurface::undefVal()) );

    if ( bboxcache )
    {
	delete bboxcache;
	bboxcache = 0;
    }

    touch( row, col );
}


SbVec3f SoMeshSurfaceSquare::getPos( int row, int col ) const
{
    const int index = getCoordIndex(row, col);

    if ( index<0 ) return SbVec3f(1e30,1e30,1e30);

    return coordptr->point[index];
}


int SoMeshSurfaceSquare::getCoordIndex( int row, int col ) const
{
    row -= origo[0];
    col -= origo[1];

    if ( row<0 || row>sidesize || col<0 || col>sidesize )
	return -1;

    return row*(sidesize+1)+col;
}


int SoMeshSurfaceSquare::getNeigborIndex( int relrow, int relcol ) const
{
    return (relrow+1)*3+relcol+1;
}



bool SoMeshSurfaceSquare::setResolution(int nr)
{
    if ( showtri==(glueswitchptr->whichChild.getValue()==-1) )
	glueswitchptr->whichChild = showtri ? 0 : -1;

    for ( int idx=nr; idx>0; idx-- )
    {
	if ( getBrick(idx)->isValid() )
	{
	    if ( idx!=currentres ||
		    (triswitchptr->whichChild.getValue()==-1)==showtri ||
		    (wireswitchptr->whichChild.getValue()==-1)==showwire )
	    {
		triswitchptr->whichChild = showtri ? idx : -1;
		wireswitchptr->whichChild = showwire ? idx : -1;
		reshaschanged = true;
		currentres = idx;
	    }

	    return idx==nr;
	}
    }

    if ( currentres ) 
    {
	triswitchptr->whichChild = showtri ? 0 : -1;
	wireswitchptr->whichChild = showwire ? 0 : -1;
	reshaschanged = true;
	currentres = 0;
    }

    return !nr;
}


int SoMeshSurfaceSquare::computeResolution(SoState* state)
{
    int32_t camerainfo = SoCameraInfoElement::get(state);
    int desiredres = 0;
    if ( !(camerainfo&(SoCameraInfo::MOVING|SoCameraInfo::INTERACTIVE)) )
    {
	computeBBox();

	SbVec2s screensize;
	SoShape::getScreenSize( state, *bboxcache, screensize );
	const float complexity =
			SbClamp(SoComplexityElement::get(state), 0.0f, 1.0f);
	const float wantednumcells =
	    		complexity*screensize[0]*screensize[1] / 16;

	int numcells = 4;
	const int numres = sizepower.getValue();
	for ( ; desiredres<numres-1; desiredres++ )
	{
	    if ( numcells>wantednumcells )
		break;

	    numcells *=4;
	}
    }
	    
    return setResolution(desiredres) ? -1 : desiredres;
}


SbBool SoMeshSurfaceSquare::shouldGLRender(SoGLRenderAction* action)
{
    //TODO Implement other checks
    return !cullTest(action->getState());
}


SbBool SoMeshSurfaceSquare::cullTest(SoState* state)
{
    computeBBox();

    state->push();

    bool retval = false;
    if (!SoCullElement::completelyInside(state) && !bboxcache->isEmpty() )
    {
	retval = SoCullElement::cullBox(state,*bboxcache);
    }

    state->pop();
    return retval;
}


bool SoMeshSurfaceSquare::hasResolutionChanged() const
{
    return reshaschanged;
}


void SoMeshSurfaceSquare::touch( int row, int col )
{
    const int startrow = origo[0];
    const int size = get2Power( sizepower.getValue() );
    const int halfsize = size/2;
    const int lastrow = startrow + size;

    if ( row<startrow-halfsize || row>lastrow+halfsize )
	return;

    const int startcol = origo[1];
    const int lastcol = startcol+size;

    if ( col<startcol-halfsize || col>lastcol+halfsize )
	return;

    const int relrow = row-startrow;
    const int relcol = col-startcol;
    if ( relrow>=0 && relrow<=sidesize &&
	 relcol>=0 && relcol<=sidesize && 
	 (relrow==sidesize || relcol==sidesize) )
    {
	const int index = getCoordIndex( row, col );

	if ( neigbors[5] && relcol==sidesize )
	{
	    const SbVec3f np = neigbors[5]->getPos(row,col);
	    if ( SoMeshSurface::isUndefined(np) ||
		    SoMeshSurface::isUndefined( coordptr->point[index]) )
		updateglue = true;

	    coordptr->point.set1Value(index,np);
	}
	else if ( neigbors[7] && relrow==sidesize )
	{
	    const SbVec3f np = neigbors[7]->getPos(row,col);
	    if ( SoMeshSurface::isUndefined(np) ||
		    SoMeshSurface::isUndefined( coordptr->point[index]) )
		updateglue = true;

	    coordptr->point.set1Value(index,np);
	}
    }

    const int nrres = sizepower.getValue();
    for ( int idx=0; idx<nrres; idx++ )
    {
	SoMeshSurfaceBrick* brick = getBrick(idx);
	const int spacing = brick->spacing.getValue();
	const int sidesize = brick->sideSize.getValue();

	if ( relrow%spacing || relcol%spacing )
	    continue;

	if ( relrow<0 || relrow>=size || relcol<0 || relcol>=size )
	{
	    for ( int rowidx=-1; rowidx<=1; rowidx++ )
	    {
		const int localrow = relrow/spacing+rowidx;
		if ( localrow<0 || localrow>=sidesize)
		    continue;

		for ( int colidx=-1; colidx<=1; colidx++ )
		{
		    const int localcol = relcol/spacing+colidx;
		    if ( localcol<0 || localcol>=sidesize)
			continue;

		    brick->invalidateNormal(localrow*(sidesize+1)+localcol);
		}
	    }
	}
	else
	{
	    brick->invalidate();
	    getWire(idx)->invalidate();
	}
    }
}


int SoMeshSurfaceSquare::getResolution() const
{
    return currentres;
}


void SoMeshSurfaceSquare::setNeighbor( int relrow, int relcol,
				   SoMeshSurfaceSquare* part, bool callback )
{
    const int index = getNeigborIndex(relrow,relcol);
    neigbors[getNeigborIndex(relrow,relcol)] = part;

    if ( relrow==1 )
    {
	const int row = origo[0]+sidesize;
	for ( int idx=0; idx<sidesize; idx++ )
	{
	    const int col = origo[1]+idx;
	    touch( row, col );
	}
    }
    else if ( relcol==1 )
    {
	const int col = origo[1]+sidesize;
	for ( int idx=0; idx<sidesize; idx++ )
	{
	    const int row = origo[0]+idx;
	    touch( row, col );
	}
    }
	    
    if ( callback ) part->setNeighbor( -relrow, -relcol, this, false );
}


void SoMeshSurfaceSquare::turnOnWireFrame( bool yn )
{
    showwire = yn;
    wireswitchptr->whichChild.touch();
}


bool SoMeshSurfaceSquare::isWireFrameOn() const
{
    return showwire;
}


bool SoMeshSurfaceSquare::isUndefined( int row, int col ) const
{
    const SbVec3f vec = getPos( row, col );
    return SoMeshSurface::isUndefined(vec);
}


void SoMeshSurfaceSquare::addGlueIndex( int row, int col, int res, int& idx )
{
    if ( isUndefined(row,col) )
    {
	if ( row > end[0] ) row = end[0];
	if ( col > end[1] ) col = end[1];
    }

    int crdidx = getCoordIndex(row,col);
    if ( crdidx==-1 || isUndefined(row,col) ) return;

    if ( idx && crdidx == glueptr->coordIndex[idx-1] ) return;

    glueptr->coordIndex.set1Value( idx, crdidx );
    gluenormalptr->vector.set1Value( idx, getNormal(row,col,res) );
    glueptr->normalIndex.set1Value( idx, idx );
    idx++;
}


void SoMeshSurfaceSquare::updateGlue()
{
    const int ownres = getResolution();
    const int ownblocksize = getBlockSize( ownres );
    if ( ownres==-1 )
	return;

    if ( origo[0] == end[0] || origo[1] == end[1] )
	return;

    if ( updateglue || (neigbors[5] && neigbors[5]->hasResolutionChanged()) ||
	 (neigbors[7] && neigbors[7]->hasResolutionChanged()) ||
	 hasResolutionChanged() )
    {
	updateglue = false;
	glueptr->coordIndex.deleteValues(0);
	glueptr->normalIndex.deleteValues(0);

	const int row7 = origo[0] + sidesize; const int col7 = origo[1];
	bool glue7 = !(isUndefined(row7,col7) && 
		       isUndefined(row7-ownblocksize,col7) );
	const int row5 = origo[0]; const int col5 = origo[1] + sidesize;
	bool glue5 = !(isUndefined(row5,col5) && 
		       isUndefined(row5,col5-ownblocksize) );

	if ( !glue5 && !glue7 ) return;

	int row = glue7 ? row7 : row7 - ownblocksize;
	int col = glue7 ? col7 : col5 - ownblocksize;

	int idx = 0;
	if ( glue7 ) getOwn7CrdIdxs( row, col, idx );
	if ( glue5 )
	{
	    getOwn5CrdIdxs( row, col, idx );
	    getNeighbour5CrdIdxs( row, col, idx );
	}
	else
	    row += ownblocksize;

	if ( glue7 ) getNeighbour7CrdIdxs( row, col, idx );

	glueptr->coordIndex.set1Value( idx, -1 );
    }
}


void SoMeshSurfaceSquare::getOwn5CrdIdxs( int& row, int& col, int& idx )
{
    const int ownres = getResolution();
    const int ownblocksize = getBlockSize( ownres );
    const int nrownblocks = getNrBlocks( ownres );

    for ( int idy=1; idy<nrownblocks; idy++ )
    {
	row -= ownblocksize;
	addGlueIndex( row, col, ownres, idx );
    }

    col += ownblocksize;
}


void SoMeshSurfaceSquare::getOwn7CrdIdxs( int& row, int& col, int& idx )
{
    const int ownres = getResolution();
    const int ownblocksize = getBlockSize( ownres );
    const int nrownblocks = getNrBlocks( ownres );

    row -= ownblocksize;
    addGlueIndex( row, col, ownres, idx );

    for ( int idy=1; idy<nrownblocks; idy++ )
    {
	col += ownblocksize;
	addGlueIndex( row, col, ownres, idx );
    }
}


void SoMeshSurfaceSquare::getNeighbour5CrdIdxs( int& row, int& col, int& idx )
{
    SoMeshSurfaceSquare* square5 = neigbors[5];
    const int res5 = square5 ? square5->getResolution() : getResolution();

    addGlueIndex( row, col, res5, idx );

    const int blocksize = getBlockSize( res5 );
    const int nrblocks = getNrBlocks( res5 );

    for ( int idy=0; idy<nrblocks; idy++ )
    {
	row += blocksize;
	addGlueIndex( row, col, res5, idx );
    }
}


void SoMeshSurfaceSquare::getNeighbour7CrdIdxs( int& row, int& col, int& idx )
{
    SoMeshSurfaceSquare* square7 = neigbors[7];
    const int res7 = square7 ? square7->getResolution() : getResolution();
    const int blocksize = getBlockSize( res7 );
    const int nrblocks = getNrBlocks( res7 );

    addGlueIndex( row, col, res7, idx );

    for ( int idy=0; idy<nrblocks; idy++ )
    {
	col -= blocksize;
	addGlueIndex( row, col, res7, idx );
    }
}


SbVec3f SoMeshSurfaceSquare::getNormal(int row, int col, int res)
{
    const int relrow=row-origo[0];
    const int relcol=col-origo[1];

    if ( relrow==sidesize || relcol==sidesize )
    {
	SoMeshSurfaceSquare* neigbor = 0;
	if ( relrow==sidesize )
	    neigbor = neigbors[relcol==sidesize?8:7];
	else 
	    neigbor = neigbors[5];

	if ( neigbor ) return neigbor->getNormal( row, col, res );
    }

    SoMeshSurfaceBrick* brick = getBrick(res);

    const int normalindex = brick->getNormalIndex(relrow,relcol);
    if ( !brick->isNormalValid(normalindex) );
	brick->computeNormal( normalindex );

    return brick->normals[normalindex];
}


SoMeshSurfaceBrick* SoMeshSurfaceSquare::getBrick(int resolution)
{
    SoSeparator* sep = resolution<triswitchptr->getNumChildren()
		? (SoSeparator*) triswitchptr->getChild(resolution)
		: 0;
    if ( !sep ) return 0;
    return (SoMeshSurfaceBrick*) sep->getChild(0);
}


const SoMeshSurfaceBrick* SoMeshSurfaceSquare::getBrick(int resolution) const
{
    return const_cast<SoMeshSurfaceSquare*>(this)->getBrick(resolution);
}


SoMeshSurfaceBrickWire* SoMeshSurfaceSquare::getWire(int resolution)
{
    SoSeparator* sep = resolution<wireswitchptr->getNumChildren()
		? (SoSeparator*) wireswitchptr->getChild(resolution)
		: 0;
    return sep && sep->getNumChildren() 
		? (SoMeshSurfaceBrickWire*) sep->getChild(0)
		: 0;
}


const SoMeshSurfaceBrickWire* SoMeshSurfaceSquare::getWire(int resolution)const
{
    return const_cast<SoMeshSurfaceSquare*>(this)->getWire(resolution);
}


void SoMeshSurfaceSquare::addPickCB( SoMeshSurfaceSquareCB* cb, void* data )
{
    if ( !pickcallbacks )
    {
	pickcallbacks = new SoCallbackList;
	SoEventCallback* eventc =
	    (SoEventCallback*) getAnyPart("eventCatcher",true);
	eventc->addEventCallback( SoMouseButtonEvent::getClassTypeId(),
				  pickCB, this );
    }

    pickcallbacks->addCallback( (SoCallbackListCB *)cb, data );
}


void SoMeshSurfaceSquare::removePickCB( SoMeshSurfaceSquareCB* cb, void* data )
{
    if ( !pickcallbacks ) return;

    pickcallbacks->removeCallback( (SoCallbackListCB *)cb, data );
}


void SoMeshSurfaceSquare::getPickedRowCol( int& row, int& col ) const
{
    row = pickedrow; col=pickedcol;
}


void SoMeshSurfaceSquare::pickCB( void* ptr, SoEventCallback* ecb )
{
    if ( ecb->isHandled() ) return;

    SoMeshSurfaceSquare* thisp = (SoMeshSurfaceSquare*) ptr;
    if ( !thisp->pickcallbacks ) return;

    const SoPickedPoint* pickedpoint = ecb->getPickedPoint();
    if ( !pickedpoint || !pickedpoint->isOnGeometry() ) return;

    const SoMouseButtonEvent* event =
			(const SoMouseButtonEvent*) ecb->getEvent();

    if ( event->getButton()!=SoMouseButtonEvent::BUTTON1 ) return;
    if ( !SoMouseButtonEvent::isButtonPressEvent( event, event->getButton() ))
	return;

    const SoPath* path = pickedpoint->getPath();
    if ( !path || !path->containsNode( thisp ) )
	return;

    const SoFaceDetail* detail =
	dynamic_cast<const SoFaceDetail*>(pickedpoint->getDetail() );
    if ( !detail ) return;

    SbVec3f pickedvector;
    pickedpoint->getWorldToObject().multVecMatrix(pickedpoint->getPoint(),
	    					 pickedvector );

    const int nrpoints = detail->getNumPoints();
    if ( !nrpoints ) return;

    const SoPointDetail* pointdetail = detail->getPoint(0);
    float maxdist;
    int closestidx;

    for ( int idx=0; idx<nrpoints; idx++ )
    {
	const int coordidx = pointdetail[idx].getCoordinateIndex();
	const SbVec3f facevec = thisp->coordptr->point[coordidx];
	const float dist = (facevec-pickedvector).length();
	if ( dist<maxdist || !idx )
	{
	    maxdist = dist;
	    closestidx = coordidx;
	}
    }

    thisp->pickedrow = closestidx/(thisp->sidesize+1)+thisp->origo[0];
    thisp->pickedcol = closestidx%(thisp->sidesize+1)+thisp->origo[1];

    ecb->setHandled();
    thisp->pickcallbacks->invokeCallbacks(thisp);
}


void SoMeshSurfaceSquare::GLRender(SoGLRenderAction* action)
{
    reshaschanged = false;
    SoBaseKit::GLRender( action );
}


void SoMeshSurfaceSquare::getBoundingBox(SoGetBoundingBoxAction* action)
{
    computeBBox();
    if ( bboxcache->isEmpty() ) return;

    action->extendBy( *bboxcache );
    action->setCenter( bboxcache->getCenter(), true );
}


void SoMeshSurfaceSquare::computeBBox()
{
    if ( !bboxcache )
    {
	bboxcache = new SbBox3f;
	const int nrcoords = coordptr->point.getNum();
	for ( int idx=0; idx<nrcoords; idx++ )
	{
	    const SbVec3f point = coordptr->point[idx];
	    const float x = point[0];
	    if ( x>9.99999e29 && x<1.00001e30 )
		continue;

	    bboxcache->extendBy( point );
	}
    }
}


int SoMeshSurfaceSquare::getBlockSize( int resolution ) const
{
    return get2Power(sizepower.getValue()-resolution-1);
}


int SoMeshSurfaceSquare::getNrBlocks( int resolution ) const
{
    return get2Power(resolution+1);
}


int SoMeshSurfaceSquare::get2Power( int nr )
{
    int res = 1;
    for ( int idx=nr; idx; idx-- )
	res *= 2;

    return res;
}


void SoMeshSurfaceSquare::sizePowerCB( void* object, SoSensor* )
{
    SoMeshSurfaceSquare* square = (SoMeshSurfaceSquare*) object;

    if ( square->sizepower.getValue()==square->triswitchptr->getNumChildren() )
	return;

    const int sidesize = get2Power( square->sizepower.getValue() );
    square->sidesize = sidesize;
    const int numcoords = (sidesize+1)*(sidesize+1);

    square->coordptr->point.setNum(numcoords);
    square->texturecoordptr->point.setNum(numcoords);
    SbVec3f* coordptr = square->coordptr->point.startEditing();
    SbVec2f* tcoordptr = square->texturecoordptr->point.startEditing();

    const SbVec3f udefval(  SoMeshSurface::undefVal(),
	    		    SoMeshSurface::undefVal(),
			    SoMeshSurface::undefVal() );
    const SbVec2f udefval2( 0, 0 );
    for ( int idx=0; idx<numcoords; idx++ )
    {
	coordptr[idx] = udefval;
	tcoordptr[idx] = udefval2;
    }

    square->coordptr->point.finishEditing();
    square->texturecoordptr->point.finishEditing();

    square->triswitchptr->removeAllChildren();
    square->wireswitchptr->removeAllChildren();
    for ( int idx=0; idx<square->sizepower.getValue(); idx++ )
    {
	SoSeparator* wiresep = new SoSeparator;
	square->wireswitchptr->addChild( wiresep );
	SoMeshSurfaceBrickWire* wire = new SoMeshSurfaceBrickWire;
	wiresep->addChild( wire );

	SoSeparator* trisep = new SoSeparator;
	square->triswitchptr->addChild( trisep );
	SoMeshSurfaceBrick* brick = new SoMeshSurfaceBrick;
	trisep->addChild( brick );

	const int spacing =
	    		square->get2Power( square->sizepower.getValue()-1-idx );
	brick->spacing.setValue( spacing );
	brick->sideSize.setValue( sidesize/spacing-1 );
	brick->setCoordPtr( square->coordptr->point.getValues(0) );
	brick->coordIndex.deleteValues(0);

	wire->spacing.setValue( spacing );
	wire->sideSize.setValue( sidesize/spacing-1 );
	wire->setCoordPtr( square->coordptr->point.getValues(0) );
	wire->coordIndex.deleteValues(0);
    }

    square->setResolution( 0 );
}
