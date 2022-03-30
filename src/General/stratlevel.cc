/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/


#include "stratlevel.h"
#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
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

static const char* sLevelExt = "stratlevels";

namespace Strat
{

const Level& Level::undef()
{
    mDefineStaticLocalObject( PtrMan<Level>, lvl, = nullptr );
    if ( !lvl )
    {
	Level* newlvl = new Level( "Undefined", nullptr );
	newlvl->id_ = -1;
	newlvl->color_ = OD::Color::Black();

	lvl.setIfNull(newlvl,true);
    }
    return *lvl;
}


class LevelSetMgr : public CallBacker
{
public:

LevelSetMgr()
{
    mAttachCB( IOM().surveyChanged, LevelSetMgr::surveyChangedCB );
    mAttachCB( IOM().applicationClosing, LevelSetMgr::surveyChangedCB );
}

~LevelSetMgr()
{
    detachAllNotifiers();
}


LevelSet& curSet()
{
    Threads::Locker locker( lock_ );
    if ( lss_.isEmpty() )
	createSet();

    return *lss_.last();
}


void add( LevelSet* lss )
{
    Threads::Locker locker( lock_ );
    if ( lss )
	lss_.add( lss );
}


void pop()
{
    Threads::Locker locker( lock_ );
    lss_.pop();
}


const LevelSet* first() const
{
    Threads::Locker locker( lock_ );
    return lss_.first();
}


void ensurePresent( LevelSet& lss )
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


private:

void createSet()
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
	Repos::Source rsrc = ls->readOldRepos();
	if ( rsrc != Repos::Temp )
	    ls->store( rsrc );
    }

    lss_.add( ls );
}


void surveyChangedCB( CallBacker* )
{
    lss_.erase();
}

    ManagedObjectSet<LevelSet>	lss_;
    mutable Threads::Lock lock_;
};


static LevelSetMgr& lvlSetMgr()
{
    static PtrMan<LevelSetMgr> lssmgr_ = new LevelSetMgr();
    return *lssmgr_.ptr();
}

const LevelSet& LVLS()
{ return lvlSetMgr().curSet(); }
void pushLevelSet( LevelSet* ls )
{ lvlSetMgr().add( ls ); }
void popLevelSet()
{ lvlSetMgr().pop(); }
const LevelSet& unpushedLVLS()
{ return *lvlSetMgr().first(); }


void setLVLS( LevelSet* ls )
{
    if ( ls )
	lvlSetMgr().ensurePresent( *ls );
}


Level::Level( const char* nm, const LevelSet* ls )
    : NamedCallBacker(nm)
    , id_(-1)
    , lvlset_(ls)
    , pars_(*new IOPar)
    , changed(this)
    , toBeRemoved(this)
{
}


Level::Level( const Level& oth )
    : NamedCallBacker(oth)
    , id_(-1)
    , color_(oth.color_)
    , pars_(*new IOPar(oth.pars_))
    , lvlset_(oth.lvlset_)
    , changed(this)
    , toBeRemoved(this)
{
}


Level::~Level()
{
    toBeRemoved.trigger();
    delete &pars_;
}


bool Level::operator ==( const Level& lvl ) const
{
    return id_ == -1 ? this == &lvl : id_ == lvl.id_;
}


bool Level::isDifferentFrom( const Level& lvl ) const
{
    if ( this == &lvl ) return false;

    return name() != lvl.name()
	|| color_ != lvl.color_
	|| pars_ != lvl.pars_;
}


const char* Level::checkName( const char* nm ) const
{
    if ( !nm || !*nm )
	return "Level names may not be empty";
    if ( lvlset_ && lvlset_->isPresent(nm) )
	return "Name already in use";
    return nullptr;
}


void Level::setName( const char* nm )
{
    if ( checkName(nm) || name() == nm )
	return;

    NamedObject::setName(nm); changed.trigger();
}


void Level::setColor( OD::Color c )
{
    if ( color_ != c )
	{ color_ = c; changed.trigger(); }
}


void Level::setPars( const IOPar& iop )
{
    if ( pars_ != iop )
	{ pars_ = iop; changed.trigger(); }
}


void Level::fillPar( IOPar& iop ) const
{
    iop.set( sKey::ID(), id_ );
    iop.set( sKey::Name(), name() );
    iop.set( sKey::Color(), color_ );
    iop.merge( pars_ );
}


void Level::usePar( const IOPar& iop )
{
    iop.get( sKey::ID(), id_ );
    BufferString nm; iop.get( sKey::Name(), nm ); setName( nm );
    iop.get( sKey::Color(), color_ );

    pars_.merge( iop );
    pars_.removeWithKey( sKey::Name() );
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::ID() );
}


#define mLvlSetInitList(id) \
    : lastlevelid_(id) \
    , levelAdded(this) \
    , levelChanged(this) \
    , levelToBeRemoved(this) \
    , ischanged_(false)

LevelSet::LevelSet() mLvlSetInitList(0)				{}
LevelSet::LevelSet( Level::ID id ) mLvlSetInitList(id)	{}


LevelSet::LevelSet( const LevelSet& oth )
    : lastlevelid_(oth.lastlevelid_)
    , levelAdded(this)
    , levelChanged(this)
    , levelToBeRemoved(this)
    , ischanged_(false)
{
    getLevelsFrom( oth );
}


LevelSet::~LevelSet()
{
    for ( int idx=0; idx<size(); idx++ )
	lvls_[idx]->toBeRemoved.disable();
    deepErase( lvls_ );
}


LevelSet& LevelSet::operator =( const LevelSet& oth )
{
    if ( this != &oth )
    {
	getLevelsFrom( oth );
	lastlevelid_ = oth.lastlevelid_;
	notiflvlidx_ = -1;
	levelChanged.trigger();
    }
    return *this;
}


void LevelSet::getLevelsFrom( const LevelSet& oth )
{
    deepErase(lvls_);

    for ( int ilvl=0; ilvl<oth.size(); ilvl++ )
    {
	Level* newlvl = new Level( *oth.lvls_[ilvl] );
	addLvl( newlvl );
    }
}


void LevelSet::makeMine( Level& lvl )
{
    lvl.lvlset_ = this;
    lvl.changed.notify( mCB(this,LevelSet,lvlChgCB) );
    lvl.toBeRemoved.notify( mCB(this,LevelSet,lvlRemCB) );
}


int LevelSet::gtIdxOf( const char* nm, Level::ID id ) const
{
    const bool useid = id >= 0;
    for ( int ilvl=0; ilvl<size(); ilvl++ )
    {
	const Level& lvl = *lvls_[ilvl];
	if ( (useid && lvl.id() == id) || (!useid && lvl.name() == nm) )
	    return ilvl;
    }
    return -1;
}


Level* LevelSet::gtLvl( const char* nm, Level::ID id ) const
{
    const int ilvl = gtIdxOf( nm, id );
    return ilvl < 0 ? 0 : const_cast<Level*>( lvls_[ilvl] );
}


void LevelSet::getNames( BufferStringSet& nms ) const
{
    for ( int ilvl=0; ilvl<size(); ilvl++ )
	nms.add( lvls_[ilvl]->name().buf() );
}


Level* LevelSet::getNew( const Level* lvl ) const
{
    Level* newlvl = nullptr;
    const char* newnmbase = lvl ? lvl->name().buf() : "New";
    BufferString newnm( "<", newnmbase, ">" ); int itry = 1;
    while ( isPresent(newnm.buf()) )
	{ newnm = "<"; newnm.add(newnmbase).add( ++itry ).add( ">" ); }

    if ( !lvl )
	newlvl = new Level( newnm, this );
    else
    {
	newlvl = new Level( *lvl );
	newlvl->NamedObject::setName( newnm );
    }

    newlvl->id_ = ++lastlevelid_;
    const_cast<LevelSet*>(this)->makeMine( *newlvl );
    return newlvl;
}


Level* LevelSet::add( const Level& lvl )
{
    Level* newlvl = getNew( &lvl );
    addLvl( newlvl );
    return newlvl;
}


void LevelSet::remove( Level::ID id )
{
    if ( !isPresent( id ) ) return;

    const int idx = indexOf( id );
    if ( idx >=0 )
    {
	delete lvls_[idx];
	lvls_.removeSingle( idx );
    }
}


void LevelSet::add( const BufferStringSet& lvlnms,
				const TypeSet<OD::Color>& cols )
{
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	add( lvlnms.get(idx), cols[idx] );
}


void LevelSet::addLvl( Level* lvl )
{
    if ( !lvl ) return;
    makeMine( *lvl );
    lvls_ += lvl;
    ischanged_ = true;
    levelAdded.trigger();
}


Level* LevelSet::set( const char* nm, const OD::Color& col, int idx )
{
    int curidx = indexOf( nm );
    Level* lvl = nullptr;
    if ( curidx >= 0 )
    {
	lvl = lvls_[curidx];
	const bool poschged = idx >= 0 && curidx != idx;
	notiflvlidx_ = -1;
	if ( poschged )
	    lvls_.swap( curidx, idx );
	if ( lvl->color_ != col )
	{
	    if ( !poschged ) notiflvlidx_ = curidx;
	    lvl->setColor( col );
	}
	if ( poschged || notiflvlidx_ >= 0 )
	    levelChanged.trigger();
    }
    else
    {
	if ( idx < 0 ) idx = size();
	lvl = getNew();
	lvl->setName( nm );
	lvl->color_ = col;
	if ( idx >= size() )
	{
	    lvls_ += lvl;
	    notiflvlidx_ = size() - 1;
	}
	else
	{
	    lvls_.insertAt( lvl, idx );
	    notiflvlidx_ = -1;
	}
	makeMine( *lvl );
	ischanged_ = true;
	levelAdded.trigger();
    }

    return lvl;
}


void LevelSet::notif( CallBacker* cb, bool chg )
{
    mDynamicCastGet(Level*,lvl,cb) if ( !lvl ) return;

    int ilvl = indexOf( lvl->id() );
    if ( ilvl < 0 ) return; // Huh?

    notiflvlidx_ = ilvl;
    ischanged_ = true;
    (chg ? levelChanged : levelToBeRemoved).trigger();
}


void LevelSet::readPars( ascistream& astrm, bool isold )
{
    while ( true )
    {
	IOPar iop; iop.getFrom( astrm );
	if ( iop.isEmpty() ) break;
	if ( isold && iop.name() != "Level" ) continue;

	const int llid = lastlevelid_;
	Level* lvl = getNew();
	lastlevelid_ = llid; lvl->id_ = -1;

	lvl->usePar( iop );
	if ( lvl->id() < 0 )
	    delete lvl;
	else
	{
	    if ( isold )
	    {
		// Remove legacy keys
		lvl->pars_.removeWithKey( "Unit" );
		lvl->pars_.removeWithKey( "Time" );
	    }
	    addLvl( lvl );
	    if ( lvl->id() > lastlevelid_ )
		lastlevelid_ = lvl->id();
	}
    }
}


bool LevelSet::readFrom( const char* fnm )
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

    setEmpty();
    readPars( astrm, false );
    sfio.closeSuccess();
    ischanged_ = false;
    return true;
}


Repos::Source LevelSet::readOldRepos()
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
    readPars( astrm, true );

    ischanged_ = false;
    return rsrc;
}


bool LevelSet::store( Repos::Source rsrc ) const
{
    Repos::FileProvider rfp( "StratLevels" );
    return writeTo( rfp.fileName(rsrc) );
}


bool LevelSet::read( Repos::Source rsrc )
{
    Repos::FileProvider rfp( "StratLevels" );
    return readFrom( rfp.fileName(rsrc) );
}


bool LevelSet::writeTo( const char* fnm ) const
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


BufferString getStdFileName( const char* inpnm, const char* basenm )
{
    BufferString nm( inpnm );
    nm.replace( ' ', '_' );
    FilePath fp( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,"Strat",1) );
    if ( basenm )
	fp.add( basenm );
    if ( nm && *nm )
	fp.setExtension( nm );
    return fp.fullPath();
}


void LevelSet::getStdNames( BufferStringSet& nms )
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


LevelSet* LevelSet::createStd( const char* nm )
{
    LevelSet* ret = new LevelSet;
    ret->readFrom( getStdFileName(nm,"Levels") );
    return ret;
}


LevelSet* LevelSet::read( const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return nullptr;

    FilePath fp( ioobj->fullUserExpr() );
    fp.setExtension( sLevelExt );

    LevelSet* ret = new LevelSet;
    if ( !ret->readFrom(fp.fullPath()) )
    {
	delete ret;
	ret = nullptr;
    }

    return ret;
}


bool LevelSet::write( const LevelSet& ls, const MultiID& key )
{
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    FilePath fp( ioobj->fullUserExpr() );
    fp.setExtension( sLevelExt );

    return ls.writeTo( fp.fullPath() );
}

} // namespace Strat
