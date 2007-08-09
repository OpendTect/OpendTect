/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.16 2007-08-09 14:15:55 cvsbert Exp $";

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


const Strat::UnitRepository& Strat::UnRepo()
{
    static Strat::UnitRepository* unrepo = 0;
    if ( !unrepo )
    {
	if ( DBG::isOn() ) DBG::message( "Creating Strat::UnitRepository" );
	unrepo = new Strat::UnitRepository;
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


bool Strat::RefTree::addUnit( const char* code, const char* dumpstr )
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

    parnode->add( newun );
    return true;
}


void Strat::RefTree::removeEmptyNodes()
{
    Strat::UnitRef::Iter it( *this );
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


const Strat::Level* Strat::RefTree::getLevel( const char* nm ) const
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	if ( lvls_[idx]->name_ == nm )
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


void Strat::RefTree::remove( const Strat::Level*& lvl )
{
    for ( int idx=0; idx<lvls_.size(); idx++ )
    {
	Strat::Level* curlvl = lvls_[idx];
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
    astrm.put( sKey::Name, treename_ );
    const UnitRepository& repo = UnRepo();
    BufferString str;
    for ( int idx=0; idx<repo.nrLiths(); idx++ )
    {
	const Lithology& lith = repo.lith( idx );
	lith.fill( str );
	astrm.put( Strat::UnitRepository::sKeyLith, str );
    }

    astrm.newParagraph();
    Strat::UnitRef::Iter it( *this );
    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( str );
	astrm.put( un.fullCode(), str );
    }

    astrm.newParagraph();
    for ( int idx=0; idx<lvls_.size(); idx ++ )
    {
	const Strat::Level& lvl = *lvls_[idx];
	if ( lvl.unit_ )
	{
	    lvl.fill( str );
	    astrm.put( lvl.name_, str );
	    lvl.color().fill( str.buf() );
	    astrm.put( lvl.name_, str );
	}
    }

    astrm.newParagraph();
    return strm.good();
}


Strat::UnitRepository::UnitRepository()
    	: curtreeidx_(-1)
{
    reRead();
    IOM().afterSurveyChange.notify( mCB(this,Strat::UnitRepository,survChg) );
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

    curtreeidx_ = trees_.size() - 1;
}


bool Strat::UnitRepository::write( Repos::Source src ) const
{
    const Strat::RefTree* tree = 0;
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( trees_[idx]->source() == src )
	    { tree = trees_[idx]; break; }
    }
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

    Strat::RefTree* tree = 0;
    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    tree = new Strat::RefTree( astrm.value(), src );
	else if ( astrm.hasKeyword(sKeyLith) )
	    addLith( astrm.value(), src );
    }
    if ( !tree )
    {
	BufferString msg( "No name specified for Stratigraphic tree in:\n" );
	msg += fnm; ErrMsg( fnm );
	sfio.closeFail(); delete tree; return;
    }

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

    while ( !atEndOfSection( astrm.next() ) )
    {
	BufferString lvldesc = astrm.keyWord();
	Strat::Level* lvl = new Strat::Level( lvldesc, 0, true );
	lvl->use( astrm.value(), *tree );
	ascistream tmpastrm = astrm;
	if ( !atEndOfSection( tmpastrm.next() ) &&
	     !strcmp( lvldesc += ".Color", tmpastrm.keyWord() ) )
	{
	    Color color;
	    color.use( tmpastrm.value() );
	    lvl->setColor( color );
	    astrm = tmpastrm;
	}
	
	tree->addLevel( lvl );
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
	if ( liths_[idx]->id() == lithid )
	    return idx;
    }
    return -1;
}


void Strat::UnitRepository::addLith( const char* str, Repos::Source src )
{
    if ( !str || !*str ) return;

    Strat::Lithology* newlith = new Strat::Lithology;
    if ( !newlith->use(str) )
	{ delete newlith; return; }
    newlith->setSource( src );
    if ( findLith(newlith->name()) >= 0 )
	unusedliths_ += newlith;
    else
	liths_ += newlith;
}


BufferString Strat::UnitRepository::getLithName( int lithid ) const
{
    int idx = findLith( lithid );
    if ( idx<0 || idx>= liths_.size() ) return "";
    return liths_[idx] ? liths_[idx]->name() : "";
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
	 : const_cast<Strat::UnitRef*>( trees_[curtreeidx_]->find( code ) );
}


Strat::UnitRef* Strat::UnitRepository::fndAny( const char* code ) const
{
    const Strat::UnitRef* ref = find( code );
    if ( !ref )
    {
	for ( int idx=0; idx<trees_.size(); idx++ )
	{
	    if ( idx == curtreeidx_ ) continue;
	    const Strat::UnitRef* r = trees_[idx]->find( code );
	    if ( r )
		{ ref = r; break; }
	}
    }

    return const_cast<Strat::UnitRef*>( ref );
}


Strat::UnitRef* Strat::UnitRepository::fnd( const char* code, int idx ) const
{
    if ( idx < 0 )			return fnd(code);
    else if ( idx >= trees_.size() )	return 0;

    return const_cast<Strat::UnitRef*>( trees_[idx]->find( code ) );
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
