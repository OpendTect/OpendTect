/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 2000
-*/

#include "seistrc.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "iostrm.h"
#include "progressmeter.h"
#include "survinfo.h"
#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>

#include "prog.h"
#include "seisfact.h"


int mProgMainFnName( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " inpfile outpfile"<< std::endl
	     << "Format: CBVS.\n" << std::endl;
	return 1;
    }

    File::Path fp( argv[1] );

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
    if ( !tri->initRead(new StreamConn(fname,Conn::Read)) )
        { std::cerr << tri->errMsg() << std::endl; return 1; }

    const CBVSReadMgr& rdmgr = *tri->readMgr();
    const CBVSInfo::SurvGeom& geom = rdmgr.info().geom;

    fp.set( argv[2] );
    if ( !fp.isAbsolute() ) { fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

    SeisTrc trc;
    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();
    int nrwr = 0;
    TextStreamProgressMeter pm( std::cerr );

    for ( int linenr = geom.start.crl; linenr <= geom.stop.crl;
	    linenr += geom.step.crl )
    {
	for ( int trcnr = geom.start.inl; trcnr <= geom.stop.inl;
		trcnr += geom.step.inl )
	{
	    pm.setNrDone( nrwr );
	    if ( !tri->goTo(BinID(trcnr,linenr)) )
		continue;

	    else if ( !tri->read(trc) )
		{ std::cerr << "Cannot read " << linenr << '/' << trcnr
		       << std::endl; return 1; }

	    std::swap( trc.info().binid.inl, trc.info().binid.crl );
	    trc.info().coord = SI().transform( trc.info().binid );

	    if ( !nrwr
		    && !tro->initWrite(new StreamConn(fname,Conn::Write),trc) )
		{ std::cerr << "Cannot start write!" << std::endl; return 1; }

	    if ( !tro->write(trc) )
		{ std::cerr << "Cannot write!" << std::endl; return 1; }

	    nrwr++;
	}
    }

    return nrwr ? 0 : 1;
}

