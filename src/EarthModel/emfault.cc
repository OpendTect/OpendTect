/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.3 2002-09-20 06:39:44 kristofer Exp $";

#include "emfault.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "emfaulttransl.h"
#include "geomgridsurfaceimpl.h"

EarthModel::Fault::Fault(EarthModel::EMManager & emm_, const MultiID& mid_)
    : EMObject( emm_, mid_ )
    , surface ( 0 )
{ }


EarthModel::Fault::~Fault()
{
    delete surface;
}


EarthModel::PosID EarthModel::Fault::setPos(int row, int col,
					    const Geometry::Pos& pos )
{
    const RowCol node( row, col );
    surface->setGridPos( node, pos );
	
    EarthModel::PosID result;
    result.subid = surface->getPosId(node);
    result.objid = id();
    
    return result;    
}    


Geometry::Pos EarthModel::Fault::getPos( int row, int col ) const
{
    const RowCol node( row, col );
    return surface->getGridPos( node );
}	


EarthModel::PosID EarthModel::Fault::addPosOnRow( int row, bool start,
						  const Geometry::Pos& pos )
{
    EarthModel::PosID result;
    if ( row>=surface->firstRow() || row<=surface->lastRow() )
    {
	const int rowid = row-surface->firstRow();

	int col;
	if ( start ) col = surface->firstCol(rowid)-1;
	else col = surface->lastCol(rowid)+1;

	const RowCol node( row, col );

	surface->setGridPos( node, pos );
	result.subid = surface->getPosId(node);
	result.objid = id();
    }

    return result;
}
    
EarthModel::PosID EarthModel::Fault::insertPosOnRow( int row, int column,
						      bool moveup,
						      const Geometry::Pos& pos )
{
    EarthModel::PosID result;
    if ( row>=surface->firstRow() || row<=surface->lastRow() )
    {
	const int rowid = row-surface->firstRow();

	if ( moveup )
	{
	    for ( int curcol=surface->lastCol(rowid); curcol>=column; curcol-- )
	    {
		RowCol sourceNode( row, curcol );
		RowCol destNode( row, curcol+1 );

		surface->setGridPos( destNode, surface->getGridPos( sourceNode ) );
	    }
	}
	else
	{
  	    for ( int curcol=surface->firstCol(rowid); curcol<=column; curcol++ )
	    {
		RowCol sourceNode( row, curcol );
		RowCol destNode( row, curcol-1 );
	    }
	}

	RowCol node( row, column );
	surface->setGridPos( node, pos );
	result.subid = surface->getPosId(node);
	result.objid = id();
    }

    return result;
}


void EarthModel::Fault::insertRow( int row, bool moveup )
{   
    float undef = surface->undefVal();
    Geometry::Pos undefpos;
    undefpos.x = undef;
    undefpos.y = undef;
    undefpos.z = undef;
    
    if ( moveup )   
    {	
	for ( int currow = surface->lastRow(); currow>=row-1; currow-- )
    	{	
    	    const int currowid = currow-surface->firstRow();
	    for ( int curcol = surface->firstCol(currowid); 
		  curcol<=surface->lastCol(currowid); curcol++ )
	    {
		RowCol sourceNode( currow, curcol );
		RowCol destNode( currow+1, curcol );

		Geometry::Pos sourcePos = surface->getGridPos(sourceNode);
		surface->setGridPos( destNode, sourcePos );
		surface->setGridPos( sourceNode, undefpos );
	    }
	}
    }
    else
    {	
	for ( int currow = surface->firstRow(); currow<=row+1; currow++ )
	{    
	    const int currowid = currow-surface->firstRow();
	    for ( int curcol = surface->firstCol(currowid); 
		     curcol<=surface->lastCol(currowid); curcol++ )
	    {
		RowCol sourceNode( currow, curcol );
		RowCol destNode( currow+1, curcol );
		    
		Geometry::Pos sourcePos = surface->getGridPos(sourceNode);
		surface->setGridPos( destNode, sourcePos );
		surface->setGridPos( sourceNode, undefpos );
	    }
	}
    }

    surface->shrink();
}


Executor* EarthModel::Fault::loader()
{
    if ( surface ) delete surface;
    surface = new Geometry::GridSurfaceImpl;

    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EarthModelFaultTranslator::reader( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}

    
Executor* EarthModel::Fault::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    Executor* exec = EarthModelFaultTranslator::writer( *this, ioobj, errmsg);
    if ( errmsg[0] )
    {
	delete exec;
	exec = 0;
    }

    return exec;
}

    
