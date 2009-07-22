/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.30 2009-07-22 16:01:35 cvsbert Exp $";

#include "stratunitrepos.h"
#include "stratlith.h"
#include "stratlevel.h"
#include "safefileio.h"
#include "ascstream.h"
#include "separstr.h"
#include "filegen.h"
#include "keystrs.h"
#include "errh.h"
#include "debug.h"
#include "iopar.h"
#include "ioman.h"
#include "color.h"


static const char* filenamebase = "StratUnits";
static const char* filetype = "Stratigraphic Tree";
const char* Strat::UnitRepository::sKeyLith = "Lithology";

static const char* sKeyGeneral = "General";
static const char* sKeyUnits = "Units";
static const char* sKeyLevel = "Level";


const Strat::UnitRepository& Strat::UnRepo()
{
    static UnitRepository* unrepo = 0;
    if ( !unrepo )
    {
	if ( DBG::isOn() ) DBG::message( "Creating Strat::UnitRepository" );
	unrepo = new UnitRepository;
	unrepo->reRead();
	if ( DBG::isOn() )
	{
	    BufferString msg( "Total strat trees found: " );
	    msg += unrepo->nrTrees();
	    if ( unrepo->nrTrees() > 0 )
	    {
		msg += "; last tree name: ";
		msg += unrepo->tree(unrepo->nrTrees()-1)->treeName();
	    }
	    DBG::message( msg );
	}
    }
    return *unrepo;
}


Strat::RefTree::~RefTree()
{
    deepErase( lvls_ );
}


bool Strat::RefTree::addUnit( const char* code, const char* dumpstr, bool rev )
{
    if ( !code || !*code )
	use( dumpstr );

    CompoundKey ck( code );
    UnitRef* par = find( ck.upLevel().buf() );
    if ( !par || par->isLeaf() )
	return false;

    const bool isleaf = strchr( dumpstr, '`' ); // a bit of a hack, really
    const BufferString ky( ck.key( ck.nrKeys()-1 ) );
    NodeUnitRef* parnode = (NodeUnitRef*)par;
    UnitRef* newun = isleaf ? (UnitRef*)new LeafUnitRef( parnode, ky )
			    : (UnitRef*)new NodeUnitRef( parnode, ky );
    if ( !newun->use(dumpstr) )
	{ delete newun; return false; }

    parnode->add( newun, rev );
    return true;
}


bool Strat::RefTree::addCopyOfUnit( const Strat::UnitRef& ur, bool rev )
{
    BufferString str;
    ur.fill( str );
    if ( !addUnit( ur.fullCode(), str ) )
	return false;

    Level* toplvl = const_cast<Level*>(getLevel( &ur, true ));
    Level* baselvl = const_cast<Level*>(getLevel( &ur, false ));
    if ( toplvl ) toplvl->unit_ = find( ur.fullCode() );
    if ( baselvl ) baselvl->unit_ = find( ur.fullCode() );
    return true;
}


void Strat::RefTree::removeEmptyNodes()
{
    UnitRef::Iter it( *this );
    ObjectSet<UnitRef> torem;
    while ( it.next() )
    {
	UnitRef* curun = it.unit();
	if ( !curun->isLeaf() && ((NodeUnitRef*)curun)->nrRefs() < 1 )
	    torem += curun;
    }

    for ( int idx=0; idx<torem.size(); idx++ )
    {
	UnitRef* un = torem[idx];
	NodeUnitRef& par = *un->upNode();
	par.remove( par.indexOf(un) );
    }
}


void Strat::RefTree::addLevel( Strat::Level* l )
{
    if ( l->id_ < 0 )
	l->id_ = getFreeLevelID();

    lvls_ += l;
}


const Strat::Level* Strat::RefTree::getLevel( const char* nm ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	if ( lvls_[idx]->name() == nm )
	    return lvls_[idx];
    }
    return 0;
}


const Strat::Level* Strat::RefTree::getLevel( const Strat::UnitRef* u,
						bool top ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	if ( lvls_[idx]->unit_ == u && lvls_[idx]->top_ == top )
	    return lvls_[idx];
    }
    return 0;
}


const Strat::Level* Strat::RefTree::levelFromID( int id ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	if ( lvls_[idx]->id_ == id )
	    return lvls_[idx];
    }
    return 0;
}


void Strat::RefTree::remove( const Strat::Level*& lvl )
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	Level* curlvl = lvls_[idx];
	if ( curlvl == lvl )
	{
	    lvls_ -= curlvl;
	    delete curlvl;
	    lvl = 0;
	    return;
	}
    }
}


bool Strat::RefTree::write( std::ostream& strm ) const
{
    ascostream astrm( strm );
    astrm.putHeader( filetype );
    astrm.put( sKeyGeneral );
    astrm.put( sKey::Name, treename_ );
    const UnitRepository& repo = UnRepo();
    BufferString str;
    Lithology::undef().fill( str );
    astrm.put( UnitRepository::sKeyLith, str );
    for ( int idx=0; idx<repo.nrLiths(); idx++ )
    {
	const Lithology& lith = repo.lith( idx );
	lith.fill( str );
	astrm.put( UnitRepository::sKeyLith, str );
    }

    astrm.newParagraph();
    astrm.put( sKeyUnits );
    UnitRef::Iter it( *this );
    if ( !it.unit() ) return strm.good();
    const UnitRef& firstun = *it.unit(); firstun.fill( str );
    astrm.put( firstun.fullCode(), str );
    
    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( str );
	astrm.put( un.fullCode(), str );
    }
    astrm.newParagraph();

    IOPar iop( sKeyLevel );
    for ( int idx=0; idx<lvls_.size(); idx ++ )
    {
	iop.clear();
	lvls_[idx]->putTo( iop );
	iop.putTo( astrm );
    }

    return strm.good();
}


void Strat::RefTree::untieLvlsFromUnit( const Strat::UnitRef* ur,
					bool freechildren )
{
    Level* toplvl = const_cast<Level*>(getLevel( ur, true ));
    Level* baselvl = const_cast<Level*>(getLevel( ur, false ));
    if ( toplvl ) toplvl->unit_ = 0;
    if ( baselvl ) baselvl->unit_ = 0;

    mDynamicCastGet(const NodeUnitRef*,nur,ur);
    if ( freechildren && nur )
    {
	UnitRef::Iter it( *nur );
	while ( it.next() )
	{
	    const UnitRef* tmpun = it.unit();
	    toplvl = const_cast<Level*>(getLevel( tmpun, true ));
	    baselvl = const_cast<Level*>(getLevel( tmpun, false ));
	    if ( toplvl ) toplvl->unit_ = 0;
	    if ( baselvl ) baselvl->unit_ = 0;
	}
    }
}


int Strat::RefTree::getFreeLevelID() const
{
    return UnRepo().getNewLevelID();
}


Strat::UnitRepository::UnitRepository()
    : curtreeidx_(-1)
    , lastlevelid_(-1)
    , lastlithid_(-1)
    , changed(this)
{
    IOM().surveyChanged.notify( mCB(this,Strat::UnitRepository,survChg) );
}


Strat::UnitRepository::~UnitRepository()
{
    deepErase( trees_ );
    deepErase( liths_ );
    deepErase( unusedliths_ );
}


void Strat::UnitRepository::survChg( CallBacker* )
{
    reRead();
}


void Strat::UnitRepository::reRead()
{
    deepErase( trees_ );
    deepErase( liths_ );

    Repos::FileProvider rfp( filenamebase );
    addTreeFromFile( rfp, Repos::Rel );
    addTreeFromFile( rfp, Repos::ApplSetup );
    addTreeFromFile( rfp, Repos::Data );
    addTreeFromFile( rfp, Repos::User );
    addTreeFromFile( rfp, Repos::Survey );

    if ( trees_.isEmpty() )
	createDefaultTree();

    curtreeidx_ = trees_.size() - 1;
    changed.trigger();
}


bool Strat::UnitRepository::write( Repos::Source src ) const
{
    const RefTree* tree = getTreeFromSource( src );
    if ( !tree )
	return false;

    Repos::FileProvider rfp( filenamebase );
    const BufferString fnm = rfp.fileName( src );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(false) )
	{ ErrMsg(sfio.errMsg()); return false; }

    if ( !tree->write(sfio.ostrm()) )
	{ sfio.closeFail(); return false; }

    sfio.closeSuccess();
    return true;
}


void Strat::UnitRepository::addTreeFromFile( const Repos::FileProvider& rfp,
					     Repos::Source src )
{
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) ) return;

    ascistream astrm( sfio.istrm(), true );
    if ( !astrm.isOfFileType(filetype) )
	{ sfio.closeFail(); return; }

    RefTree* tree = 0;
    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    tree = new RefTree( astrm.value(), src );
	else if ( astrm.hasKeyword(sKeyLith) )
	    addLith( astrm.value(), src );
    }
    if ( !tree )
    {
	BufferString msg( "No name specified for Stratigraphic tree in:\n" );
	msg += fnm; ErrMsg( fnm );
	sfio.closeFail(); delete tree; return;
    }

    astrm.next(); // Read away 'Units'
    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( !tree->addUnit(astrm.keyWord(),astrm.value()) )
	{
	    BufferString msg( fnm );
	    msg += ": Invalid unit: '";
	    msg += astrm.keyWord(); msg += "'";
	    ErrMsg( msg );
	}
    }
    tree->removeEmptyNodes();

    while ( astrm.next().type() != ascistream::EndOfFile )
    {
	if ( atEndOfSection(astrm) )
	    continue;
	if ( !astrm.hasKeyword(sKeyLevel) )
	    break;

	Level* lvl = new Level( "", 0, true );
	IOPar iop; iop.getFrom( astrm );
	lvl->getFrom( iop, *tree );
	tree->addLevel( lvl );
	if ( lvl->id_ > lastlevelid_ )
	    lastlevelid_ = lvl->id_;
    }

    sfio.closeSuccess();
    if ( tree->nrRefs() > 0 )
	trees_ += tree;
    else
    {
	BufferString msg( "No valid layers found in:\n" );
	msg += fnm; ErrMsg( fnm );
	delete tree;
    }
}


int Strat::UnitRepository::findLith( const char* str ) const
{
    if ( !str ) return -1;
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	if ( liths_[idx]->name() == str )
	    return idx;
    }
    return -1;
}


int Strat::UnitRepository::findLith( int lithid ) const
{
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	if ( liths_[idx]->id_ == lithid )
	    return idx;
    }
    return -1;
}


void Strat::UnitRepository::addLith( const char* str, Repos::Source src )
{
    if ( !str || !*str ) return;

    Lithology* newlith = new Lithology;
    if ( !newlith->use(str) )
	{ delete newlith; return; }
    if ( newlith->id_ < 0 )
    {
	newlith->id_ = -1;
	Lithology* udf = const_cast<Lithology*>(&Lithology::undef());
	*udf = *newlith;
	delete newlith; return;
    }

    newlith->src_ = src;
    if ( findLith(newlith->name()) >= 0 )
	unusedliths_ += newlith;
    else
	liths_ += newlith;

    if ( newlith->id_ > lastlithid_ )
	lastlithid_ = newlith->id_;
}


void Strat::UnitRepository::addLith( Lithology* newlith )
{
    if ( !newlith ) return;

    if ( findLith(newlith->name()) >= 0 )
	unusedliths_ += newlith;
    else
	liths_ += newlith;
}


void Strat::UnitRepository::removeLith( int lid )
{
    if ( lid < 0 ) return;
    int idx = findLith( lid );
    if ( idx < 0 ) return;

    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	Strat::RefTree& tree = *trees_[idx];
	Strat::UnitRef::Iter it( tree, Strat::UnitRef::Iter::Leaves );
	while ( it.next() )
	{
	    LeafUnitRef* lur = (LeafUnitRef*)it.unit();
	    if ( lur->lithology() == lid )
		lur->setLithology( Strat::Lithology::undef().id_ );
	}
    }

    delete liths_.remove( idx );
}


BufferString Strat::UnitRepository::getLithName( int lithid ) const
{
    int idx = findLith( lithid );
    if ( idx<0 || idx>= liths_.size() ) return "";
    return liths_[idx] ? liths_[idx]->name() : "";
}


int Strat::UnitRepository::getLithID( BufferString name ) const
{
    int idx = findLith( name );
    if ( idx<0 || idx>= liths_.size() ) return -1;
    return liths_[idx] ? liths_[idx]->id_ : -1;
}


int Strat::UnitRepository::indexOf( const char* tnm ) const
{
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( trees_[idx]->treeName() == tnm )
	    return idx;
    }
    return -1;
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code ) const
{
    return curtreeidx_ < 0 ? 0
	 : const_cast<UnitRef*>( trees_[curtreeidx_]->find( code ) );
}


Strat::UnitRef* Strat::UnitRepository::fndAny( const char* code ) const
{
    const UnitRef* ref = find( code );
    if ( !ref )
    {
	for ( int idx=0; idx<trees_.size(); idx++ )
	{
	    if ( idx == curtreeidx_ ) continue;
	    const UnitRef* r = trees_[idx]->find( code );
	    if ( r )
		{ ref = r; break; }
	}
    }

    return const_cast<UnitRef*>( ref );
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code, int idx ) const
{
    if ( idx < 0 )			return fnd(code);
    else if ( idx >= trees_.size() )	return 0;

    return const_cast<UnitRef*>( trees_[idx]->find( code ) );
}


int Strat::UnitRepository::treeOf( const char* code ) const
{
    if ( fnd(code,curtreeidx_) )
	return curtreeidx_;

    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( idx == curtreeidx_ ) continue;
	else if ( fnd(code,idx) )
	    return idx;
    }
    return -1;
}


const Strat::RefTree* Strat::UnitRepository::getTreeFromSource(
					Repos::Source src ) const
{
    const RefTree* tree = 0;
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( trees_[idx]->source() == src )
	{ tree = trees_[idx]; break; }
    }

    return tree;
}


void Strat::UnitRepository::copyCurTreeAtLoc( Repos::Source loc )
{
    if ( tree( currentTree() )->source() == loc )
	return;
    
    BufferString str;
    const RefTree* curtree = tree( currentTree() );
    RefTree* tree = new RefTree( curtree->treeName(), loc );
    for ( int idx=0; idx<curtree->nrLevels(); idx++ )
    {
	Level* lvl = new Level( *curtree->levelFromIdx( idx ) );
	tree->addLevel( lvl );
    }
    UnitRef::Iter it( *curtree );
    const UnitRef& firstun = *it.unit(); firstun.fill( str );
    tree->addUnit( firstun.fullCode(), str );
    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( str );
	tree->addUnit( un.fullCode(), str );
    }

    const RefTree* oldsrctree  = getTreeFromSource( loc );
    if ( oldsrctree )
	replaceTree( tree, trees_.indexOf(oldsrctree) );
    else
	trees_ += tree;
}


void Strat::UnitRepository::replaceTree( Strat::RefTree* newtree, int treeidx )
{
    if ( treeidx == -1 )
	treeidx = currentTree();

    delete trees_.replace( treeidx, newtree );
}


void Strat::UnitRepository::createDefaultTree()
{
    RefTree* tree = new RefTree( "Stratigraphic tree", Repos::Survey );
    tree->addUnit( "base", "example of unit" );
    trees_ += tree;
}
