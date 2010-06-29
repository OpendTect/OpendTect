/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.37 2010-06-29 13:03:30 cvsbruno Exp $";

#include "stratunitrepos.h"
#include "stratlith.h"
#include "safefileio.h"
#include "ascstream.h"
#include "keystrs.h"
#include "bufstringset.h"
#include "separstr.h"
#include "iopar.h"
#include "ioman.h"
#include "color.h"
#include "debug.h"
#include "sorting.h"

static const char* filenamebase = "StratUnits";
static const char* filetype = "Stratigraphic Tree";
const char* Strat::UnitRepository::sKeyLith = "Lithology";

static const char* sKeyProp = "Properties";
static const char* sKeyGeneral = "General";
static const char* sKeyUnits = "Units";


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

    newun->acquireID();
    parnode->add( newun, rev );
    return true;
}


bool Strat::RefTree::addUnit( const char* fullnm, 
				const UnitRef::Props& props, bool rev )
{
    FileMultiString unitdesc(
	             toString(Strat::UnRepo().getLithID(props.lithnm_.buf())) );
    unitdesc += props.desc_;
    addUnit( fullnm, unitdesc, rev );
    setUnitProps( fullnm, props );
    return true;
}


bool Strat::RefTree::addCopyOfUnit( const Strat::UnitRef& ur, bool rev )
{
    BufferString str;
    ur.fill( str );
    return addUnit( ur.fullCode(), str );
}


void Strat::RefTree::setUnitProps( const char* fnm, const UnitRef::Props& props)
{
    Strat::UnitRef* ur = find( fnm );
    if ( !ur ) return;
    ur->setProps( props );
    constraintUnits( *ur );
}


void Strat::RefTree::setUnitProps( int id, const UnitRef::Props& props )
{
    Strat::UnitRef* ur = getFromID( id );
    if ( !ur ) return;
    ur->setProps( props );
    constraintUnits( *ur );
}


int Strat::RefTree::getID( const char* code ) const
{
    const Strat::UnitRef* ur = find( code );
    return ur? ur->getID() : -1;
}


void Strat::RefTree::getUnitIDs( TypeSet<int>& ids ) const
{
    ids.erase();
    Strat::UnitRef::Iter it( *this );
    Strat::UnitRef* un = it.unit();
    while ( un )
    {
	ids += un->getID();
	if ( !it.next() ) break;
	un = it.unit();
    }
}


Strat::UnitRef* Strat::RefTree::fnd( int id ) const 
{
    UnitRef::Iter it( *this );
    const UnitRef* un = it.unit();
    while ( un )
    {
	un = it.unit();
	if ( un->getID() == id )
	    return const_cast<Strat::UnitRef*>(un);
	if ( !it.next() ) break;
	un = it.unit();
    }
    return 0;
}



Color Strat::RefTree::getUnitColor( int id ) const
{
    const Strat::UnitRef* ur = getFromID( id );
    return ur ? ur->props().color_ : Color::NoColor();
}


const char* Strat::RefTree::getUnitLvlName( int id ) const
{
    const Strat::UnitRef* ur = getFromID( id );
    return ur ? ur->props().lvlname_.buf() : 0;
}


void Strat::RefTree::gatherChildrenByTime( const NodeUnitRef& un, 
					ObjectSet<UnitRef>& refunits ) const
{
    TypeSet<float> timestarts; TypeSet<int> sortedidxs;
    for ( int idunit=0; idunit<un.nrRefs(); idunit++ )
    {
	 timestarts += un.ref(idunit).props().timerg_.start;
	 sortedidxs += idunit;
    }
    sort_coupled( timestarts.arr(), sortedidxs.arr(), un.nrRefs() );
    for ( int idunit=0; idunit<sortedidxs.size(); idunit++ )
	refunits += const_cast<UnitRef*>( &un.ref(sortedidxs[idunit]) );
}


void Strat::RefTree::constraintUnits( UnitRef& ur )
{
    constraintUnitTimes( *ur.upNode() );
    constraintUnitLvlNames( *ur.upNode() );
}


#define mSetRangeOverlap(rg1,rg2)\
    if ( rg1.start < rg2.start )\
	rg1.start = rg2.start;\
    if ( rg1.stop > rg2.stop )\
	rg1.stop = rg2.stop;
void Strat::RefTree::constraintUnitTimes( NodeUnitRef& parun )
{
    Strat::UnitRef::Iter it( parun );
    if ( !it.next() ) return;
    Strat::UnitRef* ur = it.unit();
    while ( ur )
    {
	Interval<float>& timerg = ur->props().timerg_;
	NodeUnitRef* upur = ur->upNode();
	if ( upur && !upur->code().isEmpty() )
	{
	    //parent's times
	    const Interval<float>& urtimerg = upur->props().timerg_;
	    mSetRangeOverlap( timerg, urtimerg )
	    
	    //children's times
	    bool found = false; ObjectSet<UnitRef> refunits;
	    gatherChildrenByTime( *upur, refunits );
	    for ( int idunit=0; idunit<refunits.size(); idunit++ )
	    {
		const UnitRef& un = *refunits[idunit];
		if ( un.code() == ur->code() ) 
		{ found = true; continue; }
		const Interval<float> cmptimerg = un.props().timerg_;
		if ( !found )
		{
		    if( timerg.start < cmptimerg.stop )
			timerg.start = cmptimerg.stop;
		}
		else if( timerg.stop > cmptimerg.start )
		    timerg.stop = cmptimerg.start;
	    }
	}
	if ( !it.next() ) break;
	ur = it.unit();
    }
}


void Strat::RefTree::constraintUnitLvlNames( const NodeUnitRef& ur )
{
    Strat::UnitRef::Iter it( ur );
    Strat::UnitRef* un = it.unit();
    if ( un && (un->props().timerg_.start == ur.props().timerg_.start) )
	un->props().lvlname_ = ur.props().lvlname_;
    while ( un )
    {
	float lvltime = un->props().timerg_.start;
	const char* lvlnm = un->props().lvlname_;
	if ( !it.next() ) break;
	un = it.unit();
	if ( un->props().timerg_.start == lvltime )
	    un->props().lvlname_ =lvlnm; 
    }
}


void Strat::RefTree::resetChildrenNames( const NodeUnitRef& ur )
{
    UnitRef::Iter it( ur );
    Strat::UnitRef* un = it.unit();
    while ( un )
    {
	UnitRef* curun = un;
	BufferString bs = curun->code();
	while ( curun->upNode() )
	{
	    curun = curun->upNode();
	    BufferString tmpb( curun->code() );
	    if ( tmpb.isEmpty() ) 
		break;
	    CompoundKey kc( curun->code() );
	    kc += bs.buf();
	    bs = kc.buf();
	}
	un->props().code_ = bs.buf();
	if ( !it.next() ) break;
	un = it.unit();
    }
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
    ObjectSet<const UnitRef> unitrefs;
    unitrefs += &firstun;
    
    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( str );
	astrm.put( un.fullCode(), str );
	unitrefs += &un;
    }
    astrm.newParagraph();

    IOPar uniop( sKeyProp );
    for ( int idx=0; idx<unitrefs.size(); idx++ )
    {
	uniop.clear();
	unitrefs[idx]->putTo( uniop );
	uniop.putTo( astrm );
    }
    
    astrm.newParagraph();

    return strm.good();
}



Strat::UnitRepository::UnitRepository()
    : curtreeidx_(-1)
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

    BufferStringSet uncodes;
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
    uncodes.add( astrm.keyWord() );
}
tree->removeEmptyNodes();

int unitidx = 0;
while ( astrm.next().type() != ascistream::EndOfFile )
{
    if ( !astrm.hasKeyword(sKeyProp) )
	break;

    if ( unitidx >= uncodes.size() ) 
	break;

    Strat::UnitRef* ur = tree->find( uncodes[unitidx]->buf() );
    if ( ur )
    {
	IOPar iop;
	iop.getFrom( astrm );
	ur->getFrom( iop );
    }
    unitidx ++;
}
while ( astrm.type() != ascistream::EndOfFile )
{
    if ( atEndOfSection(astrm) || !astrm.hasKeyword(sKeyProp) )
    { astrm.next(); continue; }
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
UnitRef::Iter it( *curtree );
const UnitRef& firstun = *it.unit(); firstun.fill( str );
tree->addUnit( firstun.fullCode(), str );
    tree->setUnitProps( firstun.fullCode(), firstun.props() );
    while ( it.next() )
    {
	const UnitRef& un = *it.unit(); un.fill( str );
	tree->addUnit( un.fullCode(), str );
	tree->setUnitProps( un.fullCode(), un.props() );
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
    UnitRef::Props props;
    props.code_ =  "Super Group";
    props.timerg_.set( 0, 4.5e3 );
    props.lvlname_ = "Top Level";
    props.color_ = Color::DgbColor();
    props.desc_ = "Stratigraphic Column";
    tree->addUnit( props.code_, props );
    trees_ += tree;
}
