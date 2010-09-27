/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : Sept 2010
-*/

static const char* rcsID = "$Id: stratreftree.cc,v 1.4 2010-09-27 11:05:19 cvsbruno Exp $";


#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "ascstream.h"

static const char* sKeyStratTree = "Stratigraphic Tree";
static const char* sKeyLith = "Lithology";
static const char* sKeyLevelID = "Level.ID";
static const char* sKeyTree = "Tree";


Strat::RefTree::RefTree()
    : NodeOnlyUnitRef(0,"","Contains all units")
    , unitAdded(this)
    , unitChanged(this)
    , unitToBeDeleted(this)
    , notifun_(0)
{
    initTree();
}


void Strat::RefTree::initTree()
{
    src_ = Repos::Temp;
    addUnit( sKeyNoCode(), "-1`", Leaved );
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


bool Strat::RefTree::addUnit( const char* fullcode, const char* dumpstr,
       			      Strat::UnitRef::Type typ )
{
    if ( !fullcode || !*fullcode )
	return false;

    CompoundKey ck( fullcode );
    UnitRef* par = find( ck.upLevel().buf() );
    if ( !par || par->isLeaf() )
	return false;

    const BufferString newcode( ck.key( ck.nrKeys()-1 ) );
    NodeUnitRef* parnode = (NodeUnitRef*)par;
    UnitRef* newun =
	   typ == Leaf ?     (UnitRef*)new LeafUnitRef( parnode, newcode )
	: (typ == NodeOnly ? (UnitRef*)new NodeOnlyUnitRef( parnode, newcode )
			   : (UnitRef*)new LeavedUnitRef( parnode, newcode ) );

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
	LeafUnitRef* newun = new LeafUnitRef( par, un->code(), un->levelID(),
					      un->description() );
	IOPar iop; un->putPropsTo( iop ); newun->getPropsFrom( iop );
	delete par->replace( par->indexOf(un), newun );
    }
    for ( int idx=0; idx<norefs.size(); idx++ )
    {
	LeavedUnitRef* un = norefs[idx];
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
    deepErase( refs_ ); liths_.setEmpty();
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
    }

    astrm.next(); // Read away the line: 'Units'
    while ( !atEndOfSection( astrm.next() ) )
	addUnit( astrm.keyWord(), astrm.value(), Strat::UnitRef::Leaved );
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

    astrm.newParagraph();
    astrm.put( "Units" );
    UnitRefIter it( *this );
    ObjectSet<const UnitRef> unitrefs;
    unitrefs += it.unit();;

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
