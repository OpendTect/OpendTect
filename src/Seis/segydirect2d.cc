/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Dec 2014
-*/


#include "segydirect2d.h"

#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisseldata.h"
#include "seisbuf.h"
#include "staticstring.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"

#define mCapChar '^'

static const char* sExtSEGDirect = "sgydef";

int SEGYDirect2DLineIOProvider::factid_
	= (S2DLIOPs() += new SEGYDirect2DLineIOProvider).size() - 1;

static Pos::GeomID getGeomIDFromFileName( const char* fnm )
{
    Pos::GeomID geomid = mUdfGeomID;
    BufferString basenm = File::Path(fnm).baseName();
    char* capstr = basenm.find( mCapChar );
    if ( !capstr )
	return geomid;
    capstr++;
    geomid = Pos::GeomID( toInt(capstr,Pos::GeomID().getI()) );
    const auto* geom2d = SurvGeom::get(geomid).as2D();
    return geom2d ? geomid : mUdfGeomID;
}


BufferString SEGYDirect2DLineIOProvider::getFileName( const IOObj& obj,
						      Pos::GeomID geomid )
{
    mDeclStaticString( ret );
    ret = obj.mainFileName();
    if ( ret.isEmpty() )
	return ret;

    ret = SEGY::DirectDef::get2DFileName( ret, geomid );
    return ret;
}


SEGYDirect2DLineIOProvider::SEGYDirect2DLineIOProvider()
	: Seis2DLineIOProvider(SEGYDirectSeisTrc2DTranslator::translKey() )
{
}


bool SEGYDirect2DLineIOProvider::isEmpty( const IOObj& obj,
					Pos::GeomID geomid ) const
{
    const BufferString fnm = getFileName( obj, geomid );
    return fnm.isEmpty() || File::isEmpty(fnm);
}


static SEGYSeisTrcTranslator* gtTransl( const char* fnm )
{
    SEGY::DirectDef sgydef( fnm );
    return SEGYDirectSeisTrcTranslator::createTranslator( sgydef, 0 );
}


bool SEGYDirect2DLineIOProvider::getTxtInfo( const IOObj& obj,
					     Pos::GeomID geomid,
					     BufferString& uinf,
					     BufferString& stdinf ) const
{
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    PtrMan<SEGYSeisTrcTranslator> trans = gtTransl( fnm );
    if ( !trans ) return false;

    const SeisPacketInfo& pinf = trans->packetInfo();
    uinf = pinf.usrinfo;
    stdinf = pinf.stdinfo;
    return true;
}

bool SEGYDirect2DLineIOProvider::getRanges( const IOObj& obj,
					    Pos::GeomID geomid,
					    StepInterval<int>& trcrg,
					    StepInterval<float>& zrg ) const
{
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
	return false;

    SEGY::DirectDef sgydef( fnm );
    const PosInfo::Line2DData& ld = sgydef.lineData();
    trcrg = ld.trcNrRange(); zrg = ld.zRange();
    return true;
}


bool SEGYDirect2DLineIOProvider::removeImpl( const IOObj& obj,
					   Pos::GeomID geomid ) const
{
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    // Also remove SEGY files if created by OD?
    return File::remove( fnm.buf() );
}


bool SEGYDirect2DLineIOProvider::renameImpl( const IOObj& obj,
					   const char* newnm ) const
{
    const BufferString msk( "*.", sExtSEGDirect );
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
	fp.setExtension( sExtSEGDirect, false );
	if ( !File::rename(dl.fullPath(idx),fp.fullPath()) )
	    ret = false;
    }

    return ret;
}


uiRetVal SEGYDirect2DLineIOProvider::getGeomIDs( const IOObj& obj,
						 GeomIDSet& geomids ) const
{
    geomids.erase();
    const BufferString msk( "*.", sExtSEGDirect );
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


class SEGYDirect2DTraceGetter : public Seis2DTraceGetter
{
public:

SEGYDirect2DTraceGetter( const IOObj& obj, Pos::GeomID geomid,
			 const Seis::SelData* sd )
    : Seis2DTraceGetter(obj,geomid,sd)
{
}

void mkTranslator() const
{
    tr_ = gtTransl( SEGYDirect2DLineIOProvider::getFileName(ioobj_,geomid_) );
    if ( tr_ )
	tr_->setIs2D( true );
}

};

#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }

uiRetVal SEGYDirect2DLineIOProvider::getGeometry( const IOObj& obj,
		Pos::GeomID geomid, PosInfo::Line2DData& geom ) const
{
    uiRetVal uirv;
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return uirv;
    }

    SEGY::DirectDef def( fnm );
    geom = def.lineData();
    return uirv;
}


Seis2DTraceGetter* SEGYDirect2DLineIOProvider::getTraceGetter( const IOObj& obj,
		Pos::GeomID geomid, const Seis::SelData* sd, uiRetVal& uirv )
{
    const BufferString fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	uirv.set( tr("2D seismic line file '%1' does not exist").arg(fnm) );
	return 0;
    }

    return new SEGYDirect2DTraceGetter( obj, geomid, sd );
}

#undef mErrRet
#define mErrRet(s) { pErrMsg( s ); return 0; }

Seis2DLinePutter* SEGYDirect2DLineIOProvider::getPutter( const IOObj& obj,
					   Pos::GeomID geomid, uiRetVal& uirv )
{
    SEGYDirect2DLinePutter* ret = new SEGYDirect2DLinePutter( obj, geomid );
    uirv.set( ret->errMsg() );
    return ret;
}


//-------------------SEGYDirect2DLinePutter-----------------

SEGYDirect2DLinePutter::SEGYDirect2DLinePutter( const IOObj& obj,
					    Pos::GeomID geomid )
	: nrwr_(0)
	, preseldt_(OD::AutoDataRep)
	, fname_(SEGYDirect2DLineIOProvider::getFileName(obj,geomid))
{
    bid_.inl() = geomid.lineNr();
    File::Path fp( fname_ );
    if ( !File::exists(fp.pathOnly()) )
	File::createDir( fp.pathOnly() );

    DataCharacteristics::getUserTypeFromPar( obj.pars(), preseldt_ );
}


SEGYDirect2DLinePutter::~SEGYDirect2DLinePutter()
{
    delete tr_;
}


bool SEGYDirect2DLinePutter::put( const SeisTrc& trc )
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

bool SEGYDirect2DLinePutter::close()
{
    if ( !tr_ ) return true;
    tr_->setIs2D( true );
    bool ret = tr_->close();
    if ( ret ) errmsg_ = tr_->errMsg();
    return ret;
}


const char* SEGYDirectSurvGeom2DTranslator::sKeySEGYDirectID()
{ return "SEGY Direct ID"; }

SurvGeom2D* SEGYDirectSurvGeom2DTranslator::readGeometry(
				const IOObj& ioobj, uiString& errmsg ) const
{
    DBKey segydirectid;
    if ( !ioobj.pars().get(sKeySEGYDirectID(),segydirectid) )
	return 0;

    PtrMan<IOObj> segydirectobj = segydirectid.getIOObj();
    if ( !segydirectobj )
	return 0;

    const Pos::GeomID geomid = geomIDOf( ioobj.key() );
    const BufferString segydeffnm =
	SEGYDirect2DLineIOProvider::getFileName( *segydirectobj, geomid );
    SEGY::DirectDef sgydef( segydeffnm );
    const PosInfo::Line2DData& ld = sgydef.lineData();
    if ( ld.isEmpty() )
	return 0;

    PosInfo::Line2DData* data = new PosInfo::Line2DData( ld );
    data->setLineName( ioobj.name() );
    SurvGeom2D* geom = new SurvGeom2D( data );
    geom->setGeomID( geomid );
    geom->commitChanges();
    return geom;
}


bool SEGYDirectSurvGeom2DTranslator::writeGeometry( IOObj& ioobj,
						    const Geometry2D& geom,
						    uiString& errmsg ) const
{
    pErrMsg("This function should not be called");
    return false;
}
