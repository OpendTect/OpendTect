/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.1 2002-09-09 06:54:31 niclas Exp $";

#include "emfault.h"
#include "geomgridsurfaceimpl.h"

EarthModel::Fault::Fault(EarthModel::EMManager & emm_, const MultiID& mid_)
    : EMObject( emm_, mid_ )
    , surface ( *new Geometry::GridSurfaceImpl )
{ }


EarthModel::Fault::~Fault()
{
    delete &surface;
}


EarthModel::PosID EarthModel::Fault::setPos(int row, int col,
					    const Geometry::Pos& pos )
{
    const Geometry::GridNode node( row, col );
    surface.setPos( node, pos );
	
    EarthModel::PosID result;
    result.subid = surface.getPosId(node);
    result.objid = id();
    
    return result;    
}    


Geometry::Pos EarthModel::Fault::getPos( int row, int col ) const
{
    const Geometry::GridNode node( row, col );
    return surface.getPos( node );
}	


EarthModel::PosID EarthModel::Fault::addPosOnRow( int row, bool start,
						  const Geometry::Pos& pos )
{
    EarthModel::PosID result;
    if ( row>=surface.firstRow() || row<=surface.lastRow() )
    {
	const int rowid = row-surface.firstRow();

	int col;
	if ( start ) col = surface.firstCol(rowid)-1;
	else col = surface.lastCol(rowid)+1;

	const Geometry::GridNode node( row, col );

	surface.setPos( node, pos );
	result.subid = surface.getPosId(node);
	result.objid = id();
    }

    return result;
}
    
EarthModel::PosID EarthModel::Fault::insertPosOnRow( int row, int column,
						      bool moveup,
						      const Geometry::Pos& pos )
{
    EarthModel::PosID result;
    if ( row>=surface.firstRow() || row<=surface.lastRow() )
    {
	const int rowid = row-surface.firstRow();

	if ( moveup )
	{
	    for ( int curcol=surface.lastCol(rowid); curcol>=column; curcol-- )
	    {
		Geometry::GridNode sourceNode( row, curcol );
		Geometry::GridNode destNode( row, curcol+1 );

		surface.setPos( destNode, surface.getPos( sourceNode ) );
	    }
	}
	else
	{
  	    for ( int curcol=surface.firstCol(rowid); curcol<=column; curcol++ )
	    {
		Geometry::GridNode sourceNode( row, curcol );
		Geometry::GridNode destNode( row, curcol-1 );
	    }
	}

	Geometry::GridNode node( row, column );
	surface.setPos( node, pos );
	result.subid = surface.getPosId(node);
	result.objid = id();
    }

    return result;
}


void EarthModel::Fault::insertRow( int row, bool moveup )
{   
    float undef = surface.undefVal();
    Geometry::Pos undefpos;
    undefpos.x = undef;
    undefpos.y = undef;
    undefpos.z = undef;
    
    if ( moveup )   
    {	
	for ( int currow = surface.lastRow(); currow>=row-1; currow-- )
    	{	
    	    const int currowid = currow-surface.firstRow();
	    for ( int curcol = surface.firstCol(currowid); 
		  curcol<=surface.lastCol(currowid); curcol++ )
	    {
		Geometry::GridNode sourceNode( currow, curcol );
		Geometry::GridNode destNode( currow+1, curcol );

		Geometry::Pos sourcePos = surface.getPos(sourceNode);
		surface.setPos( destNode, sourcePos );
		surface.setPos( sourceNode, undefpos );
	    }
	}
    }
    else
    {	
	for ( int currow = surface.firstRow(); currow<=row+1; currow++ )
	{    
	    const int currowid = currow-surface.firstRow();
	    for ( int curcol = surface.firstCol(currowid); 
		     curcol<=surface.lastCol(currowid); curcol++ )
	    {
		Geometry::GridNode sourceNode( currow, curcol );
		Geometry::GridNode destNode( currow+1, curcol );
		    
		Geometry::Pos sourcePos = surface.getPos(sourceNode);
		surface.setPos( destNode, sourcePos );
		surface.setPos( sourceNode, undefpos );
	    }
	}
    }

    surface.shrink();
}
    
    
