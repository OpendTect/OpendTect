/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/


#include "seistrc.h"
#include "seiscbvs.h"
#include "seisselectionimpl.h"
#include "conn.h"
#include "iostrm.h"
#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include "strmprov.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int mProgMainFnName( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " inpfile outpfile\n";
	std::cerr << "Format input: CBVS ; Format ouput: x y z v [v ...]"
		  << std::endl;
	return 1;
    }

    FilePath fp( argv[1] );

    if ( !File::exists(fp.fullPath()) )
    {
        std::cerr << fp.fullPath() << " does not exist" << std::endl;
        return 1;
    }

    if ( !fp.isAbsolute() )
    {
        fp.insert( File::getCurrentPath() );
    }

    BufferString fname=fp.fullPath();


    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ std::cerr << tri->errMsg() << std::endl; return 1; }

    fp.set( argv[2] );
    if ( !fp.isAbsolute() ) { fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

    StreamData outsd = StreamProvider::createOStream( fname );
    if ( !outsd.usable() )
        { std::cerr << "Cannot open output file" << std::endl; return 1; }

    SeisTrc trc;
    int nrwr = 0;
    int nrlwr = 0;
    while ( tri->read(trc) )
    {
	const int nrcomps = trc.data().nrComponents();
	const int nrsamps = trc.size();
	Coord coord = trc.info().coord;
	for ( int isamp=0; isamp<nrsamps; isamp++ )
	{
	    for ( int icomp=0; icomp<nrcomps; icomp++ )
	    {
		*outsd.ostrm << coord.x << ' ' << coord.y << ' '
			     << trc.samplePos(isamp) << ' '
			     << trc.get(isamp,icomp) << '\n';
		nrlwr++;
	    }
	}
	nrwr++;
    }

    std::cerr << nrwr << " traces written to " << nrlwr << " lines."
		<< std::endl;
    return nrwr ? 0 : 1;
}
