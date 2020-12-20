/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2004
-*/


#include "stratlevel.h"
#include "bufstringset.h"
#include "iopar.h"
#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "color.h"
#include "separstr.h"
#include "keystrs.h"
#include "safefileio.h"
#include "od_ostream.h"
#include "ascstream.h"
#include "oddirs.h"
#include "dirlist.h"

mDefineInstanceCreatedNotifierAccess(Strat::Level);
static const char* sUndefName = "<Undefined>";


namespace Strat
{

const Level& Level::undef()
{
    static Level ret( sUndefName, Color::Black() );
    return ret;
}

Level& Level::dummy()
{
    static Level ret( "", Color::Black(), ID(-2) );
    return ret;
}


class LevelSetMgr : public CallBacker
{
public:

LevelSetMgr()
{
    DBM().surveyChanged.notify( mCB(this,LevelSetMgr,doNull) );
    DBM().applicationClosing.notify( mCB(this,LevelSetMgr,doNull) );
}

~LevelSetMgr()
{
    doNull( 0 );
}

void doNull( CallBacker* )
{
    deepUnRef( lss_ );
}

void createSet()
{
    Repos::FileProvider rfp( "StratLevels", true );
    Strat::LevelSet* ls = 0;
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	Strat::LevelSet* tmp = new Strat::LevelSet;
	if ( !tmp->readFrom(rfp.fileName()) || tmp->isEmpty() )
	    delete tmp;
	else
	    { ls = tmp; break; }
    }

    if ( !ls )
    {
	ls = new Strat::LevelSet;
	Repos::Source rsrc = ls->readOldRepos();
	if ( rsrc != Repos::Temp )
	    ls->store( rsrc );
    }
    lss_ += ls;
}

LevelSet& curSet()
{
    if ( lss_.isEmpty() )
	createSet();
    return *lss_[lss_.size()-1];
}

    ObjectSet<LevelSet>	lss_;
};

} // namespace


static Strat::LevelSetMgr& lvlSetMgr()
{ mDefineStaticLocalObject( Strat::LevelSetMgr, mgr, ); return mgr; }
const Strat::LevelSet& Strat::LVLS()
{ return lvlSetMgr().curSet(); }
void Strat::pushLevelSet( Strat::LevelSet* ls )
{ lvlSetMgr().lss_ += ls; }
void Strat::popLevelSet()
{ lvlSetMgr().lss_.removeSingle( lvlSetMgr().lss_.size()-1 )->unRef(); }
const Strat::LevelSet& Strat::unpushedLVLS()
{ return *lvlSetMgr().lss_[0]; }


void Strat::setLVLS( LevelSet* ls )
{
    if ( !ls ) return;

    if ( lvlSetMgr().lss_.isEmpty() )
	lvlSetMgr().lss_ += ls;
    else
    {
	const int currentidx =  lvlSetMgr().lss_.indexOf( &LVLS() );
	lvlSetMgr().lss_.
			replace( currentidx < 0 ? 0 : currentidx, ls )->unRef();
    }
}


Strat::Level::Level( const char* nm, const Color& col, ID newid )
    : NamedMonitorable(nm)
    , color_(col)
    , id_(newid)
{
    mTriggerInstanceCreatedNotifier();
}


Strat::Level::Level( const Level& oth, ID::IDType idnr )
    : NamedMonitorable(oth)
    , id_(ID::get(idnr))
    , color_(oth.color_)
    , pars_(oth.pars_)
{
    mTriggerInstanceCreatedNotifier();
}



Strat::Level::Level( const Level& oth )
    : NamedMonitorable(oth)
    , id_(ID::get(0))
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Strat::Level::~Level()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Strat::Level, NamedMonitorable )


void Strat::Level::copyClassData( const Level& oth )
{
    const_cast<ID&>(id_) = oth.id_;
    color_ = oth.color_;
    pars_ = oth.pars_;
}


Monitorable::ChangeType Strat::Level::compareClassData( const Level& oth ) const
{
    if ( id_ != oth.id_ )
	return cEntireObjectChange();

    mStartMonitorableCompare();
    mHandleMonitorableCompare( color_, cColChange() );
    mHandleMonitorableCompare( pars_, cParsChange() );
    mDeliverMonitorableCompare();
}


bool Strat::Level::isDifferentFrom( const Level& lvl ) const
{
    if ( this == &lvl )
	return false;

    mLock4Read();
    return name_ != lvl.name_
	|| color_ != lvl.color_
	|| pars_ != lvl.pars_;
}


bool Strat::Level::isUndef() const
{
    mLock4Read();
    return name_ == sUndefName;
}


void Strat::Level::fillPar( IOPar& iop ) const
{
    mLock4Read();
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

    mLock4Write();

    iop.get( sKey::ID(), const_cast<ID&>(id_) );
    iop.get( sKey::Color(), color_ );

    pars_.merge( iop );
    pars_.removeWithKey( sKey::Name() );
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::ID() );
    mSendEntireObjChgNotif();
}



Strat::LevelSet::LevelSet( int idnr )
    : curlevelid_(idnr)
    , lvlid_(ID::get(0))
{
}


Strat::LevelSet::LevelSet( const LevelSet& oth )
    : SharedObject(oth)
    , curlevelid_(0)
    , lvlid_(ID::get(0))
{
    copyClassData( oth );
}


Strat::LevelSet::~LevelSet()
{
    sendDelNotif();
    detachAllNotifiers();
    deepErase( lvls_ );
}


mImplMonitorableAssignment( Strat::LevelSet, SharedObject )

void Strat::LevelSet::copyClassData( const LevelSet& oth )
{
    deepCopy( lvls_, oth.lvls_ );
    curlevelid_ = oth.curlevelid_;
}


Monitorable::ChangeType Strat::LevelSet::compareClassData(
					const LevelSet& oth ) const
{
    if ( lvls_.size() != oth.lvls_.size() )
	return cEntireObjectChange();

    for ( int idx=0; idx<lvls_.size(); idx++ )
	if ( *lvls_[idx] != *oth.lvls_[idx] )
	    return cEntireObjectChange();

    return cNoChange();
}


Strat::LevelSet::size_type Strat::LevelSet::size() const
{
    mLock4Read();
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
    mLock4Write();
    doSetEmpty();
    mSendEntireObjChgNotif();
}


bool Strat::LevelSet::isPresent( ID id ) const
{
    mLock4Read();
    return gtIdxOf( 0, id ) >= 0;
}


bool Strat::LevelSet::isPresent( const char* nm ) const
{
    mLock4Read();
    return gtIdxOf( nm, ID::getInvalid() ) >= 0;
}


Strat::LevelSet::ID Strat::LevelSet::getIDByName( const char* nm ) const
{
    mLock4Read();
    const idx_type idx = gtIdxOf( nm, ID::getInvalid() );
    return idx < 0 ? ID::getInvalid() : lvls_[idx]->id();
}


Strat::LevelSet::ID Strat::LevelSet::getIDByIdx( idx_type idx ) const
{
    if ( idx < 0 )
	return ID::getInvalid();
    mLock4Read();
    return idx >= lvls_.size() ? ID::getInvalid() : lvls_[idx]->id();
}


Strat::LevelSet::idx_type Strat::LevelSet::indexOf( ID id ) const
{
    mLock4Read();
    return gtIdxOf( 0, id );
}


Strat::LevelSet::idx_type Strat::LevelSet::indexOf( const char* nm ) const
{
    mLock4Read();
    return gtIdxOf( nm, ID::getInvalid() );
}


Strat::LevelSet::idx_type Strat::LevelSet::gtIdxOf( const char* nm,
						   Level::ID id ) const
{
    const bool useid = id.isValid();
    const bool usenm = nm && *nm;
    if ( !useid && !usenm )
	return -1;

    for ( int ilvl=0; ilvl<lvls_.size(); ilvl++ )
    {
	const Level& lvl = *lvls_[ilvl];
	if ( (useid && lvl.id() == id) || (usenm && lvl.hasName(nm)) )
	    return ilvl;
    }

    return -1;
}


BufferString Strat::LevelSet::nameOf( ID id ) const
{
    mLock4Read();
    const idx_type idx = gtIdxOf( 0, id );
    return idx < 0 ? BufferString::empty() : lvls_[idx]->getName();
}


Color Strat::LevelSet::colorOf( ID id ) const
{
    mLock4Read();
    const idx_type idx = gtIdxOf( 0, id );
    return idx < 0 ? Color() : lvls_[idx]->color();
}


IOPar Strat::LevelSet::parsOf( ID id ) const
{
    mLock4Read();
    const idx_type idx = gtIdxOf( 0, id );
    return idx < 0 ? IOPar() : lvls_[idx]->pars();
}


Strat::Level Strat::LevelSet::get( ID id ) const
{
    mLock4Read();
    return gtLvl( gtIdxOf(0,id) );
}


Strat::Level Strat::LevelSet::getByName( const char* nm ) const
{
    mLock4Read();
    return gtLvl( gtIdxOf(nm,ID::getInvalid()) );
}



Strat::Level Strat::LevelSet::getByIdx( idx_type idx ) const
{
    mLock4Read();
    return gtLvl( idx );
}



Strat::Level Strat::LevelSet::first() const
{
    mLock4Read();
    return gtLvl( 0 );
}


Strat::Level Strat::LevelSet::gtLvl( int idx ) const
{
    return lvls_.validIdx(idx) ? *lvls_[idx] : Level::undef();
}


void Strat::LevelSet::getNames( BufferStringSet& nms ) const
{
    mLock4Read();
    for ( int ilvl=0; ilvl<lvls_.size(); ilvl++ )
	nms.add( lvls_[ilvl]->name() );
}


Strat::LevelSet::ID Strat::LevelSet::doSet( const Strat::Level& lvl,
					    bool* isnew )
{
    const idx_type idx = gtIdxOf( lvl.name().buf(), ID::getInvalid() );
    Level* chglvl;
    if ( idx < 0 )
    {
	if ( isnew ) *isnew = true;
	curlevelid_++;
	lvl.id().setI( curlevelid_ );
	chglvl = new Level( lvl );
	lvls_ += chglvl;
    }
    else
    {
	if ( isnew ) *isnew = false;
	chglvl = lvls_[idx];
	*chglvl = lvl;
    }

    return chglvl->id();
}


void Strat::LevelSet::remove( ID id )
{
    mLock4Read();
    int idx = gtIdxOf( 0, id );
    if ( idx < 0 )
	return;
    if ( !mLock2Write() )
    {
	idx = gtIdxOf( 0, id );
	if ( idx < 0 )
	    return;
    }

    mSendChgNotif( cLevelToBeRemoved(), id.getI() );
    mReLock();
    idx = gtIdxOf( 0, id );
    if ( idx >= 0 )
	delete lvls_.removeSingle( idx );
}


Strat::LevelSet::ID Strat::LevelSet::add( const char* nm, const Color& col )
{
    lvlid_.setI( curlevelid_ );
    curlevelid_++;
    Level lvl( nm, col, lvlid_ );
    mLock4Write();
    return doSet( lvl );
}


Strat::LevelSet::ID Strat::LevelSet::set( const Level& lvl )
{
    mLock4Write();
    return doSet( lvl );
}


void Strat::LevelSet::add( const BufferStringSet& lvlnms,
				const TypeSet<Color>& cols )
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

	Level lvl( 0, Color() );
	lvl.usePar( iop );
	if ( !lvl.id().isInvalid() )
	{
	    if ( isold )
	    {
		lvl.pars_.removeWithKey( "Unit" );
		lvl.pars_.removeWithKey( "Time" );
	    }
	    doSet( lvl );
	    curlevelid_.setIfLarger( lvl.id().getI()+1 );
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
	{ sfio.closeFail(); return false; }

    mLock4Write();
    doSetEmpty();
    getFromStream( astrm, false );
    sfio.closeSuccess();
    mSendEntireObjChgNotif();
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


bool Strat::LevelSet::store( Repos::Source rsrc ) const
{
    Repos::FileProvider rfp( "StratLevels" );
    return writeTo( rfp.fileName(rsrc) );
}


bool Strat::LevelSet::read( Repos::Source rsrc )
{
    Repos::FileProvider rfp( "StratLevels" );
    return readFrom( rfp.fileName(rsrc) );
}


bool Strat::LevelSet::writeTo( const char* fnm ) const
{
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
	return false;

    ascostream astrm( sfio.ostrm() );
    if ( !astrm.putHeader("Stratigraphic Levels") )
	{ sfio.closeFail(); return false; }

    mLock4Read();
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
    File::Path fp( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,"Strat",1) );
    if ( basenm )
	fp.add( basenm );
    if ( nm && *nm )
	fp.setExtension( nm, false );
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
    LevelSet* ret = new LevelSet;
    ret->readFrom( getStdFileName(nm,"Levels") );
    return ret;
}
