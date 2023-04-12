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
    return mSelf().curSet();
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
    , id_(newid)
    , color_(col)
    , pars_(*new IOPar)
    , changed(this)
{
}


Strat::Level::Level( const Level& oth, int lvlid )
    : NamedCallBacker(oth)
    , id_(lvlid)
    , color_(oth.color_)
    , pars_(*new IOPar(oth.pars_))
    , changed(this)
{
}


Strat::Level::Level( const Level& oth )
    : NamedCallBacker(oth)
    , id_(0)
    , pars_(*new IOPar(oth.pars_))
    , changed(this)
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
    , curlevelid_(idnr)
    , changed(this)
    , levelAdded(this)
    , levelToBeRemoved(this)
{
}


Strat::LevelSet::LevelSet( const LevelSet& oth )
    : NamedCallBacker(oth)
    , curlevelid_(0)
    , changed(this)
    , levelAdded(this)
    , levelToBeRemoved(this)
{
    *this = oth;
}


Strat::LevelSet::~LevelSet()
{
    deepErase( lvls_ );
}


Strat::LevelSet& Strat::LevelSet::operator =( const LevelSet& oth )
{
    if ( this != &oth )
    {
	NamedObject::operator =( oth );
	deepCopy( lvls_, oth.lvls_ );
	curlevelid_ = oth.curlevelid_;
	changed.trigger( Level::cEntireChange() );
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
    deepErase( lvls_ );
}


void Strat::LevelSet::setEmpty()
{
    if ( isEmpty() )
	return;

    doSetEmpty();
    changed.trigger( Level::cEntireChange() );
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
    return gtIdxOf( 0, id );
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


Strat::LevelID Strat::LevelSet::doSet( const Strat::Level& lvl,
					 bool* isnew )
{
    const int idx = indexOf( lvl.name().buf() );
    Level* chglvl;
    if ( lvls_.validIdx(idx) )
    {
	if ( isnew ) *isnew = false;
	chglvl = lvls_.get( idx );
	*chglvl = lvl;
    }
    else
    {
	if ( isnew ) *isnew = true;
	if ( lvl.id().asInt() < curlevelid_ )
	    const_cast<Level&>( lvl ).setID( LevelID(++curlevelid_) );
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
    if ( !isPresent(id) )
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
	if ( lvl.id().isValid() )
	{
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
    changed.trigger( Level::cEntireChange() );
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
	IOPar iop; lvl.fillPar( iop );
	iop.putTo( astrm );
    }

    sfio.closeSuccess();
    return true;
}


BufferString Strat::getStdFileName( const char* inpnm, const char* basenm )
{
    BufferString nm( inpnm );
    nm.replace( ' ', '_' );
    FilePath fp( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,"Strat",1) );
    if ( basenm )
	fp.add( basenm );

    if ( !nm.isEmpty()	)
	fp.setExtension( nm );

    return fp.fullPath();
}


void Strat::LevelSet::getStdNames( BufferStringSet& nms )
{
    DirList dl( getStdFileName(0,0), File::FilesInDir, "Levels.*" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm( dl.get(idx) );
	BufferString nm( fnm.buf() + 7 );
	nm.replace( ' ', '_' );
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
    return dbky_.isUdf() ? false : write( *this, dbky_ );
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
