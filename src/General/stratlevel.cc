/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlevel.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "dirlist.h"
#include "genc.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "safefileio.h"


namespace Strat
{

static const char* sLevelExt = "stratlevels";
static const char* sUndefName = "<Undefined>";
static const char* sRegMarkerFnm = "well.regm";

static const int cRegMarkerIDStart = 10000;

const Level& Level::undef()
{
    static PtrMan<Level> lvl = new Level( sUndefName, OD::Color::Black() );
    return *lvl.ptr();
}


Level& Level::dummy()
{
    static PtrMan<Level> lvl =
		new Level( nullptr, OD::Color::Black(), LevelID::udf() );
    return *lvl.ptr();
}

} // namespace Strat


Strat::LevelSetMgr::LevelSetMgr()
    : curChanged(this)
{
    if ( !NeedDataBase() )
	return;

    if ( IOMan::isOK() )
	iomReadyCB( nullptr );
    else
	mAttachCB( IOMan::iomReady(), LevelSetMgr::iomReadyCB );
}


Strat::LevelSetMgr::~LevelSetMgr()
{
    detachAllNotifiers();
}


void Strat::LevelSetMgr::pushLevelSet( LevelSet* ls )
{
    Threads::Locker locker( lock_ );
    if ( ls )
	lss_.add( ls );

    curChanged.trigger();
}


void Strat::LevelSetMgr::popLevelSet()
{
    Threads::Locker locker( lock_ );
    lss_.pop();

    curChanged.trigger();
}


const Strat::LevelSet& Strat::LevelSetMgr::unpushedLVLS() const
{
    Threads::Locker locker( lock_ );
    return *lss_.first();
}


void Strat::LevelSetMgr::setLVLS( LevelSet* ls )
{
    if ( ls )
	ensurePresent( *ls );
}


const Strat::LevelSet& Strat::LevelSetMgr::curSet() const
{
    return getNonConst(*this).curSet();
}


Strat::LevelSet& Strat::LevelSetMgr::curSet()
{
    Threads::Locker locker( lock_ );
    if ( lss_.isEmpty() )
	createSet();

    return *lss_.last();
}


void Strat::LevelSetMgr::ensurePresent( LevelSet& lss )
{
    Threads::Locker locker( lock_ );
    if ( lss_.isEmpty() )
	lss_.add( &lss );
    else
    {
	const int lastidx = lss_.size()-1;
	lss_.replace( lastidx, &lss );
    }
}


void Strat::LevelSetMgr::createSet()
{
    Repos::FileProvider rfp( "StratLevels", true );
    LevelSet* ls = nullptr;
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	auto* tmp = new LevelSet;
	if ( !tmp->readFrom(rfp.fileName()) || tmp->isEmpty() )
	    delete tmp;
	else
	    { ls = tmp; break; }
    }

    if ( !ls )
    {
	ls = new LevelSet;
	ls->readOldRepos();
    }

    lss_.add( ls );
}


void Strat::LevelSetMgr::iomReadyCB( CallBacker* )
{
    mAttachCB( IOM().surveyChanged, LevelSetMgr::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, LevelSetMgr::surveyChangedCB );
}


void Strat::LevelSetMgr::surveyChangedCB( CallBacker* )
{
    lss_.erase();
}


Strat::LevelSetMgr& Strat::lvlSetMgr()
{
    static PtrMan<Strat::LevelSetMgr> lssmgr_ = new Strat::LevelSetMgr();
    return *lssmgr_.ptr();
}


const Strat::LevelSet& Strat::LVLS()
{
    const LevelSetMgr& mgr = lvlSetMgr();
    return mgr.curSet();
}


Strat::Level::Level( const char* nm, const OD::Color& col, LevelID newid )
    : NamedCallBacker(nm)
    , changed(this)
    , id_(newid)
    , color_(col)
    , pars_(*new IOPar)
{
}


Strat::Level::Level( const Level& oth, int lvlid )
    : NamedCallBacker(oth.name())
    , changed(this)
    , id_(lvlid)
    , color_(oth.color_)
    , pars_(*new IOPar(oth.pars_))
{
}


Strat::Level::Level( const Level& oth )
    : NamedCallBacker(oth.name())
    , changed(this)
    , id_(0)
    , pars_(*new IOPar(oth.pars_))
{
    *this = oth;
}


Strat::Level::~Level()
{
    delete &pars_;
}


Strat::Level& Strat::Level::operator =( const Level& oth )
{
    if ( &oth == this )
	return *this;

    NamedObject::operator =( oth );
    const_cast<LevelID&>( id_ ) = oth.id_;
    color_ = oth.color_;
    pars_ = oth.pars_;

    return *this;
}


bool Strat::Level::operator ==( const Level& oth ) const
{
    return id_ == oth.id_ && color_ == oth.color_ &&
	    NamedCallBacker::operator==( oth ) && pars_ == oth.pars_;
}


bool Strat::Level::operator !=( const Level& oth ) const
{
    return !(*this == oth);
}


bool Strat::Level::isDifferentFrom( const Level& lvl ) const
{
    if ( this == &lvl )
	return false;

    return name() != lvl.name()
	|| color_ != lvl.color_
	|| pars_ != lvl.pars_;
}


bool Strat::Level::isUndef() const
{
    return name() == sUndefName;
}


Strat::Level& Strat::Level::setID( LevelID id )
{
    if ( id_ == id )
	return *this;

    const_cast<LevelID&>( id_ ) = id;
    return *this;
}


void Strat::Level::setName( const char* nm )
{
    if ( name() == nm )
	return;

    NamedObject::setName( nm );
    changed.trigger( cNameChange() );
}


Strat::Level& Strat::Level::setColor( OD::Color c )
{
    if ( color_ == c )
	return *this;

    color_ = c;
    changed.trigger( cColChange() );
    return *this;
}


Strat::Level& Strat::Level::setPars( const IOPar& iop )
{
    if ( pars_ == iop )
	return *this;

    pars_ = iop;
    changed.trigger( cParsChange() );
    return *this;
}


void Strat::Level::fillPar( IOPar& iop ) const
{
    iop.set( sKey::ID(), id_ );
    iop.set( sKey::Name(), name() );
    iop.set( sKey::Color(), color_ );
    iop.merge( pars_ );
}


void Strat::Level::usePar( const IOPar& iop )
{
    BufferString nm( name() );
    iop.get( sKey::Name(), nm );
    setName( nm );

    iop.get( sKey::ID(), const_cast<LevelID&>(id_) );
    iop.get( sKey::Color(), color_ );

    pars_.merge( iop );
    pars_.removeWithKey( sKey::Name() );
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::ID() );
    changed.trigger( cEntireChange() );
}



Strat::LevelSet::LevelSet( int idnr )
    : NamedCallBacker("")
    , setChanged(this)
    , levelAdded(this)
    , levelToBeRemoved(this)
    , curlevelid_(idnr)
{
    const Strat::RegMarkerSet& regmset = RGMLVLS();
    mAttachCB( regmset.levelAdded, Strat::LevelSet::regMarkerAdded );
    mAttachCB( regmset.levelToBeRemoved, Strat::LevelSet::regMarkerRemoved );
    addRegMarkers();
}


Strat::LevelSet::LevelSet( const LevelSet& oth )
    : NamedCallBacker(oth.name())
    , setChanged(this)
    , levelAdded(this)
    , levelToBeRemoved(this)
{
    *this = oth;
}


Strat::LevelSet::~LevelSet()
{
    detachAllNotifiers();
    deepErase( lvls_ );
}


void Strat::LevelSet::addRegMarkers()
{
    const Strat::RegMarkerSet& regmset = RGMLVLS();
    for ( int idx=0; idx<regmset.size(); idx++ )
    {
	const Level& lvl = regmset.getByIdx( idx );
	lvls_.add( new Level(lvl) );
    }
}


void Strat::LevelSet::regMarkerAdded( CallBacker* cb )
{
    mCBCapsuleUnpack( Strat::LevelID, regmid, cb );
    if ( regmid.isUdf() || !RegMarker::isRegMarker(regmid)
			|| isPresent(regmid) )
	return;

    const Level& regm = Strat::RGMLVLS().get( regmid );
    lvls_.add( new Level(regm) );
}


void Strat::LevelSet::regMarkerRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack( Strat::LevelID, regmid, cb );
    if ( regmid.isUdf() || !RegMarker::isRegMarker(regmid) )
	return;

    const int idx = indexOf( regmid );
    if ( !lvls_.validIdx(idx) || lvls_[idx]->id()!=regmid )
	return;

    lvls_.removeSingle( idx );
}


Strat::LevelSet& Strat::LevelSet::operator =( const LevelSet& oth )
{
    if ( this != &oth )
    {
	NamedObject::operator =( oth );
	deepCopy( lvls_, oth.lvls_ );
	curlevelid_ = oth.curlevelid_;
	setChanged.trigger();
    }

    return *this;
}


bool Strat::LevelSet::operator ==( const LevelSet& oth ) const
{
    if ( &oth == this )
	return true;

    if ( lvls_.size() != oth.lvls_.size() )
	return false;

    for ( int idx=0; idx<lvls_.size(); idx++ )
	if ( *lvls_[idx] != *oth.lvls_[idx] )
	    return false;

    return true;
}


int Strat::LevelSet::size() const
{
    return lvls_.size();
}


void Strat::LevelSet::doSetEmpty()
{
    for ( int idx=lvls_.size()-1; idx>=0; idx-- )
    {
	const LevelID lvlid = lvls_.get(idx)->id();
	if ( lvlid.isValid() && RegMarker::isRegMarker(lvlid) )
	    continue;

	delete lvls_.removeSingle( idx );
    }
}


void Strat::LevelSet::setEmpty()
{
    if ( isEmpty() )
	return;

    doSetEmpty();
    setChanged.trigger();
}


bool Strat::LevelSet::isPresent( LevelID id ) const
{
    return lvls_.validIdx( indexOf(id) );
}


bool Strat::LevelSet::isPresent( const char* nm ) const
{
    return lvls_.validIdx( indexOf(nm) );
}


Strat::LevelID Strat::LevelSet::getIDByName( const char* nm ) const
{
    const int idx = indexOf( nm );
    return lvls_.validIdx(idx) ? lvls_.get(idx)->id() : LevelID::udf();
}


Strat::LevelID Strat::LevelSet::getIDByIdx( int idx ) const
{
    return lvls_.validIdx(idx) ? lvls_.get(idx)->id() : LevelID::udf();
}


int Strat::LevelSet::indexOf( LevelID id ) const
{
    return gtIdxOf( nullptr, id );
}


int Strat::LevelSet::indexOf( const char* nm ) const
{
    return gtIdxOf( nm, LevelID::udf() );
}


int Strat::LevelSet::gtIdxOf( const char* nm, LevelID id ) const
{
    const bool useid = id.isValid();
    const bool usenm = nm && *nm;
    if ( !useid && !usenm )
	return -1;

    for ( int ilvl=0; ilvl<lvls_.size(); ilvl++ )
    {
	const Level& lvl = *lvls_[ilvl];
	if ( (useid && lvl.id() == id) || (usenm && lvl.name() == nm) )
	    return ilvl;
    }

    return -1;
}


BufferString Strat::LevelSet::nameOf( LevelID id ) const
{
    const int idx = indexOf( id );
    return lvls_.validIdx(idx) ? lvls_.get(idx)->name() : BufferString::empty();
}


OD::Color Strat::LevelSet::colorOf( LevelID id ) const
{
    const int idx = indexOf( id );
    return lvls_.validIdx(idx) ? lvls_.get(idx)->color() : OD::Color();
}


IOPar Strat::LevelSet::parsOf( LevelID id ) const
{
    const int idx = indexOf( id );
    return lvls_.validIdx(idx) ? lvls_.get(idx)->pars() : IOPar();
}


Strat::Level Strat::LevelSet::get( LevelID id ) const
{
    return gtLvl( indexOf(id) );
}


Strat::Level Strat::LevelSet::getByIdx( int idx ) const
{
    return gtLvl( idx );
}


Strat::Level Strat::LevelSet::getByName( const char* nm ) const
{
    return gtLvl( indexOf(nm) );
}


Strat::Level Strat::LevelSet::first() const
{
    return gtLvl( 0 );
}


Strat::Level Strat::LevelSet::gtLvl( int idx ) const
{
    return lvls_.validIdx(idx) ? *lvls_.get(idx) : Level::undef();
}


void Strat::LevelSet::getNames( BufferStringSet& nms ) const
{
    for ( const auto* lvl : lvls_ )
	nms.add( lvl->name().buf() );
}


Strat::LevelID Strat::LevelSet::doSet( const Strat::Level& lvl, bool* isnew )
{
    if ( RegMarker::isRegMarker(lvl) )
    {
	pErrMsg( "Please use RGMLVLS to set Regional markers" );
	return Strat::LevelID::udf();
    }

    const int idx = indexOf( lvl.name().buf() );
    Level* chglvl;
    if ( lvls_.validIdx(idx) )
    {
	if ( isnew )
	    *isnew = false;

	chglvl = lvls_.get( idx );
	*chglvl = lvl;
    }
    else
    {
	if ( isnew )
	    *isnew = true;

	if ( lvl.id().isUdf() || lvl.id().asInt() < curlevelid_ )
	    const_cast<Level&>( lvl ).setID( LevelID(++curlevelid_) );

	const LevelID id = lvl.id();
	if ( isPresent(id) )
	{
	    remove( id );
	    levelToBeRemoved.trigger( id );
	}

	chglvl = new Level( lvl );
	lvls_.add( chglvl );
	levelAdded.trigger( chglvl->id() );
    }

    return chglvl->id();
}


Strat::LevelID Strat::LevelSet::add( const char* nm, const OD::Color& col )
{
    const LevelID lvlid( curlevelid_++ );
    Level lvl( nm, col, lvlid );
    return doSet( lvl );
}


void Strat::LevelSet::remove( LevelID id )
{
    if ( !isPresent(id) || RegMarker::isRegMarker(id) )
	return;

    levelToBeRemoved.trigger( id );
    const int idx = indexOf( id );
    lvls_.removeSingle( idx );
}


Strat::LevelID Strat::LevelSet::set( const Level& lvl )
{
    return doSet( lvl );
}


void Strat::LevelSet::add( const BufferStringSet& lvlnms,
			   const TypeSet<OD::Color>& cols )
{
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	add( Level(lvlnms.get(idx),cols[idx]) );
}


void Strat::LevelSet::getFromStream( ascistream& astrm, bool isold )
{
    while ( true )
    {
	IOPar iop; iop.getFrom( astrm );
	if ( iop.isEmpty() )
	    break;
	if ( isold && iop.name() != "Level" )
	    continue;

	Level lvl( nullptr, OD::Color() );
	lvl.usePar( iop );
	if ( lvl.id().isUdf() )
	     lvl.setID( LevelID(++curlevelid_) );

	if ( isold )
	{
	    // Remove legacy keys
	    lvl.pars_.removeWithKey( "Unit" );
	    lvl.pars_.removeWithKey( "Time" );
	}

	doSet( lvl );
	curlevelid_.setIfLarger( lvl.id().asInt()+1 );
    }
}


bool Strat::LevelSet::readFrom( const char* fnm )
{
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) )
	return false;

    ascistream astrm( sfio.istrm(), true );
    if ( astrm.type() == ascistream::EndOfFile )
    {
	sfio.closeFail();
	return false;
    }

    doSetEmpty();
    getFromStream( astrm, false );
    sfio.closeSuccess();
    setChanged.trigger();
    return true;
}



Repos::Source Strat::LevelSet::readOldRepos()
{
    Repos::FileProvider rfp( "StratUnits" );
    BufferString bestfile;
    Repos::Source rsrc = Repos::Temp;
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	if ( File::exists(fnm) )
	    { bestfile = fnm; rsrc = rfp.source(); }
    }
    if ( bestfile.isEmpty() )
	return Repos::Temp;

    od_istream strm( bestfile );
    if ( !strm.isOK() )
	return Repos::Temp;

    ascistream astrm( strm, true );
    getFromStream( astrm, true );

    return rsrc;
}


bool Strat::LevelSet::writeTo( const char* fnm ) const
{
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
	return false;

    ascostream astrm( sfio.ostrm() );
    if ( !astrm.putHeader("Stratigraphic Levels") )
	{ sfio.closeFail(); return false; }

    for ( int ilvl=0; ilvl<lvls_.size(); ilvl++ )
    {
	const Level& lvl = *lvls_[ilvl];
	if ( RegMarker::isRegMarker(lvl) )
	    continue;

	IOPar iop;
	lvl.fillPar( iop );
	iop.putTo( astrm );
    }

    sfio.closeSuccess();
    return true;
}


BufferString Strat::getStdFileName( const char* inpnm, const char* basenm )
{
    BufferString filenm( basenm );
    BufferString nm( inpnm );
    nm.replace( ' ', '_' );
    if ( !nm.isEmpty()	)
	filenm.add( "." ).add( nm );

    const BufferString ret = GetSetupShareFileInDir( "Strat", filenm.buf() );
    return ret;
}


void Strat::LevelSet::getStdNames( BufferStringSet& nms )
{
    BufferStringSet levelfnms;
    if ( !GetSetupShareFilesInDir("Strat","Levels.*",levelfnms,true) )
	return;

    for ( const auto* fnm : levelfnms )
    {
	const FilePath fp( fnm->str() );
	BufferString nm( fp.extension() );
	nm.replace( '_', ' ' );
	nms.add( nm );
    }
}


Strat::LevelSet* Strat::LevelSet::createStd( const char* nm )
{
    auto* ret = new LevelSet;
    ret->readFrom( getStdFileName(nm,"Levels") );
    return ret;
}


Strat::LevelSet* Strat::LevelSet::read( const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return nullptr;

    FilePath fp( ioobj->fullUserExpr() );
    fp.setExtension( sLevelExt );

    auto* ret = new LevelSet;
    if ( !ret->readFrom(fp.fullPath()) )
	deleteAndNullPtr( ret );

    if ( ret )
	ret->dbky_ = key;

    return ret;
}


bool Strat::LevelSet::write() const
{
    return dbky_.isUdf() ? false : write(*this, dbky_);
}


bool Strat::LevelSet::write( const LevelSet& ls, const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    FilePath fp( ioobj->fullUserExpr() );
    fp.setExtension( sLevelExt );

    return ls.writeTo( fp.fullPath() );
}


void Strat::LevelSet::removeAllRegMarkers()
{
    for ( int idx=lvls_.size()-1; idx>=0; idx-- )
    {
	const Strat::LevelID lvlid = lvls_.get(idx)->id();
	if ( lvlid.isValid() && !RegMarker::isRegMarker(lvlid) )
	    continue;

	lvls_.removeSingle( idx );
    }
}


Strat::RegMarker::RegMarker( const char* nm, const OD::Color& col, LevelID id )
    : Level(nm,col,id)
{}


Strat::RegMarker::RegMarker( const RegMarker& oth )
    : Level(oth.name().buf(),oth.color(),oth.id())
{
    *this = oth;
}


Strat::RegMarker::~RegMarker()
{}


Strat::RegMarker& Strat::RegMarker::setID( LevelID id )
{
    if ( id.asInt() < cRegMarkerIDStart )
	return *this;

    const_cast<LevelID&>( id_ ) = id;
    return *this;
}


bool Strat::RegMarker::isRegMarker( const LevelID lvlid )
{
    if ( lvlid.isUdf() || !lvlid.isValid() )
	return false;

    return lvlid.asInt() >= cRegMarkerIDStart;
}


bool Strat::RegMarker::isRegMarker( const Level& lvl )
{
    return isRegMarker( lvl.id() );
}


int Strat::RegMarker::minID()
{
    return cRegMarkerIDStart;
}


Strat::RegMarkerSet::RegMarkerSet()
    : levelChanged(this)
    , levelAdded(this)
    , levelToBeRemoved(this)
{
    if ( IOMan::isOK() )
	iomReadyCB( nullptr );
    else
	mAttachCB( IOMan::iomReady(), RegMarkerSet::iomReadyCB );

    readRegMarkers();
}


Strat::RegMarkerSet::~RegMarkerSet()
{}


int Strat::RegMarkerSet::size() const
{
    return rgmlvls_.size();
}


bool Strat::RegMarkerSet::isEmpty() const
{
    return size() <= 0;
}


void Strat::RegMarkerSet::setEmpty()
{
    Strat::eLVLS().removeAllRegMarkers();
    rgmlvls_.setEmpty();
}


bool Strat::RegMarkerSet::isPresent( const LevelID& id ) const
{
    return rgmlvls_.validIdx( indexOf(id) );
}


bool Strat::RegMarkerSet::isPresent( const char* nm ) const
{
    return rgmlvls_.validIdx( indexOf(nm) );
}


int Strat::RegMarkerSet::indexOf( const LevelID& id ) const
{
    return gtIdxOf( nullptr, id );
}


int Strat::RegMarkerSet::indexOf( const char* nm ) const
{
    return gtIdxOf( nm, LevelID::udf() );
}


const Strat::RegMarker& Strat::RegMarkerSet::get( const LevelID& id ) const
{
    return getRegMarker( indexOf(id) );
}


Strat::RegMarker& Strat::RegMarkerSet::get( const LevelID& id )
{
    return const_cast<RegMarker&>(
		    static_cast<const RegMarkerSet&>(*this).get(id) );
}


const Strat::RegMarker& Strat::RegMarkerSet::getByIdx( int idx ) const
{
    return getRegMarker(idx);
}


Strat::RegMarker& Strat::RegMarkerSet::getByIdx( int idx )
{
    return const_cast<RegMarker&>(
		    static_cast<const RegMarkerSet&>(*this).getByIdx(idx) );
}


const Strat::RegMarker& Strat::RegMarkerSet::getByName( const char* nm ) const
{
    return getRegMarker( indexOf(nm) );
}


Strat::RegMarker& Strat::RegMarkerSet::getByName( const char* nm )
{
    return const_cast<RegMarker&>(
		    static_cast<const RegMarkerSet&>(*this).getByName(nm) );
}


const Strat::RegMarker& Strat::RegMarkerSet::getRegMarker( int idx ) const
{
    return rgmlvls_.validIdx(idx) ? *rgmlvls_.get(idx)
				  : (Strat::RegMarker&)Level::undef();
}


int Strat::RegMarkerSet::gtIdxOf( const char* nm, const LevelID& id ) const
{
    const bool useid = id.isValid();
    const bool usenm = nm && *nm;
    if ( !useid && !usenm )
	return -1;

    for ( int ilvl=0; ilvl<rgmlvls_.size(); ilvl++ )
    {
	const Level& lvl = *rgmlvls_[ilvl];
	if ( (useid && lvl.id() == id) || (usenm && lvl.name() == nm) )
	    return ilvl;
    }

    return -1;
}


void Strat::RegMarkerSet::readRegMarkers()
{
    FilePath regmfp( sRegMarkerFnm );
    const BufferString modeldir
		= IOObjContext::getDataDirName( IOObjContext::Mdl, false );
    regmfp.setPath( modeldir.buf() );
    if ( !File::exists(regmfp.fullPath()) )
	return;

    od_istream strm( regmfp );
    ascistream istrm( strm );
    if ( !istrm.isOK() )
	return;

    while ( true )
    {
	IOPar iop;
	iop.getFrom( istrm );
	if ( iop.isEmpty() )
	    break;

	RegMarker lvl( nullptr, OD::Color() );
	lvl.usePar( iop );
	if ( lvl.id().isUdf() )
	    lvl.setID( LevelID(++curregmid_) );

	doSet( lvl );
	curregmid_.setIfLarger( lvl.id().asInt()+1 );
    }
}


bool Strat::RegMarkerSet::save() const
{
    return writeRegMarkers();
}


bool Strat::RegMarkerSet::writeRegMarkers() const
{
    FilePath regmfp( sRegMarkerFnm );
    const BufferString modeldir
		= IOObjContext::getDataDirName( IOObjContext::Mdl, false );
    regmfp.setPath( modeldir.buf() );
    od_ostream strm( regmfp );
    ascostream ostrm( strm );
    if ( !ostrm.isOK() )
	return false;

    if ( !ostrm.putHeader("Regional Marker Levels") )
	return false;

    for ( int ilvl=0; ilvl<rgmlvls_.size(); ilvl++ )
    {
	const RegMarker& lvl = *rgmlvls_[ilvl];
	IOPar iop;
	lvl.fillPar( iop );
	iop.putTo( ostrm );
    }

    return true;
}


Strat::LevelID Strat::RegMarkerSet::doSet( const Strat::RegMarker& regm,
					   bool* isnew )
{
    const int idx = indexOf( regm.name().buf() );
    RegMarker* chgregm;
    if ( rgmlvls_.validIdx(idx) )
    {
	if ( isnew )
	    *isnew = false;

	chgregm = rgmlvls_.get( idx );
	*chgregm = regm;
	levelChanged.trigger( chgregm->id() );
    }
    else
    {
	if ( isnew )
	    *isnew = true;

	if ( regm.id().isUdf() || regm.id().asInt() < curregmid_ )
	    const_cast<RegMarker&>( regm ).setID( LevelID(++curregmid_) );

	const LevelID id = regm.id();
	if ( isPresent(id) )
	{
	    remove( id );
	    levelToBeRemoved.trigger( id );
	}

	chgregm = new RegMarker( regm );
	rgmlvls_.add( chgregm );
	levelAdded.trigger( chgregm->id() );
    }

    return chgregm->id();
}


Strat::LevelID Strat::RegMarkerSet::add( const char* nm, const OD::Color& col )
{
    const LevelID lvlid( curregmid_++ );
    RegMarker regm( nm, col, lvlid );
    return doSet( regm );
}


void Strat::RegMarkerSet::remove( const LevelID& id )
{
    if ( !isPresent(id) )
	return;

    levelToBeRemoved.trigger( id );
    const int idx = indexOf( id );
    rgmlvls_.removeSingle( idx );
}


Strat::LevelID Strat::RegMarkerSet::set( const RegMarker& regm )
{
    return doSet( regm );
}


void Strat::RegMarkerSet::add( const BufferStringSet& lvlnms,
			       const TypeSet<OD::Color>& cols )
{
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	add( RegMarker(lvlnms.get(idx),cols[idx]) );
}


void Strat::RegMarkerSet::iomReadyCB( CallBacker* )
{
    mAttachCB( IOM().surveyChanged, RegMarkerSet::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, RegMarkerSet::applClosingCB );
}


void Strat::RegMarkerSet::applClosingCB( CallBacker* )
{
    rgmlvls_.erase();
}


void Strat::RegMarkerSet::surveyChangedCB( CallBacker* )
{
    reset();
}


void Strat::RegMarkerSet::reset()
{
    setEmpty();
    curregmid_ = Strat::RegMarker::minID();
    readRegMarkers();
}


const Strat::RegMarkerSet& Strat::RGMLVLS()
{
    static Strat::RegMarkerSet* regms = new Strat::RegMarkerSet();
    return *regms;
}
