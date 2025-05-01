/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitreeitemmanager.h"

#include "uitreeview.h"


#define mEnabSelChg(yn) \
   if ( uitreeviewitem_ && uitreeviewitem_->treeView() ) \
        uitreeviewitem_->treeView()->selectionChanged.enable( yn );



uiTreeItemFactory::uiTreeItemFactory()
{}


uiTreeItemFactory::~uiTreeItemFactory()
{}



uiTreeItem::uiTreeItem( const uiString& nm )
    : name_(nm)
{
}


uiTreeItem::~uiTreeItem()
{
    while ( !children_.isEmpty() )
	removeChild( children_.first() );

    delete uitreeviewitem_;
}


uiString uiTreeItem::name() const
{
    return name_;
}


void uiTreeItem::setToolTip( int column, const uiString& text )
{
    if ( uitreeviewitem_ )
	uitreeviewitem_->setToolTip( column, text );
}


bool uiTreeItem::areAllParentsChecked()
{
    return parent_ ?
	parent_->isChecked() &&	parent_->areAllParentsChecked() : true;
}


void uiTreeItem::entryInEditMode( int col )
{
    if ( getItem() )
	getItem()->edit( col );
}


bool uiTreeItem::rightClick( uiTreeViewItem* item )
{
    if ( item==uitreeviewitem_ )
    {
	select();
	showSubMenu();
	return true;
    }

    for ( auto* child : children_ )
	if ( child->rightClick(item) )
	    return true;

    return false;
}


bool uiTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item==uitreeviewitem_ )
	return select();

    for ( auto* child : children_ )
	if ( child->anyButtonClick(item) )
	    return true;

    return false;
}


bool uiTreeItem::doubleClick( uiTreeViewItem* item )
{
    for ( auto* child : children_ )
	if ( child->doubleClick(item) )
	    return true;

    return false;
}


void uiTreeItem::updateSelection( int selid, bool downward )
{
    if ( uitreeviewitem_ )
    {
	const bool sel = shouldSelect( selid );
        uitreeviewitem_->setSelected( sel );
	uitreeviewitem_->setBold( 0, sel );
    }

    if ( downward )
    {
	for ( auto* child : children_ )
	    child->updateSelection( selid, downward );
    }
    else if ( parent_ )
	parent_->updateSelection( selid, false );
}


bool uiTreeItem::shouldSelect( int selid ) const
{ return selid!=-1 && selid==selectionKey(); }


bool uiTreeItem::selectWithKey( int selkey )
{ return parent_ ? parent_->selectWithKey(selkey) : false; }


bool uiTreeItem::select()
{ return selectWithKey(selectionKey()); }

bool uiTreeItem::isSelected() const
{ return uitreeviewitem_ ? uitreeviewitem_->isSelected() : false; }


void uiTreeItem::prepareForShutdown()
{
    for ( auto* child : children_ )
	child->prepareForShutdown();
}


bool uiTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    for ( auto* child : children_ )
	child->askContinueAndSaveIfNeeded( withcancel );

    return true;
}


void uiTreeItem::setChecked( bool yn, bool trigger )
{ if ( uitreeviewitem_ ) uitreeviewitem_->setChecked( yn, trigger ); }


bool uiTreeItem::isChecked() const
{ return uitreeviewitem_ ? uitreeviewitem_->isChecked() : false; }

NotifierAccess* uiTreeItem::checkStatusChange()
{ return uitreeviewitem_ ? &uitreeviewitem_->stateChanged : nullptr; }

NotifierAccess* uiTreeItem::keyPressed()
{ return uitreeviewitem_ ? &uitreeviewitem_->keyPressed : nullptr; }

void uiTreeItem::expand()
{
    if ( uitreeviewitem_ )
	uitreeviewitem_->setOpen(true);
}

bool uiTreeItem::isExpanded() const
{
    return uitreeviewitem_ ? uitreeviewitem_->isOpen() : true;
}

void uiTreeItem::collapse()
{
    if ( uitreeviewitem_ )
	uitreeviewitem_->setOpen(false);
}


bool uiTreeItem::isCollapsed() const
{
    return !isExpanded();
}


bool uiTreeItem::hasChildren() const
{
    return !children_.isEmpty();
}


bool uiTreeItem::hasGrandChildren() const
{
    for ( const auto* child : children_ )
	if ( child->hasChildren() )
	    return true;

    return false;
}


bool uiTreeItem::allChildrenExpanded() const
{
    for ( const auto* child : children_ )
	if ( !child->isExpanded() )
	    return false;

    return true;
}


bool uiTreeItem::allChildrenCollapsed() const
{
    for ( const auto* child : children_ )
	if ( !child->isCollapsed() )
	    return false;

    return true;
}


void uiTreeItem::collapseAllChildren()
{
    for ( auto* child : children_ )
	child->collapse();
}


void uiTreeItem::expandAllChildren()
{
    for ( auto* child : children_ )
	child->expand();
}


bool uiTreeItem::allChildrenChecked() const
{
    for ( const auto* child : children_ )
	if ( !child->isChecked() )
	    return false;

    return true;
}


bool uiTreeItem::allChildrenUnchecked() const
{
    for ( const auto* child : children_ )
	if ( child->isChecked() )
	    return false;

    return true;
}


void uiTreeItem::updateSelTreeColumnText( int col )
{
    for ( auto* child : children_ )
    {
	if ( child->isSelected() )
	    child->updateColumnText( col );
	else
	    child->updateSelTreeColumnText( col );
    }
}


void uiTreeItem::updateColumnText( int col )
{
    for ( auto* child : children_ )
	child->updateColumnText( col );

    if ( !uitreeviewitem_ )
	return;

    if ( !col )
        uitreeviewitem_->setText( name_, col );
}


void uiTreeItem::updateCheckStatus()
{
    for ( auto* child : children_ )
	child->updateCheckStatus();
}


const uiTreeItem* uiTreeItem::findChild( const char* nm ) const
{
    return getNonConst(*this).findChild( nm );
}


const uiTreeItem* uiTreeItem::findChild( int selkey ) const
{
    return getNonConst(*this).findChild( selkey );
}


uiTreeItem* uiTreeItem::findChild( const char* nm )
{
    const BufferString treenm( toString(name_) );
    if ( treenm == nm )
	return this;

    for ( auto* child : children_ )
    {
	uiTreeItem* res = child->findChild( nm );
	if ( res )
	    return res;
    }

    return nullptr;
}


void uiTreeItem::findChildren( const char* nm, ObjectSet<uiTreeItem>& set )
{
    if ( name_.getFullString() == nm )
	set += this;

    for ( auto* child : children_ )
	child->findChildren( nm, set );
}


uiTreeItem* uiTreeItem::findChild( int selkey )
{
    if ( selectionKey()==selkey )
	return this;

    for ( auto* child : children_ )
    {
	uiTreeItem* res = child->findChild( selkey );
	if ( res )
	    return res;
    }

    return nullptr;
}


void uiTreeItem::moveItem( uiTreeItem* below )
{
    mEnabSelChg( false )
    getItem()->moveItem( below->getItem() );
    mEnabSelChg( true )
}


void uiTreeItem::moveItemToTop()
{
    mEnabSelChg( false )
    if ( parent_ && parent_->getItem() && getItem() )
    {
        uiTreeViewItem* item = getItem();
	const bool issel = item->isSelected();
	const bool isopen = item->isOpen();
	parent_->getItem()->takeItem( item );
	parent_->getItem()->insertItem( 0, item );
	item->setSelected( issel );
	item->setBold( 0, issel );
	item->setOpen( isopen );
    }

    mEnabSelChg( true )
}


int uiTreeItem::uiTreeViewItemType() const
{
    return uiTreeViewItem::Standard;
}


uiParent* uiTreeItem::getUiParent() const
{
    return parent_ ? parent_->getUiParent() : nullptr;
}


void uiTreeItem::setTreeViewItem( uiTreeViewItem* item )
{
    uitreeviewitem_ = item;
    if ( uitreeviewitem_ )
        uitreeviewitem_->setSelectable( isSelectable() );
}


int uiTreeItem::siblingIndex() const
{
    return uitreeviewitem_ ? uitreeviewitem_->siblingIndex() : -1;
}


uiTreeItem* uiTreeItem::siblingAbove()
{
    if ( !parent_ || !uitreeviewitem_ )
	return nullptr;

    uiTreeViewItem* itemabove = uitreeviewitem_->itemAbove();
    if ( !itemabove )
	return nullptr;

    for ( auto* child : parent_->children_ )
	if ( child->getItem()==itemabove )
	    return child;

    return nullptr;
}


uiTreeItem* uiTreeItem::siblingBelow()
{
    if ( !parent_ || !uitreeviewitem_ )
	return nullptr;

    uiTreeViewItem* itembelow = uitreeviewitem_->itemBelow();
    if ( !itembelow )
	return nullptr;

    for ( auto* child : parent_->children_ )
	if ( child->getItem()==itembelow )
	    return child;

    return nullptr;
}


uiTreeItem* uiTreeItem::lastChild()
{
    if ( !uitreeviewitem_ )
	return nullptr;

    uiTreeViewItem* lastchild = uitreeviewitem_->lastChild();
    for ( auto* child : children_ )
	if ( child->getItem()==lastchild )
	    return child;

    return nullptr;
}


const uiTreeItem* uiTreeItem::getChild( int idx ) const
{
    if ( !children_.validIdx(idx) )
	return nullptr;

    return children_[idx];
}


uiTreeItem* uiTreeItem::getChild( int idx )
{
    if ( !children_.validIdx(idx) )
	return nullptr;

    return children_[idx];
}


bool uiTreeItem::addChildImpl( CallBacker* parent, uiTreeItem* newitem,
			       bool below, bool downwards )
{
    if ( !useParentType() ||
	 StringView(newitem->parentType()) == typeid(*this).name() )
    {
	mEnabSelChg( false )
	children_ += newitem;
	newitem->parent_ = this;
	uiTreeViewItem::Setup setup( newitem->name(),
		    (uiTreeViewItem::Type)newitem->uiTreeViewItemType() );
	mDynamicCastGet(uiTreeViewItem*,lvi,parent)
	mDynamicCastGet(uiTreeView*,lv,parent)
	uiTreeViewItem* item = nullptr;
	if ( lvi )
	    item = new uiTreeViewItem( lvi, setup );
	else if ( lv )
	    item = new uiTreeViewItem( lv, setup );
	if ( !item ) return false;

	newitem->setTreeViewItem( item );
	if ( !newitem->init() )
	{
	    removeChild( newitem );
	    mEnabSelChg( true );
	    return true;
	}

	const bool itmisopen = item->isOpen();
	if ( !below ) newitem->moveItemToTop();
	item->setOpen( itmisopen );

	if ( uitreeviewitem_ && !uitreeviewitem_->isOpen() )
	    uitreeviewitem_->setOpen( true );
	newitem->updateColumnText(0); newitem->updateColumnText(1);
	mEnabSelChg( true );
	return true;
    }

    if ( downwards )
    {
	for ( auto* child : children_ )
	    if ( child->addChld(newitem,below,downwards) )
		return true;
    }
    else if ( parent_ )
	return parent_->addChld( newitem, below, downwards );

    return false;
}


bool uiTreeItem::addChild( uiTreeItem* newitem, bool below )
{ return addChld( newitem, below, false ); }


bool uiTreeItem::addChld( uiTreeItem* newitem, bool below, bool downwards )
{ return addChildImpl( uitreeviewitem_, newitem, below, downwards ); }


void uiTreeItem::removeChild( uiTreeItem* treeitem )
{
    mEnabSelChg( false )
    if ( !children_.isPresent(treeitem) )
    {
	for ( auto* child : children_ )
	    child->removeChild(treeitem);

	return;
    }

    const int idx=children_.indexOf( treeitem );
    removeItem( treeitem->getItem() );
    delete children_.removeSingle( idx );
    mEnabSelChg( true )
}


void uiTreeItem::removeAllChildren()
{
    mEnabSelChg( false )
    for ( auto* child : children_ )
	removeItem( child->getItem() );
    deepErase( children_ );
    mEnabSelChg( true )
}


void uiTreeItem::removeItem( uiTreeViewItem* itm )
{
    if ( uitreeviewitem_ )
	uitreeviewitem_->removeItem( itm );
}


void uiTreeItem::renameItem( uiTreeViewItem* itm )
{
    if ( itm == uitreeviewitem_ )
    {
	BufferString newnm = uitreeviewitem_->text( 0 );
	name_ = toUiString(newnm);
	return;
    }

    for ( auto* child : children_ )
	child->renameItem( itm );
}



// uiTreeTopItem
uiTreeTopItem::uiTreeTopItem( uiTreeView* listview, bool disab )
    : uiTreeItem( toUiString(listview->name()) )
    , disabselcngresp_(disab)
    , listview_( listview )
    , disabrightclick_(false)
    , disabanyclick_(false)
{
    mAttachCB( listview_->rightButtonPressed, uiTreeTopItem::rightClickCB );
    mAttachCB( listview_->mouseButtonPressed, uiTreeTopItem::anyButtonClickCB );
    mAttachCB( listview_->doubleClicked, uiTreeTopItem::doubleClickCB );
    mAttachCB( listview_->selectionChanged, uiTreeTopItem::selectionChanged );
    mAttachCB( listview_->itemRenamed, uiTreeTopItem::itemRenamed );
}


uiTreeTopItem::~uiTreeTopItem()
{
    detachAllNotifiers();
}


bool uiTreeTopItem::addChild( uiTreeItem* newitem, bool below )
{
    return addChld( newitem, below, true );
}


bool uiTreeTopItem::addChld( uiTreeItem* newitem, bool below, bool downwards )
{
    downwards = true;		//We are at the top, so we should go downwards
    return addChildImpl( listview_, newitem, below, downwards );
}


void uiTreeTopItem::removeItem( uiTreeViewItem* itm )
{
    if ( listview_ )
    {
	NotifyStopper ns( listview_->selectionChanged );
	listview_->takeItem( itm );
    }
}


void uiTreeTopItem::itemRenamed( CallBacker* )
{
    if ( !listview_->itemNotified() )
	return;

    if ( listview_->columnNotified()==0 )
	renameItem( listview_->itemNotified() );
}


void uiTreeTopItem::handleSelectionChanged( bool frombutclick )
{
    if ( disabselcngresp_ && !frombutclick )
	return;

    if ( disabanyclick_ || !listview_->itemNotified() )
	return;

    const bool oldstatus = disabanyclick_;
    disabanyclick_ = true;
    if ( anyButtonClick(listview_->itemNotified()) )
	listview_->unNotify();

    disabanyclick_ = oldstatus;
}


void uiTreeTopItem::selectionChanged( CallBacker* )
{
    handleSelectionChanged( false );
}


void uiTreeTopItem::anyButtonClickCB( CallBacker* )
{
    handleSelectionChanged( true );
}


void uiTreeTopItem::rightClickCB( CallBacker* )
{
    if ( disabanyclick_ || disabrightclick_ || !listview_->itemNotified() )
	return;

    if ( rightClick(listview_->itemNotified()) )
	listview_->unNotify();
}


void uiTreeTopItem::doubleClickCB( CallBacker* )
{
    uiTreeViewItem* itm = listview_->itemNotified();
    if ( !itm )
	return;

    if ( doubleClick(itm) )
	listview_->unNotify();
}


void uiTreeTopItem::updateSelection( int selectionkey, bool dw )
{
    NotifyStopper temp( listview_->selectionChanged );
    uiTreeItem::updateSelection( selectionkey, true );
    listview_->triggerUpdate();
}


void uiTreeTopItem::updateColumnText(int col)
{
    //Is only impl to have it nicely together with updateSelection at the
    //public methods in the headerfile.
    uiTreeItem::updateColumnText( col );
}


uiParent* uiTreeTopItem::getUiParent() const
{
    return listview_->parent();
}


uiTreeFactorySet::uiTreeFactorySet()
    : addnotifier( this )
    , removenotifier( this )
{}


uiTreeFactorySet::~uiTreeFactorySet()
{
    for ( int idx=0; idx<factories_.size(); idx++ )
	removenotifier.trigger( idx );
    deepErase( factories_ );
}


void uiTreeFactorySet::addFactory( uiTreeItemFactory* ptr, int placement,
				   OD::Pol2D3D pol2d )
{
    if ( !ptr )
	return;

    const BufferString newfactnm( ptr->name() );
    for ( const auto* factory : factories_ )
	if ( newfactnm == factory->name() )
	    return;

    factories_ += ptr;
    placementidxs_ += placement;
    pol2ds_ += int(pol2d);
    addnotifier.trigger( factories_.size()-1 );
}


void uiTreeFactorySet::remove( const char* nm )
{
    int index = -1;
    for ( int idx=0; idx<factories_.size(); idx++ )
    {
	if ( StringView(nm) == factories_[idx]->name() )
	    { index = idx; break; }
    }

    if ( index<0 )
	return;

    removenotifier.trigger( index );
    delete factories_[index];
    factories_.removeSingle( index );
    placementidxs_.removeSingle( index );
    pol2ds_.removeSingle( index );
}


int uiTreeFactorySet::nrFactories() const
{ return factories_.size(); }

const uiTreeItemFactory* uiTreeFactorySet::getFactory( int idx ) const
{ return factories_.validIdx( idx ) ? factories_[idx] : nullptr; }

int uiTreeFactorySet::getPlacementIdx( int idx ) const
{ return placementidxs_[idx]; }

OD::Pol2D3D uiTreeFactorySet::getPol2D3D( int idx ) const
{
    return sCast(OD::Pol2D3D,pol2ds_[idx]);
}


uiTreeItemRemover::uiTreeItemRemover(uiTreeItem* parent,uiTreeItem* child)
    : parent_( parent )
    , child_( child )
{}


uiTreeItemRemover::~uiTreeItemRemover()
{}


int uiTreeItemRemover::nextStep()
{
    child_->prepareForShutdown();
    parent_->removeChild( child_ );
    return Finished();
}
