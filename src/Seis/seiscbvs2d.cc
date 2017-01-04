/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : June 2004
-*/


#include "seiscbvs2d.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "posinfo2d.h"
#include "cbvsio.h"
#include "dirlist.h"
#include "executor.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "keystrs.h"
#include "file.h"
#include "filepath.h"
#include "ptrman.h"

#define mCapChar '^'
static const char* sExtCBVS = "cbvs";

int SeisCBVS2DLineIOProvider::factid_
	= (S2DLIOPs() += new SeisCBVS2DLineIOProvider).size() - 1;

static Pos::GeomID getGeomIDFromFileName( const char* fnm )
{
    Pos::GeomID geomid = mUdfGeomID;
    BufferString basenm = File::Path(fnm).baseName();
    char* capstr = basenm.find( mCapChar );
    if ( !capstr ) return geomid;
    capstr++;
    geomid = toInt( capstr, mUdfGeomID );
    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(geomid) );
    return geom2d ? geomid : mUdfGeomID;
}


const OD::String& SeisCBVS2DLineIOProvider::getFileName( const IOObj& obj,
							 Pos::GeomID geomid )
{
    mDeclStaticString( ret );
    ret = obj.fullUserExpr();
    if ( ret.isEmpty() ) return ret;

    File::Path fp( ret );
    BufferString fnm = fp.fileName();
    fnm.add( mCapChar ).add( geomid );
    fp.add( fnm );
    fp.setExtension( sExtCBVS, false );
    ret = fp.fullPath();
    return ret;
}


SeisCBVS2DLineIOProvider::SeisCBVS2DLineIOProvider()
	: Seis2DLineIOProvider(CBVSSeisTrc2DTranslator::translKey() )
{
}


bool SeisCBVS2DLineIOProvider::isEmpty( const IOObj& obj,
					Pos::GeomID geomid ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    return fnm.isEmpty() || File::isEmpty(fnm);
}


static CBVSSeisTrcTranslator* gtTransl( const char* fnm, bool infoonly,
					uiString* msg=0 )
{
    return CBVSSeisTrcTranslator::make( fnm, infoonly, true, msg );
}


bool SeisCBVS2DLineIOProvider::getTxtInfo( const IOObj& obj, Pos::GeomID geomid,
		BufferString& uinf, BufferString& stdinf ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    PtrMan<CBVSSeisTrcTranslator> trans = gtTransl( fnm, true );
    if ( !trans ) return false;

    const SeisPacketInfo& pinf = trans->packetInfo();
    uinf = pinf.usrinfo;
    stdinf = pinf.stdinfo;
    return true;
}

bool SeisCBVS2DLineIOProvider::getRanges( const IOObj& obj, Pos::GeomID geomid,
		StepInterval<int>& trcrg, StepInterval<float>& zrg ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    PtrMan<CBVSSeisTrcTranslator> trans = gtTransl( fnm, true );
    if ( !trans ) return false;

    const SeisPacketInfo& pinf = trans->packetInfo();
    trcrg = pinf.crlrg; zrg = pinf.zrg;
    return true;
}


bool SeisCBVS2DLineIOProvider::removeImpl( const IOObj& obj,
					   Pos::GeomID geomid ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    const bool ret = File::remove( fnm.buf() );
    File::Path parfp( fnm );
    parfp.setExtension( "par" );
    if ( File::exists(parfp.fullPath()) )
       File::remove( parfp.fullPath() );

    return ret;
}


bool SeisCBVS2DLineIOProvider::renameImpl( const IOObj& obj,
					   const char* newnm ) const
{
    const BufferString msk( "*.", sExtCBVS );
    DirList dl( obj.fullUserExpr(), DirList::FilesOnly, msk );
    bool ret = true;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const Pos::GeomID geomid = getGeomIDFromFileName( dl.fullPath(idx) );
	if ( mIsUdfGeomID(geomid) )
	    continue;

	File::Path fp( dl.fullPath(idx) );
	BufferString newfnm( newnm );
	newfnm.add( mCapChar ).add( geomid );
	fp.setFileName( newfnm );
	fp.setExtension( sExtCBVS, false );
	if ( !File::rename(dl.fullPath(idx),fp.fullPath()) )
	    ret = false;

	File::Path oldparfp( dl.fullPath(idx) );
	oldparfp.setExtension( "par" );
	if ( !File::exists(oldparfp.fullPath()) )
	    continue;

	fp.setExtension( "par" );
	File::rename( oldparfp.fullPath(), fp.fullPath() );
    }

    return ret;
}


uiRetVal SeisCBVS2DLineIOProvider::getGeomIDs( const IOObj& obj,
					   TypeSet<Pos::GeomID>& geomids ) const
{
    geomids.erase();
    const BufferString msk( "*.", sExtCBVS );
    DirList dl( obj.fullUserExpr(), DirList::FilesOnly, msk );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const Pos::GeomID geomid = getGeomIDFromFileName( dl.fullPath(idx) );
	if ( mIsUdfGeomID(geomid) )
	    continue;

	geomids += geomid;
    }

    return uiRetVal::OK();
}


#undef mErrRet
#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

SeisCBVS2DLineGetter::SeisCBVS2DLineGetter( const char* fnm, SeisTrcBuf& b,
					    int ntps, const Seis::SelData* sd )
	: Seis2DLineGetter(b,ntps,sd)
	, curnr_(0)
	, totnr_(0)
	, fname_(fnm)
	, trcstep_(1)
	, trcsperstep_(ntps)
{
    geomid_ = getGeomIDFromFileName( fname_ );
    tr_ = gtTransl( fname_, false, &msg_ );
    if ( !tr_ )
	return;

    tr_->setSelData( seldata_ );
    seldata_ = 0;

    totnr_ = tr_->packetInfo().crlrg.nrSteps() + 1;
}


SeisCBVS2DLineGetter::~SeisCBVS2DLineGetter()
{
    delete tr_;
}


const SeisTrcTranslator* SeisCBVS2DLineGetter::translator() const
{
    return tr_;
}


void SeisCBVS2DLineGetter::addTrc( SeisTrc* trc )
{
    const int tnr = trc->info().trcNr();
    if ( !isEmpty(seldata_) )
    {
	if ( seldata_->type() == Seis::Range )
	{
	    const BinID bid( geomid_, tnr );
	    if ( !seldata_->isOK(bid) )
		{ delete trc; return; }
	}
    }

    TrcKey tk( geomid_, tnr );
    trc->info().trckey_ = tk;
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
	    const uiString emsg = tr_->errMsg();
	    if ( !emsg.isEmpty() )
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


uiRetVal SeisCBVS2DLineIOProvider::getGeometry( const IOObj& obj,
			Pos::GeomID geomid, PosInfo::Line2DData& geom ) const
{
    uiRetVal uirv;
    geom.setEmpty();
    BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return uirv;
    }

    uiString errmsg;
    PtrMan<CBVSSeisTrcTranslator> trans = gtTransl( fnm, false, &errmsg );
    if ( !trans )
    {
	uirv.set( errmsg );
	return uirv;
    }

    const CBVSInfo& cbvsinf = trans->readMgr()->info();
    TypeSet<Coord> coords; TypeSet<BinID> binids;
    trans->readMgr()->getPositions( coords );
    trans->readMgr()->getPositions( binids );

    StepInterval<float> zrg( cbvsinf.sd_.start, 0, cbvsinf.sd_.step );
    zrg.stop = cbvsinf.sd_.start + (cbvsinf.nrsamples_-1) * cbvsinf.sd_.step;
    geom.setZRange( zrg );
    const int sz = mMIN(coords.size(),binids.size());
    for ( int idx=0; idx<sz; idx++ )
    {
	PosInfo::Line2DPos p( binids[idx].crl() );
	p.coord_ = coords[idx];
	geom.add( p );
    }

    return uirv;
}


Seis2DTraceGetter* SeisCBVS2DLineIOProvider::getTraceGetter( const IOObj& obj,
			Pos::GeomID geomid, uiRetVal& uirv )
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return 0;
    }

    // return new SeisCBVS2DTraceGetter( fnm );
    uirv.set( mTODONotImplPhrase() );
    return 0;
}


Seis2DLineGetter* SeisCBVS2DLineIOProvider::getLineGetter( const IOObj& obj,
			    Pos::GeomID geomid, SeisTrcBuf& tbuf,
			    const Seis::SelData* sd, uiRetVal& uirv, int npts )
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return 0;
    }

    return new SeisCBVS2DLineGetter( fnm, tbuf, npts, sd );
}


#undef mErrRet
#define mErrRet(s) { pErrMsg( s ); return 0; }

Seis2DLinePutter* SeisCBVS2DLineIOProvider::getPutter( const IOObj& obj,
				   Pos::GeomID geomid, uiRetVal& uirv )
{
    SeisCBVS2DLinePutter* ret = new SeisCBVS2DLinePutter( obj, geomid );
    uirv.set( ret->errMsg() );
    return ret;
}


//-------------------SeisCBVS2DLinePutter-----------------

SeisCBVS2DLinePutter::SeisCBVS2DLinePutter( const IOObj& obj,
					    Pos::GeomID geomid )
	: nrwr_(0)
	, fname_(SeisCBVS2DLineIOProvider::getFileName(obj,geomid))
	, tr_(CBVSSeisTrcTranslator::getInstance())
	, preseldt_(OD::AutoFPRep)
{
    tr_->set2D( true );
    bid_.inl() = geomid;
    File::Path fp( fname_ );
    const BufferString dirnm( fp.pathOnly() );
    DataCharacteristics::UserTypeDef().parse(
	    obj.pars().find(sKey::DataStorage()), preseldt_ );
    if ( !File::exists(dirnm) && !File::createDir(dirnm) )
	errmsg_ = tr("Cannot create directory '%1'").arg(fp.pathOnly() );
}


SeisCBVS2DLinePutter::~SeisCBVS2DLinePutter()
{
    delete tr_;
}


bool SeisCBVS2DLinePutter::put( const SeisTrc& trc )
{
    if ( nrwr_ == 0 )
    {
	tr_->setIs2D( true );
	bool res = tr_->initWrite(new StreamConn(fname_.buf(),Conn::Write),trc);
	if ( !res )
	{
	    errmsg_ = tr("Cannot open 2D line file:\n%1").arg(tr_->errMsg());
	    return false;
	}
	if ( preseldt_ != OD::AutoFPRep )
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
    if ( res )
	nrwr_++;
    else
    {
	errmsg_ = tr( "Cannot write %1 trace to 2D line file:\n%2")
	    .arg( uiString::getOrderString( nrwr_+1 ) )
	    .arg( tr_->errMsg() );
	return false;
    }
    return true;
}


bool SeisCBVS2DLinePutter::close()
{
    if ( !tr_ ) return true;
    tr_->setIs2D( true );
    bool ret = tr_->close();
    if ( !ret )
	errmsg_ = tr_->errMsg();
    return ret;
}
