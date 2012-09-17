/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. K. Singh
 * DATE     : July 2007
-*/
static const char* rcsID = "$Id: od_create_ridge.cc,v 1.9 2012/07/10 13:05:58 cvskris Exp $";

#include "conn.h"
#include "cubesampling.h"
#include "file.h"
#include "filepath.h"
#include "iostrm.h"
#include "ptrman.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "seiscbvs.h"
#include "strmprov.h"
#include "valseriesevent.h"

#include "prog.h"


static int doWork( int argc, char** argv )
{
    if ( argc < 3 )
    {
	FilePath fp( argv[0] );
	std::cerr << "Usage: " << fp.fileName()
		  << " [Event Type (Min/Max/Both)] inpfile outpfile"
		  << std::endl;
	return 1;
    }

    bool is2d = false;

    FilePath fp( argv[2] ); 
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

    fp.set( argv[3] ); 
    if ( !fp.isAbsolute() ) { fp.insert( File::getCurrentPath() ); }
    fname = fp.fullPath();

    PtrMan<CBVSSeisTrcTranslator> tro = CBVSSeisTrcTranslator::getInstance();
    tro->set2D( is2d );
    tro->packetInfo() = tri->packetInfo();

    VSEvent::Type evtype;
    VSEvent::Type revtype;
    bool isboth = false;
    if ( !strcmp( argv[1], "Max" ) ) evtype = VSEvent::Max; 
    else if ( !strcmp( argv[1], "Min" ) ) evtype = VSEvent::Min;
    else if ( !strcmp( argv[1], "Both" ) ) 
    {
	evtype = VSEvent::Max;
	revtype = VSEvent::Min;
	isboth = true;
    }
    else return 1;

    SeisTrc inptrc; int nrwr = 0;
    while ( tri->read(inptrc) )
    {
	SeisTrc outptrc( inptrc );
	outptrc.zero();

	SeisTrcValueSeries stvs( inptrc, 0 );
	SamplingData<float> sd( 0, 1 ) ;
	ValueSeriesEvFinder<float,float> evf( stvs, outptrc.size() - 1, sd );
	Interval<float> trcrg( 0, outptrc.size() - 1 );

	TypeSet<float> evset;
	evf.findEvents( evset, trcrg, evtype );
	for ( int idx=0; idx<evset.size(); idx++ )
	{
	    const int sampnr = mNINT32( evset[idx] );
	    outptrc.set( sampnr, 1, 0 );
	}

	if ( isboth )
	{
	    evf.findEvents( evset, trcrg, revtype );
    	    for ( int idx=0; idx<evset.size(); idx++ )
    	    {
    		const int sampnr = mNINT32( evset[idx] );
    		outptrc.set( sampnr, -1, 0 );
    	    }
	}

	if ( nrwr == 0
	  && !tro->initWrite(new StreamConn(fname,Conn::Write),outptrc) )
	    { std::cerr << tro->errMsg() << std::endl; return 1; }

	if ( !tro->write(outptrc) )
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
