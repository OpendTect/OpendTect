/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoMeshSurfaceSquare.cc,v 1.8 2003-10-09 11:59:34 kristofer Exp $";


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

    origo.set1Value(1,0);

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


void SoMeshSurfaceSquare::setTextureRange( int firstrow, int firstcol,
	                                   int lastrow, int lastcol )
{
    const int nrrows = lastrow-firstrow+1;
    const float rowinc = nrrows!=1 ? 1.0 / (nrrows-1) : 1;
    const int nrcols = lastcol-firstcol+1;
    const float colinc = nrcols!=1 ? 1.0 / (nrcols-1) : 1;

    const float startrowval = rowinc*(origo[0]-firstrow);
    const float startcolval = colinc*(origo[1]-firstcol);

    SbList<float> colcoords(sidesize+1);

    for ( int idy=0; idy<=sidesize; idy++ )
    {
	float colcoord = idy*colinc+startcolval;
	if ( colcoord<0 ) colcoord=0;
	else if ( colcoord>1 ) colcoord=1;
	colcoords[idy] = colcoord;
    }

    SbVec2f* tcoords = texturecoordptr->point.startEditing();

    for ( int idx=0; idx<=sidesize; idx++ )
    {
	float rowcoord = idx*rowinc+startrowval;
	if ( rowcoord<0 ) rowcoord=0;
	else if ( rowcoord>1 ) rowcoord=1;

	for ( int idy=0; idy<=sidesize; idy++ )
	{
	    tcoords[idx*(sidesize+1)+idy] = SbVec2f( rowcoord, colcoords[idy] );
	}
    }

    texturecoordptr->point.finishEditing();
}


void SoMeshSurfaceSquare::setPos( int row, int col, const SbVec3f& np )
       			
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



bool SoMeshSurfaceSquare::setResolution( int nr )
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


int SoMeshSurfaceSquare::computeResolution( SoState* state )
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
	    		complexity*screensize[0]*screensize[1] / 32;

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
    if ( (relrow==sidesize || relcol==sidesize) &&
	 relrow>=0 && relrow<=sidesize &&
	 relcol>=0 && relcol<=sidesize )
    {
	const int index = getCoordIndex( row, col );
	int neigborindex = -1;

	if ( neigbors[8] && relrow==sidesize && relcol==sidesize )
	    neigborindex = 8;
	else if ( neigbors[5] && relcol==sidesize )
	    neigborindex = 5;
	else if ( neigbors[7] && relrow==sidesize )
	    neigborindex = 7;

	if ( neigborindex!=-1 )
	{
	    const SbVec3f np = neigbors[neigborindex]->getPos(row,col);
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


#define mAddCoordToIndexes(row_,col_,res_) \
{ \
    const int index_ = getCoordIndex( row_, col_ ); \
    if ( !SoMeshSurface::isUndefined(coordptr->point[index_])) \
    { \
	indexes.push(index_); \
	normals.push(getNormal(row_,col_,res_)); \
    } \
}

void SoMeshSurfaceSquare::updateGlue()
{
    const int ownres = getResolution();
    const int ownblocksize = getBlockSize( ownres );
    if ( ownres==-1 )
	return;

    if ( updateglue || (neigbors[5] && neigbors[5]->hasResolutionChanged()) ||
	 (neigbors[7] && neigbors[7]->hasResolutionChanged()) ||
	 hasResolutionChanged() )
    {
	updateglue = false;
	int normalindex=0;
	int coordindex = 0;

	const int res5 = neigbors[5] ? neigbors[5]->getResolution() : ownres;
	const int blocksize5 = getBlockSize(res5);
	const int res7 = neigbors[7] ? neigbors[7]->getResolution() : ownres;
	const int blocksize7 = getBlockSize(res7);
	const int res8 = neigbors[8] ? neigbors[8]->getResolution() : res7;

	SbList<SbVec2s> rowgluecells;
	const int rowglueblocksize = ownblocksize>blocksize7
	    				? ownblocksize : blocksize7;
	int row = origo[0]+sidesize-ownblocksize;
	int col = origo[1];
	for ( int idx=0; idx<sidesize-ownblocksize; idx+=rowglueblocksize )
	{
	    rowgluecells.push( SbVec2s(row,col+idx) );
	}

	SbList<SbVec2s> colgluecells;
	const int colglueblocksize = ownblocksize>blocksize5
					? ownblocksize : blocksize5;

	row = origo[0];
	col = origo[1]+sidesize-ownblocksize;
	for ( int idx=0; idx<sidesize-ownblocksize; idx+=colglueblocksize )
	{
	    colgluecells.push( SbVec2s(row+idx,col) );
	}

	//Make corner.
	if ( res5>=ownres || res7>=ownres )
	{
	    if ( res5<ownres )
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
	    else if ( res7<ownres )
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
	    else
	    {
		rowgluecells.push(SbVec2s(origo[0]+sidesize-ownblocksize,
					  origo[1]+sidesize-ownblocksize));
	    }
	}
	else
	{
	    /*	The resolution on both neigbors are lower than our, so
	     *	our corner will look like this:
		2--3
		|  |
	     6--1  |
	     |     |
	     5-----4

	    */
	    SbList<int> indexes;
	    SbList<SbVec3f> normals;
	    row = origo[0]+sidesize-ownblocksize;
	    col = origo[1]+sidesize-ownblocksize;
	    mAddCoordToIndexes( row, col, ownres );

	    const int minrow = origo[0]+sidesize-blocksize5;
	    while ( row>minrow )
	    {
		row-=ownblocksize;
		mAddCoordToIndexes( row, col, ownres );
		const int cellindex = colgluecells.find(SbVec2s(row,col));
		if ( cellindex!=-1 ) colgluecells.removeFast(cellindex);
	    }

	    col+=ownblocksize;
	    mAddCoordToIndexes( row, col, res5 );
	    row+=blocksize5;
	    mAddCoordToIndexes( row, col, res8 );

	    col-=blocksize7;
	    mAddCoordToIndexes( row, col, res7 );
	    row-=ownblocksize;
	    mAddCoordToIndexes( row, col, ownres );

	    const int cellindex = rowgluecells.find(SbVec2s(row,col));
	    if ( cellindex!=-1 ) rowgluecells.removeFast(cellindex);

	    const int maxcol = origo[1]+sidesize-ownblocksize*2;
	    while ( col<maxcol )
	    {
		col += ownblocksize;
		mAddCoordToIndexes( row, col, ownres );
		const int cellindex = rowgluecells.find(SbVec2s(row,col));
		if ( cellindex!=-1 ) rowgluecells.removeFast(cellindex);
	    }

	    addGlueShape( coordindex, normalindex, indexes, normals );
	}

	while ( rowgluecells.getLength() )
	{
	    const SbVec2s rc = rowgluecells.pop();
	    row = rc[0];
	    const int startcol = col = rc[1];

	    SbList<int> indexes;
	    SbList<SbVec3f> normals;
	    const int maxcol = col+rowglueblocksize;
	    const int bordercol = origo[1]+sidesize;
	    mAddCoordToIndexes( row, col, ownres );
	    while ( col<maxcol )
	    {
		col += ownblocksize;
		if ( col==bordercol )
		{
		    mAddCoordToIndexes( row, col, res5 );
		}
		else
		{
		    mAddCoordToIndexes( row, col, ownres );
		}
	    }

	    row +=ownblocksize;
	    if ( col==bordercol )
	    {
		mAddCoordToIndexes( row, col, res8 );
	    }
	    else
	    {
		mAddCoordToIndexes( row, col, res7 );
	    }

	    while ( col>startcol )
	    {
		col -= blocksize7;
		mAddCoordToIndexes( row, col, res7 );
	    }

	    addGlueShape( coordindex, normalindex, indexes, normals );
	}


	while ( colgluecells.getLength() )
	{
	    const SbVec2s rc = colgluecells.pop();
	    const int startrow = row = rc[0];
	    col = rc[1];

	    SbList<int> indexes;
	    SbList<SbVec3f> normals;

	    mAddCoordToIndexes( row, col, ownres );

	    col +=ownblocksize;
	    mAddCoordToIndexes( row, col, res5 );

	    const int maxrow = row+colglueblocksize;
	    const int borderrow = origo[1]+sidesize;
	    while ( row<maxrow )
	    {
		row += blocksize5;
		if ( row==borderrow )
		{
		    mAddCoordToIndexes( row, col, res8 );
		}
		else
		{
		    mAddCoordToIndexes( row, col, res5 );
		}
	    }

	    col -=ownblocksize;
	    if ( row==borderrow )
	    {
		mAddCoordToIndexes( row, col, res7 );
	    }
	    else
	    {
		mAddCoordToIndexes( row, col, ownres );
	    }

	    while ( row-ownblocksize>startrow )
	    {
		row -= ownblocksize;
		mAddCoordToIndexes( row, col, ownres );
	    }

	    addGlueShape( coordindex, normalindex, indexes, normals );
	}

	glueptr->coordIndex.setNum(coordindex);
	glueptr->normalIndex.setNum(coordindex);
	glueptr->textureCoordIndex.setNum(coordindex);
	gluenormalptr->vector.setNum(normalindex);
    }
}


void SoMeshSurfaceSquare::addGlueShape( int& coordindexidx, int& normalidx,
					const SbList<int>& coordindexes,
       					const SbList<SbVec3f>& normals )
{
    const int nrindexes = coordindexes.getLength();
    if ( nrindexes<3 ) return;

    for ( int idx=0; idx<nrindexes; idx++ )
    {
	gluenormalptr->vector.set1Value(normalidx, normals[idx]);

	const int coordindex = coordindexes[idx];
	glueptr->coordIndex.set1Value( coordindexidx, coordindex );
	glueptr->textureCoordIndex.set1Value( coordindexidx, coordindex );
	glueptr->normalIndex.set1Value( coordindexidx, normalidx );

	coordindexidx++;
	normalidx++;
    }

    glueptr->coordIndex.set1Value( coordindexidx, -1 );
    glueptr->textureCoordIndex.set1Value( coordindexidx, -1 );
    glueptr->normalIndex.set1Value( coordindexidx, -1 );

    coordindexidx++;
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
