/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "stratlevel.h"
#include "bufstringset.h"
#include "iopar.h"
#include "ioman.h"
#include "file.h"
#include "filepath.h"
#include "color.h"
#include "separstr.h"
#include "keystrs.h"
#include "safefileio.h"
#include "strmprov.h"
#include "ascstream.h"
#include "oddirs.h"
#include "dirlist.h"


namespace Strat
{

const Level& Level::undef()
{
    static Level* lvl = 0;
    if ( !lvl )
    {
	lvl = new Level( "Undefined", 0 );
	lvl->id_ = -1;
	lvl->color_ = Color::LightGrey();
    }
    return *lvl;
}


class LevelSetMgr : public CallBacker
{
public:

LevelSetMgr()
{
    IOM().surveyChanged.notify( mCB(this,LevelSetMgr,doNull) );
}

~LevelSetMgr()
{
    doNull( 0 );
}

void doNull( CallBacker* )
{
    deepErase( lss_ );
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
{ static Strat::LevelSetMgr mgr; return mgr; }
const Strat::LevelSet& Strat::LVLS()
{ return lvlSetMgr().curSet(); }
void Strat::pushLevelSet( Strat::LevelSet* ls )
{ lvlSetMgr().lss_ += ls; }
void Strat::popLevelSet()
{ delete lvlSetMgr().lss_.removeSingle( lvlSetMgr().lss_.size()-1 ); }


void Strat::setLVLS( LevelSet* ls )
{
    if ( !ls ) return;

    if ( lvlSetMgr().lss_.isEmpty() )
	lvlSetMgr().lss_ += ls;
    else
    {
	const int currentidx =  lvlSetMgr().lss_.indexOf( &LVLS() );
	delete lvlSetMgr().lss_.replace( currentidx < 0 ? 0 : currentidx, ls );
    }
}


Strat::Level::Level( const char* nm, const Strat::LevelSet* ls )
    : NamedObject(nm)
    , id_(-1)
    , lvlset_(ls)
    , pars_(*new IOPar)
    , changed(this)
    , toBeRemoved(this)
{
}


Strat::Level::Level( const Level& oth )
    : NamedObject(oth) 
    , id_(-1)
    , color_(oth.color_)
    , pars_(*new IOPar(oth.pars_))
    , lvlset_(oth.lvlset_)
    , changed(this)
    , toBeRemoved(this)
{
}


Strat::Level::~Level()
{
    toBeRemoved.trigger();
    delete &pars_;
}


bool Strat::Level::operator ==( const Level& lvl ) const
{
    return id_ == -1 ? this == &lvl : id_ == lvl.id_;
}


bool Strat::Level::isDifferentFrom( const Level& lvl ) const
{
    if ( this == &lvl ) return false;

    return name() != lvl.name()
	|| color_ != lvl.color_
	|| pars_ != lvl.pars_;
}


const char* Strat::Level::checkName( const char* nm ) const
{
    if ( !nm || !*nm )
	return "Level names may not be empty";
    if ( lvlset_ && lvlset_->isPresent(nm) )
	return "Name already in use";
    return 0;
}


void Strat::Level::setName( const char* nm )
{
    if ( checkName(nm) || name() == nm )
	return;

    NamedObject::setName(nm); changed.trigger();
}


void Strat::Level::setColor( Color c )
{
    if ( color_ != c )
	{ color_ = c; changed.trigger(); }
}


void Strat::Level::setPars( const IOPar& iop )
{
    if ( pars_ != iop )
	{ pars_ = iop; changed.trigger(); }
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
    iop.get( sKey::ID(), id_ );
    BufferString nm; iop.get( sKey::Name(), nm ); setName( nm );
    iop.get( sKey::Color(), color_ );

    pars_.merge( iop );
    pars_.removeWithKey( sKey::Name() );
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::ID() );
}


Strat::LevelSet::LevelSet()
    : lastlevelid_(0)
    , levelAdded(this)
    , levelChanged(this)
    , levelToBeRemoved(this)
    , ischanged_(false)
{
}


Strat::LevelSet::LevelSet( const Strat::LevelSet& oth )
    : lastlevelid_(oth.lastlevelid_)
    , levelAdded(this)
    , levelChanged(this)
    , levelToBeRemoved(this)
    , ischanged_(false)
{
    getLevelsFrom( oth );
}


Strat::LevelSet::~LevelSet()
{
    for ( int idx=0; idx<size(); idx++ )
	lvls_[idx]->toBeRemoved.disable();
    deepErase( lvls_ );
}


Strat::LevelSet& Strat::LevelSet::operator =( const Strat::LevelSet& oth )
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


void Strat::LevelSet::getLevelsFrom( const Strat::LevelSet& oth )
{
    deepErase(lvls_);
	
    for ( int ilvl=0; ilvl<oth.size(); ilvl++ )
    {
	Strat::Level* newlvl = new Strat::Level( *oth.lvls_[ilvl] );
	addLvl( newlvl );
    }
}


void Strat::LevelSet::makeMine( Strat::Level& lvl )
{
    lvl.lvlset_ = this;
    lvl.changed.notify( mCB(this,LevelSet,lvlChgCB) );
    lvl.toBeRemoved.notify( mCB(this,LevelSet,lvlRemCB) );
}


int Strat::LevelSet::gtIdxOf( const char* nm, Level::ID id ) const
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


Strat::Level* Strat::LevelSet::gtLvl( const char* nm, Level::ID id ) const
{
    const int ilvl = gtIdxOf( nm, id );
    return ilvl < 0 ? 0 : const_cast<Level*>( lvls_[ilvl] );
}


Strat::Level* Strat::LevelSet::getNew( const Level* lvl ) const
{
    Level* newlvl = 0;
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
    const_cast<Strat::LevelSet*>(this)->makeMine( *newlvl );
    return newlvl;
}


Strat::Level* Strat::LevelSet::add( const Strat::Level& lvl )
{
    Level* newlvl = getNew( &lvl );
    addLvl( newlvl );
    return newlvl;
}


void Strat::LevelSet::remove( Level::ID id )
{
    if ( !isPresent( id ) ) return;

    const int idx = indexOf( id );
    if ( idx >=0 )
    {
	delete lvls_[idx];
	lvls_.removeSingle( idx );
    }
}


void Strat::LevelSet::add( const BufferStringSet& lvlnms, 
				const TypeSet<Color>& cols ) 
{
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	add( lvlnms.get(idx), cols[idx] );
}


void Strat::LevelSet::addLvl( Level* lvl )
{
    if ( !lvl ) return;
    makeMine( *lvl );
    lvls_ += lvl;
    ischanged_ = true;
    levelAdded.trigger();
}


Strat::Level* Strat::LevelSet::set( const char* nm, const Color& col, int idx ) 
{
    int curidx = indexOf( nm );
    Level* lvl = 0;
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


void Strat::LevelSet::notif( CallBacker* cb, bool chg )
{
    mDynamicCastGet(Level*,lvl,cb) if ( !lvl ) return;

    int ilvl = indexOf( lvl->id() );
    if ( ilvl < 0 ) return; // Huh?

    notiflvlidx_ = ilvl;
    ischanged_ = true;
    (chg ? levelChanged : levelToBeRemoved).trigger();
}


void Strat::LevelSet::readPars( ascistream& astrm, bool isold )
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


bool Strat::LevelSet::readFrom( const char* fnm )
{
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) )
	return false;

    ascistream astrm( sfio.istrm(), true );
    if ( astrm.type() == ascistream::EndOfFile )
	{ sfio.closeFail(); return false; }

    setEmpty();
    readPars( astrm, false );
    sfio.closeSuccess();
    ischanged_ = false;
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

    StreamData sd( StreamProvider(bestfile).makeIStream() );
    if ( !sd.usable() )
	return Repos::Temp;

    ascistream astrm( *sd.istrm, true );
    readPars( astrm, true );
    sd.close();

    ischanged_ = false;
    return rsrc;
}


bool Strat::LevelSet::store( Repos::Source rsrc ) const
{
    Repos::FileProvider rfp( "StratLevels" );
    return writeTo( rfp.fileName(rsrc) );
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
    replaceCharacter( nm.buf(), ' ', '_' );
    FilePath fp( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,"Strat",1) );
    if ( basenm )
	fp.add( basenm );
    if ( nm && *nm )
	fp.setExtension( nm );
    return fp.fullPath();
}


void Strat::LevelSet::getStdNames( BufferStringSet& nms )
{
    DirList dl( getStdFileName(0,0), DirList::FilesOnly, "Levels.*" );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	BufferString fnm( dl.get(idx) );
	char* nm = fnm.buf() + 7;
	replaceCharacter( nm, '_', ' ' );
	nms.add( nm );
    }
}


Strat::LevelSet* Strat::LevelSet::createStd( const char* nm )
{
    LevelSet* ret = new LevelSet;
    ret->readFrom( getStdFileName(nm,"Levels") );
    return ret;
}
