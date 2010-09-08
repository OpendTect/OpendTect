/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/

static const char* rcsID = "$Id: stratunitrepos.cc,v 1.52 2010-09-08 07:57:32 cvsbruno Exp $";

#include "stratunitrepos.h"
#include "ascstream.h"
#include "keystrs.h"
#include "bufstringset.h"
#include "iopar.h"
#include "ioman.h"
#include "debug.h"
#include "separstr.h"
#include "safefileio.h"
#include "stratlith.h"


namespace Strat
{

const char* UnitRepository::sKeyLith()		{ return "Lithology"; }
const char* UnitRepository::filenamebase() 	{ return "StratUnits"; }
const char* UnitRepository::filetype()		{ return "Stratigraphic Tree"; }
const char* UnitRepository::sKeyProp()		{ return "Properties"; }
const char* UnitRepository::sKeyBottomLvlID() 	{ return "Bottom Level"; }
const char* UnitRepository::sKeyLevel() 	{ return UnitRef::sKeyLevel(); }

const UnitRepository& UnRepo()
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



UnitRepository::UnitRepository()
    : curtreeidx_(-1)
    , lastlithid_(-1)
    , changed(this)
    , levelChanged(this)
    , unitCreated(this)
    , unitChanged(this)
    , unitRemoved(this)
    , lithoCreated(this)
    , lithoChanged(this)
    , lithoRemoved(this)
{
    IOM().surveyChanged.notify( mCB(this,UnitRepository,survChg) );
}


UnitRepository::~UnitRepository()
{
    if ( tmptree_ ) delete tmptree_;
    deepErase( trees_ );
    deepErase( liths_ );
    deepErase( unusedliths_ );
    deepErase( levels() );
}


void UnitRepository::survChg( CallBacker* )
{
    reRead();
}


void UnitRepository::createTmpTree( bool force )
{
    if ( !force && tmptree_ ) return;
    if ( force && tmptree_ ) delete tmptree_;

    const Strat::RefTree* rt = &Strat::RT();
    tmptree_ = new Strat::RefTree( rt->treeName(), rt->source() );

    Strat::UnitRef::Iter it( *rt );
    if ( !it.unit() ) return;
    const Strat::UnitRef& firstun = *it.unit();
    tmptree_->addCopyOfUnit( firstun );
    while ( it.next() )
	tmptree_->addCopyOfUnit( *it.unit() );
}


#define mSaveAtLoc( loc )\
{\
    const_cast<UnitRepository*>(this)->copyCurTreeAtLoc( loc );\
    write( loc );\
    if ( tmptree_ ) createTmpTree( true );\
    needsave_ = false; \
}\
    

void UnitRepository::save()
{
    mSaveAtLoc( Repos::Survey );
}


void UnitRepository::saveAs( Repos::Source& src )
{
    mSaveAtLoc( src );
}


void UnitRepository::reset( bool iseditmode )
{
    if ( !tmptree_ ) return;
    replaceTree( tmptree_ );
    tmptree_ = 0;

    if ( iseditmode )
    createTmpTree( false );
}


int UnitRepository::getUnitLvlID( int unid ) const
{
    const Strat::UnitRef* un = RT().getByID( unid );
    return un ? un->getLvlID() : -1;
}


void UnitRepository::setBotLvlID( int id )
{
    Strat::eRT().setBotLvlID( id );
    levelChanged.trigger();
}


int UnitRepository::botLvlID() const
{
    return RT().botLvlID();
}


void UnitRepository::getNewUnitTimeRange( const char* parcode,
					    Interval<float>& rg ) const
{
    mDynamicCastGet(const UnitRef*,un,find(parcode))
    if ( !un ) return;
    IOPar iop; un->putTo( iop ); iop.get( sKey::Time, rg ); 

    mDynamicCastGet(const NodeUnitRef*,parun,un)
    if ( !parun ) return;
    TypeSet< Interval<float> > timergs;
    RT().getLeavesTimeGaps( *parun, timergs );
    if ( timergs.size() )
	rg = timergs[timergs.size()-1];
}


void UnitRepository::prepareParentUnit( const char* parcode )
{
    Strat::UnitRef* parun = find( parcode );
    if ( !parun || !parun->isLeaf() ) return;

    NodeUnitRef* upnode = parun->upNode();
    if ( !upnode ) return;

    NodeUnitRef* nodeun = new NodeUnitRef( upnode, parun->code(),
	    					   parun->description() );
    for ( int idx=0; idx<parun->nrProperties(); idx++ )
	((UnitRef*)nodeun)->add( parun->property(idx) );

    nodeun->copyParFrom( *parun );
    int parunidx = upnode->indexOf( upnode->find( parun->code() ) );
    delete upnode->replace( parunidx, nodeun );
}


void UnitRepository::removeUnit( const char* uncode )
{
    UnitRef* ur = find( uncode );
    if ( !ur ) return;

    NodeUnitRef* upnode = ur->upNode();
    if ( upnode )
	upnode->remove( upnode->indexOf(ur) );
    else
	((NodeUnitRef&)eRT()).remove( RT().indexOf( ur ) );
    needsave_ = true;
    unitRemoved.trigger();
}


const RefTree* UnitRepository::getCurTree() const
{
    return &RT();
}


const RefTree* UnitRepository::getBackupTree() const
{
    return tmptree_;
}


void UnitRepository::getLithoNames( BufferStringSet& lithnms ) const
{
    for ( int idx=0; idx<nrLiths(); idx++ )
	lithnms.add( lith(idx).name() );
}


Strat::Lithology* UnitRepository::createNewLith( const char* lithnm, bool ispor )
{
    Lithology* newlith = new Strat::Lithology( lithnm );
    newlith->id_ = getNewLithID();
    newlith->src_ = RT().source();
    newlith->porous_ = ispor;
    addLith( newlith );
    needsave_ = true;
    lithoCreated.trigger();
    return newlith;
}


BufferString UnitRepository::getLithName( const Strat::LeafUnitRef& lur ) const
{
    return getLithName(lur.lithology());
}


const Strat::Lithology* UnitRepository::getLith( const char* nm ) const
{
    int nr = findLith( nm );
    return nr < 0 ? 0 : &UnRepo().lith( nr );
}


void UnitRepository::deleteLith( int id )
{
    eUnRepo().removeLith( id );
    needsave_ = true;
    lithoRemoved.trigger();
}


bool UnitRepository::addUnit( const char* code, bool rev )
{
    if ( eRT().addUnit( code, "", rev ) )
    {
	unitCreated.trigger();
	return true;
    }
    return false;
}


void UnitRepository::updateUnit( int id, const UnitRef& refunit )
{
    UnitRef* ur = eRT().getByID( id );
    if ( !ur ) return;
    ur->setCode( refunit.code() );
    ur->setDescription( refunit.description() );
    ur->copyParFrom( refunit );
    unitChanged.trigger();
}


void UnitRepository::updateUnitLith( int id, const char* lithotxt)
{
    Strat::UnitRef* ur = find( id );
    mDynamicCastGet( Strat::LeafUnitRef*, leafur, ur );
    if ( leafur )
	leafur->setLithology( Strat::UnRepo().getLithID(lithotxt) );
    unitChanged.trigger();
}


void UnitRepository::moveUnit( const char* uncode, bool up )
{
    UnitRef* ur = find( uncode );
    if ( !ur ) return;

    NodeUnitRef* upnode = const_cast<NodeUnitRef*>(ur->upNode());
    if ( !upnode ) return;

    int uridx = upnode->indexOf( ur );
    int targetidx = up ? uridx-1 : uridx+1;
    upnode->swapChildren( uridx, targetidx );
    needsave_ = true;
}


bool UnitRepository::isNewUnitName( const char* urcode ) const
{
    return find( urcode ) ? false : true;
}


void UnitRepository::reRead()
{
    deepErase( trees_ );
    deepErase( liths_ );

    Repos::FileProvider rfp( filenamebase() );
    addTreeFromFile( rfp, Repos::Rel );
    addTreeFromFile( rfp, Repos::ApplSetup );
    addTreeFromFile( rfp, Repos::Data );
    addTreeFromFile( rfp, Repos::User );
    addTreeFromFile( rfp, Repos::Survey );
    addLvlsFromFile( rfp, Repos::Survey );
    resetUnitLevels();

    if ( trees_.isEmpty() )
	createDefaultTree();

    curtreeidx_ = trees_.size() - 1;
    changed.trigger();
}


bool UnitRepository::write( Repos::Source src ) const
{
    const RefTree* tree = getTreeFromSource( src );
    if ( !tree )
	return false;

    Repos::FileProvider rfp( filenamebase() );
    const BufferString fnm = rfp.fileName( src );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(false) )
	{ ErrMsg(sfio.errMsg()); return false; }

    if ( !tree->write(sfio.ostrm()) || !writeLvls(sfio.ostrm()) )
	{ sfio.closeFail(); return false; }

    sfio.closeSuccess();
    return true;
}


bool UnitRepository::writeLvls( std::ostream& strm ) const
{
    ascostream astrm( strm );
    const UnitRepository& repo = UnRepo();
    BufferString str;
    
    astrm.newParagraph();
    IOPar iop( sKeyLevel() );
    for ( int idx=0; idx<levels().size(); idx ++ )
    {
	iop.clear();
	levels()[idx]->putTo( iop );
	iop.putTo( astrm );
    }
    return strm.good();
}


void UnitRepository::addTreeFromFile( const Repos::FileProvider& rfp,
					     Repos::Source src )
{
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) ) return;

    ascistream astrm( sfio.istrm(), true );
    if ( !astrm.isOfFileType(filetype()) )
	{ sfio.closeFail(); return; }

    RefTree* tree = 0;
    while ( !atEndOfSection( astrm.next() ) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    tree = new RefTree( astrm.value(), src );
	else if ( astrm.hasKeyword(sKeyLith()) )
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
	else
	    uncodes.add( astrm.keyWord() );
    }
    tree->removeEmptyNodes();

    int unitidx = 0;
    while ( astrm.next().type() != ascistream::EndOfFile )
    {
	if ( astrm.hasKeyword(sKeyBottomLvlID()) )
	    tree->setBotLvlID( atoi( astrm.next().keyWord() ) );

	else if ( !astrm.hasKeyword(sKeyProp()) )
	    break;

	if ( unitidx >= uncodes.size() ) 
	    break;

	UnitRef* ur = tree->find( uncodes[unitidx]->buf() );
	if ( ur )
	{
	    IOPar iop;
	    iop.getFrom( astrm );
	    ur->getFrom( iop );
	}
	unitidx ++;
    }
    sfio.closeSuccess();
    if ( tree->nrRefs() > 0 )
    {
	/*!support for older version
	TODO make this whole time structure optionnal 
	( then no graph display but still the ui-tree available )!*/
	IOPar iop; Interval<float> timerg; tree->ref(0).putTo( iop );
	iop.get( sKey::Time, timerg );
	float timestop = timerg.stop;
	if ( mIsUdf( timestop ) || timestop == 0 )
	    tree->assignEqualTimesToUnits( Interval<float>( 0, 4.5e3 ) );
	trees_ += tree;
    }
    else
    {
	BufferString msg( "No valid layers found in:\n" );
	msg += fnm; ErrMsg( fnm );
	delete tree;
    }
}


void UnitRepository::addLvlsFromFile( const Repos::FileProvider& rfp,
					     Repos::Source src )
{
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) ) return;

    ascistream astrm( sfio.istrm(), true );
    if ( !astrm.isOfFileType(filetype()) )
	{ sfio.closeFail(); return; }

    deepErase( lvls_ );
    while ( astrm.next().type() != ascistream::EndOfFile )
    {
	if ( atEndOfSection(astrm) )
	    continue;
	if ( !astrm.hasKeyword(sKeyLevel()) )
	    continue;

	Level* lvl = new Level( "" );
	IOPar iop; iop.getFrom( astrm );
	lvl->getFrom( iop );
	levels() += lvl;
	levels().constraintID( lvl->id_ );
    }
}


void UnitRepository::resetUnitLevels() 
{
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	RefTree* tree = trees_[idx];
	UnitRef::Iter it( *tree );
	UnitRef* un = it.unit(); 
	while ( un )
	{
	    if ( !levels().getByID( un->getLvlID() ) ) 
		un->setLvlID( -1 );
	    if ( !it.next() )
		break;
	    un = it.unit();
	}
	if ( !levels().getByID( tree->botLvlID() ) )
	    tree->setBotLvlID( -1 );
    }
}


int UnitRepository::findLith( const char* str ) const
{
    if ( !str ) return -1;
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	if ( liths_[idx]->name() == str )
	    return idx;
    }
    return -1;
}


int UnitRepository::findLith( int lithid ) const
{
    for ( int idx=0; idx<liths_.size(); idx++ )
    {
	if ( liths_[idx]->id_ == lithid )
	    return idx;
    }
    return -1;
}


void UnitRepository::addLith( const char* str, Repos::Source src )
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


void UnitRepository::addLith( Lithology* newlith )
{
    if ( !newlith ) return;

    if ( findLith(newlith->name()) >= 0 )
	unusedliths_ += newlith;
    else
	liths_ += newlith;
    lithoCreated.trigger();
}


void UnitRepository::removeLith( int lid )
{
    if ( lid < 0 ) return;
    int idx = findLith( lid );
    if ( idx < 0 ) return;

    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	RefTree& tree = *trees_[idx];
	UnitRef::Iter it( tree, UnitRef::Iter::Leaves );
	while ( it.next() )
	{
	    LeafUnitRef* lur = (LeafUnitRef*)it.unit();
	    if ( lur->lithology() == lid )
		lur->setLithology( Lithology::undef().id_ );
	}
    }

    delete liths_.remove( idx );
    lithoRemoved.trigger();
}


BufferString UnitRepository::getLithName( int lithid ) const
{
    int idx = findLith( lithid );
    if ( idx<0 || idx>= liths_.size() ) return "";
    return liths_[idx] ? liths_[idx]->name() : "";
}


int UnitRepository::getLithID( BufferString name ) const
{
    int idx = findLith( name );
    if ( idx<0 || idx>= liths_.size() ) return -1;
    return liths_[idx] ? liths_[idx]->id_ : -1;
}


int UnitRepository::indexOf( const char* tnm ) const
{
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( trees_[idx]->treeName() == tnm )
	    return idx;
    }
    return -1;
}


UnitRef* UnitRepository::fnd( int id ) const
{
    return curtreeidx_ < 0 ? 0
	 : const_cast<UnitRef*>( trees_[curtreeidx_]->getByID( id ) );
}



UnitRef* UnitRepository::fnd( const char* code ) const
{
    return curtreeidx_ < 0 ? 0
	 : const_cast<UnitRef*>( trees_[curtreeidx_]->find( code ) );
}


UnitRef* UnitRepository::fndAny( const char* code ) const
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


UnitRef* UnitRepository::fnd( const char* code, int idx ) const
{
    if ( idx < 0 )			return fnd(code);
    else if ( idx >= trees_.size() )	return 0;

    return const_cast<UnitRef*>( trees_[idx]->find( code ) );
}


int UnitRepository::treeOf( const char* code ) const
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


const RefTree* UnitRepository::getTreeFromSource( Repos::Source src ) const
{
    const RefTree* tree = 0;
    for ( int idx=0; idx<trees_.size(); idx++ )
    {
	if ( trees_[idx]->source() == src )
	{ tree = trees_[idx]; break; }
    }

    return tree;
}


void UnitRepository::copyCurTreeAtLoc( Repos::Source loc )
{
    if ( tree( currentTree() )->source() == loc )
	return;

    BufferString str;
    const RefTree* curtree = tree( currentTree() );
    RefTree* tree = new RefTree( curtree->treeName(), loc );
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


void UnitRepository::replaceTree( RefTree* newtree, int treeidx )
{
    if ( treeidx == -1 )
	treeidx = currentTree();

    delete trees_.replace( treeidx, newtree );
}


void UnitRepository::createDefaultTree()
{
    RefTree* tree = new RefTree( "Stratigraphic tree", Repos::Survey );
    const char* code = "Super Group";
    tree->addUnit( code, "Stratigraphic Column" );
    UnitRef* newur = fnd( code );
    if ( newur )
    {
	IOPar iop; 
	iop.set( sKey::Time, Interval<float>(0,4.5e3) );
	iop.set( sKey::Color, Color::DgbColor() );
	newur->getFrom( iop );
    }
    trees_ += tree;
}


void UnitRepository::addLevels( const BufferStringSet& ns, 
					const TypeSet<Color>& cols )
{
    eUnRepo().levels().addToList( ns, cols );
    eUnRepo().levelChanged.trigger();
}


int UnitRepository::addLevel( const char* lvlnm, const Color& col )
{
    int id = eUnRepo().levels().addToList( lvlnm, col );
    eUnRepo().levelChanged.trigger();
    return id;
}


void UnitRepository::removeLevel( const char* lvlnm )
{
    Level* curlvl = const_cast<Level*>(
			    UnRepo().levels().getByName( lvlnm ) );
    eUnRepo().levels() -= curlvl;
    eUnRepo().levelChanged.trigger();
}


void UnitRepository::removeAllLevels()
{
    deepErase( eUnRepo().levels() );
    eUnRepo().levelChanged.trigger();
}


const Level* UnitRepository::getLvl( int id ) const
{
    return UnRepo().levels().getByID( id );
}


void UnitRepository::getLvlsPars( BufferStringSet& nms, TypeSet<Color>& cols, 
				    TypeSet<int>* ids ) const
{
    const int nrlevels = UnRepo().levels().size();
    for ( int idx=0; idx<nrlevels; idx++ )
    {
	const Level* lvl = UnRepo().levels()[idx];
	if ( !lvl ) continue;
	nms.add( lvl->name() );
	cols += lvl->color_;
	if ( ids ) *ids += lvl->id_;
    }
}


void UnitRepository::getLvlPars(int id,BufferString& nm, Color& col ) const
{
    const Strat::Level* lvl = Strat::UnRepo().levels().getByID( id );
    if ( !lvl ) return;
    nm = lvl->name();
    col = lvl->color_;
}


bool UnitRepository::getLvlPars( const char* lvlnm, Color& color ) const
{
    const Level* curlvl = UnRepo().levels().getByName( lvlnm );
    if ( !curlvl ) return false;

    color = curlvl->color_;
    return true;
}


void UnitRepository::setLvlPars( const char* oldnm, const char* newnm, 
					Color newcol )
{
    Level* curlvl = oldnm
	? const_cast<Level*>(eUnRepo().levels().getByName(oldnm))
	: 0;
    if ( !curlvl )
    {
	addLevel( newnm, newcol );
    }
    else
    {
	curlvl->setName( newnm );
	curlvl->color_ = newcol;
    }
    eUnRepo().levelChanged.trigger();
}

};
