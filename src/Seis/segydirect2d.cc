/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "segydirect2d.h"

#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "seistrc.h"
#include "seispacketinfo.h"
#include "seisselection.h"
#include "seisbuf.h"
#include "survgeom2d.h"
#include "survinfo.h"

#define mCapChar '^'

static const char* sExtSEGDirect = "sgydef";

int SEGYDirect2DLineIOProvider::factid_
	= (S2DLIOPs() += new SEGYDirect2DLineIOProvider).size() - 1;

static Pos::GeomID getGeomIDFromFileName( const char* fnm )
{
    Pos::GeomID geomid = mUdfGeomID;
    BufferString basenm = FilePath(fnm).baseName();
    char* capstr = basenm.find( mCapChar );
    if ( !capstr )
	return geomid;

    capstr++;
    geomid.set( toInt(capstr,mUdfGeomID.asInt()) );
    mDynamicCastGet( const Survey::Geometry2D*, geom2d,
		     Survey::GM().getGeometry(geomid) );
    return geom2d ? geomid : mUdfGeomID;
}


const OD::String& SEGYDirect2DLineIOProvider::getFileName( const IOObj& obj,
							 Pos::GeomID geomid )
{
    mDeclStaticString( ret );
    ret = obj.fullUserExpr();
    if ( ret.isEmpty() ) return ret;

    ret = SEGY::DirectDef::get2DFileName( ret, geomid );
    return ret;
}


SEGYDirect2DLineIOProvider::SEGYDirect2DLineIOProvider()
	: Seis2DLineIOProvider(SEGYDirectSeisTrc2DTranslator::translKey() )
{
}


SEGYDirect2DLineIOProvider::~SEGYDirect2DLineIOProvider()
{}


bool SEGYDirect2DLineIOProvider::isEmpty( const IOObj& obj,
					Pos::GeomID geomid ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    return fnm.isEmpty() || File::isEmpty(fnm);
}


bool SEGYDirect2DLineIOProvider::getTxtInfo( const IOObj& obj,
					     Pos::GeomID geomid,
					     BufferString& uinf,
					     BufferString& stdinf ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    SEGY::DirectDef sgydef( fnm );
    PtrMan<SEGYSeisTrcTranslator> trans =
		    SEGYDirectSeisTrcTranslator::createTranslator( sgydef, 0 );
    if ( !trans )
	return false;

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
    const OD::String& fnm = getFileName( obj, geomid );
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
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() )
	return false;

    // Also remove SEGY files if created by OD?
    return File::remove( fnm.buf() );
}


bool SEGYDirect2DLineIOProvider::renameImpl( const IOObj& obj,
					   const char* newnm ) const
{
    const BufferString msk( "*.", sExtSEGDirect );
    DirList dl( obj.fullUserExpr(), File::FilesInDir, msk );
    bool ret = true;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const Pos::GeomID geomid = getGeomIDFromFileName( dl.fullPath(idx) );
	if ( geomid == mUdfGeomID )
	    continue;

	FilePath fp( dl.fullPath(idx) );
	BufferString newfnm( newnm );
	newfnm.add( mCapChar ).add( geomid.asInt() );
	fp.setFileName( newfnm );
	fp.setExtension( sExtSEGDirect, false );
	if ( !File::rename(dl.fullPath(idx),fp.fullPath()) )
	    ret = false;
    }

    return ret;
}


bool SEGYDirect2DLineIOProvider::getGeomIDs( const IOObj& obj,
					   TypeSet<Pos::GeomID>& geomids ) const
{
    geomids.erase();
    const BufferString msk( "*.", sExtSEGDirect );
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

SEGYDirect2DLineGetter::SEGYDirect2DLineGetter( const char* fnm, SeisTrcBuf& b,
					    int ntps, const Seis::SelData& sd )
    : Seis2DLineGetter(b,ntps,sd)
    , fname_(fnm)
    , trcsperstep_(ntps)
{
    SEGY::DirectDef sgydef( fname_ );
    tr_ = SEGYDirectSeisTrcTranslator::createTranslator( sgydef, 0 );
    if ( !tr_ )
	return;

    if ( !sd.isAll() && sd.type() == Seis::Range )
	tr_->setSelData( seldata_ );

    totnr_ = tr_->packetInfo().crlrg.nrSteps() + 1;
}


SEGYDirect2DLineGetter::~SEGYDirect2DLineGetter()
{
    delete tr_;
}

const SeisTrcTranslator* SEGYDirect2DLineGetter::translator() const
{ return tr_; }

void SEGYDirect2DLineGetter::addTrc( SeisTrc* trc )
{
    if ( !isEmpty(seldata_) )
    {
	if ( seldata_->type() == Seis::Range )
	{
	    if ( !seldata_->isOK(trc->info().geomID(),trc->info().trcNr()) )
		{ delete trc; return; }
	}
    }

    tbuf_.add( trc );
}


int SEGYDirect2DLineGetter::nextStep()
{
    if ( !tr_ )
	return ErrorOccurred();

    if ( curnr_ == 0 )
    {
	if ( !isEmpty(seldata_) )
	{
	    const BinID tstepbid( 1, 1 );
	    const int nrsel = seldata_->expectedNrTraces( true, &tstepbid );
	    if ( nrsel < totnr_ ) totnr_ = nrsel;
	}
    }

    int lastnr = curnr_ + trcsperstep_;
    for ( ; curnr_<lastnr; curnr_++ )
    {
	auto* trc = new SeisTrc;
	if ( !tr_->read(*trc) )
	{
	    delete trc;
	    const uiString emsg = tr_->errMsg();
	    if ( emsg.isSet() )
		mErrRet(emsg)
	    return Finished();
	}

	addTrc( trc );
    }

    return MoreToDo();
}


#undef mErrRet


bool SEGYDirect2DLineIOProvider::getGeometry( const IOObj& obj,
			Pos::GeomID geomid, PosInfo::Line2DData& geom ) const
{
    const OD::String& fnm = getFileName( obj, geomid );
    if ( fnm.isEmpty() || !File::exists(fnm) )
    {
	BufferString errmsg = "2D seismic line file '"; errmsg += fnm;
	errmsg += "' does not exist";
	ErrMsg( errmsg );
	return 0;
    }

    SEGY::DirectDef def( fnm );
    geom = def.lineData();
    return geom.size();
}


Executor* SEGYDirect2DLineIOProvider::getFetcher( const IOObj& obj,
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

    return new SEGYDirect2DLineGetter( fnm, tbuf, ntps, *usedsd );
}



Seis2DLinePutter* SEGYDirect2DLineIOProvider::getPutter( const IOObj& obj,
						       Pos::GeomID geomid )
{ return new SEGYDirect2DLinePutter( obj, geomid ); }


//-------------------SEGYDirect2DLinePutter-----------------

SEGYDirect2DLinePutter::SEGYDirect2DLinePutter( const IOObj& obj,
					    Pos::GeomID geomid )
    : preseldt_(DataCharacteristics::Auto)
    , fname_(SEGYDirect2DLineIOProvider::getFileName(obj,geomid))
{
    bid_.row() = geomid.asInt();
    FilePath fp( fname_ );
    if ( !File::exists(fp.pathOnly()) )
	File::createDir( fp.pathOnly() );

    DataCharacteristics::parseEnumUserType(
	    obj.pars().find(sKey::DataStorage()), preseldt_ );
}


SEGYDirect2DLinePutter::~SEGYDirect2DLinePutter()
{
    delete tr_;
}


bool SEGYDirect2DLinePutter::put( const SeisTrc& trc )
{
    SeisTrcInfo& info = const_cast<SeisTrcInfo&>( trc.info() );
    bid_.trcNr() = info.trcNr();
    const SeisTrcInfo::IdxType oldtrcnr = info.trcNr();
    info.setTrcNr( bid_.trcNr() );

    if ( nrwr_ == 0 )
    {
	tr_->setIs2D( true );
	bool res = tr_->initWrite(new StreamConn(fname_.buf(),Conn::Write),trc);
	if ( !res )
	{
	    info.setTrcNr( oldtrcnr );
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
    info.setTrcNr( oldtrcnr );
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


SEGYDirectSurvGeom2DTranslator::SEGYDirectSurvGeom2DTranslator(const char* s1,
							       const char* s2)
    : SurvGeom2DTranslator(s1,s2)
{}


SEGYDirectSurvGeom2DTranslator::~SEGYDirectSurvGeom2DTranslator()
{}


Survey::Geometry* SEGYDirectSurvGeom2DTranslator::readGeometry(
				const IOObj& ioobj, uiString& errmsg ) const
{
    MultiID segydirectid;
    if ( !ioobj.pars().get(sKeySEGYDirectID(),segydirectid) )
	return nullptr;

    PtrMan<IOObj> segydirectobj = IOM().get( segydirectid );
    if ( !segydirectobj )
	return nullptr;

    const Pos::GeomID geomid( ioobj.key().objectID() );
    const OD::String& segydeffnm =
	SEGYDirect2DLineIOProvider::getFileName( *segydirectobj, geomid );
    SEGY::DirectDef sgydef( segydeffnm );
    const PosInfo::Line2DData& ld = sgydef.lineData();
    if ( ld.isEmpty() )
	return nullptr;

    auto* data = new PosInfo::Line2DData( ld );
    data->setLineName( ioobj.name() );
    auto* geom = new Survey::Geometry2D( data );
    geom->setID( geomid );
    geom->touch();
    return geom;
}


bool SEGYDirectSurvGeom2DTranslator::writeGeometry( IOObj& ioobj,
						    Survey::Geometry& geom,
						    uiString& errmsg ) const
{
    pErrMsg("This function should not be called");
    return false;
}
