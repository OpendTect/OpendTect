/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seiscbvs2d.h"
#include "seiscbvs2dlinegetter.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "posinfo2d.h"
#include "cbvsio.h"
#include "executor.h"
#include "survinfo.h"
#include "keystrs.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "errh.h"
#include "ptrman.h"


int SeisCBVS2DLineIOProvider::factid_
	= (S2DLIOPs() += new SeisCBVS2DLineIOProvider).size() - 1;


static const BufferString& gtFileName( const char* fnm )
{
    static BufferString ret;
    ret = fnm;
    if ( ret.isEmpty() ) return ret;

    FilePath fp( ret );
    if ( !fp.isAbsolute() )
	fp.setPath( IOObjContext::getDataDirName(IOObjContext::Seis) );
    ret = fp.fullPath();

    return ret;
}

static const BufferString& gtFileName( const IOPar& iop )
{
    return gtFileName( iop.find( sKey::FileName() ).str() );
}

const char* SeisCBVS2DLineIOProvider::getFileName( const IOPar& iop )
{
    return gtFileName(iop).buf();
}


SeisCBVS2DLineIOProvider::SeisCBVS2DLineIOProvider()
    	: Seis2DLineIOProvider(CBVSSeisTrcTranslator::translKey() )
{
}


bool SeisCBVS2DLineIOProvider::isUsable( const IOPar& iop ) const
{
    return Seis2DLineIOProvider::isUsable(iop) && iop.find( sKey::FileName() );
}


bool SeisCBVS2DLineIOProvider::isEmpty( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return true;

    const BufferString& fnm = gtFileName( iop );
    return fnm.isEmpty() || File::isEmpty(fnm);
}


static CBVSSeisTrcTranslator* gtTransl( const char* fnm, bool infoonly,
					BufferString* msg=0 )
{
    return CBVSSeisTrcTranslator::make( fnm, infoonly, true, msg );
}


bool SeisCBVS2DLineIOProvider::getTxtInfo( const IOPar& iop,
		BufferString& uinf, BufferString& stdinf ) const
{
    if ( !isUsable(iop) ) return true;

    PtrMan<CBVSSeisTrcTranslator> tr = gtTransl( gtFileName(iop), true );
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

    PtrMan<CBVSSeisTrcTranslator> tr = gtTransl( gtFileName(iop), true );
    if ( !tr ) return false;

    const SeisPacketInfo& pinf = tr->packetInfo();
    trcrg = pinf.crlrg; zrg = pinf.zrg;
    return true;
}


void SeisCBVS2DLineIOProvider::removeImpl( const IOPar& iop ) const
{
    if ( !isUsable(iop) ) return;
    const BufferString& fnm = gtFileName(iop);
    FilePath parfp( fnm );
    parfp.setExtension( "par" );
    File::remove( parfp.fullPath() );
    File::remove( fnm.buf() );
}


#undef mErrRet
#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

SeisCBVS2DLineGetter::SeisCBVS2DLineGetter( const char* fnm, SeisTrcBuf& b,
					    int ntps, const Seis::SelData& sd )
    	: Executor("Load 2D line")
	, tbuf_(b)
	, curnr_(0)
	, totnr_(0)
	, fname_(fnm)
	, msg_("Reading traces")
	, seldata_(0)
	, trcstep_(1)
	, linenr_(CBVSIOMgr::getFileNr(fnm))
	, trcsperstep_(ntps)
{
    tr_ = gtTransl( fname_, false, &msg_ );
    if ( !tr_ ) return;

    if ( !sd.isAll() && sd.type() == Seis::Range )
    {
	seldata_ = sd.clone();
	tr_->setSelData( seldata_ );
    }
    
    totnr_ = tr_->packetInfo().crlrg.nrSteps() + 1;
}


SeisCBVS2DLineGetter::~SeisCBVS2DLineGetter()
{
    delete tr_;
    delete seldata_;
}


void SeisCBVS2DLineGetter::addTrc( SeisTrc* trc )
{
    const int tnr = trc->info().binid.crl;
    if ( !isEmpty(seldata_) )
    {
	if ( seldata_->type() == Seis::Range )
	{
	    const BinID bid( seldata_->inlRange().start, tnr );
	    if ( !seldata_->isOK(bid) )
		{ delete trc; return; }
	}
    }

    trc->info().nr = tnr;
    trc->info().binid = SI().transform( trc->info().coord );
    tbuf_.add( trc );
}


int SeisCBVS2DLineGetter::nextStep()
{
    if ( !tr_ ) return -1;

    if ( curnr_ == 0 )
    {
	if ( !isEmpty(seldata_) )
	{
	    const BinID tstepbid( 1, trcstep_ );
	    const int nrsel = seldata_->expectedNrTraces( true, &tstepbid );
	    if ( nrsel < totnr_ ) totnr_ = nrsel;
	}
    }

    int lastnr = curnr_ + trcsperstep_;
    for ( ; curnr_<lastnr; curnr_++ )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !tr_->read(*trc) )
	{
	    delete trc;
	    const char* emsg = tr_->errMsg();
	    if ( emsg && *emsg )
		mErrRet(emsg)
	    return 0;
	}
	addTrc( trc );

	for ( int idx=1; idx<trcstep_; idx++ )
	{
	    if ( !tr_->skip() )
		return 0;
	}
    }

    return 1;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }


bool SeisCBVS2DLineIOProvider::getGeometry( const IOPar& iop,
					    PosInfo::Line2DData& geom ) const
{
    geom.setEmpty();
    BufferString fnm = gtFileName(iop);
    if ( !isUsable(iop) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg( errmsg );
	return false;
    }

    BufferString errmsg;
    PtrMan<CBVSSeisTrcTranslator> tr = gtTransl( fnm, false, &errmsg );
    if ( !tr )
    {
	ErrMsg( errmsg );
	return false;
    }

    const CBVSInfo& cbvsinf = tr->readMgr()->info();
    TypeSet<Coord> coords; TypeSet<BinID> binids;
    tr->readMgr()->getPositions( coords );
    tr->readMgr()->getPositions( binids );

    StepInterval<float> zrg( cbvsinf.sd_.start, 0, cbvsinf.sd_.step );
    zrg.stop = cbvsinf.sd_.start + (cbvsinf.nrsamples_-1) * cbvsinf.sd_.step;
    geom.setZRange( zrg );
    const int sz = mMIN(coords.size(),binids.size());
    for ( int idx=0; idx<sz; idx++ )
    {
	PosInfo::Line2DPos p( binids[idx].crl );
	p.coord_ = coords[idx];
	geom.add( p );
    }

    return true;
}


Executor* SeisCBVS2DLineIOProvider::getFetcher( const IOPar& iop,
						SeisTrcBuf& tbuf, int ntps,
						const Seis::SelData* sd )
{
    const BufferString& fnm = gtFileName(iop);
    if ( !isUsable(iop) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg( errmsg );
	return 0;
    }

    const Seis::SelData* usedsd = sd;
    PtrMan<Seis::SelData> tmpsd = 0;
    if ( !usedsd )
    {
	tmpsd = Seis::SelData::get(Seis::Range);
	usedsd = tmpsd;
    }

    return new SeisCBVS2DLineGetter( fnm, tbuf, ntps, *usedsd );
}


#undef mErrRet
#define mErrRet(s) { pErrMsg( s ); return 0; }

Seis2DLinePutter* SeisCBVS2DLineIOProvider::getReplacer(
				const IOPar& iop )
{
    if ( !Seis2DLineIOProvider::isUsable(iop) ) return 0;

    const char* res = iop.find( sKey::FileName() );
    if ( !res )
	mErrRet("Knurft")

    return new SeisCBVS2DLinePutter( res, iop );
}


Seis2DLinePutter* SeisCBVS2DLineIOProvider::getAdder( IOPar& iop,
						      const IOPar* previop,
						      const char* lsetnm )
{
    if ( !Seis2DLineIOProvider::isUsable(iop) ) return 0;

    BufferString fnm = iop.find( sKey::FileName() ).str();
    if ( fnm.isEmpty() )
    {
	if ( previop )
	    fnm = CBVSIOMgr::baseFileName(previop->find(sKey::FileName())).buf();
	else
	{
	    if ( lsetnm && *lsetnm )
		fnm = lsetnm;
	    else
		fnm = iop.name();
	    fnm += ".cbvs";
	    cleanupString( fnm.buf(), false, true, true );
	}
	const char* prevfnm = previop ? previop->find(sKey::FileName()) : 0;
	const int prevlnr = CBVSIOMgr::getFileNr( prevfnm );
	fnm = CBVSIOMgr::getFileName( fnm, previop ? prevlnr+1 : 0 );
	iop.set( sKey::FileName(), fnm );
    }

    return new SeisCBVS2DLinePutter( fnm.buf(), iop );
}


//-------------------SeisCBVS2DLinePutter-----------------

SeisCBVS2DLinePutter::SeisCBVS2DLinePutter( const char* fnm, const IOPar& iop )
    	: nrwr_(0)
	, fname_(gtFileName(fnm))
	, tr_(CBVSSeisTrcTranslator::getInstance())
	, preseldt_(DataCharacteristics::Auto)
{
    tr_->set2D( true );
    bid_.inl = CBVSIOMgr::getFileNr( fnm );
    DataCharacteristics::parseEnumUserType(
	    iop.find(sKey::DataStorage()), preseldt_ );
}


SeisCBVS2DLinePutter::~SeisCBVS2DLinePutter()
{
    delete tr_;
}


bool SeisCBVS2DLinePutter::put( const SeisTrc& trc )
{
    SeisTrcInfo& info = const_cast<SeisTrcInfo&>( trc.info() );
    bid_.crl = info.nr;
    const BinID oldbid = info.binid;
    info.binid = bid_;

    if ( nrwr_ == 0 )
    {
	tr_->setIs2D( true );
	bool res = tr_->initWrite(new StreamConn(fname_.buf(),Conn::Write),trc);
	if ( !res )
	{
	    info.binid = oldbid;
	    errmsg_ = "Cannot open 2D line file:\n";
	    errmsg_ += tr_->errMsg();
	    return false;
	}
	if ( preseldt_ != DataCharacteristics::Auto )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
				= tr_->componentInfo();
	    DataCharacteristics dc( preseldt_ );
	    for ( int idx=0; idx<ci.size(); idx++ )
	    {
		SeisTrcTranslator::TargetComponentData& cd = *ci[idx];
		cd.datachar = dc;
	    }
	}
    }

    tr_->setIs2D( true );
    bool res = tr_->write(trc);
    info.binid = oldbid;
    if ( res )
	nrwr_++;
    else
    {
	errmsg_ = "Cannot write "; errmsg_ += nrwr_ + 1;
	errmsg_ += getRankPostFix( nrwr_ + 1 );
	errmsg_ += " trace to 2D line file:\n";
	errmsg_ += tr_->errMsg();
	return false;
    }
    return true;
}

bool SeisCBVS2DLinePutter::close()
{
    if ( !tr_ ) return true;
    tr_->setIs2D( true );
    bool ret = tr_->close();
    if ( ret ) errmsg_ = tr_->errMsg();
    return ret; 
}

