/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Feb 2005
 RCS:           $Id: horizonscanner.cc,v 1.10 2006-02-20 18:49:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "horizonscanner.h"
#include "posgeomdetector.h"
#include "iopar.h"
#include "strmprov.h"
#include "survinfo.h"
#include "oddirs.h"
#include "cubesampling.h"


HorizonScanner::HorizonScanner( const BufferStringSet& fnms )
    : Executor("Scan horizon file(s)")
    , geomdetector(*new PosGeomDetector(true))
    , nrattribvals(0)
{
    filenames = fnms;
    init();
}


HorizonScanner::HorizonScanner( const char* fnm )
    : Executor("Scan horizon file(s)")
    , geomdetector(*new PosGeomDetector(true))
{
    filenames.add( fnm );
    init();
}


HorizonScanner::~HorizonScanner()
{
    delete &geomdetector;
}


void HorizonScanner::init()
{
    totalnr = -1;
    firsttime = true;
    valranges.erase();
    nrattribvals = 0;
    geomdetector.reInit();
    analyzeData();
}


const char* HorizonScanner::message() const
{
    return "Scanning";
}


const char* HorizonScanner::nrDoneText() const
{
    return "Positions handled";
}


int HorizonScanner::nrDone() const
{
    return geomdetector.nrpositions;
}


int HorizonScanner::totalNr() const
{
    if ( totalnr > 0 ) return totalnr;

    totalnr = 0;
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	StreamProvider sp( filenames.get(0) );
	StreamData sd = sp.makeIStream();
	if ( !sd.usable() ) continue;

	char buf[80];
	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 80 );
	    totalnr++;
	}
	sd.close();
    }

    return totalnr;
}


void HorizonScanner::report( IOPar& iopar ) const
{
    iopar.clear();

    BufferString str = "Report for horizon file(s):\n";
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	str += filenames.get(idx); str += "\n";
    }
    str += "\n\n";
    iopar.setName( str );

    iopar.add( "->", "Geometry" );
    HorSampling hs;
    hs.set( geomdetector.inlrg, geomdetector.crlrg );
    hs.fillPar( iopar );
    if ( valranges.size() )
	iopar.set( sKey::ZRange, valranges[0].start, valranges[0].stop );
    iopar.setYN( "Inline gaps found", gapsFound(true) );
    iopar.setYN( "Crossline gaps found", gapsFound(false) );

    if ( valranges.size() > 1 )
    {
	iopar.add( "->", "Data values" );
	iopar.set( "Nr of attributes", valranges.size()-1 );
	for ( int idx=1; idx<valranges.size(); idx++ )
	{
	    iopar.set( IOPar::compKey("Minimum value",idx),
		       valranges[idx].start );
	    iopar.set( IOPar::compKey("Maximum value",idx),
		       valranges[idx].stop );
	}
    }
    else
	iopar.add( "->", "No data values" );
}



const char* HorizonScanner::defaultUserInfoFile()
{
    static BufferString ret;
    ret = GetProcFileName( "scan_horizon" );
    if ( GetSoftwareUser() )
	{ ret += "_"; ret += GetSoftwareUser(); }
    ret += ".txt";
    return ret.buf();
}


void HorizonScanner::launchBrowser( const char* fnm ) const
{
    if ( !fnm || !*fnm )
	fnm = defaultUserInfoFile();
    IOPar iopar; report( iopar );
    iopar.write( fnm, "_pretty" );

    ExecuteScriptCommand( "FileBrowser", fnm );
}


bool HorizonScanner::analyzeData()
{
    StreamProvider sp( filenames.get(0) );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return false;

    const float fac = SI().zIsTime() ? 0.001
				     : (SI().zInMeter() ? .3048 : 3.28084);
    Interval<float> validrg( SI().zRange(false) );
    const float zwidth = validrg.width();
    validrg.sort();
    validrg.start -= zwidth;
    validrg.stop += zwidth;

    int maxcount = 100;
    int count, nrxy, nrbid, nrscale, nrnoscale;
    count = nrxy = nrbid = nrscale = nrnoscale = 0;
    Coord crd;
    float val;
    char buf[1024]; char valbuf[80];
    while ( *sd.istrm )
    {
	if ( count > maxcount ) 
	{
	    if ( nrscale == nrnoscale ) maxcount *= 2;
	    else break;
	}

	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	if ( !ptr || !*ptr ) 
	    continue;
	crd.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );
	crd.y = atof( valbuf );
	BinID bid( mNINT(crd.x), mNINT(crd.y) );

	bool validplacement = false;
	if ( SI().isReasonable(crd) ) { nrxy++; validplacement=true; }
	if ( SI().isReasonable(bid) ) { nrbid++; validplacement=true; }

	ptr = getNextWord( ptr, valbuf );
	val = atof( valbuf );

	bool validvert = false;
	if ( !mIsUndefined(val) ) 
	{
	    if ( validrg.includes(val) ) { nrnoscale++; validvert=true; }
	    else if ( validrg.includes(val*fac) ) { nrscale++; validvert=true; }
	}

	if ( validplacement && validvert )
	    count++;

	int validx = 0;
	while ( *ptr )
	{
	    ptr = getNextWord( ptr, valbuf );
	    validx++;
	}

	nrattribvals = validx;
    }

    isxy = nrxy > nrbid;
    doscale = nrscale > nrnoscale;
    return true;
}


int HorizonScanner::nextStep()
{
    Coord crd;
    BinID bid;
    char buf[1024]; char valbuf[80];
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	StreamProvider sp( filenames.get(idx) );
	StreamData sd = sp.makeIStream();
	if ( !sd.usable() ) continue;

	while ( *sd.istrm )
	{
	    sd.istrm->getline( buf, 1024 );
	    const char* ptr = getNextWord( buf, valbuf );
	    if ( !ptr || !*ptr )
		continue;

	    if ( isxy )
	    {
		crd.x = atof( valbuf );
		ptr = getNextWord( ptr, valbuf );
		crd.y = atof( valbuf );
		bid = SI().transform( crd );
	    }
	    else
	    {
		bid.inl = atoi( valbuf );
		ptr = getNextWord( ptr, valbuf );
		bid.crl = atoi( valbuf );
		crd = SI().transform( bid );
	    }

	    geomdetector.add( bid, crd );

	    int validx = 0;
	    while ( *ptr )
	    {
		ptr = getNextWord( ptr, valbuf );
		if ( firsttime )
		    valranges += Interval<float>(mUndefValue,-mUndefValue);
		const float val = atof( valbuf );
		if ( !mIsUndefined(val) && validx<valranges.size() )
		    valranges[validx].include( val, false );
		validx++;
	    }

	    firsttime = false;

	}
	sd.close();
    }

    nrattribvals = valranges.size() - 1;

    return Executor::Finished;
}


StepInterval<int> HorizonScanner::inlRg() const
{ return geomdetector.inlrg; }

StepInterval<int> HorizonScanner::crlRg() const
{ return geomdetector.crlrg; }

bool HorizonScanner::gapsFound( bool inl ) const
{ return inl ? geomdetector.inlgapsfound : geomdetector.crlgapsfound; }

int HorizonScanner::nrAttribValues() const
{ return nrattribvals; }

