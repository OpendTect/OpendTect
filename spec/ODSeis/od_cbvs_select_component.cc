/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
 * RCS      : $Id: od_cbvs_select_component.cc,v 1.20 2010/10/14 09:58:06 cvsbert Exp $
-*/

static const char* rcsID = "$Id: od_cbvs_select_component.cc,v 1.20 2010/10/14 09:58:06 cvsbert Exp $";

#include "seistrc.h"
#include "seiscbvs.h"
#include "conn.h"
#include "iostrm.h"
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
    if ( argc < 4 )
    {
	std::cerr << "Usage: " << argv[0]
		  << " component inpfile outpfile\nFormat: CBVS.\n"
		     "set component to -1 for polar dip, -2 for azimuth\n"
		     " (2 components required!)" << std::endl;
	return 1;
    }

    FilePath fp( argv[2] );
    
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

    const int selcomp = toInt( argv[1] );

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { std::cerr << tri->errMsg() << std::endl; return 1; }

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
	= tri->componentInfo();
    const int nrincomp = ci.size();
    if ( selcomp >= 0 )
	for ( int idx=0; idx<nrincomp; idx++ )
	    ci[idx]->destidx = idx == selcomp ? idx : -1;

    fp.set( argv[3] ); 
    if ( !fp.isAbsolute() ) { fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();
    SeisTrc trc;
    SeisTrc& outtrc = selcomp < 0 ? *new SeisTrc : trc;
    int nrwr = 0;
    while ( tri->read(trc) )
    {
	if ( selcomp < 0 )
	{
	    // assume user knows that this is a dip cube:

	    const int trcsz = trc.size();
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
	    { std::cerr << "Cannot start write!" << std::endl; return 1; }

	if ( !tro->write(outtrc) )
	    { std::cerr << "Cannot write!" << std::endl; return 1; }

	nrwr++;
    }

    std::cerr << nrwr << " traces written.";
    return nrwr ? 0 : 1;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
