/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : CBVS Seismic data translator
-*/

static const char* rcsID = "$Id: seiscbvs.cc,v 1.50 2004-07-22 16:14:07 bert Exp $";

#include "seiscbvs.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "seistrcsel.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "iostrm.h"
#include "cubesampling.h"
#include "iopar.h"
#include "binidselimpl.h"
#include "survinfoimpl.h"
#include "strmprov.h"
#include "separstr.h"
#include "filegen.h"

static const char* sKeyDataStorage = "Data storage";

const IOPar& CBVSSeisTrcTranslator::datatypeparspec
	= *new IOPar( "CBVS option");


CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, headerdone(false)
	, donext(false)
	, forread(true)
	, storinterps(0)
	, userawdata(0)
	, blockbufs(0)
	, targetptrs(0)
	, preseldatatype(0)
	, rdmgr(0)
	, wrmgr(0)
	, nrdone(0)
    	, minimalhdrs(false)
    	, brickspec(*new VBrickSpec)
{
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
    delete &brickspec;
}


void CBVSSeisTrcTranslator::cleanUp()
{
    SeisTrcTranslator::cleanUp();
    headerdone = false;
    donext =false;
    nrdone = 0;
    destroyVars();
}


void CBVSSeisTrcTranslator::destroyVars()
{
    delete rdmgr; rdmgr = 0;
    delete wrmgr; wrmgr = 0;

    if ( !blockbufs ) return;

    const int nrcomps = nrSelComps();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	delete [] blockbufs[idx];
	delete storinterps[idx];
    }

    delete [] blockbufs; blockbufs = 0;
    delete [] storinterps; storinterps = 0;
    delete [] compsel; compsel = 0;
    delete [] userawdata; userawdata = 0;
    delete [] targetptrs; targetptrs = 0;
}


bool CBVSSeisTrcTranslator::getFileName( BufferString& fnm )
{
    if ( !conn || !conn->ioobj )
    {
	if ( !conn )
	    { errmsg = "Cannot reconstruct file name"; return false; }

	mDynamicCastGet(StreamConn*,strmconn,conn)
	if ( !strmconn )
	    { errmsg = "Wrong connection from Object Manager"; return false; }
	fnm = strmconn->fileName();
	return true;
    }

    mDynamicCastGet(IOStream*,iostrm,conn->ioobj)
    if ( !iostrm )
	{ errmsg = "Object manager provides wrong type"; return false; }

    // Catch the 'stdin' pretty name (currently "Std-IO")
    StreamProvider sp;
    fnm = iostrm->getExpandedName(true);
    if ( fnm == sp.fullName() )
	fnm = StreamProvider::sStdIO;

    conn->close();
    return true;
}


bool CBVSSeisTrcTranslator::initRead_()
{
    forread = true;
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    rdmgr = new CBVSReadMgr( fnm );
    if ( rdmgr->failed() )
	{ errmsg = rdmgr->errMsg(); return false; }

    minimalhdrs = !rdmgr->hasAuxInfo();
    const int nrcomp = rdmgr->nrComponents();
    const CBVSInfo& info = rdmgr->info();
    insd = info.sd;
    innrsamples = info.nrsamples;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	BasicComponentInfo& cinf = *info.compinfo[idx];
	addComp( cinf.datachar, cinf.name(), cinf.datatype );
    }
    pinfo->usrinfo = info.usertext;
    pinfo->stdinfo = info.stdtext;
    pinfo->nr = info.seqnr;
    pinfo->inlrg.start = info.geom.start.inl;
    pinfo->inlrg.stop = info.geom.stop.inl;
    pinfo->inlrg.step = info.geom.step.inl;
    pinfo->crlrg.start = info.geom.start.crl;
    pinfo->crlrg.stop = info.geom.stop.crl;
    pinfo->crlrg.step = info.geom.step.crl;
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    if ( !trc.data().nrComponents() ) return false;
    forread = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	BufferString nm( "Component " );
	nm += idx+1;
	addComp( dc, nm );
	if ( preseldatatype )
	    tarcds[idx]->datachar = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype );
    }

    return true;
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    float fsampnr = (outsd.start - insd.start) / insd.step;
    samps.start = mNINT( fsampnr );
    samps.stop = samps.start + outnrsamples - 1;

    if ( forread )
    {
	CubeSampling cs;
	if ( seldata && seldata->type_ == SeisSelData::Range )
	{
	    cs.hrg.start = BinID(seldata->inlrg_.start,seldata->crlrg_.start);
	    cs.hrg.stop = BinID(seldata->inlrg_.stop,seldata->crlrg_.stop);
	}
	cs.zrg.start = outsd.start; cs.zrg.step = outsd.step;
	cs.zrg.stop = outsd.start + (outnrsamples-1) * outsd.step;

	if ( !rdmgr->pruneReaders( cs ) )
	    { errmsg = "Input contains no relevant data"; return false; }
    }

    const int nrcomps = nrSelComps();
    userawdata = new bool [nrcomps];
    storinterps = new TraceDataInterpreter* [nrcomps];
    targetptrs = new unsigned char* [nrcomps];
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	userawdata[idx] = inpcds[idx]->datachar == outcds[idx]->datachar;
	storinterps[idx] = new TraceDataInterpreter(
                  forread ? inpcds[idx]->datachar : outcds[idx]->datachar );
    }

    blockbufs = new unsigned char* [nrcomps];
    int bufsz = innrsamples + 1;
    for ( int iselc=0; iselc<nrcomps; iselc++ )
    {
	int nbts = inpcds[iselc]->datachar.nrBytes();
	if ( outcds[iselc]->datachar.nrBytes() > nbts )
	    nbts = outcds[iselc]->datachar.nrBytes();

	blockbufs[iselc] = new unsigned char [ nbts * bufsz ];
	if ( !blockbufs[iselc] ) { errmsg = "Out of memory"; return false; }
    }

    compsel = new bool [tarcds.size()];
    for ( int idx=0; idx<tarcds.size(); idx++ )
	compsel[idx] = tarcds[idx]->destidx >= 0;

    if ( !forread )
	return startWrite();
    else if ( seldata && seldata->selRes(rdmgr->binID()) )
	return toNext();
    return true;
}


bool CBVSSeisTrcTranslator::toNext()
{
    if ( !seldata )
	return rdmgr->toNext();

    const CBVSInfo& info = rdmgr->info();
    if ( info.nrtrcsperposn > 1 )
    {
	if ( !rdmgr->toNext() )
	    return false;
	else if ( !seldata->selRes(rdmgr->binID()) )
	    return true;
    }

    BinID nextbid = rdmgr->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;
    if ( !seldata->selRes(nextbid) )
	return rdmgr->toNext();

    // find next requested BinID
    while ( true )
    {

	while ( true )
	{
	    int res = seldata->selRes( nextbid );
	    if ( !res ) break;

	    if ( res%256 == 2 )
		{ if ( !info.geom.toNextInline(nextbid) ) return false; }
	    else if ( !info.geom.toNextBinID(nextbid) )
		return false;
	}

	if ( goTo(nextbid) ) break;
	if ( !info.geom.toNextBinID(nextbid) )
	    return false;

    }

    return true;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( rdmgr->goTo(bid) )
    {
	headerdone = false;
	donext = false;
	return true;
    }
    return false;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterps && !commitSelections() ) return false;
    if ( headerdone ) return true;

    donext = donext || (seldata && seldata->selRes(rdmgr->binID()));

    if ( donext && !toNext() ) return false;
    donext = true;

    if ( !rdmgr->getAuxInfo(auxinf) )
	return false;

    ti.getFrom( auxinf );
    ti.sampling.start = outsd.start;
    ti.sampling.step = outsd.step;
    ti.nr = ++nrdone;

    if ( !rdmgr->info().auxinfosel.coord )
	ti.coord = SI().transform( ti.binid );
    else if ( ti.binid.inl == 0 && ti.binid.crl == 0 )
	ti.binid = SI().transform( ti.coord );

    return (headerdone = true);
}


bool CBVSSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerdone && !readInfo(trc.info()) )
	return false;

    prepareComponents( trc, outnrsamples );
    const CBVSInfo& info = rdmgr->info();
    const int nselc = nrSelComps();
    for ( int iselc=0; iselc<nselc; iselc++ )
    {
	const BasicComponentInfo& ci = *info.compinfo[ selComp(iselc) ];
	unsigned char* trcdata = trc.data().getComponent( iselc )->data();
	targetptrs[iselc] = userawdata[iselc] ? trcdata : blockbufs[iselc];
    }

    if ( !rdmgr->fetch( (void**)targetptrs, compsel, &samps ) )
    {
	errmsg = rdmgr->errMsg();
	return false;
    }

    for ( int iselc=0; iselc<nselc; iselc++ )
    {
	if ( !userawdata[iselc] )
	{
	    // Convert data into other format
	    for ( int isamp=0; isamp<outnrsamples; isamp++ )
		trc.set( isamp,
			 storinterps[iselc]->get( targetptrs[iselc], isamp ),
			 iselc );
	}
    }

    headerdone = false;
    return true;
}


bool CBVSSeisTrcTranslator::skip( int )
{
    if ( !rdmgr->skip(true) ) return false;
    donext = false;
    headerdone = false;
    return true;
}


BinID2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr || !rdmgr->nrReaders() )
	return SI().is3D() ? SI3D().binID2Coord() : BinID2Coord();
    return rdmgr->info().geom.b2c;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    CBVSInfo info;
    info.auxinfosel.setAll( !minimalhdrs );
    info.geom.fullyrectandreg = false;
    if ( SI().is3D() )
	info.geom.b2c = SI3D().binID2Coord();
    info.stdtext = pinfo->stdinfo;
    info.usertext = pinfo->usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo += new BasicComponentInfo(*outcds[idx]);
    info.sd = insd;
    info.nrsamples = innrsamples;

    wrmgr = new CBVSWriteMgr( fnm, info, &auxinf, &brickspec );
    if ( wrmgr->failed() )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	const unsigned char* trcdata
	    		= trc.data().getComponent( selComp(iselc) )->data();
	unsigned char* blockbuf = blockbufs[iselc];
	targetptrs[iselc] = userawdata[iselc]
	    		  ? const_cast<unsigned char*>( trcdata )
			  : blockbuf;
	if ( !userawdata[iselc] )
	{
	    // Convert data into other format
	    int icomp = selComp(iselc);
	    for ( int isamp=samps.start; isamp<=samps.stop; isamp++ )
		storinterps[iselc]->put( blockbuf, isamp-samps.start,
					 trc.get(isamp,icomp) );
	}
    }

    trc.info().putTo( auxinf );
    if ( !wrmgr->put( (void**)targetptrs ) )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


void CBVSSeisTrcTranslator::blockDumped( int nrtrcs )
{
    if ( nrtrcs > 1 && wrmgr )
	wrmgr->ensureConsistent();
}


const IOPar* CBVSSeisTrcTranslator::parSpec( Conn::State ) const
{
    if ( !datatypeparspec.size() )
    {
	FileMultiString fms;
	const char* ptr = DataCharacteristics::UserTypeNames[0];
	for ( int idx=0; DataCharacteristics::UserTypeNames[idx]
		     && *DataCharacteristics::UserTypeNames[idx]; idx++ )
	    fms += DataCharacteristics::UserTypeNames[idx];
	IOPar& ps = const_cast<IOPar&>( datatypeparspec );
	ps.set( sKeyDataStorage, fms );
    }
    return &datatypeparspec;
}


void CBVSSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    const char* res = iopar.find( sKeyDataStorage );
    if ( res && *res )
	preseldatatype = (DataCharacteristics::UserType)(*res-'0');

    res = iopar.find( "Optimized direction" );
    if ( res && *res )
    {
	brickspec.setStd( *res == 'H' );
	if ( *res == 'H' && *res && *(res+1) == '`' )
	{
	    FileMultiString fms( res + 2 );
	    const int sz = fms.size();
	    int tmp = atoi( fms[0] );
	    if ( tmp > 0 )
		brickspec.nrsamplesperslab = tmp < 100000 ? tmp : 100000;
	    if ( sz > 1 )
	    {
		tmp = atoi( fms[1] );
		if ( tmp > 0 )
		    brickspec.maxnrslabs = tmp;
	    }
	}
    }
}


#define mImplStart(fn) \
    if ( !ioobj || strcmp(ioobj->translator(),"CBVS") ) return false; \
    mDynamicCastGet(const IOStream*,iostrm,ioobj) \
    if ( !iostrm ) return false; \
    if ( iostrm->isMulti() ) \
	return const_cast<IOStream*>(iostrm)->fn; \
 \
    BufferString pathnm = iostrm->dirName(); \
    BufferString basenm = iostrm->fileName()

#define mImplLoopStart \
	StreamProvider sp( CBVSIOMgr::getFileName(basenm,nr) ); \
	sp.addPathIfNecessary( pathnm ); \
	if ( !sp.exists(true) ) \
	    return true;


bool CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(implRemove());

    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.remove(false) )
	    return nr ? true : false;
    }
}


bool CBVSSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
       					const CallBack* cb ) const
{
    mImplStart( implRename(newnm) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	StreamProvider spnew( CBVSIOMgr::getFileName(newnm,nr) );
	spnew.addPathIfNecessary( pathnm );
	if ( !sp.rename(spnew.fileName(),cb) )
	    rv = false;
    }
}


bool CBVSSeisTrcTranslator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    mImplStart( implSetReadOnly(yn) );

    bool rv = true;
    for ( int nr=0; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.setReadOnly(yn) )
	    rv = false;
    }
}
