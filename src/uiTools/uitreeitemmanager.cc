/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uitreeitemmanager.h"

#include "errh.h"
#include "uitreeview.h"


#define mEnabSelChg(yn) \
   if ( uitreeviewitem_ && uitreeviewitem_->treeView() ) \
        uitreeviewitem_->treeView()->selectionChanged.enable( yn );

uiTreeItem::uiTreeItem( const char* name__ )
    : parent_( 0 )
    , name_( name__ )
    , uitreeviewitem_( 0 )
{
}


uiTreeItem::~uiTreeItem()
{
    while ( children_.size() )
	removeChild( children_[0] );

    delete uitreeviewitem_;
}


const char* uiTreeItem::name() const
{ return name_.buf(); }


bool uiTreeItem::rightClick( uiTreeViewItem* item )
{
    if ( item==uitreeviewitem_ )
    {
	select();
	showSubMenu();
	return true;
    }

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( children_[idx]->rightClick(item) )
	    return true;
    }

    return false;
}


bool uiTreeItem::anyButtonClick( uiTreeViewItem* item )
{
    if ( item==uitreeviewitem_ )
	return select();

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( children_[idx]->anyButtonClick(item) )
	    return true;
    }

    return false;
}


void uiTreeItem::updateSelection( int selid, bool downward )
{
    if ( uitreeviewitem_ )
        uitreeviewitem_->setSelected( shouldSelect(selid) );

    if ( downward )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	    children_[idx]->updateSelection(selid,downward);
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
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->prepareForShutdown();
}


bool uiTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->askContinueAndSaveIfNeeded( withcancel );
    return true;
}


void uiTreeItem::setChecked( bool yn, bool trigger )
{ if ( uitreeviewitem_ ) uitreeviewitem_->setChecked( yn, trigger ); }


bool uiTreeItem::isChecked() const
{ return uitreeviewitem_ ? uitreeviewitem_->isChecked() : false; }


NotifierAccess* uiTreeItem::checkStatusChange() 
{ return uitreeviewitem_ ? &uitreeviewitem_->stateChanged : 0; }

void uiTreeItem::expand()
{ if ( uitreeviewitem_ ) uitreeviewitem_->setOpen(true); }

void uiTreeItem::collapse()
{ if ( uitreeviewitem_ ) uitreeviewitem_->setOpen(false); }


void uiTreeItem::updateColumnText( int col )
{
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->updateColumnText(col);

    if ( !uitreeviewitem_ ) return;

    if ( !col )
        uitreeviewitem_->setText( name_, col );
}


void uiTreeItem::updateCheckStatus()
{
    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->updateCheckStatus();
}


const uiTreeItem* uiTreeItem::findChild( const char* nm ) const
{
    return const_cast<uiTreeItem*>(this)->findChild(nm);
}
	

const uiTreeItem* uiTreeItem::findChild( int selkey ) const
{
    return const_cast<uiTreeItem*>(this)->findChild(selkey);
}


uiTreeItem* uiTreeItem::findChild( const char* nm )
{
    if ( !strcmp( nm, name_ ) )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	uiTreeItem* res = children_[idx]->findChild(nm);
	if ( res )
	    return res;
    }

    return 0;
}


void uiTreeItem::findChildren( const char* nm, ObjectSet<uiTreeItem>& set )
{
    if ( !strcmp(nm,name_) )
	set += this;

    for ( int idx=0; idx<children_.size(); idx++ )
	children_[idx]->findChildren( nm, set );
}


uiTreeItem* uiTreeItem::findChild( int selkey )
{
    if ( selectionKey()==selkey )
	return this;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	uiTreeItem* res = children_[idx]->findChild(selkey);
	if ( res )
	    return res;
    }

    return 0;
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
    return parent_ ? parent_->getUiParent() : 0;
}


void uiTreeItem::setTreeViewItem( uiTreeViewItem* item )
{
    uitreeviewitem_ = item;
    if ( uitreeviewitem_ )
        uitreeviewitem_->setSelectable( isSelectable() );
}


int uiTreeItem::siblingIndex() const
{
    if ( !uitreeviewitem_ ) return -1;
    return uitreeviewitem_->siblingIndex();
}


uiTreeItem* uiTreeItem::siblingAbove()
{
    if ( !parent_ || !uitreeviewitem_ ) return 0;

    uiTreeViewItem* itemabove = uitreeviewitem_->itemAbove();
    if ( !itemabove ) return 0;

    for ( int idx=0; idx<parent_->children_.size(); idx++ )
    {
	if ( parent_->children_[idx]->getItem()==itemabove )
	    return parent_->children_[idx];
    }

    return 0;
}


uiTreeItem* uiTreeItem::siblingBelow()
{
    if ( !parent_ || !uitreeviewitem_ ) return 0;

    uiTreeViewItem* itembelow = uitreeviewitem_->itemBelow();
    if ( !itembelow ) return 0;

    for ( int idx=0; idx<parent_->children_.size(); idx++ )
    {
	if ( parent_->children_[idx]->getItem()==itembelow )
	    return parent_->children_[idx];
    }

    return 0;
}


uiTreeItem* uiTreeItem::lastChild()
{
    if ( !uitreeviewitem_ ) return 0;
    uiTreeViewItem* lastchild = uitreeviewitem_->lastChild();

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	if ( children_[idx]->getItem()==lastchild )
	    return children_[idx];
    }

    return 0;
}


const uiTreeItem* uiTreeItem::getChild( int idx ) const
{
    if ( idx < 0 || idx >= children_.size() )
	return 0;

    return children_[idx];
}


uiTreeItem* uiTreeItem::getChild( int idx )
{
    if ( idx < 0 || idx >= children_.size() )
	return 0;

    return children_[idx];
}


bool uiTreeItem::addChildImpl( CallBacker* parent, uiTreeItem* newitem,
			       bool below, bool downwards )
{
    if ( !strcmp(newitem->parentType(),typeid(*this).name()) )
    {
	mEnabSelChg( false )
	children_ += newitem;
	newitem->parent_ = this;
	uiTreeViewItem::Setup setup( newitem->name(),
		    (uiTreeViewItem::Type)newitem->uiTreeViewItemType() );
	mDynamicCastGet(uiTreeViewItem*,lvi,parent)
	mDynamicCastGet(uiTreeView*,lv,parent)
	uiTreeViewItem* item = 0;
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
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    if ( children_[idx]->addChld(newitem,below,downwards) )
		return true;
	}
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
    const int idx=children_.indexOf( treeitem );
    if ( idx<0 )
    {
	for ( int idy=0; idy<children_.size(); idy++ )
	    children_[idy]->removeChild(treeitem);

	return;
    }

    if ( uitreeviewitem_ )
        uitreeviewitem_->removeItem( treeitem->getItem() );
    
    delete children_.removeSingle( idx );
    mEnabSelChg( true )
}


uiTreeTopItem::uiTreeTopItem( uiTreeView* listview, bool disab )
    : uiTreeItem( listview->name() )
    , disabselcngresp_(disab)
    , listview_( listview )
    , disabrightclick_(false)
    , disabanyclick_(false)
{
    listview_->rightButtonClicked.notify(
	    		mCB(this,uiTreeTopItem,rightClickCB) );
    listview_->mouseButtonClicked.notify(
	    		mCB(this,uiTreeTopItem,anyButtonClickCB) );
    listview_->selectionChanged.notify(
	    		mCB(this,uiTreeTopItem,selectionChanged) );
}


uiTreeTopItem::~uiTreeTopItem()
{
    listview_->rightButtonClicked.remove(
	    		mCB(this,uiTreeTopItem,rightClickCB) );
    listview_->mouseButtonClicked.remove(
	    		mCB(this,uiTreeTopItem,anyButtonClickCB) );
    listview_->selectionChanged.remove(
	    		mCB(this,uiTreeTopItem,selectionChanged) );
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


void uiTreeTopItem::handleSelectionChanged( bool frombutclick )
{
    if ( disabselcngresp_ && !frombutclick )
	return;

    if ( disabanyclick_ || !listview_->itemNotified() ) return;

    const bool oldstatus = disabanyclick_;
    disabanyclick_ = true;
    anyButtonClick( listview_->itemNotified() );
    disabanyclick_ = oldstatus;
}


void uiTreeTopItem::selectionChanged( CallBacker* )
{
    handleSelectionChanged( false );
}


void uiTreeTopItem::rightClickCB( CallBacker* )
{
    if ( disabanyclick_ || disabrightclick_ || !listview_->itemNotified() )
	return;

    if ( rightClick( listview_->itemNotified() ) )
	listview_->unNotify();
}


void uiTreeTopItem::anyButtonClickCB( CallBacker* cb )
{
    handleSelectionChanged( true );
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
    deepErase( factories_ );
}


void uiTreeFactorySet::addFactory( uiTreeItemFactory* ptr, int placement,
       				   int pol2d )
{
    factories_ += ptr;
    placementidxs_ += placement;
    pol2ds_ += pol2d;
    addnotifier.trigger( factories_.size()-1 );
}


void uiTreeFactorySet::remove( const char* nm )
{
    int index = -1;
    for ( int idx=0; idx<factories_.size(); idx++ )
    {
	if ( !strcmp(nm,factories_[idx]->name()) )
	{
	    index=idx;
	    break;
	}
    }

    if ( index<0 )
	return;

    removenotifier.trigger( index );
    delete factories_[index];
    factories_.remove( index );
    placementidxs_.removeSingle( index );
    pol2ds_.removeSingle( index );
}


int uiTreeFactorySet::nrFactories() const
{ return factories_.size(); }

const uiTreeItemFactory* uiTreeFactorySet::getFactory( int idx ) const
{ return idx<nrFactories() ? factories_[idx] : 0; }

int uiTreeFactorySet::getPlacementIdx( int idx ) const
{ return placementidxs_[idx]; }

int uiTreeFactorySet::getPol2D( int idx ) const
{ return pol2ds_[idx]; }


uiTreeItemRemover::uiTreeItemRemover(uiTreeItem* parent,uiTreeItem* child)
    : parent_( parent ), child_( child )
{}


int uiTreeItemRemover::nextStep()
{
    child_->prepareForShutdown();
    parent_->removeChild( child_ );
    return Finished();
}
