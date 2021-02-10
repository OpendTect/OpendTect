/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

#include "prog.h"

#include "dbman.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "initearthmodel.h"
#include "ioobjctxt.h"
#include "linesetposinfo.h"
#include "od_ostream.h"
#include "position.h"
#include "ptrman.h"
#include "survinfo.h"


struct HorLine2D
{
    TypeSet<Coord>		pos_;
    TypeSet<int>		traces_;
    TypeSet< TypeSet<float> >	zvals_;

    BufferString		linename_;
};


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 1;
}


static bool isLineUsed( const ObjectSet<const EM::Horizon2D>& horizons,
			const char* linenm )
{
    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	const int lineidx = horizons[idx]->geometry().lineIndex( linenm );
	if ( lineidx >= 0 ) return true;
    }

    return false;
}


int mProgMainFnName( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: LineSet_ID output_ascii_filenm [udfval]"
		  << std::endl;
	return 1;
    }

    EarthModel::initStdClasses();

    PtrMan<IOObj> ioobj = DBKey(argv[1]).getIOObj();
    if ( !ioobj )
	return prError( "No lineset found" );

    const float udfval = argc==4 ? toFloat(argv[3]) : mUdf(float);

    Seis2DLineSet s2dls( *ioobj );
    PosInfo::LineSet2DData lsdata;
    s2dls.getGeometry( lsdata );

    IOObjContext ctxt = EMHorizon2DTranslatorGroup::ioContext();
    DBM().to( ctxt.getSelDirID() );
    DBDirEntryList list( DBM().dirPtr(), ctxt );
    DBKeySet horids;
    for ( int idx=0; idx<list.size(); idx++ )
	horids += list.key( idx );

    PtrMan<Executor> loader = EM::Hor2DMan().objectLoader( horids, 0 );
    if ( !loader->execute() )
	return prError( "Cannot load horizons" );

    ObjectSet<const EM::Horizon2D> horizons;
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	mDynamicCastGet(const EM::Horizon2D*,hor,
			EM::Hor2DMan().getObject(horids[idx]))
	if ( !hor ) continue;

	bool doadd = false;
	for ( int lineidx=0; lineidx<lsdata.nrLines(); lineidx++ )
	{
	    const BufferString& linenm = lsdata.lineName( lineidx );
	    if ( hor->geometry().lineIndex(linenm.buf()) >= 0 )
	    { doadd = true; break; }
	}

	if ( doadd )
	    horizons += hor;
    }

    if ( horizons.isEmpty() ) return prError( "No valid horizons found" );

    BufferString fnm( argv[2] );
    od_ostream outstrm( fnm );
    if ( !outstrm.isOK() )
	return prError( "Cannot open output file" );

    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	if ( idx>0 )
	    outstrm << '\t';
	outstrm << horizons[idx]->name();
    }
    outstrm << od_endl;

    BinID bid;
    for ( int lineidx=0; lineidx<lsdata.nrLines(); lineidx++ )
    {
	const BufferString& linenm = lsdata.lineName( lineidx );
	const PosInfo::Line2DData& linedata = lsdata.lineData( lineidx );
	for ( int posidx=0; posidx<linedata.posns.size(); posidx++ )
	{
	    if ( !isLineUsed(horizons,linenm.buf()) )
		continue;

	    outstrm << linenm.buf() << '\t'
		    << linedata.posns[posidx].nr_ << '\t';
	    outstrm << toString( linedata.posns[posidx].coord_.x ) << '\t';
	    outstrm << toString( linedata.posns[posidx].coord_.y );
	    bid.crl = linedata.posns[posidx].nr_;
	    for ( int horidx=0; horidx<horizons.size(); horidx++ )
	    {
		const EM::Horizon2D* hor = horizons[horidx];
		bid.inl = hor->geometry().lineIndex( linenm.buf() );
		Coord3 crd = Coord3::udf();
		if ( bid.inl>=0 )
		    crd = hor->getPos( hor->sectionID(0), bid.getSerialized() );
		const float val = mIsUdf(crd.z) ? udfval : crd.z;
		outstrm << '\t' << toString(val);
	    }
	    outstrm << od_endl;
	}
    }

    return 0;
}
