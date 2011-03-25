/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : CBVS Seismic data translator
-*/

static const char* rcsID = "$Id: seiscbvs.cc,v 1.88 2011-03-25 15:02:34 cvsbert Exp $";

#include "seiscbvs.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "filepath.h"
#include "iostrm.h"
#include "survinfo.h"
#include "strmprov.h"
#include "separstr.h"

const char* CBVSSeisTrcTranslator::sKeyDataStorage()	{ return "Data storage"; }
const char* CBVSSeisTrcTranslator::sKeyDefExtension()	{ return "cbvs"; }


CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, headerdone(false)
	, donext(false)
	, forread(true)
	, storinterps(0)
	, blockbufs(0)
	, preseldatatype(0)
	, rdmgr(0)
	, wrmgr(0)
	, nrdone(0)
    	, brickspec(*new VBrickSpec)
    	, single_file(false)
    	, forceusecbvsinfo(false)
    	, is2d(false)
    	, coordpol((int)CBVSIO::NotStored)
{
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
    delete &brickspec;
}


CBVSSeisTrcTranslator* CBVSSeisTrcTranslator::make( const char* fnm,
	bool infoonly, bool is2d, BufferString* msg, bool forceusecbvsinf )
{
    if ( !fnm || !*fnm )
	{ if ( msg ) *msg = "Empty file name"; return 0; }

    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::getInstance();
    tr->set2D( is2d );
    tr->setForceUseCBVSInfo( forceusecbvsinf );
    if ( msg ) *msg = "";
    if ( !tr->initRead(new StreamConn(fnm,Conn::Read),
			infoonly ? Seis::PreScan : Seis::Prod) )
    {
	if ( msg ) *msg = tr->errMsg();
	delete tr; tr = 0;
    }
    return tr;
}



void CBVSSeisTrcTranslator::cleanUp()
{
    const int nrcomps = nrSelComps();
    SeisTrcTranslator::cleanUp();
    headerdone = donext =false;
    nrdone = 0;
    destroyVars( nrcomps );
}


void CBVSSeisTrcTranslator::destroyVars( int nrcomps )
{
    delete rdmgr; rdmgr = 0;
    delete wrmgr; wrmgr = 0;
    if ( !blockbufs ) return;

    for ( int idx=0; idx<nrcomps; idx++ )
    {
	delete [] blockbufs[idx];
	delete storinterps[idx];
    }

    delete [] blockbufs; blockbufs = 0;
    delete [] storinterps; storinterps = 0;
    delete [] compsel; compsel = 0;
}


void CBVSSeisTrcTranslator::setCoordPol( bool dowrite, bool intrailer )
{
    if ( !dowrite )
	coordpol = (int)CBVSIO::NotStored;
    else if ( intrailer )
	coordpol = (int)CBVSIO::InTrailer;
    else
	coordpol = (int)CBVSIO::InAux;
}


void CBVSSeisTrcTranslator::set2D( bool yn )
{
    is2d = yn;
    if ( is2d )
    {
	single_file = true;
	coordpol = (int)CBVSIO::InTrailer;
    }
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
	fnm = StreamProvider::sStdIO();

    conn->close();
    return true;
}


bool CBVSSeisTrcTranslator::initRead_()
{
    forread = true;
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    rdmgr = new CBVSReadMgr( fnm, 0, single_file, 
	    		read_mode == Seis::PreScan, forceusecbvsinfo );
    if ( rdmgr->failed() )
	{ errmsg = rdmgr->errMsg(); return false; }

    const int nrcomp = rdmgr->nrComponents();
    const CBVSInfo& info = rdmgr->info();
    insd = info.sd;
    innrsamples = info.nrsamples;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const BasicComponentInfo& cinf = *info.compinfo[idx];
	addComp( cinf.datachar, cinf.name(), cinf.datatype );
    }

    pinfo.usrinfo = info.usertext;
    pinfo.stdinfo = info.stdtext;
    pinfo.nr = info.seqnr;
    pinfo.fullyrectandreg = info.geom.fullyrectandreg;
    pinfo.inlrg.start = info.geom.start.inl;
    pinfo.inlrg.stop = info.geom.stop.inl;
    pinfo.inlrg.step = abs(info.geom.step.inl);
    pinfo.inlrg.sort();
    pinfo.crlrg.start = info.geom.start.crl;
    pinfo.crlrg.stop = info.geom.stop.crl;
    pinfo.crlrg.step = abs(info.geom.step.crl);
    if ( !pinfo.fullyrectandreg )
	pinfo.cubedata = &info.geom.cubedata;

    rdmgr->getIsRev( pinfo.inlrev, pinfo.crlrev );
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    if ( !trc.data().nrComponents() ) return false;
    forread = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	addComp( dc, 0 );
	if ( preseldatatype )
	    tarcds[idx]->datachar = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype );
    }

    return true;
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    if ( forread && !is2d && seldata && !seldata->isAll() )
    {
	CubeSampling cs;
	Interval<int> inlrg = seldata->inlRange();
	Interval<int> crlrg = seldata->crlRange();
	cs.hrg.start.inl = inlrg.start; cs.hrg.start.crl = crlrg.start;
	cs.hrg.stop.inl = inlrg.stop; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = outsd.start; cs.zrg.step = outsd.step;
	cs.zrg.stop = outsd.start + (outnrsamples-1) * outsd.step;

	if ( !rdmgr->pruneReaders( cs ) )
	    { errmsg = "Input contains no relevant data"; return false; }
    }

    const int nrcomps = nrSelComps();
    storinterps = new TraceDataInterpreter* [nrcomps];
    for ( int idx=0; idx<nrcomps; idx++ )
	storinterps[idx] = new TraceDataInterpreter(
                  forread ? inpcds[idx]->datachar : outcds[idx]->datachar );

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

    if ( is2d && seldata && seldata->type() == Seis::Range )
    {
	// For 2D, inline is just an index number
	Seis::SelData& sd = *const_cast<Seis::SelData*>( seldata );
	sd.setInlRange( Interval<int>(rdmgr->binID().inl,rdmgr->binID().inl) );
    }

    if ( selRes(rdmgr->binID()) )
	return toNext();

    return true;
}


bool CBVSSeisTrcTranslator::inactiveSelData() const
{
    return isEmpty( seldata );
}


int CBVSSeisTrcTranslator::selRes( const BinID& bid ) const
{
    if ( inactiveSelData() )
	return 0;

    // Table for 2D: can't select because inl/crl in file is not 'true'
    if ( is2d && seldata->type() == Seis::Table )
	return 0;

    return seldata->selRes(bid);
}


bool CBVSSeisTrcTranslator::toNext()
{
    if ( inactiveSelData() )
	return rdmgr->toNext();

    const CBVSInfo& info = rdmgr->info();
    if ( info.nrtrcsperposn > 1 )
    {
	if ( !rdmgr->toNext() )
	    return false;
	else if ( !selRes(rdmgr->binID()) )
	    return true;
    }

    BinID nextbid = rdmgr->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;

    if ( !selRes(nextbid) )
	return rdmgr->toNext();

    // find next requested BinID
    while ( true )
    {
	while ( true )
	{
	    int res = selRes( nextbid );
	    if ( !res ) break;

	    if ( res%256 == 2 )
		{ if ( !info.geom.moveToNextInline(nextbid) ) return false; }
	    else if ( !info.geom.moveToNextPos(nextbid) )
		return false;
	}

	if ( goTo(nextbid) )
	    break;
	else if ( !info.geom.moveToNextPos(nextbid) )
	    return false;
    }

    return true;
}


bool CBVSSeisTrcTranslator::toStart()
{
    if ( rdmgr->toStart() )
	{ headerdone = donext = false; return true; }
    return false;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( rdmgr->goTo(bid) )
	{ headerdone = donext = false; return true; }
    return false;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterps && !commitSelections() ) return false;
    if ( headerdone ) return true;

    donext = donext || selRes( rdmgr->binID() );

    if ( donext && !toNext() ) return false;
    donext = true;

    if ( !rdmgr->getAuxInfo(auxinf) )
	return false;

    ti.getFrom( auxinf );
    ti.sampling.start = outsd.start;
    ti.sampling.step = outsd.step;
    ti.nr = ++nrdone;

    if ( ti.binid.inl == 0 && ti.binid.crl == 0 )
	ti.binid = SI().transform( ti.coord );

    return (headerdone = true);
}


bool CBVSSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerdone && !readInfo(trc.info()) )
	return false;

    prepareComponents( trc, outnrsamples );
    if ( !rdmgr->fetch( (void**)blockbufs, compsel, &samps ) )
    {
	errmsg = rdmgr->errMsg();
	return false;
    }

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	for ( int isamp=0; isamp<outnrsamples; isamp++ )
	    trc.set( isamp,
		     storinterps[iselc]->get( blockbufs[iselc], isamp ),
		     iselc );
    }

    headerdone = false;
    return true;
}


bool CBVSSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
	if ( !rdmgr->toNext() ) return false;
    donext = headerdone = false;
    return true;
}


RCol2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr || !rdmgr->nrReaders() )
	return SI().binID2Coord();
    return rdmgr->info().geom.b2c;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    CBVSInfo info;
    info.auxinfosel.setAll( true );
    info.geom.fullyrectandreg = false;
    info.geom.b2c = SI().binID2Coord();
    info.stdtext = pinfo.stdinfo;
    info.usertext = pinfo.usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo += new BasicComponentInfo(*outcds[idx]);
    info.sd = insd;
    info.nrsamples = innrsamples;

    wrmgr = new CBVSWriteMgr( fnm, info, &auxinf, &brickspec, single_file,
	    			(CBVSIO::CoordPol)coordpol );
    if ( wrmgr->failed() )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    if ( !wrmgr )
	{ pErrMsg("initWrite not done or failed"); return false; }

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	const unsigned char* trcdata
	    		= trc.data().getComponent( selComp(iselc) )->data();
	unsigned char* blockbuf = blockbufs[iselc];
	int icomp = selComp(iselc);
	for ( int isamp=samps.start; isamp<=samps.stop; isamp++ )
	    storinterps[iselc]->put( blockbuf, isamp-samps.start,
				     trc.get(isamp,icomp) );
    }

    trc.info().putTo( auxinf );
    if ( !wrmgr->put( (void**)blockbufs ) )
	{ errmsg = wrmgr->errMsg(); return false; }

    return true;
}


void CBVSSeisTrcTranslator::blockDumped( int nrtrcs )
{
    if ( nrtrcs > 1 && wrmgr )
	wrmgr->ensureConsistent();
}


void CBVSSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    const char* res = iopar.find( sKeyDataStorage() );
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
	    int tmp = toInt( fms[0] );
	    if ( tmp > 0 )
		brickspec.nrsamplesperslab = tmp < 100000 ? tmp : 100000;
	    if ( sz > 1 )
	    {
		tmp = toInt( fms[1] );
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
    BufferString pathnm = iostrm->fullDirName(); \
    BufferString basenm = iostrm->fileName()

#define mImplLoopStart \
	StreamProvider sp( CBVSIOMgr::getFileName(basenm,nr) ); \
	sp.addPathIfNecessary( pathnm ); \
	if ( !sp.exists(true) ) \
	    return true;


bool CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart(implRemove());

    FilePath fpar( ioobj->fullUserExpr(true) );
    fpar.setExtension( "par" );
    StreamProvider parsp( fpar.fullPath() );
    if ( parsp.exists(true) )
	parsp.remove(false);

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

    FilePath fpar( ioobj->fullUserExpr(true) );
    fpar.setExtension( "par" );
    StreamProvider parsp( fpar.fullPath() );
    if ( parsp.exists(true) )
    {
	FilePath fparnew( newnm );
	fparnew.setExtension( "par" );
	parsp.rename( fparnew.fullPath() );
    }

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
