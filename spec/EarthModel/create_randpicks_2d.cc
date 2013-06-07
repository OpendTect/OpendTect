/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: create_randpicks_2d.cc,v 1.9 2010/10/14 09:58:06 cvsbert Exp $";

#include "prog.h"

#include "conn.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emsurfacegeometry.h"
#include "errh.h"
#include "executor.h"
#include "ioobj.h"
#include "ioman.h"
#include "posinfo.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "statrand.h"
#include "strmprov.h"
#include "survinfo.h"

#include <math.h>


#define mErrRet(err) { std::cerr << err << std::endl; return 1; }

static int doWork( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " lineset_id horizon_id nrpicks "
	    	     "xyz_filename " << std::endl;
	return 1;
    }

    Stats::RandGen::init();

    const int nrpicks = toInt( argv[3] );
    BufferString fname( argv[4] );
    StreamData outsd = StreamProvider( fname ).makeOStream();
    if ( !outsd.usable() )
	mErrRet( "Cannot open output file" );

    PtrMan<IOObj> ioobj = IOM().get( argv[1] );
    if ( !ioobj ) mErrRet( "Lineset ID not OK" );
    PtrMan<Seis2DLineSet> lset = new Seis2DLineSet( ioobj->fullUserExpr(true) );
    if ( !lset )
	mErrRet( "Cannot find input lineset" );
    if ( lset->nrLines()==0 )
	mErrRet( "Input lineset is empty" );

    PtrMan<IOObj> horioobj = IOM().get( argv[2] );
    if ( !horioobj ) mErrRet( "Horizon ID not OK" );
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( horioobj->key() );
    exec->execute( &std::cerr );
    EM::EMObject* emobj = em.getObject( em.getObjectID(horioobj->key()) );
    mDynamicCastGet(EM::Horizon*,horizon,emobj)
    if ( !horizon ) mErrRet( "ID is not horizon" );

//  Get 2D geometry
    ObjectSet<PosInfo::Line2DData> geoms;
    for ( int lineidx=0; lineidx<lset->nrLines(); lineidx++ )
    {
	PosInfo::Line2DData* geometry = new PosInfo::Line2DData;
	if ( !lset->getGeometry(lineidx,*geometry) )
	{
	    delete geometry;
	    continue;
	}

	geoms += geometry;
    }

    TypeSet<Coord3> picklocations;
    const int nrlines = geoms.size();
    int nrpicksadded = 0;
    while ( true )
    {
	const int lineidx = Stats::RandGen::getIndex( nrlines );
	PosInfo::Line2DData& geometry = *geoms[lineidx];
	const int nrcoords = geometry.posns.size();
	const int crdidx = Stats::RandGen::getIndex( nrcoords );
	const Coord& pos = geometry.posns[crdidx].coord_;

	const BinID bid = SI().transform( pos );
	const EM::SectionID sid = horizon->sectionID( 0 );
	const float zpos = horizon->getPos( sid, bid.getSerialized() ).z;
	if ( mIsUdf(zpos) ) continue;

	const Coord3 pickpos( pos, zpos );
	if ( picklocations.indexOf(pickpos) < 0 )
	    picklocations += pickpos;

	if ( picklocations.size() == nrpicks )
	    break;
    }

    for ( int idx=0; idx<picklocations.size(); idx++ )
    {
	const Coord3& pos = picklocations[idx];
	*outsd.ostrm << pos.x << ' ' << pos.y << ' ' << pos.z << '\n';
    }

    outsd.close();
    std::cerr << nrpicks << " positions written to " << fname.buf() 
	      << std::endl;

    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
