#include "seistrc.h"
#include "seiscbvs.h"
#include "cbvsinfo.h"
#include "cbvsreadmgr.h"
#include "conn.h"
#include "iostrm.h"
#include "filegen.h"
#include "datachar.h"
#include "strmprov.h"
#include <iostream.h>
#include <math.h>

#include "prog.h"
defineTranslatorGroup(SeisTrc,"Seismic Data");
defineTranslator(CBVS,SeisTrc,"CBVS");


static void putComps( const ObjectSet<BasicComponentInfo>& cinfo )
{
    cout << "Data is written on a "
	 << (cinfo[0]->datachar.littleendian ? "little" : "big")
	 << " endian machine." << endl;

    for ( int idx=0; idx<cinfo.size(); idx++ )
    {
	const BasicComponentInfo& bci = *cinfo[idx];
	cout << "\nComponent '" << (const char*)bci.name() << ':' << endl;
	cout << "Data Characteristics: "
	     << (bci.datachar.isInteger() ? "Integer" : "Floating point") <<' ';
	if ( bci.datachar.isInteger() )
	     cout << (bci.datachar.isSigned() ? "(Signed) " : "(Unsigned) ");
	cout << (int)bci.datachar.nrBytes() << " bytes" << endl;
	cout << "Z/T start: " << bci.sd.start
	     << " step: " << bci.sd.step << endl;
	cout << "Number of samples: " << bci.nrsamples << '\n' << endl;
    }
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
	cerr << "Usage: " << argv[0] << " cbvs_file" << endl;
	return 1;
    }
    else if ( !File_exists(argv[1]) )
    {
	cerr << argv[1] << " does not exist" << endl;
	return 1;
    }

    FileNameString fname( argv[1] );
    if ( !File_isAbsPath(argv[1]) )
	fname = File_getFullPath( ".", argv[1] );

    CBVSSeisTrcTranslator tri;
    StreamConn inconn( fname, Conn::Read );
    IOStream inioobj( "cin" );
    inioobj.setFileName( fname );
    inconn.ioobj = &inioobj;
    if ( !tri.initRead(inconn) )
	{ cerr << tri.errMsg() << endl;  return 1; }

    const CBVSReadMgr& mgr = *tri.readMgr();
    const int nrreaders = mgr.nrReaders();
    if ( nrreaders > 1 )
	cout << "\nCube is stored in " << nrreaders << " files" << endl;
    cout << "\n";
    const CBVSInfo& info = mgr.info();
    if ( info.nrtrcsperposn > 1 )
	cout << info.nrtrcsperposn << " traces per position" << endl;

    const ObjectSet<BasicComponentInfo>& cinfo = info.compinfo;
    putComps( cinfo );

    cout << "The cube is " << (info.geom.fullyrectandreg ? "100% rectangular."
				: "irregular.") << endl;
    cout << "In-line range: " << info.geom.start.inl << " - "
	 << info.geom.stop.inl << " (step "
	 << info.geom.step.inl << ")." << endl;
    cout << "X-line range: " << info.geom.start.crl << " - "
	 << info.geom.stop.crl << " (step "
	 << info.geom.step.crl << ")." << endl;
    cout << endl;

    SeisTrc trc;
    BinID bid;
    StepInterval<int> samps;
    const int nrcomps = cinfo.size();
    while ( 1 )
    {
	cout << "\nExamine In-line ( 0 to stop ): "; cin >> bid.inl;
	if ( !bid.inl ) return 0;

	if ( info.geom.fullyrectandreg )
	{
	    bid.crl = info.geom.start.crl;
	    if ( !info.geom.includes(bid) )
	    {
		cout << "The inline range is " << info.geom.start.inl
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
		cout << "This inline is not present in the cube" << endl;
		continue;
	    }
	    cout << "Xline range available: ";
	    for ( int idx=0; idx<inlinf->segments.size(); idx++ )
	    {
		cout << inlinf->segments[idx].start << " - "
		     << inlinf->segments[idx].stop;
		if ( idx < inlinf->segments.size()-1 )
		    cout << " and ";
	    }
	    cout << endl;
	}

	cout << "X-line: "; cin >> bid.crl;

	if ( !tri.goTo( bid ) )
	    { cout << "Position not in data" << endl; continue; }
	if ( !tri.read(trc) )
	    { cout << "Cannot read trace!" << endl; continue; }

	if ( !mIS_ZERO(trc.info().pick) && !mIsUndefined(trc.info().pick) )
	    cout << "Pick position: " << trc.info().pick << endl;
	if ( !mIS_ZERO(trc.info().refpos) && !mIsUndefined(trc.info().refpos) )
	    cout << "Reference position: " << trc.info().refpos << endl;
	if ( !mIS_ZERO(trc.info().offset) && !mIsUndefined(trc.info().offset) )
	    cout << "Offset: " << trc.info().offset << endl;

	while ( 1 )
	{
	    cout << "Print samples ( 1 - " << trc.size(0) << " )." << endl;
	    cout << "From ( 0 to stop ): ";
	    cin >> samps.start;
	    if ( samps.start < 1 ) break;

	    cout << "To: "; cin >> samps.stop;
	    cout << "Step: "; cin >> samps.step;
	    if ( samps.step < 1 ) samps.step = 1;
	    if ( samps.start < 1 ) samps.start = 1;
	    if ( samps.stop > trc.size(0) ) samps.stop = trc.size(0);
	    cout << endl;
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp+=samps.step )
	    {
		cout << isamp << '\t';
		for ( int icomp=0; icomp<nrcomps; icomp++ )
		    cout << trc.get( isamp-1, icomp )
			 << (icomp == nrcomps-1 ? '\n' : '\t');
	    }
	    cout << endl;
	}
    }
}
