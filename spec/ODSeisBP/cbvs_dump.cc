#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "binidselimpl.h"
#include "conn.h"
#include "iostrm.h"
#include "separstr.h"
#include "strmprov.h"
#include "filegen.h"
#include "ptrman.h"
#include "survinfo.h"
#include "iopar.h"
#include <fstream>
#include <math.h>

#include "batchprog.h"
#include "prog.h"
#include "seisfact.h"

static void wrBinID( ostream& strm, const BinID& bid, bool doasc )
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


static void prBidCoord( ostream& strm, const BinID2Coord& b2c, const BinID& bid)
{
    Coord coord = b2c.transform( bid );
    BufferString bs( getStringFromDouble("%lg",coord.x) );
    bs += " "; bs += getStringFromDouble("%lg",coord.y);
    strm << "Coords." << bid.inl << "/" << bid.crl << ": " << bs << endl;
}


#define mErrStrm logstrm << progName() << ": "

bool BatchProgram::go( ostream& logstrm )
{
    BufferString fname( StreamProvider::sStdIO );
    pars().get( "Input", fname );
    StreamProvider sp( fname );
    sp.addPathIfNecessary( File_getCurrentDir() );
    if ( !sp.exists(true) )
    {
        mErrStrm << fname << " does not exist" << endl;
        return false;
    }

    PtrMan<CBVSSeisTrcTranslator> tri = CBVSSeisTrcTranslator::getInstance();
    if ( !tri->initRead( new StreamConn(fname,Conn::Read) ) )
        { mErrStrm << tri->errMsg() << endl;  return 1; }

    StepInterval<float> zrg; assign( zrg, SI().zRange() );
    BinIDSampler bidsel;
    IOStream ioobj( "tmp" );
    ioobj.setFileName( fname );
    ioobj.setGroup( mTranslGroupName(SeisTrc) );
    ioobj.setTranslator( mTranslKey(CBVSSeisTrc) );
    SeisTrcTranslator::getRanges( ioobj, bidsel, zrg );

    fname = StreamProvider::sStdIO;
    pars().get( "Output", fname );
    sp.set( fname );
    sp.addPathIfNecessary( File_getCurrentDir() );
    if ( sp.bad() )
        { mErrStrm << "Invalid output: " << fname << endl; return false; }
    StreamData sd = sp.makeOStream();
    if ( !sd.usable() )
	{ mErrStrm << "Cannot open output " << fname << endl; return false; }
    ostream& outstrm = *sd.ostrm;

    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci = tri->componentInfo();
    const char* res = pars().find( "Output.Mode" );
    if ( res && *res == 'I' )
    {
	outstrm << "Range.Inline: " << bidsel.start.inl << ' '
		<< bidsel.stop.inl << ' ' << bidsel.step.inl << endl;
	outstrm << "Range.Xline: " << bidsel.start.crl << ' '
		<< bidsel.stop.crl << ' ' << bidsel.step.crl << endl;
	outstrm << "Range.Z: " << zrg.start << ' ' << zrg.stop << ' '
		<< zrg.step << endl;
	if ( ci.size() > 1 )
	    outstrm << "Components.Nr: " << ci.size() << endl;

	BinID2Coord b2c( tri->getTransform() );
	BinID bid;
	bid.inl = bidsel.start.inl; bid.crl = bidsel.start.crl;
	prBidCoord( outstrm, b2c, bid );
	bid.inl = bidsel.stop.inl;
	prBidCoord( outstrm, b2c, bid );
	bid.inl = bidsel.start.inl; bid.crl = bidsel.stop.crl;
	prBidCoord( outstrm, b2c, bid );

	char buf[80]; ci[0]->datachar.toString(buf);
	replaceCharacter( buf, '`', ' ' );
	outstrm << "Storage.Internal: " << buf << endl;
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

    if ( havesel )
    {
	FileMultiString fms;
	if ( inlselstr || crlselstr )
	{
	    if ( inlselstr )
	    {
		fms = inlselstr;
		const int sz = fms.size();
		if ( sz > 0 ) bidsel.start.inl = atoi( fms[0] );
		if ( sz > 1 ) bidsel.stop.inl = atoi( fms[1] );
		if ( sz > 2 ) bidsel.step.inl = atoi( fms[2] );
	    }
	    if ( crlselstr )
	    {
		fms = crlselstr;
		const int sz = fms.size();
		if ( sz > 0 ) bidsel.start.crl = atoi( fms[0] );
		if ( sz > 1 ) bidsel.stop.crl = atoi( fms[1] );
		if ( sz > 2 ) bidsel.step.crl = atoi( fms[2] );
	    }
	    SeisTrcSel* tsel = new SeisTrcSel;
	    tsel->bidsel = bidsel.clone();
	    tri->setTrcSel( tsel );
	}
	if ( zselstr )
	{
	    fms = zselstr;
	    const int sz = fms.size();
	    if ( sz > 0 ) zrg.start = atof( fms[0] );
	    if ( sz > 1 ) zrg.stop = atof( fms[1] );
	    if ( sz > 2 ) zrg.step = atof( fms[2] );

	    ci[compsel]->sd = SamplingData<float>( zrg.start, zrg.step );
	    ci[compsel]->nrsamples = zrg.nrSteps() + 1;
	}
    }

    res = pars().find( "Output.Type" );
    const bool doasc = !res || (*res != 'b' && *res != 'B');
    const int nrsamples = zrg.nrSteps() + 1;
    if ( doasc )
	outstrm << zrg.start << ' ' << zrg.step << ' ' << nrsamples << endl;
    else
    {
	outstrm.write( (char*)&zrg.start, sizeof(float) );
	outstrm.write( (char*)&zrg.step, sizeof(float) );
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

    mErrStrm << nrwr << " traces written." << endl;
    return nrwr ? true : false;
}
