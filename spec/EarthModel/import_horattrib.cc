/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2005
________________________________________________________________________

-*/

#include "prog.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "emsurfacegeometry.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "position.h"
#include "strmprov.h"
#include "survinfo.h"


static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: Horizon_ID attribfile attribname ic/xy";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}


static int prError( const char* msg )
{
    std::cerr << msg << std::endl;
    return 1;
}


static EM::Horizon3D* loadHorizon( const char* id, BufferString& err )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) { err = "Horizon "; err += id; err += " not OK"; return 0; }

    std::cerr << "Reading " << ioobj->name() << " ..." << std::endl;
    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( ioobj->key() );
    exec->execute( &std::cerr );
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobj->key()) );
    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon ) { err = "ID "; err += id; err += " is not horizon"; }
    return horizon;
}


int mProgMainFnName( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();

    BufferString errmsg;
    EM::Horizon3D* horizon = loadHorizon( argv[1], errmsg );
    if ( errmsg != "" ) return prError( errmsg );

    StreamData sd = StreamProvider::createIStream( argv[2] );
    if ( !sd.usable() ) return prError( "input_XYV_file not OK" );

    BufferString attribname = argv[3];
    const int dataidx = horizon->auxdata.addAuxData( attribname );
    if ( dataidx < 0 ) return prError( "Cannot add datavalues" );

    const bool doxy = !strcmp("xy",argv[4]);

    EM::PosID posid( horizon->id() );
    while ( sd.istrm->good() )
    {
	BinID bid;
	float val;
	if ( doxy )
	{
	    Coord crd;
	    *sd.istrm >> crd.x >> crd.y >> val;
	    bid = SI().transform( crd );
	}
	else
	    *sd.istrm >> bid.inl >> bid.crl >> val;

	posid.setSubID( bid.getSerialized() );
	if ( horizon->isDefined(posid) )
	    horizon->auxdata.setAuxDataVal( dataidx, posid, val );
    }
    sd.close();

    std::cerr << "Saving data ..." << std::endl;
    PtrMan<Executor> saver = horizon->auxdata.auxDataSaver();
    saver->execute();
    return 0;
}

