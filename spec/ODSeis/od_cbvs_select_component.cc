/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/

static const char* rcsID = "$Id";

#include "seistrc.h"
#include "seiscbvs.h"
#include "conn.h"
#include "iostrm.h"
#include "strmprov.h"
#include "filegen.h"
#include "filepath.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int main( int argc, char** argv )
{
    if ( argc < 4 )
    {
	std::cerr << "Usage: " << argv[0] << " component inpfile outpfile"<< std::endl
	     << "Format: CBVS." << std::endl
	     << "set component to -1 for polar dip, -2 for azimuth"
	     << " (2 components required!)" << std::endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[2]) )
    {
	std::cerr << argv[2] << " does not exist" << std::endl;
	exitProgram( 1 );
    }

    const int selcomp = atoi( argv[1] );

    BufferString fname( argv[2] );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }
    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { std::cerr << tri->errMsg() << std::endl; exitProgram( 1 ); }

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
	= tri->componentInfo();
    const int nrincomp = ci.size();
    if ( selcomp >= 0 )
	for ( int idx=0; idx<nrincomp; idx++ )
	    ci[idx]->destidx = idx == selcomp ? idx : -1;

    fname = argv[3];
    fp.set( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }

    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();
    SeisTrc trc;
    SeisTrc& outtrc = selcomp < 0 ? *new SeisTrc : trc;
    int nrwr = 0;
    while ( tri->read(trc) )
    {
	if ( selcomp < 0 )
	{
	    // assume user knows that this is a dip cube:

	    const int trcsz = trc.size(0);
	    if ( !nrwr ) outtrc.reSize( trcsz, 0 );
	    for ( int idx=0; idx<trcsz; idx++ )
	    {
		float inldip = trc.get(idx,0);
		float crldip = trc.get(idx,1);
		outtrc.set( idx, selcomp == -2 ? atan2(inldip,crldip)
			       : sqrt( inldip*inldip + crldip*crldip ), 0 );
	    }
	}

	if ( selcomp < 0 )
	    outtrc.info() = trc.info();

	if ( !nrwr && !tro->initWrite(
		    	new StreamConn(fname,Conn::Write),outtrc) )
	    { std::cerr << "Cannot start write!" << std::endl; exitProgram(1); }

	if ( !tro->write(outtrc) )
	    { std::cerr << "Cannot write!" << std::endl; exitProgram(1); }

	nrwr++;
    }

    std::cerr << nrwr << " traces written.";
    exitProgram( nrwr ? 0 : 1 ); return 0;
}
