/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 2000
 * RCS      : $Id$
-*/

#include "seistrc.h"
#include "seiscbvs.h"
#include "cubesampling.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int mProgMainFnName( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    if ( argc < 1 )
    {
	od_cout() << "Usage: " << argv[0]
	          << " inpfile\n";
	od_cout() << "Format input: CBVS ; Format ouput: inl crl v [v ...]"
		  << od_endl;
	return 1;
    }

    File::Path fp( argv[1] );

    if ( !File::exists(fp.fullPath()) )
    {
        od_cout() << fp.fullPath() << " does not exist" << od_endl;
        return 1;
    }

    if ( !fp.isAbsolute() )
    {
        fp.insert( File::getCurrentPath() );
    }

    BufferString fname=fp.fullPath();


    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ od_cout() << tri->errMsg() << od_endl; return 1; }

    fp.set( argv[2] );
    if ( !fp.isAbsolute() )
	{ fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

    od_ostream outstrm( fname );
    if ( !outstrm.isOK() )
        { od_cout() << "Cannot open output file" << od_endl; return 1; }

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
		if ( trc.get(isamp,icomp) > 10000 )
		{
		    outstrm << trc.info().binid.inl << ' '
			     << trc.info().binid.crl << ' '
			     << trc.get(isamp,icomp) << ' '<<'\n'<< od_endl;
		}
		else if (trc.get(isamp,icomp) < -10000 )
		{
		    outstrm << trc.info().binid.inl << ' '
			     << trc.info().binid.crl << ' '
			     << trc.get(isamp,icomp) << ' '<<'\n'<< od_endl;
		}
		nrlwr++;
	    }
	}
	nrwr++;
    }

    return nrwr ? 0 : 1;
}
