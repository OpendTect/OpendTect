/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoMeshSurface.cc,v 1.12 2003-12-04 16:00:18 nanne Exp $";

#include "SoMeshSurface.h"

#include "SoMeshSurfaceSquare.h"
#include "SoMeshSurfaceBrick.h"
#include "SoCameraInfoElement.h"
#include "SoCameraInfo.h"

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>

#include <Inventor/lists/SoCallbackList.h>

#include <Inventor/sensors/SoFieldSensor.h>

#include "Inventor/threads/SbThreadAutoLock.h"
#include "Inventor/threads/SbCondVar.h"
#include "Inventor/threads/SbThread.h"
#include "Inventor/misc/SoChildList.h"

SO_NODE_SOURCE(SoMeshSurface);

void SoMeshSurface::initClass()
{
    SO_NODE_INIT_CLASS(SoMeshSurface, SoGroup, "MeshSurface" );
}


SoMeshSurface::SoMeshSurface()
    : setupMutex( new SbMutex )
    , creatorqueMutex( new SbMutex )
    , creationcondvar( new SbCondVar )
    , partSizePowerSensor(
	    new SoFieldSensor( SoMeshSurface::partSizePowerCB, this ))
    , squaresize( -1 ) 
    , weAreStopping( false )
    , pickcallbacks( 0 )
    , texturerangeisset( false )
{
    SO_NODE_CONSTRUCTOR(SoMeshSurface);


    SO_NODE_ADD_FIELD( whichResolution, (-1) );
    SO_NODE_ADD_FIELD( partSizePower, (6) );

    partSizePowerSensor->attach(&partSizePower);
    partSizePowerSensor->trigger();
}


SoMeshSurface::~SoMeshSurface()
{
    stopThreads();
    delete creationcondvar;
    delete creatorqueMutex;
    delete setupMutex;

    partSizePowerSensor->detach();
    delete partSizePowerSensor;
    delete pickcallbacks;
}


void SoMeshSurface::setPos( int row, int col, const SbVec3f& pos )
{
    SbThreadAutoLock lock( setupMutex );
    if ( !getNumChildren() )
	makeFirstSquare( row, col );

    while ( col<firstcol )
	addSquareCol(true);
    while ( col>=firstcol+nrcolsquares*squaresize )
	addSquareCol(false);

    while ( row<firstrow )
	addSquareRow(true);
    while ( row>=firstrow+nrRows()*squaresize )
	addSquareRow(false);

    const int index = getSquareIndex( row, col );
    if ( index>=0 )
	getSquare(index)->setPos( row, col, pos );
}


SbVec3f SoMeshSurface::getPos( int row, int col )
{
    const int index = getSquareIndex( row, col );
    return index>=0 ? getSquare(index)->getPos( row, col )
		    : SbVec3f(undefVal(),undefVal(),undefVal());
}


void SoMeshSurface::removePos( int row, int col )
{
    SbThreadAutoLock lock( setupMutex );

    const int index = getSquareIndex( row, col );
    if ( index<0 )
	return;

    getSquare( index )->removePos( row, col );
}


void SoMeshSurface::setTextureRange(int firstrow, int firstcol,
				    int lastrow, int lastcol )
{
    if ( texturerangeisset &&
	    firstrow==texturefirstrow &&
	    firstcol==texturefirstcol &&
	    lastrow==texturelastrow &&
	    lastcol==texturelastcol )
	return;
    
    const int nrsquares = getNumChildren();
    for ( int idx=0; idx<nrsquares; idx++ )
	getSquare(idx)->setTextureRange(firstrow,firstcol,lastrow,lastcol);

    texturerangeisset = true;
    texturefirstrow = firstrow;
    texturefirstcol = firstcol;
    texturelastrow = lastrow;
    texturelastcol = lastcol;
}


void SoMeshSurface::allocSpace( int newrowstart, int newrowstop,
				int newcolstart, int newcolstop )
{
    SbThreadAutoLock lock( setupMutex );

    if ( firstrow>= newrowstop || firstrow+nrRows()*squaresize<newrowstart ||
	 firstcol>= newcolstop || firstcol+nrcolsquares*squaresize<newcolstart )
    {
	removeAllChildren();
    }

    if ( !getNumChildren() )
	makeFirstSquare( newrowstart, newrowstop );

    while ( newcolstart<firstcol )
	addSquareCol(true);
    while ( newcolstart>firstcol+nrcolsquares*squaresize )
	addSquareCol(false);
    while ( newcolstop<firstcol )
	addSquareCol(true);
    while ( newcolstop>firstcol+nrcolsquares*squaresize )
	addSquareCol(false);

    while ( newrowstart<firstrow )
	addSquareRow(true);
    while ( newrowstart>firstrow+nrRows()*squaresize )
	addSquareRow(false);
    while ( newrowstop<firstrow )
	addSquareRow(true);
    while ( newrowstop>firstrow+nrRows()*squaresize )
	addSquareRow(false);
}


void SoMeshSurface::turnOnWireFrame( bool yn )
{
    const int nrsquares = getNumChildren();
    for ( int idx=0; idx<nrsquares; idx++ )
	getSquare(idx)->turnOnWireFrame( yn );
}


bool SoMeshSurface::isWireFrameOn() const
{
    const int nrsquares = getNumChildren();
    return nrsquares && getSquare(0)->isWireFrameOn();
}


void SoMeshSurface::addPickCB( SoMeshSurfaceCB* cb, void* data )
{
    if ( !pickcallbacks )
	pickcallbacks = new SoCallbackList;

    pickcallbacks->addCallback( (SoCallbackListCB *)cb, data );
}


void SoMeshSurface::removePickCB( SoMeshSurfaceCB* cb, void* data )
{
    if ( !pickcallbacks ) return;

    pickcallbacks->removeCallback( (SoCallbackListCB *)cb, data );
}


void SoMeshSurface::pickCB( void* ptr, SoMeshSurfaceSquare* square )
{
    SoMeshSurface* thisp = (SoMeshSurface*) ptr;
    if ( !thisp->pickcallbacks ) return;
    square->getPickedRowCol(thisp->pickedrow, thisp->pickedcol);
    thisp->pickcallbacks->invokeCallbacks( thisp );
}


void SoMeshSurface::getPickedRowCol( int& row, int& col ) const
{
    row = pickedrow; col=pickedcol;
}



void SoMeshSurface::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();
    state->push();

    SbThreadAutoLock lock( setupMutex );

    if ( !(SoCameraInfoElement::get(state)&SoCameraInfo::STEREO) )
    {
	rendersquares.truncate(0);

	const int whichres = whichResolution.getValue();
	const int nrsquares = getNumChildren();
	for ( int idx=0; idx<nrsquares; idx++ )
	{
	    SoMeshSurfaceSquare* square = getSquare(idx);
	    if ( !square->shouldGLRender( action ) )
		continue;

	    rendersquares.append( square );

	    int missingresolution = -1;
	    if ( whichres==-1 )
	    {
		missingresolution = square->computeResolution(state);
	    }
	    else if ( !square->setResolution(whichres) )
	    {
		missingresolution = whichres;
	    }

	    if ( missingresolution!=-1 )
	    {
		SbThreadAutoLock quelock( creatorqueMutex );
		SoMeshSurfaceBrick* brick = square->getBrick(missingresolution);
#ifdef __win__
		brick->build(true);
#else
		const int existingindex = creationquebricks.find( brick );
		if ( existingindex!=-1 )
		{
		    if ( existingindex!=creationquebricks.getLength()-1 )
		    {
			creationquebricks[existingindex] = 0;
			creationquebricks.push( brick );
		    }
		}
		else
		    creationquebricks.push( brick );

		creationcondvar->wakeOne();
#endif
	    }
	}
    }

    const int nrtorender = rendersquares.getLength();
    SoGLCacheContextElement::shouldAutoCache(state,
				SoGLCacheContextElement::DONT_AUTO_CACHE);


    if ( !nrtorender )
	return;

    for ( int idx=0; idx<nrtorender; idx++ )
	rendersquares[idx]->updateGlue();

    for ( int idx=0; idx<nrtorender; idx++ )
	rendersquares[idx]->GLRender( action );

    state->pop();
}


void SoMeshSurface::setUpObject()
{
    SbThreadAutoLock lock( setupMutex );
    squaresize = 1;
    for ( int idx=0; idx<partSizePower.getValue(); idx++ )
	squaresize *=2;

    for ( int idx=0; idx<getNumChildren(); idx++ )
    {
	const SoMeshSurfaceSquare* square = getSquare(idx);

	if ( !idx || firstrow>square->origo[0] )
	    firstrow = square->origo[0];

	if ( !idx || firstcol>square->origo[1] )
	    firstcol = square->origo[1];
    }


    int currentcol;
    for ( int idx=0; idx<getNumChildren(); idx++ )
    {
	const SoMeshSurfaceSquare* square = getSquare(idx);

	if ( !idx || firstrow>square->origo[1]>currentcol )
	    currentcol = square->origo[1];
	else
	{
	    nrcolsquares = idx;
	    break;
	}
    }
}


int SoMeshSurface::getSquareIndex( int row, int col ) const
{
    const int colindex = (col-firstcol)/squaresize;
    if ( colindex>=nrcolsquares || colindex<0 )
	return -1;

    const int rowindex = (row-firstrow)/squaresize;
    if ( rowindex<0 || rowindex>=nrRows() )
	return -1;

    return rowindex*nrcolsquares+colindex;
}


SoMeshSurfaceSquare* SoMeshSurface::getSquare( int squareindex )
{
    return (SoMeshSurfaceSquare*) getChild(squareindex);
}


const SoMeshSurfaceSquare* SoMeshSurface::getSquare( int squareindex ) const
{
    return const_cast<SoMeshSurface*>(this)->getSquare( squareindex );
}


void SoMeshSurface::makeFirstSquare( int row, int col )
{
    firstrow = (row/squaresize)*squaresize;
    firstcol = (col/squaresize)*squaresize;

    nrcolsquares = 1;

    SoMeshSurfaceSquare* square = new SoMeshSurfaceSquare;
    square->origo.set1Value(0, firstrow);
    square->origo.set1Value(1, firstcol);
    square->sizepower = partSizePower.getValue();
    square->addPickCB( pickCB, this );
    if ( texturerangeisset )
	square->setTextureRange( texturefirstrow, texturefirstcol,
				 texturelastrow, texturelastcol );

    addChild(square);
}



void SoMeshSurface::addSquareRow( bool start )
{
    const int insertpoint = start ? 0 : getNumChildren();
    const int row = firstrow + (start?-squaresize:nrRows()*squaresize);

    for ( int idx=0; idx<nrcolsquares; idx++ )
    {
	const int col = idx*squaresize+firstcol;
	SoMeshSurfaceSquare* square = new SoMeshSurfaceSquare;
	square->origo.set1Value(0, row);
	square->origo.set1Value(1, col);
	square->sizepower = partSizePower.getValue();
  	square->addPickCB( pickCB, this );
	if ( texturerangeisset )
	    square->setTextureRange( texturefirstrow, texturefirstcol,
				     texturelastrow, texturelastcol );

	insertChild( square, insertpoint+idx );
    }

    if ( start ) firstrow=row;

    for ( int idx=0; idx<nrcolsquares; idx++ )
    {
	const int index = insertpoint+idx;
	SoMeshSurfaceSquare* square = getSquare(index);

	const int relindex = start ? nrcolsquares : -nrcolsquares;
	const int relrow = start ? -1 : 1;
	if ( idx )
	{
	    getSquare(index+relindex-1)->setNeighbor(relrow,1,square);
	    getSquare(index-1)->setNeighbor(0,1,square);
	}

	getSquare(index+relindex)->setNeighbor(relrow,0,square);

	if ( idx<nrcolsquares-1 )
	{
	    getSquare(index+relindex+1)->setNeighbor(relrow,-1,square);
	    getSquare(index+1)->setNeighbor(0,-1,square);
	}
    }
}


void SoMeshSurface::addSquareCol( bool start )
{
    const int insertpoint = start ? 0 : nrcolsquares;
    const int col = firstcol + (start?-squaresize:nrcolsquares*squaresize);

    const int nrrows = nrRows();

    for ( int idx=0; idx<nrrows; idx++ )
    {
	const int row = idx*squaresize+firstrow;
	SoMeshSurfaceSquare* square = new SoMeshSurfaceSquare;
	square->origo.set1Value(0, row);
	square->origo.set1Value(1, col);
	square->sizepower = partSizePower.getValue();
  	square->addPickCB( pickCB, this );
	if ( texturerangeisset )
	    square->setTextureRange( texturefirstrow, texturefirstcol,
				     texturelastrow, texturelastcol );

	insertChild( square, insertpoint+idx*(nrcolsquares+1) );
    }

    if ( start ) firstcol=col;
    nrcolsquares++;

    for ( int idx=0; idx<nrrows; idx++ )
    {
	const int index = insertpoint+idx*nrcolsquares;
	SoMeshSurfaceSquare* square = getSquare(index);

	const int relcol = start ? -1 : 1;
	if ( idx )
	{
	    getSquare(index-nrcolsquares-relcol)->setNeighbor(1,relcol,square);
	    getSquare(index-nrcolsquares)->setNeighbor(1,0,square);
	}

	getSquare(index-relcol)->setNeighbor(0, relcol, square);

	if ( idx<nrrows-1 )
	{
	    getSquare(index+nrcolsquares-relcol)->setNeighbor(-1,relcol,square);
	    getSquare(index+nrcolsquares)->setNeighbor(1,0,square);
	}
    }
}
   

int SoMeshSurface::nrRows() const
{
    const int nrchildren = getNumChildren();
    return nrchildren ? (nrchildren-1)/nrcolsquares + 1 : 0;
}


void SoMeshSurface::partSizePowerCB( void* object, SoSensor* )
{
    SoMeshSurface* surf = (SoMeshSurface*) object;
    int newsize = 1;
    for ( int idx=0; idx<surf->partSizePower.getValue(); idx++ )
	newsize *= 2;

    if ( newsize==surf->squaresize )
	return;

    surf->squaresize = newsize;

    surf->stopThreads();
    surf->setupMutex->lock();

    surf->removeAllChildren();
    surf->setupMutex->unlock();
    surf->startThreads(1);
}


void SoMeshSurface::stopThreads()
{
    creatorqueMutex->lock();
    creationquebricks.truncate(0);
    weAreStopping = true;
    creationcondvar->wakeAll();
    creatorqueMutex->unlock();

    for ( int idx=0; idx<threads.getLength(); idx++ )
    {
	threads[idx]->join();
	SbThread::destroy(threads[idx]);
    }

    threads.truncate( 0 );
}


void SoMeshSurface::startThreads( int nrthreads )
{
#ifndef __win__
    SbThreadAutoLock lock( creatorqueMutex );
    weAreStopping = false;

    for ( int idx=0; idx<nrthreads; idx++ )
	threads.append( SbThread::create( &creationFunc, this ));

    creationcondvar->wakeAll();
#endif
}


void* SoMeshSurface::creationFunc( void* surf_ )
{
    SoMeshSurface* gridsurf = (SoMeshSurface*) surf_;

    while ( true )
    {
	gridsurf->creatorqueMutex->lock();
	while ( gridsurf->creationquebricks.getLength()==0 &&
		!gridsurf->weAreStopping )
	{
	    gridsurf->creationcondvar->wait(*gridsurf->creatorqueMutex );
	}

	if ( gridsurf->weAreStopping )
	{
	    gridsurf->creatorqueMutex->unlock();
	    break;
	}

	SoMeshSurfaceBrick* brick = gridsurf->creationquebricks.pop();
	while ( !brick && gridsurf->creationquebricks.getLength() )
	    brick = gridsurf->creationquebricks.pop();

	gridsurf->creatorqueMutex->unlock();
	if ( brick )
	    brick->build(true);
    }

    return 0;
}
