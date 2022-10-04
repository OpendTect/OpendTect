/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratselunits.h"

#include "stratreftree.h"
#include "stratunitrefiter.h"

#include "uicombobox.h"
#include "uilistbox.h"
#include "uitreeview.h"

static const char* sUsrNameRT = "**";
#define mUnitIsValid(un) \
    ((un) && Strat::UnitRefIter::isValid((*(un)),setup_.pol_))


class uiStratSelUnitsListItem : public uiTreeViewItem
{
public:

uiStratSelUnitsListItem( uiTreeView* p, const Strat::UnitRef* ur, bool wchk )
    : uiTreeViewItem(p,getSetup(ur,wchk))
    , unit_(ur)
{}

uiStratSelUnitsListItem( uiTreeViewItem* p, const Strat::UnitRef* ur, bool wchk)
    : uiTreeViewItem(p,getSetup(ur,wchk))
    , unit_(ur)
{}

static uiTreeViewItem::Setup getSetup( const Strat::UnitRef* ur, bool wchk )
{
    const char* nm = ur == &ur->refTree() ? sUsrNameRT : ur->code().buf();
    return uiTreeViewItem::Setup( toUiString(nm),
					    wchk ? CheckBox : Standard, false );
}

    const Strat::UnitRef*	unit_;

};


// uiStratSelUnits::Setup

uiStratSelUnits::Setup::Setup( uiStratSelUnits::Type typ,
			       Strat::UnitRefIter::Pol pol )
    : type_(typ)
    , pol_(pol)
    , maxnrlines_(12)
    , autochoosechildparent_(true)
    , chooseallinitial_(false)
    , fldtxt_(typ == Multi ? "Units" : "Unit")
{
}


uiStratSelUnits::Setup::~Setup()
{
}


// uiStratSelUnits

uiStratSelUnits::uiStratSelUnits( uiParent* p, const Strat::NodeUnitRef& nur,
				  const uiStratSelUnits::Setup& su )
    : uiGroup(p,"Stratigraphic Unit Selector")
    , topnode_(nur)
    , setup_(su)
    , doingautochoose_(false)
    , curunit_(0)
    , currentChanged(this)
    , unitChosen(this)
    , unitPicked(this)
{
    if ( setup_.type_ == Simple )
	mkBoxFld();
    else
	mkTreeFld();
}


uiStratSelUnits::~uiStratSelUnits()
{
    detachAllNotifiers();
}


#define mDefFillVars() \
    const bool topisok = Strat::UnitRefIter::isValid(topnode_,setup_.pol_); \
    Strat::UnitRefIter it( topnode_, setup_.pol_ )


void uiStratSelUnits::mkBoxFld()
{
    tree_ = nullptr; mDefFillVars();

    BufferStringSet nms;
    if ( topisok )
    {
	const bool topisrt = &topnode_.refTree() == &topnode_;
	nms.add( topisrt ? sUsrNameRT : it.unit()->fullCode().buf() );
    }
    while ( it.next() )
	nms.add( it.unit()->fullCode().buf() );

    if ( setup_.fldtxt_.isEmpty() )
    {
	combo_ = new uiComboBox( this, nms, "Unit" );
	setHAlignObj( combo_ );
    }
    else
    {
	auto* cb = new uiLabeledComboBox( this, nms,
					  toUiString(setup_.fldtxt_) );
	combo_ = cb->box(); setHAlignObj( cb );
    }

    mAttachCB( combo_->selectionChanged, uiStratSelUnits::curChg );
}


void uiStratSelUnits::mkTreeFld()
{
    combo_ = nullptr; mDefFillVars();

    ObjectSet<const Strat::UnitRef> selectableuns;
    if ( topisok )
	selectableuns += it.unit();
    int nrleaves = 0;
    const bool nodeleaves = setup_.pol_ > Strat::UnitRefIter::Leaves;
    while ( it.next() )
    {
	const Strat::UnitRef* ur = it.unit();
	selectableuns += ur;
	if ( ur->isLeaf()
	  || (nodeleaves && ((Strat::NodeUnitRef*)ur)->hasLeaves()) )
	    nrleaves++;
    }

    tree_ = new uiTreeView( this, setup_.fldtxt_,
	      nrleaves<setup_.maxnrlines_ ? 0 : setup_.maxnrlines_, true );
    tree_->setColumnText( 0, mToUiStringTodo(setup_.fldtxt_) );
    tree_->setStretch( 1, 2 );

    ObjectSet<const Strat::UnitRef> dispunits;
    it.setPol( Strat::UnitRefIter::All );
    dispunits += &topnode_;
    while ( it.next() )
    {
	const Strat::UnitRef* itun = it.unit();
	const Strat::UnitRef* toadd = 0;
	if ( selectableuns.isPresent( itun ) )
	    toadd = itun;
	else if ( !itun->isLeaf() )
	{
	    for ( int iun=0; iun<selectableuns.size(); iun++ )
	    {
		const Strat::UnitRef* ur = selectableuns[iun];
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
	const bool issel = selectableuns.isPresent( curun );
	const bool needcheck = issel && isMulti();
	uiStratSelUnitsListItem* newit;
	if ( !parit )
	    newit = new uiStratSelUnitsListItem( tree_, curun, needcheck );
	else
	    newit = new uiStratSelUnitsListItem( parit, curun, needcheck );
	if ( !issel )
	    newit->setSelectable( false );
	if ( needcheck && setup_.chooseallinitial_ )
	    newit->setChecked( true );

	if ( needcheck )
	    mAttachCB( newit->stateChanged, uiStratSelUnits::choiceChg );

	lvitms_ += newit;
    }

    mAttachCB( tree_->currentChanged, uiStratSelUnits::curChg );
    mAttachCB( tree_->doubleClicked, uiStratSelUnits::treeFinalSel );
    mAttachCB( tree_->returnPressed, uiStratSelUnits::treeFinalSel );
}


bool uiStratSelUnits::isChosen( const Strat::UnitRef& ur ) const
{
    if ( combo_ )
    {
	const BufferString txt( combo_->text() );
	if ( &ur == &topnode_ )
	    return txt == sUsrNameRT;
	else
	    return txt == ur.fullCode().buf();
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
	return combo_->isPresent( ur.fullCode().buf() );
    return find( &ur );
}


const Strat::UnitRef* uiStratSelUnits::firstChosen() const
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
	getChosen( urs );
	return urs.isEmpty() ? 0 : urs[0];
    }
}


void uiStratSelUnits::getChosen( ObjectSet<const Strat::UnitRef>& urs ) const
{
    if ( combo_ )
    {
	const Strat::UnitRef* ur = firstChosen();
	if ( ur )
	    urs += ur;
    }
    else
    {
	if ( !isMulti() )
	    urs += curunit_;
	else
	{
	    for ( int idx=0; idx<lvitms_.size(); idx++ )
	    {
		const uiStratSelUnitsListItem* itm = lvitms_[idx];
		if ( itm->isChecked() )
		    urs += itm->unit_;
	    }
	}
    }
}


void uiStratSelUnits::setChosen( const Strat::UnitRef& ur, bool yn )
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
	    chooseRelated( lvitm->unit_, yn );
	    if ( yn )
		tree_->ensureItemVisible( lvitm );
	}
    }
}


void uiStratSelUnits::setCurrent( const Strat::UnitRef& ur )
{
    const Strat::UnitRef* tosel = &ur;
    if ( !mUnitIsValid(tosel) )
    {
	if ( ur.isLeaf() )
	    return;
	Strat::UnitRefIter it( *(const Strat::NodeUnitRef*)tosel, setup_.pol_ );
	if ( !it.next() )
	    return;
	tosel = it.unit();
    }

    if ( combo_ )
	combo_->setCurrentItem( tosel->fullCode().buf() );
    else if ( !isMulti() )
	setChosen( *tosel );
    else
    {
	uiStratSelUnitsListItem* lvitm = find( tosel );
	if ( lvitm )
	{
	    tree_->setCurrentItem( lvitm );
	    tree_->setSelected( lvitm, true );
	    tree_->ensureItemVisible( lvitm );
	}
    }

    curunit_ = tosel;
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
	const BufferString nm = combo_->text();
	if ( nm == sUsrNameRT )
	    curunit_ = &topnode_;
	else
	    curunit_ = topnode_.find( nm );
    }
    else
    {
        uiTreeViewItem* li = tree_->currentItem();
	if ( !li )
	    return;
	mDynamicCastGet(uiStratSelUnitsListItem*,sslvi,li)
	if ( !sslvi )
	    { pErrMsg("Huh"); return; }
	curunit_ = sslvi->unit_;
    }

    if ( !mUnitIsValid(curunit_) )
	curunit_ = 0;
    currentChanged.trigger();
}


void uiStratSelUnits::choiceChg( CallBacker* cb )
{
    if ( combo_ || !isMulti()
      || doingautochoose_ || !setup_.autochoosechildparent_ )
	return;

    mDynamicCastGet(uiStratSelUnitsListItem*,sslvi,cb)
    if ( !sslvi ) { pErrMsg("Huh"); return; }
    const Strat::UnitRef* ur = sslvi->unit_;
    if ( !ur ) return;

    unitChosen.trigger();

    const bool ischk = sslvi->isChecked();
    chooseRelated( ur, ischk );
    if ( !ischk )
	unChooseParentIfLast( ur );
}


void uiStratSelUnits::treeFinalSel( CallBacker* )
{
    if ( curunit_ )
	unitPicked.trigger();
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


void uiStratSelUnits::chooseRelated( const Strat::UnitRef* ur, bool yn )
{
    doingautochoose_ = true;
    if ( yn )
	checkParent( ur );
    checkChildren( ur, yn );
    doingautochoose_ = false;
}


void uiStratSelUnits::unChooseParentIfLast( const Strat::UnitRef* ur )
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
    doingautochoose_ = true;
    lvit->setChecked( false );
    doingautochoose_ = false;
    unChooseParentIfLast( par );
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
