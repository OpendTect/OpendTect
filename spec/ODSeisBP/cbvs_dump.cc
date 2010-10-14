/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 2000
-*/

static const char* rcsID = "$Id";

#include "seistrc.h"
#include "seiscbvs.h"
#include "seisselectionimpl.h"
#include "cubesampling.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "file.h"
#include "ptrman.h"
#include "survinfo.h"
#include "iopar.h"
#include "strmprov.h"
#include <iostream>
#include <math.h>

#include "batchprog.h"
#include "prog.h"

static void wrBinID( std::ostream& strm, const BinID& bid, bool doasc )
{
    if ( doasc )
	strm << bid.inl << ' ' << bid.crl;
    else
	strm.write( (char*)&bid.inl, 2*sizeof(int) );
}


static bool provideInfo( CBVSSeisTrcTranslator& tr )
{
    return false;
}


static void prBidCoord( std::ostream& strm, const RCol2Coord& b2c,
			const BinID& bid)
{
    Coord coord = b2c.transform( bid );
    BufferString bs( getStringFromDouble("%lg",coord.x) );
    bs += " "; bs += getStringFromDouble("%lg",coord.y);
    strm << "Coords." << bid.inl << "/" << bid.crl << ": " << bs << std::endl;
}


#define mErrStrm logstrm << progName() << ": "

bool BatchProgram::go( std::ostream& logstrm )
{
    BufferString fname( StreamProvider::sStdIO() );
    pars().get( "Input", fname );
    StreamProvider sp( fname );
    sp.addPathIfNecessary( File::getCurrentPath() );
    if ( !sp.exists(true) )
    {
        mErrStrm << fname << " does not exist" << std::endl;
        return false;
    }

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
        { mErrStrm << tri->errMsg() << std::endl;  return 1; }

    IOStream ioobj( "tmp" );
    ioobj.setFileName( fname );
    ioobj.setGroup( mTranslGroupName(SeisTrc) );
    ioobj.setTranslator( mTranslKey(CBVSSeisTrc) );
    CubeSampling datacs;
    SeisTrcTranslator::getRanges( ioobj, datacs );
    const float zstep = datacs.zrg.step;
    const BinID icstep( datacs.hrg.step );

    fname = StreamProvider::sStdIO();
    pars().get( "Output", fname );
    sp.set( fname );
    sp.addPathIfNecessary( File::getCurrentPath() );
    if ( sp.bad() )
        { mErrStrm << "Invalid output: " << fname << std::endl; return false; }
    StreamData sd = sp.makeOStream();
    if ( !sd.usable() )
	{ mErrStrm << "Cannot open output " <<fname<< std::endl; return false; }
    std::ostream& outstrm = *sd.ostrm;

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
			= tri->componentInfo();
    const char* res = pars().find( "Output.Mode" );
    if ( res && *res == 'I' )
    {
	outstrm << "Range.Inline: " << datacs.hrg.start.inl << ' '
		<< datacs.hrg.stop.inl << ' ' << datacs.hrg.step.inl << '\n';
	outstrm << "Range.Xline: " << datacs.hrg.start.crl << ' '
		<< datacs.hrg.stop.crl << ' ' << datacs.hrg.step.crl << '\n';
	outstrm << "Range.Z: " << datacs.zrg.start << ' '
	    	<< datacs.zrg.stop << ' ' << datacs.zrg.step << std::endl;
	if ( ci.size() > 1 )
	    outstrm << "Components.Nr: " << ci.size() << std::endl;

	RCol2Coord b2c( tri->getTransform() );
	BinID bid( datacs.hrg.start );
	prBidCoord( outstrm, b2c, bid );
	bid.inl = datacs.hrg.stop.inl;
	prBidCoord( outstrm, b2c, bid );
	bid.inl = datacs.hrg.start.inl; bid.crl = datacs.hrg.stop.crl;
	prBidCoord( outstrm, b2c, bid );

	char buf[80]; ci[0]->datachar.toString(buf);
	replaceCharacter( buf, '`', ' ' );
	outstrm << "Storage.Internal: " << buf << std::endl;
	return true;
    }

    PtrMan<IOPar> outselpars = pars().subselect( "Output.Selection" );
    bool havesel = outselpars && outselpars->size();
    const char* inlselstr; const char* crlselstr; const char* zselstr;
    if ( havesel )
    {
	inlselstr = outselpars->find( "Inline" );
	crlselstr = outselpars->find( "Xline" );
	zselstr = outselpars->find( "Z" );
	havesel = inlselstr || crlselstr || zselstr;
    }
    int compsel = 0;
    pars().get( "Output.Component", compsel );
    const int nrincomp = ci.size();
    for ( int idx=0; idx<nrincomp; idx++ )
    {
	if ( idx != compsel )
	    ci[idx]->destidx = -1;
    }

    Seis::RangeSelData seldata( datacs );
    if ( havesel )
    {
	CubeSampling cs( datacs );
	FileMultiString fms;
	if ( inlselstr || crlselstr )
	{
	    if ( inlselstr )
	    {
		fms = inlselstr;
		const int sz = fms.size();
		if ( sz > 0 ) cs.hrg.start.inl = toInt( fms[0] );
		if ( sz > 1 ) cs.hrg.stop.inl = toInt( fms[1] );
	    }
	    if ( crlselstr )
	    {
		fms = crlselstr;
		const int sz = fms.size();
		if ( sz > 0 ) cs.hrg.start.crl = toInt( fms[0] );
		if ( sz > 1 ) cs.hrg.stop.crl = toInt( fms[1] );
	    }
	}
	if ( zselstr )
	{
	    fms = zselstr;
	    const int sz = fms.size();
	    if ( SI().zIsTime() )
		{ cs.zrg.start *= 1000; cs.zrg.stop *= 1000; }
	    if ( sz > 0 ) cs.zrg.start = toFloat( fms[0] );
	    if ( sz > 1 ) cs.zrg.stop = toFloat( fms[1] );
	    if ( SI().zIsTime() )
		{ cs.zrg.start *= 0.001; cs.zrg.stop *= 0.001; }
	}

	seldata.cubeSampling() = cs;
	tri->setSelData( &seldata );
    }

    res = pars().find( "Output.Type" );
    const bool doasc = !res || (*res != 'b' && *res != 'B');
    const Interval<float> zrg( seldata.zRange() );
    const int nrsamples = (int)((zrg.stop-zrg.start) / datacs.zrg.step + 1.5);
    if ( doasc )
	outstrm << zrg.start << ' ' << datacs.zrg.step
	        << ' ' << nrsamples << std::endl;
    else
    {
	outstrm.write( (char*)&zrg.start, sizeof(float) );
	outstrm.write( (char*)&datacs.zrg.step, sizeof(float) );
	outstrm.write( (char*)&nrsamples, sizeof(int) );
    }

    SeisTrc trc;
    int nrwr = 0; int nrbytes = 0;
    while ( tri->read(trc) )
    {
	wrBinID( outstrm, trc.info().binid, doasc );

	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    if ( doasc )
		outstrm << ' ' << trc.get(idx,0);
	    else
	    {
		float v = trc.get(idx,0);
		outstrm.write( (char*)&v, sizeof(float) );
	    }
	}
	if ( doasc ) outstrm << '\n';
	nrwr++;
    }
    wrBinID( outstrm, BinID(0,0), doasc );
    sd.close();

    mErrStrm << nrwr << " traces written." << std::endl;
    return nrwr ? true : false;
}
