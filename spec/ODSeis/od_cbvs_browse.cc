/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/

static const char* rcsID = "$Id";

#include "seistrc.h"
#include "seiscbvs.h"
#include "cbvsinfo.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "iostrm.h"
#include "filegen.h"
#include "filepath.h"
#include "datachar.h"
#include "strmprov.h"
#include "ptrman.h"
#include <iostream>
#include <math.h>
#include <ctype.h>

#include "prog.h"
#include "seisfact.h"


static void putComps( const ObjectSet<BasicComponentInfo>& cinfo )
{
    std::cerr << "Data is written on a "
	 << (cinfo[0]->datachar.littleendian ? "little" : "big")
	 << " endian machine." << std::endl;

    for ( int idx=0; idx<cinfo.size(); idx++ )
    {
	const BasicComponentInfo& bci = *cinfo[idx];
	std::cerr << "\nComponent '" << (const char*)bci.name()
	    	  << "':\n" << "Data Characteristics: "
		  << (bci.datachar.isInteger() ? "Integer" : "Floating point")
		  << ' ';
	if ( bci.datachar.isInteger() )
	     std::cerr << (bci.datachar.isSigned() ? "(Signed) "
		     				   : "(Unsigned) ");
	std::cerr << (int)bci.datachar.nrBytes() << " bytes" << std::endl;
	std::cerr << "Z/T start: " << bci.sd.start
	     << " step: " << bci.sd.step << std::endl;
	std::cerr << "Number of samples: " << bci.nrsamples << '\n'
	    	  << std::endl;
    }
}


static void getInt( int& i )
{
    char buf[80];

    char* ptr;
    do
    {
	ptr = buf;
	std::cin.getline( ptr, 80 );
	while ( *ptr && isspace(*ptr) ) ptr++;
    }
    while ( ! *ptr );

    char* endptr = ptr + 1;
    while ( *endptr && !isspace(*endptr) ) endptr++;
    *endptr = '\0';
    i = atoi( ptr );
}


int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0] << " cbvs_file" << std::endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[1]) )
    {
	std::cerr << argv[1] << " does not exist" << std::endl;
	exitProgram( 1 );
    }

    BufferString fname( argv[1] );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.insert( File_getCurrentDir() );
	fname = fp.fullPath();
    }

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
	{ std::cerr << tri->errMsg() << std::endl;  exitProgram( 1 ); }

    std::cerr << "\n";
    const CBVSReadMgr& mgr = *tri->readMgr();
    mgr.dumpInfo( std::cerr, true );
    const CBVSInfo& info = mgr.info();

    SeisTrc trc; BinID bid;
    StepInterval<int> samps;
    const int nrcomps = info.compinfo.size();
    while ( 1 )
    {
	std::cerr << "\nExamine In-line ( 0 to stop ): "; getInt( bid.inl );
	if ( !bid.inl ) exitProgram( 0 );

	if ( info.geom.fullyrectandreg )
	{
	    bid.crl = info.geom.start.crl;
	    if ( !info.geom.includes(bid) )
	    {
		std::cerr << "The inline range is " << info.geom.start.inl
		     << " - " << info.geom.stop.inl << " step "
		     << info.geom.step.inl << std::endl;
		continue;
	    }
	}
	else
	{
	    const CBVSInfo::SurvGeom::InlineInfo* inlinf
			= info.geom.getInfoFor( bid.inl );
	    if ( !inlinf )
	    {
		std::cerr << "This inline is not present in the cube"
		    	  << std::endl;
		continue;
	    }
	    std::cerr << "Xline range available: ";
	    for ( int idx=0; idx<inlinf->segments.size(); idx++ )
	    {
		std::cerr << inlinf->segments[idx].start << " - "
		     << inlinf->segments[idx].stop;
		if ( idx < inlinf->segments.size()-1 )
		    std::cerr << " and ";
	    }
	    std::cerr << std::endl;
	}

	std::cerr << "X-line: "; getInt( bid.crl );

	if ( !tri->goTo( bid ) )
	    { std::cerr << "Position not in data" << std::endl; continue; }
	if ( !tri->read(trc) )
	    { std::cerr << "Cannot read trace!" << std::endl; continue; }

	if ( !mIsZero(trc.info().pick,mDefEps)
		&& !mIsUndefined(trc.info().pick) )
	    std::cerr << "Pick position: " << trc.info().pick << std::endl;
	if ( !mIsZero(trc.info().refpos,mDefEps)
		&& !mIsUndefined(trc.info().refpos) )
	    std::cerr << "Reference position: " << trc.info().refpos
		      << std::endl;
	if ( !mIsZero(trc.info().offset,mDefEps)
		&& !mIsUndefined(trc.info().offset) )
	    std::cerr << "Offset: " << trc.info().offset << std::endl;

	while ( 1 )
	{
	    std::cerr << "Print samples ( 1 - " << trc.size(0) << " )."
		      << std::endl;
	    std::cerr << "From ( 0 to stop ): ";
	    getInt( samps.start );
	    if ( samps.start < 1 ) break;

	    std::cerr << "To: "; getInt( samps.stop );
	    std::cerr << "Step: "; getInt( samps.step );
	    if ( samps.step < 1 ) samps.step = 1;
	    if ( samps.start < 1 ) samps.start = 1;
	    if ( samps.stop > trc.size(0) ) samps.stop = trc.size(0);
	    std::cerr << std::endl;
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp+=samps.step )
	    {
		std::cerr << isamp << '\t';
		for ( int icomp=0; icomp<nrcomps; icomp++ )
		    std::cout << trc.get( isamp-1, icomp )
			      << (icomp == nrcomps-1 ? '\n' : '\t');
	    }
	    std::cerr << std::endl;
	}
    }

    exitProgram( 0 ); return 0;
}
