/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : Sept 2010
-*/

static const char* rcsID = "$Id: stratreftree.cc,v 1.13 2012-01-24 16:40:14 cvsbert Exp $";


#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "ascstream.h"
#include "separstr.h"

static const char* sKeyStratTree = "Stratigraphic Tree";
static const char* sKeyLith = "Lithology";
static const char* sKeyContent = "Content";
static const char* sKeyLevelID = "Level.ID";
static const char* sKeyTree = "Tree";


Strat::RefTree::RefTree()
    : NodeOnlyUnitRef(0,"","Contains all units")
    , unitAdded(this)
    , unitChanged(this)
    , unitToBeDeleted(this)
    , notifun_(0)
    , udfleaf_(*new LeafUnitRef(this,-1,"Undef unit"))
{
    initTree();
}


void Strat::RefTree::initTree()
{
    src_ = Repos::Temp;
    addLeavedUnit( sKeyNoCode(), "-1`" );
    Strat::LevelSet& lvlset = Strat::eLVLS();
    lvlset.levelToBeRemoved.notify( mCB(this,Strat::RefTree,levelToBeRemoved) );
}


Strat::RefTree::~RefTree()
{
    delete &udfleaf_;
} 


void Strat::RefTree::reportChange( const Strat::UnitRef* un, bool isrem )
{
    notifun_ = un;
    if ( un )
    {
	(isrem ? unitToBeDeleted : unitChanged).trigger();
	notifun_ = 0;
    }
}


void Strat::RefTree::reportAdd( const Strat::UnitRef* un )
{
    notifun_ = un;
    if ( un )
    {
	unitAdded.trigger();
	notifun_ = 0;
    }
}


bool Strat::RefTree::addLeavedUnit( const char* fullcode, const char* dumpstr )
{
    if ( !fullcode || !*fullcode )
	return false;

    CompoundKey ck( fullcode );
    UnitRef* par = find( ck.upLevel().buf() );
    if ( !par || par->isLeaf() )
	return false;

    const BufferString newcode( ck.key( ck.nrKeys()-1 ) );
    NodeUnitRef* parnode = (NodeUnitRef*)par;
    UnitRef* newun = new LeavedUnitRef( parnode, newcode );

    newun->use( dumpstr );
    parnode->refs_ += newun;
    return true;
}


void Strat::RefTree::setToActualTypes()
{
    UnitRefIter it( *this );
    ObjectSet<LeavedUnitRef> chrefs;
    while ( it.next() )
    {
	LeavedUnitRef* un = (LeavedUnitRef*)it.unit();
	const bool haslvlid = un->levelID() >= 0;
	if ( !haslvlid || !un->hasChildren() )
	    chrefs += un;
    }

    ObjectSet<LeavedUnitRef> norefs;
    for ( int idx=0; idx<chrefs.size(); idx++ )
    {
	LeavedUnitRef* un = chrefs[idx];
	NodeUnitRef* par = un->upNode();
	if ( un->hasChildren() )
	    { norefs += un; continue; }
	LeafUnitRef* newun = new LeafUnitRef( par, un->levelID(),
						un->description() );
	IOPar iop; un->putPropsTo( iop ); newun->getPropsFrom( iop );
	delete par->replace( par->indexOf(un), newun );
    }
    for ( int idx=0; idx<norefs.size(); idx++ )
    {
	LeavedUnitRef* un = norefs[idx];
	if ( un->ref(0).isLeaf() ) 
	    continue;
	NodeUnitRef* par = un->upNode();
	NodeOnlyUnitRef* newun = new NodeOnlyUnitRef( par, un->code(),
						    un->description() );
	newun->takeChildrenFrom( un );
	IOPar iop; un->putPropsTo( iop ); newun->getPropsFrom( iop );
	delete par->replace( par->indexOf(un), newun );
    }
}


bool Strat::RefTree::read( std::istream& strm )
{
    deepErase( refs_ ); liths_.setEmpty(); deepErase( contents_ );
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(sKeyStratTree) )
	{ initTree(); return false; }

    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( astrm.hasKeyword(sKeyLith) )
	{
	    const BufferString nm( astrm.value() );
	    Lithology* lith = new Lithology(astrm.value());
	    if ( lith->id() < 0 || nm == Lithology::undef().name() )
		delete lith;
	    else
		liths_.add( lith );
	}
	else if ( astrm.hasKeyword(sKeyContent) )
	{
	    FileMultiString fms( astrm.value() );
	    const Content::ID id = toInt( fms[1] );
	    contents_ += new Content( id, fms[0] );
	}
    }

    astrm.next(); // Read away the line: 'Units'
    while ( !atEndOfSection( astrm.next() ) )
	addLeavedUnit( astrm.keyWord(), astrm.value() );
    setToActualTypes();

    const int propsforlen = strlen( sKeyPropsFor() );
    while ( !atEndOfSection( astrm.next() ) )
    {
	IOPar iop; iop.getFrom( astrm );
	const char* iopnm = iop.name().buf();
	if ( *iopnm != 'P' || strlen(iopnm) < propsforlen )
	    break;

	BufferString unnm( iopnm + propsforlen );
	UnitRef* un = unnm == sKeyTreeProps() ? this : find( unnm.buf() );
	if ( un )
	    un->getPropsFrom( iop );
    }

    if ( refs_.isEmpty() )
	initTree();
    return true;
}


bool Strat::RefTree::write( std::ostream& strm ) const
{
    ascostream astrm( strm );
    astrm.putHeader( sKeyStratTree );
    astrm.put( "General" );
    BufferString bstr;
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	liths_.getLith(idx).fill( bstr );
	astrm.put( sKeyLith, bstr );
    }
    for ( int idx=0; idx<contents_.size(); idx++ )
    {
	const Content& c( *contents_[idx] );
	FileMultiString fms; fms += c.name_; fms += ::toString(c.id_);
	astrm.put( sKeyContent, fms );
    }

    astrm.newParagraph();
    astrm.put( "Units" );
    UnitRefIter it( *this );
    ObjectSet<const UnitRef> unitrefs;
    unitrefs += it.unit();

    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( bstr );
	astrm.put( un.fullCode(), bstr );
	unitrefs += &un;
    }
    astrm.newParagraph();

    for ( int idx=0; idx<unitrefs.size(); idx++ )
    {
	IOPar iop;
	const UnitRef& un = *unitrefs[idx]; un.putPropsTo( iop );
	iop.putTo( astrm );
    }

    return strm.good();
}


void Strat::RefTree::levelToBeRemoved( CallBacker* cb )
{
    mDynamicCastGet(Strat::LevelSet*,lvlset,cb)
    if ( !lvlset ) pErrMsg( "Can't find levelSet" );
    const int lvlidx = lvlset->notifLvlIdx();
    if ( !lvlset->levels().validIdx( lvlidx ) ) return;
    const Strat::Level& lvl = *lvlset->levels()[lvlidx];
    Strat::UnitRefIter it( *this, Strat::UnitRefIter::LeavedNodes );
    while ( it.next() )
    {
	Strat::LeavedUnitRef* lur = ( Strat::LeavedUnitRef*)it.unit();
	if ( lur && lur->levelID() == lvl.id() )
	    lur->setLevelID( -1 );
    }
}

