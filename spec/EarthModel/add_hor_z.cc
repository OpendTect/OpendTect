/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "emsurfacegeometry.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "prog.h"
#include "position.h"
#include "ioman.h"
#include "ioobj.h"
#include "strmprov.h"
#include "sets.h"
#include "bufstring.h"
#include "ptrman.h"
#include "survinfo.h"
#include "executor.h"
#include "keystrs.h"
#include <iostream>

static int prUsage( const char* msg = 0 )
{
    std::cerr << "Usage: Horizon_ID input_XY_file output_XYZ_file [--incudf]";
    if ( msg ) std::cerr << '\n' << msg;
    std::cerr << std::endl;
    return 1;
}

static int doWork( int argc, char** argv )
{
    if ( argc < 4 ) return prUsage();
    bool incudf = argc > 4 && strcmp(argv[4],"--incudf");

    IOObj* ioobj = IOM().get( argv[1] );
    if ( !ioobj ) return prUsage( "Horizon_ID not OK" );
    MultiID ioobjkey( ioobj->key() );
    delete ioobj;

    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> exec = em.objectLoader( MultiID(argv[1]) );
    exec->execute( &std::cerr );
    EM::EMObject* emobj = em.getObject( em.getObjectID(ioobjkey) );
    mDynamicCastGet(EM::Horizon*,horizon,emobj)
    if ( !horizon ) return prUsage( "ID is not horizon" );

    StreamData sd = StreamProvider(argv[2]).makeIStream();
    if ( !sd.usable() ) return prUsage( "input_XY_file not OK" );

    Coord c0;
    TypeSet<Coord> coords;
    while ( sd.istrm->good() )
    {
	Coord c;
	*sd.istrm >> c.x >> c.y;
	double sqdist = c.sqDistTo( c0 );
	if ( mIsZero(sqdist,1e-4) )
	    break;
	coords += c;
    }
    sd.close();
    if ( coords.size() < 1 ) return prUsage( "No valid coordinates found" );

    sd = StreamProvider(argv[3]).makeOStream();
    if ( !sd.usable() ) return prUsage( "output_XYZ_file not OK" );

    for ( int idx=0; idx<coords.size(); idx++ )
    {
	Coord c( coords[idx] );
	const BinID bid = SI().transform( c );
	const EM::SectionID sid = horizon->sectionID( 0 );
	const float zpos = horizon->getPos( sid, bid.getSerialized() ).z;
	if ( mIsUdf(zpos) && !incudf ) continue;

	BufferString str;
	str += c.x; str += "\t"; str += c.y; str += "\t";
	str += mIsUdf(zpos) ? BufferString(sKey::FloatUdf) : BufferString(zpos);
	*sd.ostrm << str << std::endl;
    }

    return 0;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
