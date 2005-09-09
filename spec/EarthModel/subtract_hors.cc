/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2005
 RCS:           $Id: subtract_hors.cc,v 1.2 2005-09-09 15:44:55 cvsnanne Exp $
________________________________________________________________________

-*/



#include "prog.h"
#include "binidvalset.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "emposid.h"
#include "ranges.h"
#include "position.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "executor.h"


static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: Top_Horizon_ID Bot_Horizon_ID add_res_to[T/B] "
		 "name[Thickness]";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 1;
}


static EM::Horizon* loadHorizon( const char* id, BufferString& err )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) { err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj->key() );
    exec->execute( &std::cerr );
    EM::EMObject* emobj = em.getObject( em.multiID2ObjectID(ioobj->key()) );
    mDynamicCastGet(EM::Horizon*,horizon,emobj)
    if ( !horizon ) { err = "ID "; err += id; err += " is not horizon"; }
    return horizon;
}


static int doWork( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();

    BufferString errmsg;
    EM::Horizon* horizon1 = loadHorizon( argv[1], errmsg );
    if ( errmsg != "" ) return prError( errmsg );
    EM::Horizon* horizon2 = loadHorizon( argv[2], errmsg );
    if ( errmsg != "" ) return prError( errmsg );

    EM::Horizon& addtohor = !strcasecmp(argv[3],"T") ? *horizon1 : *horizon2;

    BufferString attribname = argv[4] ? argv[4] : "Thickness";
    const int dataidx = addtohor.auxdata.addAuxData( attribname );
    if ( dataidx < 0 ) return prError( "Cannot add datavalues" );

    StepInterval<int> inlrg = addtohor.geometry.rowRange();
    StepInterval<int> crlrg = addtohor.geometry.colRange();
    RowCol rc;
    EM::PosID posid( addtohor.id() );
    for ( rc.row=inlrg.start; rc.row<=inlrg.stop; rc.row+=inlrg.step )
    {
	for ( rc.col=crlrg.start; rc.col<=crlrg.stop; rc.col+=crlrg.step )
	{
	    Coord3 pos1 = horizon1->geometry.getPos( EM::SectionID(0), rc );
	    Coord3 pos2 = horizon2->geometry.getPos( EM::SectionID(0), rc );

	    float val = mUdf(float);
	    if ( pos1.isDefined() && pos2.isDefined() )
	    {
		val = pos2.z - pos1.z;
		if ( SI().zIsTime() ) val *= 1000;
	    }

	    posid.setSubID( addtohor.geometry.rowCol2SubID(rc) );
	    addtohor.auxdata.setAuxDataVal( dataidx, posid, val );
	}
    }

    std::cerr << "Saving data ..." << std::endl;
    PtrMan<Executor> saver = addtohor.auxdata.auxDataSaver();
    saver->execute();
    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}

