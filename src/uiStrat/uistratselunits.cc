/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratselunits.h"

#include "stratreftree.h"
#include "stratunitrefiter.h"

#include "uicombobox.h"
#include "uilistbox.h"
#include "uitreeview.h"

static const char* sUsrNameRT = "**";


class uiStratSelUnitsListItem : public uiTreeViewItem
{
public:

uiStratSelUnitsListItem( uiTreeView* p, const Strat::UnitRef* ur, bool wchk )
    : uiTreeViewItem(p,getSetup(ur,wchk))
    , unit_(ur)					{}

uiStratSelUnitsListItem( uiTreeViewItem* p, const Strat::UnitRef* ur, bool wchk)
    : uiTreeViewItem(p,getSetup(ur,wchk))
    , unit_(ur)					{}

static uiTreeViewItem::Setup getSetup( const Strat::UnitRef* ur, bool wchk )
{
    const char* nm = ur == &ur->refTree() ? sUsrNameRT : ur->code().buf();
    return uiTreeViewItem::Setup( nm, wchk ? CheckBox : Standard, false );
}

    const Strat::UnitRef*	unit_;

};


uiStratSelUnits::uiStratSelUnits( uiParent* p, const Strat::NodeUnitRef& nur,
       				  const uiStratSelUnits::Setup& su ) 
    : uiGroup(p,"Stratigraphic Unit Selector")
    , topnode_(nur)
    , setup_(su)
    , doingautosel_(false)
    , curunit_(0)
    , currentChanged(this)
    , selectionChanged(this)
{
    if ( setup_.type_ == Simple )
	mkBoxFld();
    else
	mkTreeFld();
}


uiStratSelUnits::~uiStratSelUnits()
{
    for ( int idx=0; idx<lvitms_.size(); idx++ )
	lvitms_[idx]->stateChanged.remove( mCB(this,uiStratSelUnits,selChg) );
}


#define mDefFillVars() \
    const bool topisok = Strat::UnitRefIter::isValid(topnode_,setup_.pol_); \
    const bool topisrt mUnusedVar = &topnode_.refTree() == &topnode_; \
    const CallBack curchgcb( mCB(this,uiStratSelUnits,curChg) ); \
    const CallBack selchgcb( mCB(this,uiStratSelUnits,selChg) ); \
    Strat::UnitRefIter it( topnode_, setup_.pol_ )


void uiStratSelUnits::mkBoxFld()
{
    tree_ = 0; mDefFillVars();

    BufferStringSet nms;
    if ( topisok )
	nms.add( topisrt ? sUsrNameRT : it.unit()->fullCode().buf() );
    while ( it.next() )
	nms.add( it.unit()->fullCode() );

    uiLabeledComboBox* cb = new uiLabeledComboBox( this, nms,
						    setup_.fldtxt_ );
    combo_ = cb->box(); setHAlignObj( cb );
    combo_->selectionChanged.notify( curchgcb );
}


void uiStratSelUnits::mkTreeFld()
{
    combo_ = 0; mDefFillVars();

    ObjectSet<const Strat::UnitRef> selunits;
    if ( topisok )
	selunits += it.unit();
    int nrleaves = 0;
    const bool nodeleaves = setup_.pol_ > Strat::UnitRefIter::Leaves;
    while ( it.next() )
    {
	const Strat::UnitRef* ur = it.unit();
	selunits += ur;
	if ( ur->isLeaf()
	  || (nodeleaves && ((Strat::NodeUnitRef*)ur)->hasLeaves()) )
	    nrleaves++;
    }

    tree_ = new uiTreeView( this, setup_.fldtxt_,
	      nrleaves<setup_.maxnrlines_ ? 0 : setup_.maxnrlines_, true );
    tree_->setColumnText( 0, setup_.fldtxt_ );
    tree_->setStretch( 1, 2 );

    ObjectSet<const Strat::UnitRef> dispunits;
    it.setPol( Strat::UnitRefIter::All );
    dispunits += &topnode_;
    while ( it.next() )
    {
	const Strat::UnitRef* itun = it.unit();
	const Strat::UnitRef* toadd = 0;
	if ( selunits.isPresent( itun ) )
	    toadd = itun;
	else if ( !itun->isLeaf() )
	{
	    for ( int iun=0; iun<selunits.size(); iun++ )
	    {
		const Strat::UnitRef* ur = selunits[iun];
		if ( itun->isParentOf(*ur) )
		    { toadd = itun; break; }
	    }
	}
	if ( toadd )
	    dispunits += toadd;
    }

    for ( int iun=0; iun<dispunits.size(); iun++ )
    {
	const Strat::UnitRef* curun = dispunits[iun];
	uiStratSelUnitsListItem* parit = 0;
	if ( !lvitms_.isEmpty() )
	{
	    for ( int ilv=0; ilv<lvitms_.size(); ilv++ )
	    {
		uiStratSelUnitsListItem* lvit = lvitms_[ilv];
		if ( lvit->unit_ == curun->upNode() )
		    { parit = lvit; break; }

	    }
	}
	const bool issel = selunits.isPresent( curun );
	const bool needcheck = issel && isMulti();
	uiStratSelUnitsListItem* newit;
	if ( !parit )
	    newit = new uiStratSelUnitsListItem( tree_, curun, needcheck );
	else
	    newit = new uiStratSelUnitsListItem( parit, curun, needcheck );
	if ( !issel )
	    newit->setSelectable( false );
	if ( needcheck && setup_.selectallinitial_ )
	    newit->setChecked( true );

	if ( needcheck )
	    newit->stateChanged.notify( selchgcb );

	lvitms_ += newit;
    }

    tree_->selectionChanged.notify( curchgcb );
}


bool uiStratSelUnits::isSelected( const Strat::UnitRef& ur ) const
{
    if ( combo_ )
    {
	const BufferString txt( combo_->text() );
	if ( &ur == &topnode_ )
	    return txt == sUsrNameRT;
	else
	    return txt == ur.fullCode();
    }
    else
    {
	const uiStratSelUnitsListItem* lvitm = find( &ur );
	if ( !lvitm ) return false;
	if ( isMulti() )
	    return lvitm->isChecked();
	return lvitm == tree_->currentItem();
    }
}


bool uiStratSelUnits::isPresent( const Strat::UnitRef& ur ) const
{
    if ( combo_ )
	return combo_->isPresent( ur.fullCode() );
    return find( &ur );
}


const Strat::UnitRef* uiStratSelUnits::firstSelected() const
{
    if ( combo_ )
    {
	const BufferString txt( combo_->text() );
	if ( txt == sUsrNameRT )
	    return &topnode_;
	return topnode_.find( txt );
    }
    else
    {
	ObjectSet<const Strat::UnitRef> urs;
	getSelected( urs );
	return urs.isEmpty() ? 0 : urs[0];
    }
}


void uiStratSelUnits::getSelected( ObjectSet<const Strat::UnitRef>& urs ) const
{
    if ( combo_ )
    {
	const Strat::UnitRef* ur = firstSelected();
	if ( ur )
	    urs += ur;
    }
    else
    {
        const uiTreeViewItem* curitm = tree_->currentItem();
	for ( int idx=0; idx<lvitms_.size(); idx++ )
	{
	    const uiStratSelUnitsListItem* itm = lvitms_[idx];
	    if ( isMulti() )
	    {
		if ( itm->isChecked() )
		    urs += itm->unit_;
	    }
	    else
	    {
		if ( itm == curitm )
		{
		    if ( Strat::UnitRefIter::isValid(*itm->unit_,setup_.pol_) )
			urs += itm->unit_;
		    break;
		}
	    }
	}
    }
}


void uiStratSelUnits::setSelected( const Strat::UnitRef& ur, bool yn )
{
    if ( combo_ )
	setCurrent( ur );
    else
    {
	uiStratSelUnitsListItem* lvitm = find( &ur );
	if ( !lvitm ) return;
	if ( !isMulti() )
	    tree_->setCurrentItem( lvitm );
	else
	{
	    lvitm->setChecked( yn );
	    selRelated( lvitm->unit_, yn );
	    if ( yn )
		tree_->ensureItemVisible( lvitm );
	}
    }
}


void uiStratSelUnits::setCurrent( const Strat::UnitRef& ur )
{
    if ( combo_ )
	combo_->setCurrentItem( ur.fullCode() );
    else if ( !isMulti() )
	setSelected( ur );
    else
    {
	uiStratSelUnitsListItem* lvitm = find( &ur );
	if ( lvitm )
	{
	    tree_->setCurrentItem( lvitm );
	    tree_->setSelected( lvitm, true );
	    tree_->ensureItemVisible( lvitm );
	}
    }
    curunit_ = &ur;
}


void uiStratSelUnits::setExpanded( int lvl )
{
    if ( combo_ ) return;

    if ( mIsUdf(lvl) )
	tree_->expandAll();
    else
	tree_->expandTo( lvl );
}


void uiStratSelUnits::curChg( CallBacker* )
{
    if ( combo_ )
    {
	const int selidx = combo_->currentItem();
	if ( selidx < 0 ) return;
	const char* nm = combo_->text();
	if ( !strcmp(nm,sUsrNameRT) )
	    curunit_ = &topnode_;
	else
	    curunit_ = topnode_.find( nm );
    }
    else
    {
        uiTreeViewItem* li = tree_->currentItem();
	mDynamicCastGet(uiStratSelUnitsListItem*,sslvi,li)
	if ( !sslvi ) { pErrMsg("Huh"); return; }
	curunit_ = sslvi->unit_;
    }
    currentChanged.trigger();
}


void uiStratSelUnits::selChg( CallBacker* cb )
{
    if ( doingautosel_ || combo_ || !isMulti() || !setup_.autoselchildparent_ )
	return;

    mDynamicCastGet(uiStratSelUnitsListItem*,sslvi,cb)
    if ( !sslvi ) { pErrMsg("Huh"); return; }
    const Strat::UnitRef* ur = sslvi->unit_;
    if ( !ur ) return;

    selectionChanged.trigger();

    const bool ischk = sslvi->isChecked();
    selRelated( ur, ischk );
    if ( !ischk )
	unselParentIfLast( ur );
}


uiStratSelUnitsListItem* uiStratSelUnits::find( const Strat::UnitRef* ur )
{
    for ( int idx=0; idx<lvitms_.size(); idx++ )
    {
	if ( lvitms_[idx]->unit_ == ur )
	    return lvitms_[idx];
    }
    return 0;
}


void uiStratSelUnits::selRelated( const Strat::UnitRef* ur, bool yn )
{
    doingautosel_ = true;
    if ( yn )
	checkParent( ur );
    checkChildren( ur, yn );
    doingautosel_ = false;
}


void uiStratSelUnits::unselParentIfLast( const Strat::UnitRef* ur )
{
    const Strat::NodeUnitRef* par = ur->upNode();
    if ( !par ) return;
    const int nrchld = par->nrRefs();
    for ( int idx=0; idx<nrchld; idx++ )
    {
	const Strat::UnitRef* chld = &par->ref( idx );
	uiStratSelUnitsListItem* lvit = find( chld );
	if ( lvit && lvit->isChecked() ) return;
    }

    uiStratSelUnitsListItem* lvit = find( par );
    if ( !lvit ) return;
    doingautosel_ = true;
    lvit->setChecked( false );
    doingautosel_ = false;
    unselParentIfLast( par );
}


void uiStratSelUnits::checkParent( const Strat::UnitRef* ur )
{
    const Strat::NodeUnitRef* par = ur->upNode();
    if ( !par ) return;
    uiStratSelUnitsListItem* lvit = find( par );
    if ( !lvit || lvit->isChecked() ) return;

    lvit->setChecked( true );
    checkParent( par );
}


void uiStratSelUnits::checkChildren( const Strat::UnitRef* ur, bool yn )
{
    mDynamicCastGet(const Strat::NodeUnitRef*,nur,ur)
    if ( !nur ) return;
    const int nrchld = nur->nrRefs();
    for ( int idx=0; idx<nrchld; idx++ )
    {
	const Strat::UnitRef* chld = &nur->ref( idx );
	uiStratSelUnitsListItem* lvit = find( chld );
	if ( lvit )
	{
	    lvit->setChecked( yn );
	    checkChildren( chld, yn );
	}
    }
}
