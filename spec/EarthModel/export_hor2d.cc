/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: export_hor2d.cc,v 1.6 2010/10/14 09:58:06 cvsbert Exp $";

#include "prog.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emsurfacetr.h"
#include "initearthmodel.h"
#include "iodirentry.h"
#include "ioman.h"
#include "linesetposinfo.h"
#include "position.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "strmprov.h"
#include "survinfo.h"
#include "rcol.h"
#include "executor.h"


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


static int doWork( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: LineSet_ID output_ascii_filenm [udfval]"
	    	  << std::endl;
	return 1;
    }

    PtrMan<IOObj> ioobj = IOM().get( argv[1] );
    if ( !ioobj ) return prError( "No lineset found" );

    const float udfval = argc==4 ? toFloat(argv[3] ) : mUdf(float);

    Seis2DLineSet s2dls( *ioobj );
    PosInfo::LineSet2DData lsdata;
    s2dls.getGeometry( lsdata );

    IOObjContext ctxt = EMHorizon2DTranslatorGroup::ioContext();
    IOM().to( ctxt.getSelKey() );
    IODirEntryList list( IOM().dirPtr(), ctxt );
    TypeSet<MultiID> horids;
    for ( int idx=0; idx<list.size(); idx++ )
	horids += list[idx]->ioobj->key();

    PtrMan<Executor> loader = EM::EMM().objectLoader( horids, 0 );
    if ( !loader->execute() )
	return prError( "Cannot load horizons" );

    ObjectSet<const EM::Horizon2D> horizons;
    for ( int idx=0; idx<horids.size(); idx++ )
    {
	EM::ObjectID emobjid = EM::EMM().getObjectID( horids[idx] );
	mDynamicCastGet(const EM::Horizon2D*,hor,EM::EMM().getObject(emobjid))
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
    StreamData sdo = StreamProvider(fnm).makeOStream();
    if ( !sdo.usable() )
	return prError( "Cannot open output file" );

    std::ostream& outstrm = *sdo.ostrm;
    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	if ( idx>0 ) outstrm << '\t';
	outstrm << horizons[idx]->name();
    }
    outstrm << std::endl;

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
	    outstrm << getStringFromDouble( 0, linedata.posns[posidx].coord_.x )
		    << '\t';
	    outstrm << getStringFromDouble( 0, linedata.posns[posidx].coord_.y);
	    bid.crl = linedata.posns[posidx].nr_;
	    for ( int horidx=0; horidx<horizons.size(); horidx++ )
	    {
		const EM::Horizon2D* hor = horizons[horidx];
		bid.inl = hor->geometry().lineIndex( linenm.buf() );
		Coord3 crd = Coord3::udf();
		if ( bid.inl>=0 )
		    crd = hor->getPos( hor->sectionID(0), bid.getSerialized() );
		const float val = mIsUdf(crd.z) ? udfval : crd.z;
		outstrm << '\t' << getStringFromFloat(0,val);
	    }
	    outstrm << std::endl;
	}
    }

    return 0;
}


int main( int argc, char** argv )
{
    EarthModel::initStdClasses();
    return ExitProgram( doWork(argc,argv) );
}
