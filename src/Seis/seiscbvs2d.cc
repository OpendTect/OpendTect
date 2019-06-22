/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : June 2004
-*/


#include "seiscbvs2d.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisseldata.h"
#include "seisbuf.h"
#include "posinfo2d.h"
#include "cbvsio.h"
#include "dirlist.h"
#include "executor.h"
#include "survgeom2d.h"
#include "survinfo.h"
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
    if ( !capstr )
	return geomid;
    capstr++;
    geomid.setI( toInt(capstr,Pos::GeomID::udfVal()) );
    return geomid.is2D() ? geomid : Pos::GeomID();
}


BufferString SeisCBVS2DLineIOProvider::getFileName( const IOObj& obj,
						    Pos::GeomID geomid )
{
    BufferString ret = obj.mainFileName();
    if ( ret.isEmpty() )
	return ret;

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
    const BufferString fnm = getFileName( obj, geomid );
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
    const BufferString fnm = getFileName( obj, geomid );
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
    const BufferString fnm = getFileName( obj, geomid );
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
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    const bool ret = File::remove( fnm.buf() );
    File::Path parfp( fnm );
    parfp.setExtension( sParFileExtension() );
    if ( File::exists(parfp.fullPath()) )
       File::remove( parfp.fullPath() );

    return ret;
}


bool SeisCBVS2DLineIOProvider::renameImpl( const IOObj& obj,
					   const char* newnm ) const
{
    const BufferString msk( "*.", sExtCBVS );
    DirList dl( obj.mainFileName(), File::FilesInDir, msk );
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
	oldparfp.setExtension( sParFileExtension() );
	if ( !File::exists(oldparfp.fullPath()) )
	    continue;

	fp.setExtension( sParFileExtension() );
	File::rename( oldparfp.fullPath(), fp.fullPath() );
    }

    return ret;
}


uiRetVal SeisCBVS2DLineIOProvider::getGeomIDs( const IOObj& obj,
					       GeomIDSet& geomids ) const
{
    geomids.erase();
    const BufferString msk( "*.", sExtCBVS );
    DirList dl( obj.mainFileName(), File::FilesInDir, msk );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const Pos::GeomID geomid = getGeomIDFromFileName( dl.fullPath(idx) );
	if ( mIsUdfGeomID(geomid) )
	    continue;

	geomids += geomid;
    }

    return uiRetVal::OK();
}



class SeisCBVS2DTraceGetter : public Seis2DTraceGetter
{
public:

SeisCBVS2DTraceGetter( const IOObj& obj, Pos::GeomID geomid,
			 const Seis::SelData* sd )
    : Seis2DTraceGetter(obj,geomid,sd)
{
}

void mkTranslator() const
{
    const BufferString fnm
	= SeisCBVS2DLineIOProvider::getFileName( ioobj_, geomid_ );
    CBVSSeisTrcTranslator* cbvstr = gtTransl( fnm, false, &initmsg_ );
    if ( cbvstr )
    {
	cbvstr->set2D( true );
	cbvstr->setSingleFile( true );
    }
    tr_ = cbvstr;
}

};

#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }

uiRetVal SeisCBVS2DLineIOProvider::getGeometry( const IOObj& obj,
			Pos::GeomID geomid, PosInfo::Line2DData& geom ) const
{
    uiRetVal uirv;
    geom.setEmpty();
    const BufferString fnm = getFileName( obj, geomid );
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
		Pos::GeomID geomid, const Seis::SelData* sd, uiRetVal& uirv )
{
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return 0;
    }

    return new SeisCBVS2DTraceGetter( obj, geomid, sd );
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
	, preseldt_(OD::AutoDataRep)
{
    tr_->set2D( true );
    bid_.inl() = geomid.lineNr();
    File::Path fp( fname_ );
    const BufferString dirnm( fp.pathOnly() );
    DataCharacteristics::getUserTypeFromPar( obj.pars(), preseldt_ );
    if ( !File::exists(dirnm) && !File::createDir(dirnm) )
	errmsg_ = tr("Cannot create directory '%1'").arg(fp.pathOnly() );
    tr_->commitSelections();
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
	if ( preseldt_ != OD::AutoDataRep )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& ci
				= tr_->componentInfo();
	    DataCharacteristics dc( preseldt_ );
	    for ( int idx=0; idx<ci.size(); idx++ )
	    {
		SeisTrcTranslator::TargetComponentData& cd = *ci[idx];
		cd.datachar_ = dc;
	    }
	}
    }

    tr_->setIs2D( true );
    bool res = tr_->write(trc);
    if ( res )
	nrwr_++;
    else
    {
	errmsg_ = tr("Cannot write %1 trace to 2D line file")
		    .arg( uiString::getOrderString(nrwr_+1) )
		    .addMoreInfo( tr_->errMsg() );
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
