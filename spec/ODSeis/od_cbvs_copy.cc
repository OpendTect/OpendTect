/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
 * RCS      : $Id$
-*/

static const char* rcsID = "$Id$";

#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcprop.h"
#include "seisresampler.h"
#include "cubesampling.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


static int doWork( int argc, char** argv )
{
    int argidx = 0;
    if ( argc < 3 )
    {
	FilePath fp( argv[0] );
	std::cerr << "Usage: " << fp.fileName()
		  << " [--2d] [--format 0] [--scale 1] inpfile outpfile"
		  << std::endl;
	return 1;
    }

    argidx++;
    bool is2d = false;
    int fmt = 0;
    bool doscale = false; float scale = 1;
    while ( *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	switch ( *(argv[argidx]+2) )
	{
	    case '2': is2d = true; break;
	    case 'f': fmt = toInt(argv[argidx+1]); argidx++; break;
	    case 's': doscale = true; scale = toFloat(argv[argidx+1]);
		      argidx++; break;
	}
	argidx++;
	if ( !argv[argidx] ) break;
    }

    FilePath fp( argv[argidx] ); argidx++;
    if ( !File::exists(fp.fullPath()) )
    {
        std::cerr << fp.fullPath() << " does not exist" << std::endl;
        return 1;
    }
    else if ( !fp.isAbsolute() )
        fp.insert( File::getCurrentPath() );

    BufferString fname=fp.fullPath();

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    tri->set2D( is2d );
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { std::cerr << tri->errMsg() << std::endl; return 1; }

    fp.set( argv[argidx] ); 
    if ( !fp.isAbsolute() ) { fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();
    tro->set2D( is2d );
    tro->setPreselDataType( fmt );
    tro->packetInfo() = tri->packetInfo();

    SeisTrc trc; int nrwr = 0;
    while ( tri->read(trc) )
    {
	if ( doscale )
	    SeisTrcPropChg(trc).scale( scale );

	if ( nrwr == 0
	  && !tro->initWrite(new StreamConn(fname,Conn::Write),trc) )
	    { std::cerr << tro->errMsg() << std::endl; return 1; }

	if ( !tro->write(trc) )
	    { std::cerr << "Cannot write!" << std::endl; return 1; }

	nrwr++;
    }

    std::cerr << nrwr << " traces written." << std::endl;
    return nrwr ? 0 : 1;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
