/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID = "$Id: seiscbvs2d.cc,v 1.13 2004-09-07 16:24:01 bert Exp $";

#include "seiscbvs2d.h"
#include "seiscbvs.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "seisbuf.h"
#include "cbvsio.h"
#include "executor.h"
#include "survinfo.h"
#include "keystrs.h"
#include "filegen.h"
#include "filepath.h"
#include "iopar.h"
#include "errh.h"


static BufferString getFileName( const char* fnm )
{
    BufferString ret = fnm;
    if ( ret == "" ) return ret;

    FilePath fp( ret );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    ret = fp.fullPath();

    return ret;
}

static BufferString getFileName( const IOPar& iop )
{
    return getFileName( iop.find( sKey::FileName ) );
}


int SeisCBVS2DLineIOProvider::factid
	= (S2DLIOPs() += new SeisCBVS2DLineIOProvider).size() - 1;


SeisCBVS2DLineIOProvider::SeisCBVS2DLineIOProvider()
    	: Seis2DLineIOProvider("CBVS")
{
}


bool SeisCBVS2DLineIOProvider::isUsable( const IOPar& iop ) const
{
    return Seis2DLineIOProvider::isUsable(iop) && iop.find( sKey::FileName );
}


bool SeisCBVS2DLineIOProvider::isEmpty( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return true;

    BufferString fnm = getFileName( iop );
    return fnm == "" || File_isEmpty(fnm);
}


static CBVSSeisTrcTranslator* gtTransl( const char* fnm, BufferString* msg=0 )
{
    if ( !fnm || !*fnm )
	{ if ( msg ) *msg = "Empty file name"; return false; }

    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::getInstance();
    tr->setSingleFile( true );
    if ( msg ) *msg = "";
    if ( !tr->initRead(new StreamConn(fnm,Conn::Read)) )
    {
	if ( msg ) *msg = tr->errMsg();
	delete tr; tr = 0;
    }
    return tr;
}


bool SeisCBVS2DLineIOProvider::getTxtInfo( const IOPar& iop,
		BufferString& uinf, BufferString& stdinf ) const
{
    if ( !isUsable(iop) ) return true;

    CBVSSeisTrcTranslator* tr = gtTransl( getFileName(iop) );
    if ( !tr ) return false;

    const SeisPacketInfo& pinf = tr->packetInfo();
    uinf = pinf.usrinfo;
    stdinf = pinf.stdinfo;
    return true;
}


bool SeisCBVS2DLineIOProvider::getRanges( const IOPar& iop,
		StepInterval<int>& trcrg, StepInterval<float>& zrg ) const
{
    if ( !isUsable(iop) ) return true;

    CBVSSeisTrcTranslator* tr = gtTransl( getFileName(iop) );
    if ( !tr ) return false;

    const SeisPacketInfo& pinf = tr->packetInfo();
    trcrg = pinf.crlrg; zrg = pinf.zrg;
    return true;
}


void SeisCBVS2DLineIOProvider::removeImpl( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return;
    BufferString fnm = getFileName(iop);
    File_remove( fnm.buf(), NO );
}


#undef mErrRet
#define mErrRet(s) { msg = s; return -1; }

class SeisCBVS2DLineGetter : public Executor
{
public:

SeisCBVS2DLineGetter( const char* fnm, SeisTrcBuf& b, const SeisSelData& sd )
    	: Executor("Load 2D line")
	, tbuf(b)
	, curnr(0)
	, totnr(0)
	, fname(fnm)
	, msg("Reading traces")
	, seldata(0)
	, trcstep(1)
	, linenr(CBVSIOMgr::getFileNr(fnm))
{
    tr = gtTransl( fname, &msg );
    if ( !tr ) return;

    if ( sd.type_ != SeisSelData::None )
    {
	seldata = new SeisSelData( sd );
	seldata->type_ = SeisSelData::Range;
	seldata->inlrg_.start = 0;
	seldata->inlrg_.stop = mUndefIntVal;
	if ( sd.type_ == SeisSelData::TrcNrs )
	{
	    assign( seldata->crlrg_, seldata->trcrg_ );
	    trcstep = seldata->trcrg_.step;
	}
	tr->setSelData( seldata );
    }
}


~SeisCBVS2DLineGetter()
{
    delete tr;
    delete seldata;
}


void addTrc( SeisTrc* trc )
{
    trc->info().nr = trc->info().binid.crl;
    trc->info().binid = SI().transform( trc->info().coord );
    tbuf.add( trc );
}


int nextStep()
{
    if ( !tr ) return -1;

    if ( curnr == 0 )
    {
	const SeisPacketInfo& pinf = tr->packetInfo();
	totnr = tr->packetInfo().crlrg.nrSteps() + 1;
	if ( seldata )
	{
	    int nrsel = seldata->crlrg_.width() / trcstep + 1;
	    if ( nrsel < totnr ) totnr = nrsel;
	}
    }

    int lastnr = curnr + 10;
    for ( ; curnr<lastnr; curnr++ )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !tr->read(*trc) )
	{
	    delete trc;
	    if ( tbuf.size() < 1 )
		mErrRet("No traces read in 2D line")
	    return 0;
	}
	addTrc( trc );

	for ( int idx=1; idx<trcstep; idx++ )
	{
	    if ( !tr->skip() )
		return 0;
	}
    }

    return 1;
}

const char*		message() const		{ return msg; }
const char*		nrDoneText() const	{ return "Traces read"; }
int			nrDone() const		{ return curnr; }
int			totalNr() const		{ return totnr; }

    int			curnr;
    int			totnr;
    SeisTrcBuf&		tbuf;
    BufferString	fname;
    BufferString	msg;
    CBVSSeisTrcTranslator* tr;
    SeisSelData*	seldata;
    int			trcstep;
    const int		linenr;

};


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }

Executor* SeisCBVS2DLineIOProvider::getFetcher( const IOPar& iop,
						SeisTrcBuf& tbuf,
						const SeisSelData* sd )
{
    BufferString fnm = getFileName(iop);
    if ( !isUsable(iop) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg(errmsg);
	return 0;
    }

    return new SeisCBVS2DLineGetter( fnm, tbuf, sd ? *sd : SeisSelData() );
}


class SeisCBVS2DLinePutter : public Seis2DLinePutter
{
public:

SeisCBVS2DLinePutter( const char* fnm )
    	: nrwr(0)
	, fname(getFileName(fnm))
	, tr(CBVSSeisTrcTranslator::getInstance())
{
    tr->setSingleFile( true );
    tr->enforceRegularWrite( false );
    bid.inl = CBVSIOMgr::getFileNr( fnm );
}


~SeisCBVS2DLinePutter()
{
    delete tr;
}

const char* errMsg() const	{ return errmsg.buf(); }
int nrWritten() const		{ return nrwr; }


bool put( const SeisTrc& trc )
{
    SeisTrcInfo& info = const_cast<SeisTrcInfo&>( trc.info() );
    bid.crl = info.nr;
    const BinID oldbid = info.binid;
    info.binid = bid;

    if ( nrwr == 0 )
    {
	bool res = tr->initWrite(new StreamConn(fname.buf(),Conn::Write),trc);
	if ( !res )
	{
	    info.binid = oldbid;
	    errmsg = "Cannot open 2D line file:\n";
	    errmsg += tr->errMsg();
	    return false;
	}
    }

    bool res = tr->write(trc);
    info.binid = oldbid;
    if ( res )
	nrwr++;
    else
    {
	errmsg = "Cannot write "; errmsg += nrwr + 1;
	errmsg += getRankPostFix( nrwr + 1 );
	errmsg += " trace to 2D line file:\n";
	errmsg += tr->errMsg();
	return false;
    }
    return true;
}

    int			nrwr;
    BufferString	fname;
    BufferString	errmsg;
    CBVSSeisTrcTranslator* tr;
    BinID		bid;

};


#undef mErrRet
#define mErrRet(s) { pErrMsg( s ); return 0; }

Seis2DLinePutter* SeisCBVS2DLineIOProvider::getReplacer(
				const IOPar& iop )
{
    if ( !Seis2DLineIOProvider::isUsable(iop) ) return 0;

    const char* res = iop.find( sKey::FileName );
    if ( !res )
	mErrRet("Knurft")

    return new SeisCBVS2DLinePutter( res );
}


Seis2DLinePutter* SeisCBVS2DLineIOProvider::getAdder( IOPar& iop,
						      const IOPar* previop,
						      const char* lgrpnm )
{
    if ( !Seis2DLineIOProvider::isUsable(iop) ) return 0;

    BufferString fnm = iop.find( sKey::FileName );
    if ( fnm == "" )
    {
	if ( previop )
	    fnm = CBVSIOMgr::baseFileName(previop->find(sKey::FileName));
	else
	{
	    if ( lgrpnm && *lgrpnm )
		fnm = lgrpnm;
	    else
		fnm = iop.name();
	    fnm += ".cbvs";
	    cleanupString( fnm.buf(), NO, YES, YES );
	}
	const char* prevfnm = previop ? previop->find(sKey::FileName) : 0;
	const int prevlnr = CBVSIOMgr::getFileNr( prevfnm );
	fnm = CBVSIOMgr::getFileName( fnm, previop ? prevlnr+1 : 0 );
	iop.set( sKey::FileName, fnm );
    }

    return new SeisCBVS2DLinePutter( fnm.buf() );
}
