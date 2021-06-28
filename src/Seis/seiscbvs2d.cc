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
    BufferString basenm = FilePath(fnm).baseName();
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

    FilePath fp( ret );
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
    FilePath parfp( fnm );
    parfp.setExtension( "par" );
    if ( File::exists(parfp.fullPath()) )
       File::remove( parfp.fullPath() );

    return ret;
}


bool SeisCBVS2DLineIOProvider::renameImpl( const IOObj& obj,
					   const char* newnm ) const
{
    const BufferString msk( "*.", sExtCBVS );
    DirList dl( obj.fullUserExpr(), File::FilesInDir, msk );
    bool ret = true;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const Pos::GeomID geomid = getGeomIDFromFileName( dl.fullPath(idx) );
	if ( geomid == mUdfGeomID )
	    continue;

	FilePath fp( dl.fullPath(idx) );
	BufferString newfnm( newnm );
	newfnm.add( mCapChar ).add( geomid );
	fp.setFileName( newfnm );
	fp.setExtension( sExtCBVS, false );
	if ( !File::rename(dl.fullPath(idx),fp.fullPath()) )
	    ret = false;

	FilePath oldparfp( dl.fullPath(idx) );
	oldparfp.setExtension( "par" );
	if ( !File::exists(oldparfp.fullPath()) )
	    continue;

	fp.setExtension( "par" );
	File::rename( oldparfp.fullPath(), fp.fullPath() );
    }

    return ret;
}


bool SeisCBVS2DLineIOProvider::getGeomIDs( const IOObj& obj,
					   TypeSet<Pos::GeomID>& geomids ) const
{
    geomids.erase();
    const BufferString msk( "*.", sExtCBVS );
    DirList dl( obj.fullUserExpr(), File::FilesInDir, msk );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const Pos::GeomID geomid = getGeomIDFromFileName( dl.fullPath(idx) );
	if ( geomid == mUdfGeomID )
	    continue;

	geomids += geomid;
    }

    return true;
}


#undef mErrRet
#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

SeisCBVS2DLineGetter::SeisCBVS2DLineGetter( const char* fnm, SeisTrcBuf& b,
					    int ntps, const Seis::SelData& sd )
	: Seis2DLineGetter(b,ntps,sd)
	, curnr_(0)
	, totnr_(0)
	, fname_(fnm)
	, trcstep_(1)
	, linenr_(CBVSIOMgr::getFileNr(fnm))
	, trcsperstep_(ntps)
{
    tr_ = gtTransl( fname_, false, &msg_ );
    if ( !tr_ ) return;

    if ( !sd.isAll() && sd.type() == Seis::Range )
	tr_->setSelData( seldata_ );

    tr_->commitSelections();

    totnr_ = tr_->packetInfo().crlrg.nrSteps() + 1;
}


SeisCBVS2DLineGetter::~SeisCBVS2DLineGetter()
{
    delete tr_;
}

const SeisTrcTranslator* SeisCBVS2DLineGetter::translator() const
{ return tr_; }

void SeisCBVS2DLineGetter::addTrc( SeisTrc* trc )
{
    const int tnr = trc->info().binid.crl();
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
	    const uiString emsg = tr_->errMsg();
	    if ( emsg.isSet() )
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


bool SeisCBVS2DLineIOProvider::getGeometry( const IOObj& obj,
			Pos::GeomID geomid, PosInfo::Line2DData& geom ) const
{
    geom.setEmpty();
    BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg( errmsg );
	return false;
    }

    uiString errmsg;
    PtrMan<CBVSSeisTrcTranslator> trans = gtTransl( fnm, false, &errmsg );
    if ( !trans )
    {
	ErrMsg( errmsg.getFullString() );
	return false;
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

    return true;
}


Executor* SeisCBVS2DLineIOProvider::getFetcher( const IOObj& obj,
						Pos::GeomID geomid,
						SeisTrcBuf& tbuf, int ntps,
						const Seis::SelData* sd )
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
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

Seis2DLinePutter* SeisCBVS2DLineIOProvider::getPutter( const IOObj& obj,
						       Pos::GeomID geomid )
{ return new SeisCBVS2DLinePutter( obj, geomid ); }


//-------------------SeisCBVS2DLinePutter-----------------

SeisCBVS2DLinePutter::SeisCBVS2DLinePutter( const IOObj& obj,
					    Pos::GeomID geomid )
	: nrwr_(0)
	, fname_(SeisCBVS2DLineIOProvider::getFileName(obj,geomid))
	, tr_(CBVSSeisTrcTranslator::getInstance())
	, preseldt_(DataCharacteristics::Auto)
{
    tr_->set2D( true );
    bid_.inl() = geomid;
    FilePath fp( fname_ );
    if ( !File::exists(fp.pathOnly()) )
	File::createDir( fp.pathOnly() );

    DataCharacteristics::parseEnumUserType(
	    obj.pars().find(sKey::DataStorage()), preseldt_ );

    tr_->commitSelections();
}


SeisCBVS2DLinePutter::~SeisCBVS2DLinePutter()
{
    delete tr_;
}


bool SeisCBVS2DLinePutter::put( const SeisTrc& trc )
{
    SeisTrcInfo& info = const_cast<SeisTrcInfo&>( trc.info() );
    bid_.crl() = info.nr;
    const BinID oldbid = info.binid;
    info.binid = bid_;

    if ( nrwr_ == 0 )
    {
	tr_->setIs2D( true );
	bool res = tr_->initWrite(new StreamConn(fname_.buf(),Conn::Write),trc);
	if ( !res )
	{
	    info.binid = oldbid;
	    errmsg_ = tr("Cannot open 2D line file:\n%1").arg(tr_->errMsg());
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
	errmsg_ = tr( "Cannot write %1 trace to 2D line file:\n%2")
	    .arg( uiString::getOrderString( nrwr_+1 ) )
	    .arg( tr_->errMsg() );
	return false;
    }
    return true;
}


bool SeisCBVS2DLinePutter::close()
{
    if ( !tr_ )
	return true;

    tr_->setIs2D( true );
    const bool ret = tr_->close();
    if ( !ret )
	errmsg_ = tr_->errMsg();

    return ret;
}


void SeisCBVS2DLinePutter::setComponentNames(const BufferStringSet& names )
{
    if ( tr_ )
	tr_->setComponentNames( names );
}
