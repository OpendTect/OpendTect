/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.35 2003-10-15 15:15:54 bert Exp $";

#include "emhorizon.h"

#include "arrayndimpl.h"
#include "emhistoryimpl.h"
#include "emhorizontransl.h"
#include "emsurfauxdataio.h"
#include "emmanager.h"
#include "executor.h"
#include "geomgridsurface.h"
#include "grid.h"
#include "ioman.h"
#include "ioobj.h"
#include "linsolv.h"
#include "ptrman.h"
#include "survinfo.h"

EM::Horizon::Horizon(EMManager& man, const MultiID& id_)
    : Surface( man, id_ )
    , a11( 1 ) , a12( 0 ) , a13( 0 ) , a21( 0 ) , a22( 1 ) , a23( 0 )
    , b11( 1 ) , b12( 0 ) , b13( 0 ) , b21( 0 ) , b22( 1 ) , b23( 0 )
{}


EM::Horizon::~Horizon()
{ }


BinID EM::Horizon::getBinID( const EM::SubID& subid )
{
    return getBinID( subID2RowCol(subid) );
}


BinID EM::Horizon::getBinID( const RowCol& rc )
{
    return BinID(rc.row, rc.col);
}



Geometry::MeshSurface* EM::Horizon::createPatchSurface( const PatchID& patchid )
    									   const
{
    Geometry::GridSurface* newsurf = new Geometry::GridSurface();
    const RowCol rc00( 0, 0 );
    const RowCol rc10( 1, 0 );
    const RowCol rc11( 1, 1 );

    const RowCol surfrc00 = subID2RowCol( getSurfSubID(rc00,patchid) );
    const RowCol surfrc10 = subID2RowCol( getSurfSubID(rc10,patchid) );
    const RowCol surfrc11 = subID2RowCol( getSurfSubID(rc11,patchid) );

    const Coord pos00 = SI().transform(BinID(surfrc00.row,surfrc00.col));
    const Coord pos10 = SI().transform(BinID(surfrc10.row,surfrc10.col));
    const Coord pos11 = SI().transform(BinID(surfrc11.row,surfrc11.col));
    
    newsurf->setTransform(  pos00.x, pos00.y, rc00.row, rc00.col,
			    pos10.x, pos10.y, rc10.row, rc10.col,
			    pos11.x, pos11.y, rc11.row, rc11.col );
    return newsurf;
}


Executor* EM::Horizon::loader( const EM::SurfaceIODataSelection* newsel,
       			       int attridx )
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<EMHorizonTranslator> tr = mTranslCreate(EMHorizon,mDGBKey);
    if ( !tr || !tr->startRead(*ioobj) )
	{ errmsg = tr ? tr->errMsg() : "Cannot find Translator"; return 0; }

    EM::SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selpatches = newsel->selpatches;
    }

    if ( attridx < 0 )
    {
	Executor* exec = tr->reader( *this );
	errmsg = tr->errMsg();
	return exec;
    }

    StreamConn* conn =dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
    if ( !conn ) return 0;
    
    const char* attrnm = sel.sd.valnames[attridx]->buf();
    int gap = 0;
    for ( int idx=0; ; idx++ )
    {
	if ( gap > 50 ) return 0;
	BufferString fnm = 
	    EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx);
	if ( File_isEmpty(fnm) ) { gap++; continue; }
	else gap = 0;

	EM::dgbSurfDataReader* rdr = new EM::dgbSurfDataReader(fnm);
	if ( strcmp(attrnm,rdr->dataName()) )
	{ delete rdr; continue; }

	rdr->setSurface( *this );
	return rdr;
    }

    return 0;
}


Executor* EM::Horizon::saver( const EM::SurfaceIODataSelection* newsel,
       			      bool auxdata, const MultiID* key )
{
    const MultiID& mid = key && !(*key=="") ? *key : id();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<EMHorizonTranslator> tr = mTranslCreate(EMHorizon,mDGBKey);
    if ( !tr || !tr->startWrite(*this) )
	{ errmsg = tr ? tr->errMsg() : "No Translator"; return 0; }

    EM::SurfaceIODataSelection& sel = tr->selections();
    if ( newsel )
    {
	sel.rg = newsel->rg;
	sel.selvalues = newsel->selvalues;
	sel.selpatches = newsel->selpatches;
    }

    if ( auxdata )
    {
	StreamConn* conn =dynamic_cast<StreamConn*>(ioobj->getConn(Conn::Read));
	if ( !conn ) return 0;

	BufferString fnm;
	int dataidx = sel.selvalues.size() ? sel.selvalues[0] : 0;
	if ( dataidx >=0 )
	{
	    fnm = EM::dgbSurfDataWriter::createHovName( conn->fileName(),
		    					dataidx );
	}
	else
	{
	    for ( int idx=0; ; idx++ )
	    {
		fnm =EM::dgbSurfDataWriter::createHovName(conn->fileName(),idx);
		if ( !File_exists(fnm) )
		    break;
	    }
	}

	Executor* exec = new EM::dgbSurfDataWriter(*this,0,0,false,fnm);
	return exec;
    }
    else
    {
	Executor* exec = tr->writer(*ioobj);
	errmsg = tr->errMsg();
	return exec;
    }
}


namespace EM
{
class HorizonImporter : public Executor
{
public:
    HorizonImporter( EM::Horizon& hor, const Grid& g )
	: Executor("Horizon Import")
	, horizon( hor )
	, grid( g )
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
	crlrange.step = abs(bid00.crl-bid11.crl);

	const RowCol step( inlrange.step, crlrange.step );
	const RowCol origo(bid00.inl,bid00.crl);
	horizon.setTranslatorData( step, step, origo, 0, 0 );
	patch = hor.addPatch( g.name(), true );

	inl = inlrange.start;
    }

    int		totalNr() const { return inlrange.nrSteps()+1; }
    int		nrDone() const { return inlrange.getIndex(inl); }
    const char*	nrDoneText() const { return "Gridlines imported"; }
    int		nextStep()
		{
		    if ( inl>inlrange.stop )
			return Finished;

		    for ( int crl=crlrange.start; crl<=crlrange.stop;
			      crl+=crlrange.step )
		    {
			const Coord coord = SI().transform(BinID(inl,crl));
			const GridNode gridnode = grid.getNode(coord);
			const float val = grid.getValue( gridnode );
			if ( mIsUndefined(val) )
			    continue;

			const EM::SubID subid =
				    horizon.rowCol2SubID(RowCol(inl,crl));

			Coord3 pos(coord.x, coord.y, val );
			horizon.setPos( patch, subid, pos, true, false );
		    }

		    inl += inlrange.step;
		    return MoreToDo;
		}


protected:
    EM::Horizon&	horizon;
    const Grid&		grid;
    StepInterval<int>	inlrange;
    StepInterval<int>	crlrange;

    int			inl;
    EM::PatchID		patch;
};

}; // namespace EM



Executor* EM::Horizon::import( const Grid& grid, int idx )
{
    if ( !idx ) cleanUp();

    return new EM::HorizonImporter( *this, grid );
}
