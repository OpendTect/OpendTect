/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 24-1-2001
 * FUNCTION : CBVS Seismic data translator
-*/


#include "seiscbvs.h"

#include "cbvsreader.h"
#include "cbvsreadmgr.h"
#include "cbvswritemgr.h"
#include "iostrm.h"
#include "seispacketinfo.h"
#include "seisseldata.h"
#include "seistrc.h"
#include "separstr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "trckeyzsampling.h"
#include "uistrings.h"

const char* CBVSSeisTrcTranslator::sKeyDefExtension()	{ return "cbvs"; }

CBVSSeisTrcTranslator::CBVSSeisTrcTranslator( const char* nm, const char* unm )
	: SeisTrcTranslator(nm,unm)
	, donext_(false)
	, forread_(true)
	, compsel_(0)
	, datarep_(OD::AutoDataRep)
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
	bool infoonly, bool is2d, uiString* msg, bool forceusecbvsinf )
{
    if ( !fnm || !*fnm )
	{ if ( msg ) *msg = tr("Empty file name"); return 0; }

    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::getInstance();
    tr->set2D( is2d );
    tr->setForceUseCBVSInfo( forceusecbvsinf );
    if ( msg ) *msg = uiString::empty();
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
    SeisTrcTranslator::cleanUp();

    donext_ =false;
    nrdone_ = 0;
    destroyVars();
}


void CBVSSeisTrcTranslator::destroyVars()
{
    deleteAndZeroPtr( rdmgr_ );
    deleteAndZeroPtr( wrmgr_ );
    deleteAndZeroArrPtr( compsel_ );
}


bool CBVSSeisTrcTranslator::forRead() const
{
    return forread_;
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
    setIs2D( yn );
    is2d_ = yn;
    if ( is2d_ )
    {
	single_file_ = true;
	coordpol_ = (int)CBVSIO::InTrailer;
    }
}


bool CBVSSeisTrcTranslator::getFileName( BufferString& fnm )
{
    if ( !conn_ )
	{ errmsg_ = tr("Cannot open CBVS file"); return false; }

    PtrMan<IOObj> ioobj = getIOObj( conn_->linkedTo() );
    mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
    if ( ioobj && !iostrm )
	{ errmsg_ = tr("Object manager provides wrong type"); return false; }

    if ( !ioobj || iostrm->isMultiConn() )
    {
	mDynamicCastGet(StreamConn*,strmconn,conn_)
	if ( !strmconn )
	{
	    errmsg_ = tr("Wrong connection from Object Manager");
	    return false;
	}
	fnm = strmconn->fileName();
	return true;
    }

    // Catch the 'stdin' pretty name (currently "Std-IO")
    StreamProvider sp;
    fnm = iostrm->fullUserExpr(true);
    if ( fnm == FixedString(sp.fileName()) )
	fnm = StreamProvider::sStdIO();

    conn_->close( true );
    return true;
}


int CBVSSeisTrcTranslator::bytesOverheadPerTrace() const
{
    return rdmgr_ ? rdmgr_->bytesOverheadPerTrace()
		  : CBVSReader::defHeaderSize();
}


bool CBVSSeisTrcTranslator::initRead_()
{
    forread_ = true; BufferString fnm;
    if ( !getFileName(fnm) )
	return false;

    rdmgr_ = new CBVSReadMgr( fnm, 0, single_file_,
			read_mode == Seis::PreScan, forceusecbvsinfo_ );
    if ( rdmgr_->failed() )
	{ errmsg_ = toUiString(rdmgr_->errMsg()); return false; }

    if ( is2d_ )
	rdmgr_->setSingleLineMode( true );

    const int nrcomp = rdmgr_->nrComponents();
    const CBVSInfo& info = rdmgr_->info();
    insd_ = info.sd_;
    innrsamples_ = info.nrsamples_;
    for ( int idx=0; idx<nrcomp; idx++ )
    {
	const BasicComponentInfo& cinf = *info.compinfo_[idx];
	addComp( cinf.datachar_, cinf.name() );
	if ( idx == 0 )
	    setDataType( (Seis::DataType)cinf.datatype_ );
    }

    pinfo_.usrinfo = info.usertext_;
    pinfo_.stdinfo = info.stdtext_;
    pinfo_.nr = info.seqnr_;
    pinfo_.fullyrectandreg = info.geom_.fullyrectandreg;
    pinfo_.inlrg.start = info.geom_.start.inl();
    pinfo_.inlrg.stop = info.geom_.stop.inl();
    pinfo_.inlrg.step = abs(info.geom_.step.inl());
    pinfo_.inlrg.sort();
    pinfo_.crlrg.start = info.geom_.start.crl();
    pinfo_.crlrg.stop = info.geom_.stop.crl();
    pinfo_.crlrg.step = abs(info.geom_.step.crl());
    pinfo_.crlrg.sort();
    if ( !pinfo_.fullyrectandreg )
	pinfo_.cubedata = &info.geom_.cubedata;

    rdmgr_->getIsRev( pinfo_.inlrev, pinfo_.crlrev );
    return true;
}


bool CBVSSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    if ( !trc.data().nrComponents() )
	return false;
    forread_ = false;

    for ( int idx=0; idx<trc.data().nrComponents(); idx++ )
    {
	DataCharacteristics dc(trc.data().getInterpreter(idx)->dataChar());
	addComp( dc, 0 );
	if ( datarep_ == OD::AutoDataRep )
	    tarcds_[idx]->datachar_ = trc.data().getInterpreter()->dataChar();
	else
	    tarcds_[idx]->datachar_ = DataCharacteristics(datarep_);
	tarcds_[idx]->datatype_ = (int)dataType();
    }

    return true;
}


void CBVSSeisTrcTranslator::updBinIDFromMgr( BinID& bid ) const
{
    if ( is2d_ && geomid_.isValid() && geomid_.is2D() )
	bid.inl() = geomid_.getI();
}


BinID CBVSSeisTrcTranslator::curMgrBinID() const
{
    auto ret = rdmgr_->binID();
    updBinIDFromMgr( ret );
    return ret;
}


bool CBVSSeisTrcTranslator::commitSelections_()
{
    if ( forread_ && !is2d_ && seldata_ && !seldata_->isAll() )
    {
	TrcKeyZSampling cs;
	Interval<int> inlrg = seldata_->inlRange();
	Interval<int> crlrg = seldata_->crlRange();
	cs.hsamp_.start_.inl() = inlrg.start;
	cs.hsamp_.start_.crl() = crlrg.start;
	cs.hsamp_.stop_.inl() = inlrg.stop;
	cs.hsamp_.stop_.crl() = crlrg.stop;
	cs.zsamp_.start = outsd_.start; cs.zsamp_.step = outsd_.step;
	cs.zsamp_.stop = outsd_.start + (outnrsamples_-1) * outsd_.step;

	if ( !rdmgr_->pruneReaders( cs ) )
	    { errmsg_ = tr("Input contains no relevant data"); return false; }
    }

    delete [] compsel_;
    compsel_ = new bool [tarcds_.size()];
    for ( int idx=0; idx<tarcds_.size(); idx++ )
	compsel_[idx] = tarcds_[idx]->selected_;

    if ( !forread_ )
	return startWrite();

    if ( selRes(curMgrBinID()) )
	return toNext();

    return true;
}


bool CBVSSeisTrcTranslator::inactiveSelData() const
{
    return isAll( seldata_ );
}


int CBVSSeisTrcTranslator::selRes( const BinID& bid ) const
{
    if ( inactiveSelData() )
	return 0;

    if ( is2d_ )
	return seldata_->selRes( Bin2D::decode(bid) );

    return seldata_->selRes( bid );
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
	else if ( !selRes(curMgrBinID()) )
	    return true;
    }

    BinID nextbid = rdmgr_->nextBinID();
    if ( nextbid == BinID(0,0) )
	return false;

    updBinIDFromMgr( nextbid );
    if ( !selRes(nextbid) )
	return rdmgr_->toNext();

    // find next requested BinID
    while ( true )
    {
	while ( true )
	{
	    int res = selRes( nextbid );
	    if ( !res )
		break;

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
    if ( !rdmgr_->toStart() )
	return false;
    headerdone_ = donext_ = false;
    return true;
}


bool CBVSSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( !rdmgr_ )
	return false;

    BinID bid2use( bid );
    if ( is2d_ )
	bid2use.inl() = rdmgr_->binID().inl();

    if ( !rdmgr_->goTo(bid2use) )
	return false;

    headerdone_ = donext_ = false;
    return true;
}


bool CBVSSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !ensureSelectionsCommitted() )
	return false;
    if ( headerdone_ )
	return true;

    donext_ = donext_ || selRes(curMgrBinID());

    if ( donext_ && !toNext() )
	return false;
    donext_ = true;

    if ( !rdmgr_->getAuxInfo(auxinf_) )
	return false;

    ti.getFrom( auxinf_ );
    ti.sampling_.start = outsd_.start;
    ti.sampling_.step = outsd_.step;

    if ( ti.lineNr() == 0 && ti.trcNr() == 0 )
	ti.setPos( SI().transform(ti.coord_) );

    return (headerdone_ = true);
}


bool CBVSSeisTrcTranslator::readData( TraceData* extbuf )
{
    if ( !ensureSelectionsCommitted() )
	return false;

    TraceData& tdata = extbuf ? *extbuf : *trcdata_;
    if ( !rdmgr_->fetch(tdata,compsel_,&samprg_) )
	{ errmsg_ = toUiString(rdmgr_->errMsg()); return false; }

    headerdone_ = false;

    return (datareaddone_ = true);
}


bool CBVSSeisTrcTranslator::skip( int nr )
{
    for ( int idx=0; idx<nr; idx++ )
	if ( !rdmgr_->toNext() ) return false;
    donext_ = headerdone_ = false;
    return true;
}


Pos::IdxPair2Coord CBVSSeisTrcTranslator::getTransform() const
{
    if ( !rdmgr_ || !rdmgr_->nrReaders() )
	return SI().binID2Coord();
    return rdmgr_->info().geom_.b2c;
}


bool CBVSSeisTrcTranslator::getGeometryInfo( LineCollData& cd ) const
{
    cd = readMgr()->info().geom_.cubedata;
    return true;
}


bool CBVSSeisTrcTranslator::startWrite()
{
    BufferString fnm;
    if ( !getFileName(fnm) )
	return false;

    CBVSInfo info;
    info.auxinfosel_.setAll( true );
    info.geom_.fullyrectandreg = false;
    info.geom_.b2c = SI().binID2Coord();
    info.stdtext_ = pinfo_.stdinfo;
    info.usertext_ = pinfo_.usrinfo;
    for ( int idx=0; idx<nrSelComps(); idx++ )
	info.compinfo_ += new BasicComponentInfo(*outcds_[idx]);
    info.sd_ = insd_;
    info.nrsamples_ = innrsamples_;

    wrmgr_ = new CBVSWriteMgr( fnm, info, &auxinf_, &brickspec_, single_file_,
				(CBVSIO::CoordPol)coordpol_ );
    if ( wrmgr_->failed() )
	{ errmsg_ = wrmgr_->errMsg(); return false; }

    if ( is2d_ )
	wrmgr_->setForceTrailers( true );
    return true;
}


bool CBVSSeisTrcTranslator::writeTrc_( const SeisTrc& trc )
{
    if ( !wrmgr_ )
	{ pErrMsg("initWrite not done or failed"); return false; }

    for ( int iselc=0; iselc<nrSelComps(); iselc++ )
    {
	const int icomp = selComp(iselc);
	for ( int isamp=samprg_.start; isamp<=samprg_.stop; isamp+=samprg_.step)
	{
	    //Parallel !!!
	    trcdata_->setValue(isamp-samprg_.start,trc.get(isamp,icomp),iselc);
	}
    }

    trc.info().putTo( auxinf_ );
    if ( !wrmgr_->put(*trcdata_) )
	{ errmsg_ = wrmgr_->errMsg(); return false; }

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

    DataCharacteristics::getUserTypeFromPar( iopar, datarep_ );

    const char* res = iopar.find( sKeyOptDir() );
    if ( res && *res )
    {
	brickspec_.setStd( *res == 'H' );
	if ( *res == 'H' && *res && *(res+1) == '`' )
	{
	    FileMultiString fms( res + 2 );
	    const int sz = fms.size();
	    int tmp = fms.getIValue( 0 );
	    if ( tmp > 0 )
		brickspec_.nrsamplesperslab = tmp < 100000 ? tmp : 100000;
	    if ( sz > 1 )
	    {
		tmp = fms.getIValue( 1 );
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
    BufferString pathnm = iostrm->fileSpec().fullDirName(); \
    BufferString basenm = iostrm->fileSpec().fileName()

#define mImplLoopStart \
	StreamProvider sp( CBVSIOMgr::getFileName(basenm,nr) ); \
	sp.addPathIfNecessary( pathnm ); \
	if ( !sp.exists(true) ) \
	    return true;


bool CBVSSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    mImplStart( implRemove() );
    if ( !SeisTrcTranslator::implRemove(ioobj) )
	return false;

    bool rv = true;
    for ( int nr=1; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.remove(false) )
	{
	    rv = nr ? true : false;
	    break;
	}
    }

    return rv;
}


bool CBVSSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
					const CallBack* cb ) const
{
    mImplStart( implRename(newnm) );
    if ( !SeisTrcTranslator::implRename(ioobj,newnm,cb) )
	return false;

    bool rv = true;
    for ( int nr=1; ; nr++ )
    {
	mImplLoopStart;

	StreamProvider spnew( CBVSIOMgr::getFileName(newnm,nr) );
	spnew.addPathIfNecessary( pathnm );
	if ( !sp.rename(spnew.fileName(),cb) )
	{
	    rv = false;
	    break;
	}
    }

    return rv;
}


bool CBVSSeisTrcTranslator::implSetReadOnly( const IOObj* ioobj, bool yn ) const
{
    mImplStart( implSetReadOnly(yn) );
    if ( !SeisTrcTranslator::implSetReadOnly(ioobj,yn) )
	return false;

    bool rv = true;
    for ( int nr=1; ; nr++ )
    {
	mImplLoopStart;

	if ( !sp.setReadOnly(yn) )
	{
	    rv = false;
	    break;
	}
    }

    return rv;
}
