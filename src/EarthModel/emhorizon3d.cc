/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: emhorizon3d.cc,v 1.27 2003-07-04 13:32:33 kristofer Exp $";

#include "emhorizon.h"

#include "arrayndimpl.h"
#include "emhistoryimpl.h"
#include "emhorizontransl.h"
#include "emmanager.h"
#include "executor.h"
#include "geom2dsnappedsurface.h"
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



Geometry::GridSurface* EM::Horizon::createPatchSurface() const
{
    Geometry::Snapped2DSurface* newsurf = new Geometry::Snapped2DSurface();
    const RowCol rc00( 0, 0 );
    const RowCol rc10( 1, 0 );
    const RowCol rc11( 1, 1 );

    const RowCol surfrc00 = subID2RowCol(getSurfSubID( rc00 ));
    const RowCol surfrc10 = subID2RowCol(getSurfSubID( rc10 ));
    const RowCol surfrc11 = subID2RowCol(getSurfSubID( rc11 ));

    const Coord pos00 = SI().transform(BinID(surfrc00.row,surfrc00.col));
    const Coord pos10 = SI().transform(BinID(surfrc10.row,surfrc10.col));
    const Coord pos11 = SI().transform(BinID(surfrc11.row,surfrc11.col));
    
    newsurf->setTransform(  pos00.x, pos00.y, rc00.row, rc00.col,
			    pos10.x, pos10.y, rc10.row, rc10.col,
			    pos11.x, pos11.y, rc11.row, rc11.col );
    return newsurf;
}


Executor* EM::Horizon::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<Translator> tr_ = ioobj->getTranslator();
    EMHorizonTranslator* tr =
			dynamic_cast<EMHorizonTranslator*>((Translator*)tr_);

    if ( !tr->startRead(*ioobj,*this) )
	{ errmsg = tr->errMsg(); return 0; }

    Executor* exec = tr->reader();
    errmsg = tr->errMsg();
    return exec;
}


Executor* EM::Horizon::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( id() );
    if ( !ioobj )
	{ errmsg = "Cannot find the horizon object"; return 0; }

    PtrMan<Translator> tr_ = ioobj->getTranslator();
    EMHorizonTranslator* tr =
			dynamic_cast<EMHorizonTranslator*>((Translator*)tr_);
    if ( !tr->startWrite(*this) )
	{ errmsg = tr->errMsg(); return 0; }

    Executor* exec = tr->writer(*ioobj);
    errmsg = tr->errMsg();
    return exec;
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
	, patch( hor.addPatch(0, true) )
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
    const EM::PatchID	patch;
};
};



Executor* EM::Horizon::import( const Grid& grid )
{
    cleanUp();

    return new EM::HorizonImporter( *this, grid );
}
