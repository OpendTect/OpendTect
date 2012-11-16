/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 24-1-2001
 * FUNCTION : CBVS Seismic data translator
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seiscbvs.h"

#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "filepath.h"
#include "iostrm.h"
#include "keystrs.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seistrc.h"
#include "separstr.h"
#include "strmprov.h"
#include "survinfo.h"


const char* CBVSSeisTrcTranslator::sKeyDefExtension()	{ return "cbvs"; }

CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, headerdone_(false)
	, donext_(false)
	, forread_(true)
	, storinterps_(0)
	, blockbufs_(0)
	, preseldatatype_(0)
	, rdmgr_(0)
	, wrmgr_(0)
	, nrdone_(0)
    	, brickspec_(*new VBrickSpec)
    	, single_file_(false)
    	, forceusecbvsinfo_(false)
    	, is2d_(false)
    	, coordpol_((int)CBVSIO::NotStored)
{
}


CBVSSeisTrcTranslator::~CBVSSeisTrcTranslator()
{
    cleanUp();
    delete &brickspec_;
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
    headerdone_ = donext_ =false;
    nrdone_ = 0;
    destroyVars( nrcomps );
}


void CBVSSeisTrcTranslator::destroyVars( int nrcomps )
{
    delete rdmgr_; rdmgr_ = 0;
    delete wrmgr_; wrmgr_ = 0;
    if ( !blockbufs_ ) return;

    for ( int idx=0; idx<nrcomps; idx++ )
    {
	delete [] blockbufs_[idx];
	delete storinterps_[idx];
    }

    delete [] blockbufs_; blockbufs_ = 0;
    delete [] storinterps_; storinterps_ = 0;
    delete [] compsel_; compsel_ = 0;
}


void CBVSSeisTrcTranslator::setCoordPol( bool dowrite, bool intrailer )
{
    if ( !dowrite )
	coordpol_ = (int)CBVSIO::NotStored;
    else if ( intrailer )
	coordpol_ = (int)CBVSIO::InTrailer;
    else
	coordpol_ = (int)CBVSIO::InAux;
}


void CBVSSeisTrcTranslator::set2D( bool yn )
{
    is2d_ = yn;
    if ( is2d_ )
    {
	single_file_ = true;
	coordpol_ = (int)CBVSIO::InTrailer;
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
    forread_ = true;
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    rdmgr_ = new CBVSReadMgr( fnm, 0, single_file_, 
	    		read_mode == Seis::PreScan, forceusecbvsinfo_ );
    if ( rdmgr_->failed() )
	{ errmsg = rdmgr_->errMsg(); return false; }

    const int nrcomp = rdmgr_->nrComponents();
    const CBVSInfo& info = rdmgr_->info();
    insd = info.sd_;
    innrsamples = info.nrsamples_;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const BasicComponentInfo& cinf = *info.compinfo_[idx];
	addComp( cinf.datachar, cinf.name(), cinf.datatype );
    }

    pinfo.usrinfo = info.usertext_;
    pinfo.stdinfo = info.stdtext_;
    pinfo.nr = info.seqnr_;
    pinfo.fullyrectandreg = info.geom_.fullyrectandreg;
    pinfo.inlrg.start = info.geom_.start.inl;
    pinfo.inlrg.stop = info.geom_.stop.inl;
    pinfo.inlrg.step = abs(info.geom_.step.inl);
    pinfo.inlrg.sort();
    pinfo.crlrg.start = info.geom_.start.crl;
    pinfo.crlrg.stop = info.geom_.stop.crl;
    pinfo.crlrg.step = abs(info.geom_.step.crl);
    if ( !pinfo.fullyrectandreg )
	pinfo.cubedata = &info.geom_.cubedata;

    rdmgr_->getIsRev( pinfo.inlrev, pinfo.crlrev );
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    if ( !trc.data().nrComponents() ) return false;
    forread_ = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	addComp( dc, 0 );
	if ( preseldatatype_ )
	    tarcds[idx]->datachar = DataCharacteristics(
			(DataCharacteristics::UserType)preseldatatype_ );
    }

    return true;
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    if ( forread_ && !is2d_ && seldata && !seldata->isAll() )
    {
	CubeSampling cs;
	Interval<int> inlrg = seldata->inlRange();
	Interval<int> crlrg = seldata->crlRange();
	cs.hrg.start.inl = inlrg.start; cs.hrg.start.crl = crlrg.start;
	cs.hrg.stop.inl = inlrg.stop; cs.hrg.stop.crl = crlrg.stop;
	cs.zrg.start = outsd.start; cs.zrg.step = outsd.step;
	cs.zrg.stop = outsd.start + (outnrsamples-1) * outsd.step;

	if ( !rdmgr_->pruneReaders( cs ) )
	    { errmsg = "Input contains no relevant data"; return false; }
    }

    const int nrcomps = nrSelComps();
    storinterps_ = new TraceDataInterpreter* [nrcomps];
    for ( int idx=0; idx<nrcomps; idx++ )
	storinterps_[idx] = new TraceDataInterpreter(
                  forread_ ? inpcds[idx]->datachar : outcds[idx]->datachar );

    blockbufs_ = new unsigned char* [nrcomps];
    int bufsz = innrsamples + 1;
    for ( int iselc=0; iselc<nrcomps; iselc++ )
    {
	int nbts = inpcds[iselc]->datachar.nrBytes();
	if ( outcds[iselc]->datachar.nrBytes() > nbts )
	    nbts = outcds[iselc]->datachar.nrBytes();

	blockbufs_[iselc] = new unsigned char [ nbts * bufsz ];
	if ( !blockbufs_[iselc] ) { errmsg = "Out of memory"; return false; }
    }

    compsel_ = new bool [tarcds.size()];
    for ( int idx=0; idx<tarcds.size(); idx++ )
	compsel_[idx] = tarcds[idx]->destidx >= 0;

    if ( !forread_ )
	return startWrite();

    if ( is2d_ && seldata && seldata->type() == Seis::Range )
    {
	// For 2D, inline is just an index number
	Seis::SelData& sd = *const_cast<Seis::SelData*>( seldata );
	sd.setInlRange( Interval<int>(rdmgr_->binID().inl,rdmgr_->binID().inl) );
    }

    if ( selRes(rdmgr_->binID()) )
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
    if ( is2d_ && seldata->type() == Seis::Table )
	return 0;

    return seldata->selRes(bid);
}


bool CBVSSeisTrcTranslator::toNext()
{
    if ( inactiveSelData() )
	return rdmgr_->toNext();

    const CBVSInfo& info = rdmgr_->info();
    if ( info.nrtrcsperposn_ > 1 )
    {
	if ( !rdmgr_->toNext() )
	    return false;
	else if ( !selRes(rdmgr_->binID()) )
	    return true;
    }

    BinID nextbid = rdmgr_->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;

    if ( !selRes(nextbid) )
	return rdmgr_->toNext();

    // find next requested BinID
    while ( true )
    {
	while ( true )
	{
	    int res = selRes( nextbid );
	    if ( !res ) break;

	    if ( res%256 == 2 )
		{ if ( !info.geom_.moveToNextInline(nextbid) ) return false; }
	    else if ( !info.geom_.moveToNextPos(nextbid) )
		return false;
	}

	if ( goTo(nextbid) )
	    break;
	else if ( !info.geom_.moveToNextPos(nextbid) )
	    return false;
    }

    return true;
}


bool CBVSSeisTrcTranslator::toStart()
{
    if ( rdmgr_->toStart() )
	{ headerdone_ = donext_ = false; return true; }
    return false;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( rdmgr_ && rdmgr_->goTo(bid) )
	{ headerdone_ = donext_ = false; return true; }
    return false;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !storinterps_ && !commitSelections() ) return false;
    if ( headerdone_ ) return true;

    donext_ = donext_ || selRes( rdmgr_->binID() );

    if ( donext_ && !toNext() ) return false;
    donext_ = true;

    if ( !rdmgr_->getAuxInfo(auxinf_) )
	return false;

    ti.getFrom( auxinf_ );
    ti.sampling.start = outsd.start;
    ti.sampling.step = outsd.step;
    ti.nr = ++nrdone_;

    if ( ti.binid.inl == 0 && ti.binid.crl == 0 )
	ti.binid = SI().transform( ti.coord );

    return (headerdone_ = true);
}


bool CBVSSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerdone_ && !readInfo(trc.info()) )
	return false;

    prepareComponents( trc, outnrsamples );
    if ( !rdmgr_->fetch( (void**)blockbufs_, compsel_, &samps ) )
    {
	errmsg = rdmgr_->errMsg();
	return false;
    }

    const bool matchingdc = *trc.data().getInterpreter(0) == *storinterps_[0];
    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	if ( matchingdc && outnrsamples > 1 )
	    memcpy( trc.data().getComponent(iselc)->data(), blockbufs_[iselc],
		    outnrsamples * storinterps_[iselc]->nrBytes() );
	else
	{
	    for ( int isamp=0; isamp<outnrsamples; isamp++ )
		trc.set( isamp,
			 storinterps_[iselc]->get( blockbufs_[iselc], isamp ),
			 iselc );
	}
    }

    headerdone_ = false;
    return true;
}


bool CBVSSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
	if ( !rdmgr_->toNext() ) return false;
    donext_ = headerdone_ = false;
    return true;
}


RCol2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr_ || !rdmgr_->nrReaders() )
	return SI().binID2Coord();
    return rdmgr_->info().geom_.b2c;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm; if ( !getFileName(fnm) ) return false;

    CBVSInfo info;
    info.auxinfosel_.setAll( true );
    info.geom_.fullyrectandreg = false;
    info.geom_.b2c = SI().binID2Coord();
    info.stdtext_ = pinfo.stdinfo;
    info.usertext_ = pinfo.usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo_ += new BasicComponentInfo(*outcds[idx]);
    info.sd_ = insd;
    info.nrsamples_ = innrsamples;

    wrmgr_ = new CBVSWriteMgr( fnm, info, &auxinf_, &brickspec_, single_file_,
	    			(CBVSIO::CoordPol)coordpol_ );
    if ( wrmgr_->failed() )
	{ errmsg = wrmgr_->errMsg(); return false; }

    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    if ( !wrmgr_ )
	{ pErrMsg("initWrite not done or failed"); return false; }

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	unsigned char* blockbuf = blockbufs_[iselc];
	int icomp = selComp(iselc);
	for ( int isamp=samps.start; isamp<=samps.stop; isamp++ )
	    storinterps_[iselc]->put( blockbuf, isamp-samps.start,
				     trc.get(isamp,icomp) );
    }

    trc.info().putTo( auxinf_ );
    if ( !wrmgr_->put( (void**)blockbufs_ ) )
	{ errmsg = wrmgr_->errMsg(); return false; }

    return true;
}


void CBVSSeisTrcTranslator::blockDumped( int nrtrcs )
{
    if ( nrtrcs > 1 && wrmgr_ )
	wrmgr_->ensureConsistent();
}


void CBVSSeisTrcTranslator::usePar( const IOPar& iopar )
{
    SeisTrcTranslator::usePar( iopar );

    const char* res = iopar.find( sKey::DataStorage() );
    if ( res && *res )
	preseldatatype_ = (DataCharacteristics::UserType)(*res-'0');

    res = iopar.find( "Optimized direction" );
    if ( res && *res )
    {
	brickspec_.setStd( *res == 'H' );
	if ( *res == 'H' && *res && *(res+1) == '`' )
	{
	    FileMultiString fms( res + 2 );
	    const int sz = fms.size();
	    int tmp = toInt( fms[0] );
	    if ( tmp > 0 )
		brickspec_.nrsamplesperslab = tmp < 100000 ? tmp : 100000;
	    if ( sz > 1 )
	    {
		tmp = toInt( fms[1] );
		if ( tmp > 0 )
		    brickspec_.maxnrslabs = tmp;
	    }
	}
    }
}


#define mImplStart(fn) \
    if ( !ioobj || ioobj->translator()!="CBVS" ) return false; \
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

    return rv;
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

    return rv;
}
