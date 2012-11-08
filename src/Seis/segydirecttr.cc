/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "segydirecttr.h"
#include "segydirectdef.h"
#include "posinfo.h"
#include "filepath.h"
#include "segytr.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "ioobj.h"
#include "ptrman.h"
#include "dirlist.h"
#include "seisselection.h"
#include "seispacketinfo.h"


SEGY::DirectReader::~DirectReader()
{
    delete tr_;
}


class SEGYDirectPSIOProvider : public SeisPSIOProvider
{
public:
			SEGYDirectPSIOProvider()
			    	: SeisPSIOProvider("SEGYDirect")
    			{}
    SeisPS3DReader*	make3DReader( const char* fnm, int ) const
			{ return new SEGYDirect3DPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return new SEGYDirect2DPSReader(dirnm,lnm); }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return 0; }
    bool		getLineNames(const char*,BufferStringSet&) const;
    static int		factid;
};


bool SEGYDirectPSIOProvider::getLineNames( const char* dirnm,
					   BufferStringSet& nms ) const
{
    DirList dl( dirnm, DirList::FilesOnly, "*.sgydef" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	FilePath fp( dl.fullPath(idx) );
	fp.setExtension( 0 );
	nms.add( fp.fileName() );
    }

    return true;
}


// This adds the SEG-Y direct pre-stack seismics data storage to the factory
int SEGYDirectPSIOProvider::factid = SPSIOPF().add(new SEGYDirectPSIOProvider);


static SEGYSeisTrcTranslator* createTranslator( const SEGY::DirectDef& def,
						int filenr )
{
    const FixedString filename = def.fileName( filenr ); 
    if ( !filename )
	return 0;

    SEGY::FileSpec fs( filename );
    PtrMan<IOObj> ioobj = fs.getIOObj();
    if ( !ioobj ) return 0;
    ioobj->pars() = *def.segyPars();

    SEGYSeisTrcTranslator* ret = new SEGYSeisTrcTranslator( "SEG-Y", "SEGY" );
    ret->usePar( *def.segyPars() );
    if ( !ret->initRead(ioobj->getConn(Conn::Read)) )
	{ delete ret; return 0; }
    if ( !ret->commitSelections() )
	{ delete ret; return 0; }

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
	tr_ = createTranslator( def_, filenr );
	curfilenr_ = filenr;
    }
    return tr_ && tr_->goToTrace( trcidx );
}


SeisTrc* SEGYDirect3DPSReader::getTrace( int filenr, int trcidx,
					 const BinID& bid ) const
{
    if ( !errmsg_.isEmpty() || !goTo(filenr,trcidx) )
	return 0;

    SeisTrc* trc = new SeisTrc;
    if ( !tr_->readInfo(trc->info()) || trc->info().binid != bid )
	{ delete trc; return 0; }
    if ( tr_->read(*trc) )
	return trc;

    delete trc; return 0;
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
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid.crl), false );
    return ti.isValid() ? goTo( ti.filenr_, mCast(int,ti.trcidx_) ) : false;
}


bool SEGYDirect2DPSReader::goTo( int filenr, int trcidx ) const
{
    if ( filenr != curfilenr_ )
    {
	delete tr_;
	tr_ = createTranslator( def_, filenr );
	curfilenr_ = filenr;
    }

    return tr_ && tr_->goToTrace( trcidx );
}


SeisTrc* SEGYDirect2DPSReader::getTrace( int filenr, int trcidx,
					 int trcnr ) const
{
    if ( !goTo(filenr,trcidx) )
	return 0;
    SeisTrc* trc = new SeisTrc;
    if ( !tr_->readInfo(trc->info()) || trc->info().nr != trcnr )
	{ delete trc; return 0; }
    if ( tr_->read(*trc) )
	return trc;

    delete trc; return 0;
}


SeisTrc* SEGYDirect2DPSReader::getTrace( const BinID& bid, int nr ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.findOcc( Seis::PosKey(bid.crl), nr );
    return ti.isValid() ? getTrace(ti.filenr_,mCast(int,ti.trcidx_),bid.crl): 0;
}


bool SEGYDirect2DPSReader::getGather( const BinID& bid, SeisTrcBuf& tb ) const
{
    SEGY::FileDataSet::TrcIdx ti = def_.find( Seis::PosKey(bid.crl), false );
    if ( !ti.isValid() )
	return 0;

    SeisTrc* trc = getTrace( ti.filenr_, mCast(int,ti.trcidx_), bid.crl );
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
    , def_(0)
{
    cleanUp();
}


SEGYDirectSeisTrcTranslator::~SEGYDirectSeisTrcTranslator()
{
    initVars();
}


void SEGYDirectSeisTrcTranslator::cleanUp()
{
    delete def_; def_ = 0;
    delete tr_; tr_ = 0;
    initVars();
}


void SEGYDirectSeisTrcTranslator::initVars()
{
    ild_ = -1; iseg_ = itrc_ = 0;
    curfilenr_ = -1;
    headerread_ = false;
}


bool SEGYDirectSeisTrcTranslator::commitSelections_()
{
    if ( !toNextTrace() )
	{ errmsg = "No (selected) trace found"; return false; }
    return true;
}


bool SEGYDirectSeisTrcTranslator::initRead_()
{
    initVars();
    mDynamicCastGet(StreamConn*,strmconn,conn)
    if ( !strmconn )
	{ errmsg = "Cannot open definition file"; return false; }

    const BufferString fnm( strmconn->fileName() );
    delete strmconn; conn = 0;
    def_ = new SEGY::DirectDef( fnm );
    if ( def_->errMsg() && *def_->errMsg() )
	{ errmsg = def_->errMsg(); return false; }
    else if ( def_->isEmpty() )
	{ errmsg = "Empty input file"; return false; }

    const SEGY::FileDataSet& fds = def_->fileDataSet();
    pinfo.cubedata = &def_->cubeData();
    insd = fds.getSampling();
    innrsamples = fds.getTrcSz();
    pinfo.nr = 1;
    pinfo.fullyrectandreg = pinfo.cubedata->isFullyRectAndReg();
    pinfo.cubedata->getInlRange( pinfo.inlrg );
    pinfo.cubedata->getCrlRange( pinfo.crlrg );
    addComp( DataCharacteristics(), "Data" );
    return true;
}


BinID SEGYDirectSeisTrcTranslator::curBinID() const
{
    if ( ild_ < 0 ) return BinID(0,0);

    const PosInfo::LineData& ld = *cubeData()[ild_];
    return BinID( ld.linenr_, ld.segments_[iseg_].atIndex( itrc_ ) );
}


bool SEGYDirectSeisTrcTranslator::positionTranslator()
{
    const BinID bid( curBinID() );
    SEGY::FileDataSet::TrcIdx fdsidx = def_->find( Seis::PosKey(bid), false );
    if ( !fdsidx.isValid() )
        { pErrMsg("Huh"); return false; }

    if ( fdsidx.filenr_ != curfilenr_ )
    {
	curfilenr_ = fdsidx.filenr_;
	delete tr_;
	tr_ = createTranslator( *def_, curfilenr_ );
    }

    return tr_ && tr_->goToTrace( mCast(int,fdsidx.trcidx_) );
}


bool SEGYDirectSeisTrcTranslator::readInfo( SeisTrcInfo& ti )
{
    if ( !def_ || def_->isEmpty() || ild_ < 0 || !positionTranslator() )
	return false;

    if ( !tr_->readInfo(ti) || ti.binid != curBinID() )
	{ errmsg = tr_->errMsg(); return false; }

    headerread_ = true;
    return true;
}


bool SEGYDirectSeisTrcTranslator::read( SeisTrc& trc )
{
    if ( !headerread_ && !readInfo(trc.info()) )
	return false;

    if ( !tr_->read(trc) )
	return false;

    headerread_ = false;
    toNextTrace();
    return true;
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

    const PosInfo::LineData& ld = *cd[ild_];
    const PosInfo::LineData::Segment& seg = ld.segments_[iseg_];
    if ( !atstart )
	itrc_++;

    while ( true )
    {
	if ( seg.atIndex(itrc_) > seg.stop )
	{
	    iseg_++; itrc_ = 0;
	    if ( iseg_ >= ld.segments_.size() )
	    {
		ild_++; iseg_ = 0;
		if ( ild_ >= cd.size() )
		    { ild_ = -2; return false; }
	    }
	}

	if ( !seldata || seldata->isOK(curBinID()) )
	    return true;

	itrc_++;
    }

    return true;
}


bool SEGYDirectSeisTrcTranslator::goTo( const BinID& bid )
{
    if ( !def_ ) return false;

    const PosInfo::CubeData& cd = cubeData();
    const int newild = cd.indexOf( bid.inl );
    if ( newild < 0 )
	return false;
    const PosInfo::LineData& ld = *cd[newild];
    const int newiseg = ld.segmentOf( bid.crl );
    if ( newiseg < 0 )
	return false;

    ild_ = newild; iseg_ = newiseg;
    itrc_ = ld.segments_[iseg_].getIndex( bid.crl );
    return positionTranslator();
}


void SEGYDirectSeisTrcTranslator::usePar( const IOPar& iop )
{
}
