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
    cerr << "Data is written on a "
	 << (cinfo[0]->datachar.littleendian ? "little" : "big")
	 << " endian machine." << endl;

    for ( int idx=0; idx<cinfo.size(); idx++ )
    {
	const BasicComponentInfo& bci = *cinfo[idx];
	cerr << "\nComponent '" << (const char*)bci.name() << "':" << endl;
	cerr << "Data Characteristics: "
	     << (bci.datachar.isInteger() ? "Integer" : "Floating point") <<' ';
	if ( bci.datachar.isInteger() )
	     cerr << (bci.datachar.isSigned() ? "(Signed) " : "(Unsigned) ");
	cerr << (int)bci.datachar.nrBytes() << " bytes" << endl;
	cerr << "Z/T start: " << bci.sd.start
	     << " step: " << bci.sd.step << endl;
	cerr << "Number of samples: " << bci.nrsamples << '\n' << endl;
    }
}


static void getInt( int& i )
{
    char buf[80];

    char* ptr;
    do
    {
	ptr = buf;
	cin.getline( ptr, 80 );
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
	cerr << "Usage: " << argv[0] << " cbvs_file" << endl;
	exitProgram( 1 );
    }
    else if ( !File_exists(argv[1]) )
    {
	cerr << argv[1] << " does not exist" << endl;
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
	{ cerr << tri->errMsg() << endl;  exitProgram( 1 ); }

    cerr << "\n";
    const CBVSReadMgr& mgr = *tri->readMgr();
    mgr.dumpInfo( cerr, true );
    const CBVSInfo& info = mgr.info();

    SeisTrc trc; BinID bid;
    StepInterval<int> samps;
    const int nrcomps = info.compinfo.size();
    while ( 1 )
    {
	cerr << "\nExamine In-line ( 0 to stop ): "; getInt( bid.inl );
	if ( !bid.inl ) exitProgram( 0 );

	if ( info.geom.fullyrectandreg )
	{
	    bid.crl = info.geom.start.crl;
	    if ( !info.geom.includes(bid) )
	    {
		cerr << "The inline range is " << info.geom.start.inl
		     << " - " << info.geom.stop.inl << " step "
		     << info.geom.step.inl << endl;
		continue;
	    }
	}
	else
	{
	    const CBVSInfo::SurvGeom::InlineInfo* inlinf
			= info.geom.getInfoFor( bid.inl );
	    if ( !inlinf )
	    {
		cerr << "This inline is not present in the cube" << endl;
		continue;
	    }
	    cerr << "Xline range available: ";
	    for ( int idx=0; idx<inlinf->segments.size(); idx++ )
	    {
		cerr << inlinf->segments[idx].start << " - "
		     << inlinf->segments[idx].stop;
		if ( idx < inlinf->segments.size()-1 )
		    cerr << " and ";
	    }
	    cerr << endl;
	}

	cerr << "X-line: "; getInt( bid.crl );

	if ( !tri->goTo( bid ) )
	    { cerr << "Position not in data" << endl; continue; }
	if ( !tri->read(trc) )
	    { cerr << "Cannot read trace!" << endl; continue; }

	if ( !mIS_ZERO(trc.info().pick) && !mIsUndefined(trc.info().pick) )
	    cerr << "Pick position: " << trc.info().pick << endl;
	if ( !mIS_ZERO(trc.info().refpos) && !mIsUndefined(trc.info().refpos) )
	    cerr << "Reference position: " << trc.info().refpos << endl;
	if ( !mIS_ZERO(trc.info().offset) && !mIsUndefined(trc.info().offset) )
	    cerr << "Offset: " << trc.info().offset << endl;

	while ( 1 )
	{
	    cerr << "Print samples ( 1 - " << trc.size(0) << " )." << endl;
	    cerr << "From ( 0 to stop ): ";
	    getInt( samps.start );
	    if ( samps.start < 1 ) break;

	    cerr << "To: "; getInt( samps.stop );
	    cerr << "Step: "; getInt( samps.step );
	    if ( samps.step < 1 ) samps.step = 1;
	    if ( samps.start < 1 ) samps.start = 1;
	    if ( samps.stop > trc.size(0) ) samps.stop = trc.size(0);
	    cerr << endl;
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp+=samps.step )
	    {
		cerr << isamp << '\t';
		for ( int icomp=0; icomp<nrcomps; icomp++ )
		    cout << trc.get( isamp-1, icomp )
			 << (icomp == nrcomps-1 ? '\n' : '\t');
	    }
	    cerr << endl;
	}
    }

    exitProgram( 0 ); return 0;
}
