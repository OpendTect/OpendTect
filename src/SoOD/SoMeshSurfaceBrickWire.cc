/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoMeshSurfaceBrickWire.cc,v 1.4 2003-10-08 11:54:42 kristofer Exp $";


#include "SoMeshSurfaceBrickWire.h"

#include <Inventor/SoPrimitiveVertex.h>

#include <Inventor/actions/SoGLRenderAction.h>

#include <Inventor/elements/SoNormalElement.h>

SO_NODE_SOURCE(SoMeshSurfaceBrickWire);

void SoMeshSurfaceBrickWire::initClass(void)
{
    SO_NODE_INIT_CLASS( SoMeshSurfaceBrickWire, SoIndexedLineSet,
	    		"IndexedLineSet" );
}


SoMeshSurfaceBrickWire::SoMeshSurfaceBrickWire()
    : coords( 0 )
    , invalidFlag( true )
{
    SO_NODE_CONSTRUCTOR(SoMeshSurfaceBrickWire);

    SO_NODE_ADD_FIELD(sideSize,(32));
    SO_NODE_ADD_FIELD(spacing,(1));
}


void SoMeshSurfaceBrickWire::setCoordPtr( const SbVec3f* crds )
{
    coords = crds;
}


void SoMeshSurfaceBrickWire::build()
{
    if ( isValid() ) return;

    coordIndex.enableNotify(false);
    coordIndex.deleteValues(0);
    int nrcrds = 0;

    const int nrcells = sideSize.getValue();
    for ( int rowidx=0; rowidx<=nrcells+1; rowidx++ )
    {
	for ( int colidx=0; colidx<=nrcells+1; colidx++ )
	{
	    const int coordindex = getCoordIndex(rowidx, colidx);
	    if ( isUndefined(coords[coordindex] ))
	    {
		if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
		{
		    coordIndex.set1Value( nrcrds, -1);
		    nrcrds++;
		}
		continue;
	    }

	    if ( (!nrcrds || coordIndex[nrcrds-1]==-1) &&
	         (colidx>nrcells ||
		  isUndefined(coords[getCoordIndex(rowidx, colidx+1)] )))
	    {
		continue;
	    }

	    coordIndex.set1Value(nrcrds, coordindex);
	    nrcrds++;
	}

	if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
	{
	    coordIndex.set1Value( nrcrds, -1 );
	    nrcrds++;
	}
    }

    for ( int colidx=0; colidx<=nrcells+1; colidx++ )
    {
	for ( int rowidx=0; rowidx<=nrcells+1; rowidx++ )
	{
	    const int coordindex = getCoordIndex(rowidx, colidx);
	    if ( isUndefined(coords[coordindex] ))
	    {
		if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
		{
		    coordIndex.set1Value( nrcrds, -1);
		    nrcrds++;
		}
		continue;
	    }

	    if ( (!nrcrds || coordIndex[nrcrds-1]==-1) &&
	         (rowidx>nrcells ||
		  isUndefined(coords[getCoordIndex(rowidx+1, colidx)] )))
	    {
		continue;
	    }

	    coordIndex.set1Value( nrcrds, coordindex );
	    nrcrds++;
	}

	if ( nrcrds && coordIndex[nrcrds-1]!=-1 )
	{
	    coordIndex.set1Value( nrcrds, -1 );
	    normalIndex.set1Value( nrcrds, -1 );
	    nrcrds++;
	}
    }

    coordIndex.enableNotify(true);
    coordIndex.touch();
    invalidFlag = false;
}


void SoMeshSurfaceBrickWire::invalidate()
{
    invalidFlag = true;
}


SbBool SoMeshSurfaceBrickWire::isValid() const
{
    return !invalidFlag;
}


void SoMeshSurfaceBrickWire::GLRender(SoGLRenderAction* action)
{
    if ( !isValid() )
	build();

    inherited::GLRender(action);
}
