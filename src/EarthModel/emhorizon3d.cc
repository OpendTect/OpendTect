/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.50 2004-12-17 12:31:09 bert Exp $";

#include "emhorizon.h"

#include "emsurfacetr.h"
#include "emsurfauxdataio.h"
#include "emmanager.h"
#include "geomgridsurface.h"
#include "executor.h"
#include "grid.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "settings.h"
#include "trigonometry.h"
#include "filegen.h"

#include <math.h>

namespace EM {

Horizon::Horizon( EMManager& man, const ObjectID& id_ )
    : Surface(man,id_,*new HorizonGeometry(*this))
{}


class HorizonImporter : public Executor
{
public:

HorizonImporter( Horizon& hor, const Grid& g, bool fh )
	: Executor("Horizon Import")
	, horizon( hor )
	, grid( g )
	, fixholes( fh )
{
    const int nrrows = grid.nrRows();
    const int nrcols = grid.nrCols();
    for ( int row=0; row<nrrows; row++ )
    {
	for ( int col=0; col<nrcols; col++ )
	{
	    GridNode gridnode( col, row );
	    const Coord coord = grid.getCoord( gridnode );
	    const BinID bid = SI().transform(coord);

	    if ( !row && !col )
	    {
		inlrange.start = bid.inl; inlrange.stop = bid.inl;
		crlrange.start = bid.crl; crlrange.stop = bid.crl;
	    }
	    else
	    {
		inlrange.include( bid.inl );
		crlrange.include( bid.crl );
	    }
	}
    }


    const GridNode node00( 0, 0 );
    const GridNode node11( 1, 1 );

    const BinID bid00 = SI().transform(grid.getCoord( node00 ));
    const BinID bid11 = SI().transform(grid.getCoord( node11 ));

    inlrange.step = abs(bid00.inl-bid11.inl);
    if ( !inlrange.step ) inlrange.step = 1;
    crlrange.step = abs(bid00.crl-bid11.crl);
    if ( !crlrange.step ) crlrange.step = 1;

    const RowCol step( inlrange.step, crlrange.step );
    const RowCol origo(bid00.inl,bid00.crl);
    horizon.geometry.setTranslatorData( step, step, origo, 0, 0 );
    section = hor.geometry.addSection( g.name(), true );

    inl = inlrange.start;
}

const char*	message() const { return "Transforming grid data"; }
int		totalNr() const { return inlrange.nrSteps()+1; }
int		nrDone() const { return inlrange.getIndex(inl); }
const char*	nrDoneText() const { return "Gridlines imported"; }

int nextStep()
{
    if ( inl>inlrange.stop )
    {
	if ( fixholes )
	{
	    bool changed = false;
	    StepInterval<int> rowrange, colrange;
	    horizon.geometry.getRange(rowrange,true);
	    horizon.geometry.getRange(colrange,false);

	    for ( int currow=rowrange.start; rowrange.includes(currow);
		  currow+=rowrange.step )
	    {
		for ( int curcol=colrange.start; colrange.includes(curcol);
		      curcol+=colrange.step )
		{
		    const RowCol rc( currow, curcol );
		    if ( horizon.geometry.isDefined(section,rc) )
			continue;

		    PosID pid(horizon.id(),section,
			      horizon.geometry.rowCol2SubID(rc));

		    TypeSet<PosID> neighbors;
		    horizon.geometry.getNeighbors( pid, &neighbors );
		    if ( neighbors.size()<6 )
			continue;

		    TypeSet<Coord3> neighborcoords;
		    for ( int nidx=0; nidx<neighbors.size(); nidx++ )
			neighborcoords += horizon.getPos(neighbors[nidx]);

		    Plane3 plane;
		    if ( !plane.set(neighborcoords) )
			continue;

		    const BinID bid = HorizonGeometry::getBinID(rc);
		    const Coord coord = SI().transform(bid);
		    const Line3 line( Coord3(coord,0), Vector3(0,0,1));

		    Coord3 interpolcoord;
		    if ( !plane.intersectWith(line,interpolcoord) )
			continue;

		    horizon.setPos(pid,interpolcoord,false);
		    changed = true;
		}
	    }

	    if ( changed )
		return MoreToDo;
	}

	return Finished;
    }

    for ( int crl=crlrange.start; crl<=crlrange.stop;
	      crl+=crlrange.step )
    {
	const BinID bid(inl,crl);
	const Coord coord = SI().transform(bid);
	const GridNode gridnode = grid.getNode(coord);
	const float val = grid.getValue( gridnode );
	if ( mIsUndefined(val) )
	    continue;

	Coord3 pos(coord.x, coord.y, val );
	horizon.geometry.setPos( section, HorizonGeometry::getRowCol(bid), pos,
			true, false );
    }

    inl += inlrange.step;
    return MoreToDo;
}


protected:

    Horizon&	horizon;
    const Grid&		grid;
    StepInterval<int>	inlrange;
    StepInterval<int>	crlrange;
    float		udfval;

    int			inl;
    bool		fixholes;
    SectionID		section;
};


Executor* Horizon::importer( const Grid& grid, int idx, bool fh )
{
    if ( !idx ) cleanUp();

    return new HorizonImporter( *this, grid, fh );
}


const IOObjContext& Horizon::getIOObjContext() const
{ return EMHorizonTranslatorGroup::ioContext(); }


HorizonGeometry::HorizonGeometry( Surface& surf )
    : SurfaceGeometry( surf ) 
{}


bool HorizonGeometry::createFromStick( const TypeSet<Coord3>& stick,
				       const SectionID& sid, float velocity )
{
    SectionID sectionid = sid;
    if ( !nrSections() || !hasSection(sid) ) 
	sectionid = addSection( "", true );

    const float idealdistance = 25; // TODO set this in some intelligent way

    for ( int idx=0; idx<stick.size()-1; idx++ )
    {
	const Coord3 startpos( stick[idx], stick[idx].z*velocity/2 );
	const Coord3 stoppos( stick[idx+1], stick[idx+1].z*velocity/2 );
	if ( !startpos.isDefined() || !stoppos.isDefined() )
	    break;

	const BinID startbid = SI().transform( startpos );
	const BinID stopbid = SI().transform( stoppos );

	const RowCol startrc = getRowCol(startbid);
	const RowCol stoprc = getRowCol(stopbid);
	const Line3 line( Coord3(startrc.row,startrc.col,0),
	     Coord3(stoprc.row-startrc.row,stoprc.col-startrc.col,0) );

	RowCol iterstep( step_ );
	if ( startrc.row==stoprc.row ) iterstep.row = 0;
	else if ( startrc.row>stoprc.row ) iterstep.row = -iterstep.row;

	if ( startrc.col==stoprc.col ) iterstep.col = 0;
	else if ( startrc.col>stoprc.col ) iterstep.col = -iterstep.col;

	RowCol rowcol = startrc;
	while ( rowcol != stoprc )
	{
	    const float rowdistbefore = rowcol.row-startrc.row;
	    const float coldistbefore = rowcol.col-startrc.col;
	    const float distbefore = sqrt(rowdistbefore*rowdistbefore+
					  coldistbefore*coldistbefore );
	    const float rowdistafter = rowcol.row-stoprc.row;
	    const float coldistafter = rowcol.col-stoprc.col;
	    const float distafter = sqrt(rowdistafter*rowdistafter+
					  coldistafter*coldistafter );

	    const Coord3 newpos(SI().transform(getBinID(rowcol)),
		    (startpos.z*distafter+stoppos.z*distbefore)/
		    (distbefore+distafter)/(velocity/2));

	    const PosID posid( surface.id(), sectionid,
				   rowCol2SubID(rowcol) );
	    setPos( posid, newpos, true );
	    if ( rowcol == startrc )
		surface.setPosAttrib( posid, EMObject::sPermanentControlNode, true);

	    float distance = mUndefValue;
	    RowCol nextstep;
	    if ( iterstep.row )
	    {
		distance = line.distanceToPoint(
			Coord3(rowcol.row+iterstep.row, rowcol.col,0) );
		nextstep = rowcol;
		nextstep.row += iterstep.row;
	    }

	    if ( iterstep.col )
	    {
		float nd = line.distanceToPoint(
			    Coord3(rowcol.row, rowcol.col+iterstep.col,0) );
		if ( nd<distance )
		{
		    nextstep = rowcol;
		    nextstep.col += iterstep.col;	
		}
	    }

	    rowcol = nextstep;
	}

	if ( idx==stick.size()-2 )
	{
	    const PosID posid( surface.id(), sectionid,
				    rowCol2SubID(rowcol));
	    setPos( posid, Coord3( stoppos, stoppos.z/(velocity/2)), true);

	    surface.setPosAttrib( posid, EMObject::sPermanentControlNode, true );
	}
    }

    return true;
}


BinID HorizonGeometry::getBinID( const SubID& subid )
{
    return getBinID( SurfaceGeometry::subID2RowCol(subid) );
}


BinID HorizonGeometry::getBinID( const RowCol& rc )
{
    return BinID(rc.row, rc.col);
}


RowCol HorizonGeometry::getRowCol( const BinID& bid )
{
    return RowCol( bid.inl, bid.crl );
}


SubID HorizonGeometry::getSubID( const BinID& bid )
{
    return rowCol2SubID(getRowCol(bid));
}


Geometry::MeshSurface*
HorizonGeometry::createSectionSurface( const SectionID& sectionid )
    									   const
{
    Geometry::GridSurface* newsurf = new Geometry::GridSurface();
    const RowCol rc00( 0, 0 );
    const RowCol rc10( 1, 0 );
    const RowCol rc11( 1, 1 );

    const RowCol surfrc00 = subID2RowCol( getSurfSubID(rc00,sectionid) );
    const RowCol surfrc10 = subID2RowCol( getSurfSubID(rc10,sectionid) );
    const RowCol surfrc11 = subID2RowCol( getSurfSubID(rc11,sectionid) );

    const Coord pos00 = SI().transform(BinID(surfrc00.row,surfrc00.col));
    const Coord pos10 = SI().transform(BinID(surfrc10.row,surfrc10.col));
    const Coord pos11 = SI().transform(BinID(surfrc11.row,surfrc11.col));
    
    newsurf->setTransform(  pos00.x, pos00.y, rc00.row, rc00.col,
			    pos10.x, pos10.y, rc10.row, rc10.col,
			    pos11.x, pos11.y, rc11.row, rc11.col );
    return newsurf;
}



}; //namespace


