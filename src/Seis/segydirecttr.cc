/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/


#include "segydirecttr.h"
#include "segydirectdef.h"
#include "posinfo.h"
#include "filepath.h"
#include "segytr.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "iostrm.h"
#include "ptrman.h"
#include "dirlist.h"
#include "seisselection.h"
#include "seispacketinfo.h"
#include "survgeom.h"
#include "uistrings.h"


SEGY::DirectReader::~DirectReader()
{
    delete tr_;
}


class SEGYDirectPSIOProvider : public SeisPSIOProvider
{
public:
			SEGYDirectPSIOProvider()
				: SeisPSIOProvider(mSEGYDirectTranslNm)
			{}

    virtual bool	canHandle( bool forread, bool for2d ) const override
			{ return forread; }

    SeisPS3DReader*	make3DReader( const char* fnm, int ) const override
			{ return new SEGYDirect3DPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* fnm ) const override
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm,
				      const char* lnm ) const override
			{ return new SEGYDirect2DPSReader(dirnm,lnm); }
    SeisPS2DReader*	make2DReader( const char* dirnm,
				      Pos::GeomID gid ) const override
			{ return new SEGYDirect2DPSReader(dirnm,gid); }
    SeisPSWriter*	make2DWriter( const char* dirnm,
				      const char* lnm ) const override
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm,
				      Pos::GeomID ) const override
			{ return 0; }

    bool		getGeomIDs(const char*,
				   TypeSet<Pos::GeomID>&) const override;
    bool		getLineNames(const char*,
				     BufferStringSet&) const override;

    static int		factid;
};


bool SEGYDirectPSIOProvider::getGeomIDs( const char* dirnm,
					TypeSet<Pos::GeomID>& geomids ) const
{
    geomids.erase();
    DirList dl( dirnm, File::FilesInDir, "*.sgydef" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString filenm = dl.get( idx );
	char* capptr = filenm.find( '^' );
	if ( !capptr ) continue;
	BufferString geomidstr( ++capptr );
	char* dotptr = geomidstr.find( '.' );
	if ( !dotptr ) continue;

	*dotptr = '\0';
	Pos::GeomID geomid = Survey::GM().cUndefGeomID();
	geomid.fromString( geomidstr );
	if ( geomid != Survey::GM().cUndefGeomID()
	     && Survey::GM().getGeometry(geomid) )
	    geomids += geomid;
    }

    return geomids.size();
}



bool SEGYDirectPSIOProvider::getLineNames( const char* dirnm,
					BufferStringSet& linenms) const
{
    linenms.setEmpty();
    TypeSet<Pos::GeomID> geomids;
    if ( !getGeomIDs(dirnm,geomids) )
	return false;

    for ( int idx=0; idx<geomids.size(); idx++ )
	linenms.add( Survey::GM().getName(geomids[idx]) );

    return linenms.size();
}


// This adds the SEG-Y direct prestack seismics data storage to the factory
int SEGYDirectPSIOProvider::factid = SPSIOPF().add(new SEGYDirectPSIOProvider);


SEGYSeisTrcTranslator* SEGYDirectSeisTrcTranslator::createTranslator(
					const SEGY::DirectDef& def, int filenr )
{
    const StringView filename = def.fileName( filenr );
    if ( !filename )
	return nullptr;

    SEGY::FileSpec fs( filename );
    PtrMan<IOObj> ioobj = fs.getIOObj( true );
    if ( !ioobj )
	return nullptr;

    const IOPar* segypars = def.segyPars();
    if ( !segypars )
	return nullptr;

    ioobj->pars() = *segypars;

    SEGYSeisTrcTranslator* ret = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    ret->usePar( *segypars );
    if ( !ret->initRead(ioobj->getConn(Conn::Read)) )
    {
	delete ret;
	return nullptr;
    }

    return ret;
}


SEGYDirect3DPSReader::SEGYDirect3DPSReader( const char* fnm )
    : def_(*new SEGY::DirectDef(fnm))
{
    errmsg_ = def_.errMsg();
}


SEGYDirect3DPSReader::~SEGYDirect3DPSReader()
{
    delete &def_;
}


const PosInfo::CubeData& SEGYDirect3DPSReader::posData() const
{ return def_.cubeData(); }


bool SEGYDirect3DPSReader::goTo( const BinID& bid )
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid), false );
    return ti.isValid() ? goTo( ti.filenr_, mCast(int,ti.trcidx_) ) : false;
}


bool SEGYDirect3DPSReader::goTo( int filenr, int trcidx ) const
{
    if ( filenr != curfilenr_ )
    {
	delete tr_;
	tr_ = SEGYDirectSeisTrcTranslator::createTranslator( def_, filenr );
	curfilenr_ = filenr;
    }
    return tr_ && tr_->goToTrace( trcidx );
}


SeisTrc* SEGYDirect3DPSReader::getTrace( int filenr, int trcidx,
					 const BinID& bid ) const
{
    if ( !errmsg_.isEmpty() || !goTo(filenr,trcidx) )
	return 0;

    auto* trc = new SeisTrc;
    if ( !tr_->readInfo(trc->info()) || trc->info().binID() != bid )
	{ delete trc; return nullptr; }
    if ( tr_->read(*trc) )
	return trc;

    delete trc; return nullptr;
}


SeisTrc* SEGYDirect3DPSReader::getTrace( const BinID& bid, int nr ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.findOcc( Seis::PosKey(bid), nr );
    return ti.isValid() ? getTrace(ti.filenr_,mCast(int,ti.trcidx_),bid) : 0;
}


bool SEGYDirect3DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    if ( !errmsg_.isEmpty() )
	return false;

    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid), false );
    if ( !ti.isValid() )
	return false;

    SeisTrc* trc = getTrace( ti.filenr_, mCast(int,ti.trcidx_), bid );
    if ( !trc ) return false;

    tb.deepErase();
    for ( int itrc=1; trc; itrc++ )
    {
	tb.add( trc );
	trc = getTrace( bid, itrc );
    }
    return true;
}



SEGYDirect2DPSReader::SEGYDirect2DPSReader( const char* dirnm, Pos::GeomID gid )
    : SeisPS2DReader(gid)
    , def_(*new SEGY::DirectDef(SEGY::DirectDef::get2DFileName(dirnm,gid)))
{
}

SEGYDirect2DPSReader::SEGYDirect2DPSReader( const char* dirnm, const char* lnm )
    : SeisPS2DReader(lnm)
    , def_(*new SEGY::DirectDef(SEGY::DirectDef::get2DFileName(dirnm,lnm)))
{
}


SEGYDirect2DPSReader::~SEGYDirect2DPSReader()
{
    delete &def_;
}


const PosInfo::Line2DData& SEGYDirect2DPSReader::posData() const
{ return def_.lineData(); }


bool SEGYDirect2DPSReader::goTo( const BinID& bid )
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid.crl()), false );
    return ti.isValid() ? goTo( ti.filenr_, mCast(int,ti.trcidx_) ) : false;
}


bool SEGYDirect2DPSReader::goTo( int filenr, int trcidx ) const
{
    if ( filenr != curfilenr_ )
    {
	delete tr_;
	tr_ = SEGYDirectSeisTrcTranslator::createTranslator( def_, filenr );
	curfilenr_ = filenr;
    }

    return tr_ && tr_->goToTrace( trcidx );
}


SeisTrc* SEGYDirect2DPSReader::getTrace( int filenr, int trcidx,
					 int trcnr ) const
{
    if ( !goTo(filenr,trcidx) )
	return nullptr;
    auto* trc = new SeisTrc;
    if ( !tr_->readInfo(trc->info()) || trc->info().trcNr() != trcnr )
	{ delete trc; return nullptr; }
    if ( tr_->read(*trc) )
    {
	trc->info().setGeomID( geomid_ );
	return trc;
    }

    delete trc; return nullptr;
}


SeisTrc* SEGYDirect2DPSReader::getTrace( const BinID& bid, int nr ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.findOcc( Seis::PosKey(bid.crl()), nr );
    return ti.isValid() ?
	getTrace( ti.filenr_, mCast(int,ti.trcidx_), bid.crl() ) : nullptr;
}


bool SEGYDirect2DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid.crl()), false );
    if ( !ti.isValid() )
	return false;

    SeisTrc* trc = getTrace( ti.filenr_, mCast(int,ti.trcidx_), bid.crl() );
    if ( !trc ) return false;

    tb.deepErase();
    for ( int itrc=1; trc; itrc++ )
    {
	tb.add( trc );
	trc = getTrace( bid, itrc );
    }
    return true;
}


SEGYDirectSeisTrcTranslator::SEGYDirectSeisTrcTranslator( const char* s1,
							  const char* s2 )
    : SeisTrcTranslator(s1,s2)
{
    cleanUp();
}


SEGYDirectSeisTrcTranslator::~SEGYDirectSeisTrcTranslator()
{
    cleanUp();
}


bool SEGYDirectSeisTrcTranslator::close()
{
    bool wrstatus = true;
    if ( def_ && !forread_ && tr_ )
    {
	tr_->close();
	wrstatus = def_->writeFootersToFile();
    }

    return SeisTrcTranslator::close() && wrstatus;
}


void SEGYDirectSeisTrcTranslator::cleanUp()
{
    SeisTrcTranslator::cleanUp();

    deleteAndZeroPtr( def_ );
    deleteAndZeroPtr( tr_ );
    deleteAndZeroPtr( fds_ );
    initVars( forread_ );
}


void SEGYDirectSeisTrcTranslator::initVars( bool fr )
{
    forread_ = fr;
    ild_ = -1; iseg_ = itrc_ = 0;
    curfilenr_ = -1;
    headerdonenew_ = false;
}


void SEGYDirectSeisTrcTranslator::setCompDataFromInput()
{
    if ( !tr_ )
	return;

    deepErase( cds_ );
    deepErase( tarcds_ );
    for ( int idx=0; idx<tr_->componentInfo().size(); idx++ )
    {
	addComp( tr_->inputComponentData()[idx]->datachar,
		 tr_->inputComponentData()[idx]->name() );
	tarcds_[idx]->datachar = tr_->componentInfo()[idx]->datachar;
    }
}


bool SEGYDirectSeisTrcTranslator::initRead_()
{
    initVars( true );
    mDynamicCastGet(StreamConn*,strmconn,conn_)
    if ( !strmconn )
	{ errmsg_ = tr("Cannot open definition file"); return false; }
    segydeffilename_ = strmconn->fileName();

    delete strmconn; conn_ = 0;
    def_ = new SEGY::DirectDef( segydeffilename_ );
    if (def_->errMsg().isSet())
	{ errmsg_ = def_->errMsg(); return false; }
    else if ( def_->isEmpty() )
	{ errmsg_ = tr("Empty input file"); return false; }

    const SEGY::FileDataSet& fds = def_->fileDataSet();
    pinfo_.cubedata = &def_->cubeData();
    insd_ = fds.getSampling();
    innrsamples_ = fds.getTrcSz();
    pinfo_.nr = 1;
    pinfo_.fullyrectandreg = pinfo_.cubedata->isFullyRectAndReg();
    pinfo_.cubedata->getInlRange( pinfo_.inlrg );
    pinfo_.cubedata->getCrlRange( pinfo_.crlrg );

    if ( !toNextTrace() || !positionTranslator() )
	return false;

    setCompDataFromInput();
    initVars( true );

    return true;
}


bool SEGYDirectSeisTrcTranslator::initWrite_( const SeisTrc& trc )
{
    initVars( false );
    mDynamicCastGet(StreamConn*,strmconn,conn_)
    if ( !strmconn || strmconn->isBad() )
	{ errmsg_ = tr("Could not open new definition file"); return false; }
    segydeffilename_ = strmconn->fileName();

    delete tr_;
    tr_ = SEGYSeisTrcTranslator::getInstance();
    tr_->usePar( segypars_ );

    FilePath outfp( segydeffilename_ );
    outfp.setExtension( tr_->defExtension() );
    segyfilename_ = outfp.fullPath();
    StreamConn* segyconn = new StreamConn( segyfilename_, false );
    if ( !segyconn || segyconn->isBad() )
    {
	errmsg_  = tr( "Cannot open new SEG-Y file:\n%1" ).arg( segyfilename_ );
	return false;
    }

    if ( !tr_->initWrite( segyconn, trc ) )
	{ errmsg_ = tr_->errMsg(); return false; }

    setCompDataFromInput();

    return true;
}


bool SEGYDirectSeisTrcTranslator::commitSelections_()
{
    if ( !conn_ || conn_->forRead() )
    {
	if ( !toNextTrace() )
	    { errmsg_ = tr("No (selected) trace found"); return false; }
	return true;
    }

    if ( !tr_->commitSelections() )
	{ errmsg_ = tr_->errMsg(); return false; }

    delete conn_; // was made for the segydeffilename_,
		  // but def_ will open a new handle
    conn_ = 0; // now curConn() will return tr_'s conn_

    def_ = new SEGY::DirectDef;
    fds_ = new SEGY::FileDataSet( segypars_ );
    fds_->addFile( segyfilename_ );
    fds_->setAuxData( Seis::Vol, *tr_ );
    def_->setData( *fds_ );
    if ( !def_->writeHeadersToFile(segydeffilename_) )
    {
	errmsg_ = tr( "Cannot write header for %1" ).arg( segydeffilename_ );
	return false;
    }

    fds_->setOutputStream( *def_->getOutputStream() );
    return true;
}


Conn* SEGYDirectSeisTrcTranslator::curConn()
{
    return conn_ ? conn_ : (tr_ ? tr_->curConn() : 0);
}


BinID SEGYDirectSeisTrcTranslator::curBinID() const
{
    if ( !cubeData().validIdx(ild_) || !cubeData()[ild_]  ) return BinID(0,0);

    const PosInfo::LineData& ld = *cubeData()[ild_];
    return BinID( ld.linenr_, ld.segments_[iseg_].atIndex( itrc_ ) );
}


bool SEGYDirectSeisTrcTranslator::positionTranslator()
{
    const BinID bid( curBinID() );
    SEGY::FileDataSet::TrcIdx fdsidx;
    while ( true )
    {
	fdsidx = def_->find( Seis::PosKey(bid), false );
	if ( !fdsidx.isValid() )
	    { pErrMsg("Huh"); return false; }
	else if ( fdsidx.filenr_ == curfilenr_ )
	    break;

	curfilenr_ = fdsidx.filenr_;
	delete tr_;
	tr_ = SEGYDirectSeisTrcTranslator::createTranslator( *def_, curfilenr_);
	if ( seldata_ && tr_ )
	    tr_->setSelData( seldata_ );

	setCompDataFromInput();
    }

    return tr_ && tr_->goToTrace( mCast(int,fdsidx.trcidx_) );
}


bool SEGYDirectSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !def_ || def_->isEmpty() )
	return false;
    else if ( ild_ < 0 && !toNextTrace() )
	return false;
    else if ( !positionTranslator() )
	return false;

    if ( !tr_->readInfo(ti) || ti.binID() != curBinID() )
	{ errmsg_ = tr_->errMsg(); return false; }

    ti.sampling.start = outsd_.start;
    ti.sampling.step = outsd_.step;
    if ( tr_->curtrcscalebase_ )
	curtrcscalebase_ = tr_->curtrcscalebase_;

    return (headerdonenew_ = true);
}


bool SEGYDirectSeisTrcTranslator::readData( TraceData* extbuf )
{
    TraceData& tdata = extbuf ? *extbuf : *storbuf_;
    if ( !tr_->readData(&tdata) )
	return false;

    toNextTrace();
    headerdonenew_ = false;

    return (datareaddone_ = true);
}


bool SEGYDirectSeisTrcTranslator::read( SeisTrc& trc )
{
    return SeisTrcTranslator::read( trc );
}


bool SEGYDirectSeisTrcTranslator::skip( int ntrcs )
{
    while ( ntrcs > 0 )
    {
	if ( !toNextTrace() )
	    return false;
	ntrcs--;
    }
    return true;
}


bool SEGYDirectSeisTrcTranslator::write( const SeisTrc& trc )
{
    if ( !fds_ && !commitSelections() )
	return false;

    if ( !tr_ || !def_ || !fds_ )
    {
	errmsg_ = toUiString("Internal: tr_, def_ or fds_ null");
	return false;
    }

    if ( !tr_->write(trc) )
	{ errmsg_ = tr_->errMsg(); return false; }

    fds_->addTrace( 0, trc.info().posKey(Seis::Vol), trc.info().coord, true );
    return true;
}


const PosInfo::CubeData& SEGYDirectSeisTrcTranslator::cubeData() const
{
    static PosInfo::CubeData empty;
    return def_ ? def_->cubeData() : empty;
}


bool SEGYDirectSeisTrcTranslator::toNextTrace()
{
    if ( !def_ )
	return false;
    const PosInfo::CubeData& cd = cubeData();
    const bool atstart = ild_ == -1;
    if ( atstart )
	ild_ = 0;
    if ( ild_ < 0 || ild_ >= cd.size() )
	return false;

    const PosInfo::LineData* ld = cd[ild_];
    PosInfo::LineData::Segment seg = ld->segments_[iseg_];
    if ( !atstart )
	itrc_++;

    while ( true )
    {
	if ( seg.atIndex(itrc_) > seg.stop )
	{
	    iseg_++; itrc_ = 0;
	    if ( iseg_ >= ld->segments_.size() )
	    {
		ild_++; iseg_ = 0;
		if ( ild_ >= cd.size() )
		    { ild_ = -2; return false; }
		ld = cd[ild_];
	    }
	    seg = ld->segments_[iseg_];
	}

	if ( !seldata_ || seldata_->isOK(curBinID()) )
	    return true;

	itrc_++;
    }

    return true;
}


bool SEGYDirectSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( !def_ ) return false;

    const PosInfo::CubeData& cd = cubeData();
    const int newild = cd.indexOf( bid.inl() );
    if ( newild < 0 )
	return false;
    const PosInfo::LineData& ld = *cd[newild];
    const int newiseg = ld.segmentOf( bid.crl() );
    if ( newiseg < 0 )
	return false;

    ild_ = newild; iseg_ = newiseg;
    itrc_ = ld.segments_[iseg_].getIndex( bid.crl() );
    return positionTranslator();
}


IOObj* SEGYDirectSeisTrcTranslator::createWriteIOObj( const IOObjContext& ctxt,
					      const MultiID& ioobjkey ) const
{
    IOObj* ioobj = ctxt.crDefaultWriteObj( *this, ioobjkey );

    if ( ioobj && !segyfilename_.isEmpty() )
	ioobj->pars().set( "SEG-Y file", segyfilename_ );

    return ioobj;
}


bool SEGYDirectSeisTrcTranslator::implRemove( const IOObj* ioobj,
					      bool deep ) const
{
    if ( !ioobj )
	return true;

    if ( deep )
    {
	SEGY::DirectDef segydef( ioobj->mainFileName() );
	const SEGY::FileDataSet& fds = segydef.fileDataSet();
	for ( int idx=0; idx<fds.nrFiles(); idx++ )
	    File::remove( fds.fileName(idx) );
    }

    Translator::implRemove( ioobj );
    return true;
}


bool SEGYDirectSeisTrcTranslator::getConfirmRemoveMsg( const IOObj* ioobj,
				uiString& msg, uiString& canceltxt,
				uiString& yestxt, uiString& notxt ) const
{
    if ( !ioobj || !ioobj->implExists(true) )
	return false;

    BufferStringSet segyfiles;
    SEGY::DirectDef segydef( ioobj->mainFileName() );
    const SEGY::FileDataSet& fds = segydef.fileDataSet();
    for ( int idx=0; idx<fds.nrFiles(); idx++ )
	if ( File::exists(fds.fileName(idx)) )
	    segyfiles.add( fds.fileName(idx) );

    if ( segyfiles.isEmpty() )
	return Translator::getConfirmRemoveMsg( ioobj, msg, canceltxt,
						yestxt, notxt );

    msg = tr("Database entry for %1 '%2' will be permanently removed "
	    "along with the linked SEGY files:\n%3")
	    .arg(userName()).arg(ioobj->name()).arg(segyfiles.cat());
    canceltxt = uiStrings::sCancel();
    yestxt = tr("Remove all");
    notxt = tr("Keep SEGY files");
    return true;
}


void SEGYDirectSeisTrcTranslator::usePar( const IOPar& iop )
{
    segypars_ = iop;
}


void SEGYDirectSeisTrcTranslator::toSupported( DataCharacteristics& dc ) const
{
    SEGYSeisTrcTranslator* tmptr = SEGYSeisTrcTranslator::getInstance();
    tmptr->toSupported( dc );
    delete tmptr;
}


bool SEGYDirectSeisTrcTranslator::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    if ( !def_ )
	return false;

    cd = def_->cubeData();
    return true;
}
