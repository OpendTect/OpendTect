/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/01/2002
 RCS:           $Id: uitreeview.cc,v 1.31 2007-12-20 13:06:15 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uilistview.h"
#include "uifont.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "pixmap.h"
#include "uishortcutsmgr.h"

#include <qevent.h>
#include <qpixmap.h>
#include <qsize.h>

#ifdef USEQT3
# define mQScrollView		QScrollView
# define mQListView		QListView
# define mQCheckListItem        QCheckListItem
#include "i_qlistview.h"
#else
# define mQScrollView		Q3ScrollView
# define mQListView		Q3ListView
# define mQCheckListItem        Q3CheckListItem
#include "i_q4listview.h"
#endif


#define mQitemFor(itm)		uiListViewItem::qitemFor(itm)
#define mItemFor(itm)		uiListViewItem::itemFor(itm)

class uiListViewBody : public uiObjBodyImpl<uiListView,mQListView>
{

public:

                        uiListViewBody( uiListView& handle, uiParent* parnt, 
					const char* nm, int nrl);

    virtual 		~uiListViewBody();
    void 		setLines( int prefNrLines )
			{ 
			    if(prefNrLines >= 0) prefnrlines=prefNrLines;

			    int hs = stretch(true,true);
			    setStretch( hs, ( nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual int 	nrTxtLines() const
			    { return prefnrlines ? prefnrlines : 7; }

    uiListView&		lvhandle()	{ return lvhandle_; }

protected:

    int 		prefnrlines;

    void		keyPressEvent(QKeyEvent*);
    bool		moveItem(QKeyEvent*);

private:

    i_listVwMessenger&	messenger_;
    uiListView&		lvhandle_;
};



uiListViewBody::uiListViewBody( uiListView& handle, uiParent* parnt, 
			const char* nm, int nrl )
    : uiObjBodyImpl<uiListView,mQListView>( handle, parnt, nm )
    , messenger_ (*new i_listVwMessenger(*this, handle))
    , prefnrlines( nrl )
    , lvhandle_(handle)
{
    setStretch( 1, ( nrTxtLines()== 1) ? 0 : 1 );
    setHSzPol( uiObject::MedVar ) ;

    setSorting( -1 );
    setAcceptDrops( TRUE );
    viewport()->setAcceptDrops( TRUE );

}

uiListViewBody::~uiListViewBody()
{
    delete &messenger_;
}


void uiListViewBody::keyPressEvent( QKeyEvent* event )
{
    if ( moveItem(event) ) return;
    
    uiListViewItem* currentitem = lvhandle_.currentItem();
    uiKeyDesc kd( event );
    CBCapsule<uiKeyDesc> cbc( kd, this );
    currentitem->keyPressed.trigger( &cbc );
    if ( cbc.data.key() != 0 )
    {
	lvhandle_.unusedKey.trigger();
	mQListView::keyPressEvent( event );
    }
}


bool uiListViewBody::moveItem( QKeyEvent* event )
{
    if ( event->state() != Qt::ShiftButton )
	return false;

    mQListViewItem* currentitem = currentItem();
    if ( !currentitem ) return false;

    mQListViewItem* parent = currentitem->parent();
    if ( !parent ) return false;

    mQListViewItem* moveafteritem = 0;
    if ( event->key() == Qt::Key_Up )
    {
	mQListViewItem* itmabove = currentitem->itemAbove();
	moveafteritem = itmabove ? itmabove->itemAbove() : 0;
    }
    else if ( event->key() == Qt::Key_Down )
	moveafteritem = currentitem->itemBelow();

    if ( !moveafteritem )
	return false;

    bool res = true;
    if ( moveafteritem->parent() == parent )
	currentitem->moveItem( moveafteritem );
    else if ( moveafteritem == parent )
    {
	parent->takeItem( currentitem );
	parent->insertItem( currentitem );
	setCurrentItem( currentitem );
    }
    else
	res = false;

    return res;
}


uiListView::uiListView( uiParent* p, const char* nm, int nl, bool dec )
    : uiObject( p, nm, mkbody(p,nm,nl) )
    , selectionChanged(this)
    , currentChanged(this)
    , clicked(this)
    , pressed(this)
    , doubleClicked(this)
    , returnPressed(this)
    , spacePressed(this)
    , rightButtonClicked(this)
    , rightButtonPressed(this)
    , mouseButtonPressed(this)
    , mouseButtonClicked(this)
    , contextMenuRequested(this)
    , onItem(this)
    , itemRenamed(this)
    , expanded(this)
    , collapsed(this)
    , unusedKey(this)
    , lastitemnotified(0) 
{
    setRootDecorated( dec );
}


uiListViewBody& uiListView::mkbody( uiParent* p, const char* nm, int nl)
{
    body_ = new uiListViewBody(*this,p,nm,nl);
    return *body_;
}


void uiListView::setHScrollBarMode( ScrollMode mode )
{
    mQScrollView::ScrollBarMode qmode;
    if ( mode==AlwaysOn ) qmode = mQScrollView::AlwaysOn;
    else if ( mode==AlwaysOff ) qmode = mQScrollView::AlwaysOff;
    body_->setHScrollBarMode( qmode );
}


void uiListView::setVScrollBarMode( ScrollMode mode )
{
    mQScrollView::ScrollBarMode qmode;
    if ( mode==AlwaysOn ) qmode = mQScrollView::AlwaysOn;
    else if ( mode==AlwaysOff ) qmode = mQScrollView::AlwaysOff;
    body_->setVScrollBarMode( qmode );
}


/*! \brief Set preferred number of lines. 
    If set to 0, then it is determined by the number of items in list.
    If set to 1, then the list has a fixed height of 1 textline and 
    therefore can not grow/shrink vertically.
*/
void uiListView::setLines( int prefNrLines )
    { body_->setLines(prefNrLines); }


int  uiListView::treeStepSize() const
    { return body_->treeStepSize(); }


void uiListView::setTreeStepSize(int sz)
    { body_->setTreeStepSize(sz); }


bool uiListView::rootDecorated() const
    { return body_->rootIsDecorated(); }


void uiListView::setRootDecorated( bool yn )
    { body_->setRootIsDecorated(yn); }

/*! \brief insert an already existing item in this object's list of children

    If you need to move an item from one place in the hierarchy to
    another you can use takeItem() to remove the item from the list view
    and then insertItem() to put the item back in its new position.

    \sa uiListView::takeItem
*/
void uiListView::insertItem( uiListViewItem* itm )
    { body_->insertItem( mQitemFor(itm) ); }


/*! \brief take an item from this object's list of children

    Removes item from this object's list of children and causes an update
    of the screen display.  The item is not deleted.  You should normally not
    need to call this function because QListViewItem::~QListViewItem() calls it.
    The normal way to delete an item is delete.
    If you need to move an item from one place in the hierarchy to
    another you can use takeItem() to remove the item from the list view
    and then insertItem() to put the item back in its new position.

    Warning: This function leaves item and its children in a state
    where most member functions are unsafe. Only a few functions work
    correctly on an item in this state, most notably insertItem(). The
    functions that work on detached items are explicitly documented as
    such.

    \sa uiListView::insertItem
*/
void uiListView::takeItem( uiListViewItem* itm )
    { body_->takeItem( mQitemFor(itm) ); }


/*! \brief Adds a column
    Adds a column with the column header label to this uiListView, 
    and returns the index of the new column.
    All columns apart from the first one are inserted to the right of the
    existing ones.

    \return index of the new column
*/
int uiListView::addColumn( const char* label, int size)
    { return body_->addColumn( label, size ); }


void uiListView::removeColumn(int idx)
    { body_->removeColumn(idx); }


void uiListView::setColumnText( int column, const char* label )
    { body_->setColumnText( column, label ); }


const char* uiListView::columnText( int idx ) const
{
    if ( idx < 0  ) return "";

    rettxt = (const char*) body_->columnText(idx);
    return rettxt;
}


void uiListView::setColumnWidth( int column, int width )
    { body_->setColumnWidth( column, width ); }


int uiListView::columnWidth( int column ) const
    { return body_->columnWidth(column); }


void uiListView::setColumnWidthMode( int column, WidthMode mod )
{
    switch( mod )
    {
    case Manual : 
	body_->setColumnWidthMode( column, mQListView::Manual );
    break;

    default:
    case Maximum : 
	body_->setColumnWidthMode( column, mQListView::Maximum );
    break;
    }
}


uiListView::WidthMode uiListView::columnWidthMode( int column ) const
{
    switch( body_->columnWidthMode(column) )
    {
    case mQListView::Manual :
	return Manual;

    default:
    case mQListView::Maximum : 
	return Maximum;
    }
}


int uiListView::columns() const			
    { return body_->columns(); }


void uiListView::setColumnAlignment( int idx , int al )
    { body_->setColumnAlignment(idx, al); }


int uiListView::columnAlignment( int idx) const 
    { return body_->columnAlignment(idx); }


void uiListView::ensureItemVisible( const uiListViewItem* itm )
    { body_->ensureItemVisible( mQitemFor(itm) ); }


void uiListView::setMultiSelection( bool yn )
    { body_->setMultiSelection( yn ); }


bool uiListView::isMultiSelection() const
    { return body_->isMultiSelection(); }


void uiListView::setSelectionMode( SelectionMode mod )
{
    switch( mod )
    {
    case Single : 
	body_->setSelectionMode( mQListView::Single );
    break;

    case Multi : 
	body_->setSelectionMode( mQListView::Multi );
    break;

    case Extended : 
	body_->setSelectionMode( mQListView::Extended );
    break;

    default:
    case NoSelection : 
	body_->setSelectionMode( mQListView::NoSelection );
    break;
    }
}

uiListView::SelectionMode uiListView::selectionMode() const
{
    switch( body_->selectionMode() )
    {
    case mQListView::Single :
	return Single;

    case mQListView::Multi :
	return Multi;

    case mQListView::Extended :
	return Extended;

    default:
    case mQListView::NoSelection : 
	return NoSelection;
    }
}


void uiListView::clearSelection()
    { body_->clearSelection(); }


void uiListView::setSelected( uiListViewItem* itm, bool yn )
    { body_->setSelected( mQitemFor(itm), yn); }


bool uiListView::isSelected( const uiListViewItem* itm ) const
    { return body_->isSelected( mQitemFor(itm) ); }


uiListViewItem* uiListView::selectedItem() const
    { return mItemFor( body_->selectedItem() ); }


void uiListView::setOpen( uiListViewItem* itm, bool yn )
    { body_->setOpen( mQitemFor(itm), yn ); }


bool uiListView::isOpen( const uiListViewItem* itm ) const
    { return body_->isOpen( mQitemFor(itm) ); }


void uiListView::setCurrentItem( uiListViewItem* itm )
    { body_->setCurrentItem( mQitemFor(itm) ); }


uiListViewItem* uiListView::currentItem() const
    { return mItemFor( body_->currentItem() ); }


uiListViewItem* uiListView::firstChild() const
    { return mItemFor( body_->firstChild() ); }


uiListViewItem* uiListView::lastItem() const
    { return mItemFor( body_->lastItem() ); }


int uiListView::childCount() const
    { return body_->childCount(); }


void uiListView::setItemMargin( int mrg )
    { body_->setItemMargin( mrg ); }


int uiListView::itemMargin() const
    { return body_->itemMargin(); }


void uiListView::setSorting( int column, bool increasing )
    { body_->setSorting( column, increasing ); }


void uiListView::sort()
    { body_->sort(); }


void uiListView::setShowSortIndicator( bool yn )
    { body_->setShowSortIndicator( yn ); }


bool uiListView::showSortIndicator() const
    { return body_->showSortIndicator(); }


void uiListView::setShowToolTips( bool yn )
    { body_->setShowToolTips( yn ); }


bool uiListView::showToolTips() const
    { return body_->showToolTips(); }


uiListViewItem* uiListView::findItem( const char* text, int column ) const
    { return mItemFor( body_->findItem(text, column) ); }

/*!
    Removes and deletes all the items in this list view and triggers an
    update.
*/
void uiListView::clear()
    { body_->mQListView::clear(); }

void uiListView::invertSelection()
    { body_->invertSelection(); }

void uiListView::selectAll( bool yn )
    { body_->selectAll(yn); }

/*! \brief Triggers contents update.
    Triggers a size, geometry and content update during the next
    iteration of the event loop.  Ensures that there'll be
    just one update to avoid flicker.
*/
void uiListView::triggerUpdate()
    { body_->triggerUpdate(); }

void uiListView::setNotifiedItem( mQListViewItem* itm)
    { lastitemnotified = mItemFor( itm ); }


class uiQListViewItem : public mQListViewItem
{
public:
				uiQListViewItem( uiListViewItem& it, 
						 mQListViewItem* parent ) 
				: mQListViewItem( parent )
				, item_( it )
				{}

				uiQListViewItem( uiListViewItem& it, 
						 mQListView* parent ) 
				: mQListViewItem( parent )
				, item_( it )
				{}

    uiListViewItem&		uiItem() 	{ return item_; }
    const uiListViewItem&	uiItem() const 	{ return item_; }

#define mBaseItemClsss		mQListViewItem
#include			"i_uilistview.h"

protected:
    uiListViewItem&		item_;
};

class uiQCheckListItem : public mQCheckListItem
{
public:
				uiQCheckListItem( uiListViewItem& it, 
						  mQListViewItem* parent,
						  Type tt) 
				: mQCheckListItem( parent, QString(), tt )
				, item_( it )
				{}

				uiQCheckListItem( uiListViewItem& it, 
						  uiQCheckListItem* parent,
						  Type tt) 
				: mQCheckListItem( parent, QString(), tt )
				, item_( it )
				{}

				uiQCheckListItem( uiListViewItem& it, 
						  mQListView* parent,
						  Type tt) 
				: mQCheckListItem( parent, QString(), tt )
				, item_( it )
				{}

    uiListViewItem&		uiItem() 	{ return item_; }
    const uiListViewItem&	uiItem() const 	{ return item_; }

    virtual void 		stateChange ( bool )
				    { item_.stateChanged.trigger(&item_); }


#define mBaseItemClsss		mQCheckListItem
#include			"i_uilistview.h"

protected:
    uiListViewItem&		item_;
};


class uiListViewItemBody
{
public:
				uiListViewItemBody(uiListViewItem&,
						uiListView*, uiListViewItem*,
						const uiListViewItem::Setup& );

    mQListViewItem&		item()			{ return itm; }
    const mQListViewItem&	item() const		{ return itm; }
    uiQCheckListItem*		chklstitem()		{ return clitm; }
    const uiQCheckListItem*	chklstitem() const	{ return clitm; }

private:

    mQListViewItem& 		mkitem( uiListViewItem&, uiListView*, 
					uiListViewItem*, 
					const uiListViewItem::Setup& setup );

    mQListViewItem&		itm;
    uiQCheckListItem*		clitm;

};

uiListViewItemBody::uiListViewItemBody( uiListViewItem& handle,uiListView* lv,
		     uiListViewItem* lvitm, const uiListViewItem::Setup& setup )
: itm( mkitem(handle,lv,lvitm,setup) )
, clitm( 0 )
{
    clitm = dynamic_cast<uiQCheckListItem*>( &itm );
}

mQListViewItem& uiListViewItemBody::mkitem( uiListViewItem& handle,
    uiListView* lv, uiListViewItem* lvitm, const uiListViewItem::Setup& setup )
{
    if( setup.type_ == uiListViewItem::Standard )
    {
	if( lv ) return *new uiQListViewItem( handle, lv->lvbody() );

	if( lvitm && lvitm->itmbody() )
	    return *new uiQListViewItem( handle, &lvitm->itmbody()->item() );

	return *new uiQListViewItem( handle, (mQListView*)0 );

    }
    else
    {
	mQCheckListItem::Type tt = mQCheckListItem::Controller;
	switch( setup.type_ )
	{
	case uiListViewItem::RadioButton : tt = mQCheckListItem::RadioButton; 
	break;
	case uiListViewItem::CheckBox	 : tt = mQCheckListItem::CheckBox; 
	break;
	}

	if( lv ) return *new uiQCheckListItem( handle, lv->lvbody(), tt );

	if( lvitm && lvitm->itmbody() )
	{
	    uiQCheckListItem* cli = lvitm->itmbody()->chklstitem();
	    if( cli )
		return *new uiQCheckListItem( handle, cli, tt );

	    return *new uiQCheckListItem(handle, &lvitm->itmbody()->item(),tt);
	}

	return *new uiQCheckListItem( handle, (mQListView*)0, tt );
    }
}

#define mQthing()		body()->item()



uiListViewItem::uiListViewItem( uiListView*  parent, const char* txt )
: uiHandle<uiListViewItemBody>( txt, &mkbody( parent, 0, Setup(txt) ) )
, stateChanged( this )
, keyPressed( this )
{ 
    init(Setup(txt)); 
}


uiListViewItem::uiListViewItem( uiListViewItem*  parent, const char* txt )
: uiHandle<uiListViewItemBody>( txt, &mkbody( 0, parent, Setup(txt) ) )
, stateChanged( this )
, keyPressed( this )
{ 
    init(Setup(txt)); 
}


uiListViewItem::uiListViewItem( uiListView*  parent, const Setup& setup )
: uiHandle<uiListViewItemBody>( *setup.labels_[0], &mkbody( parent, 0, setup ) )
, stateChanged( this )
, keyPressed( this )
{ 
    init(setup); 
}


uiListViewItem::uiListViewItem( uiListViewItem*  parent, const Setup& setup )
: uiHandle<uiListViewItemBody>( *setup.labels_[0], &mkbody( 0, parent, setup ) )
, stateChanged( this )
, keyPressed( this )
{ 
    init(setup); 
}

uiListViewItemBody& uiListViewItem::mkbody( uiListView* p1,uiListViewItem* p2, 
					    const Setup& setup )
{
    body_= new uiListViewItemBody( *this, p1, p2, setup ) ;
    return *body_;
}

void uiListViewItem::init( const Setup& setup )
{
    if ( setup.after_ )		moveItem( setup.after_ );
    if ( setup.pixmap_ )	setPixmap( 0, *setup.pixmap_ );
    if ( setup.labels_.size() )
    {
	for( int idx=0; idx < setup.labels_.size() ; idx++ )
	    { setText( *setup.labels_[idx], idx ); }
    }
}


void uiListViewItem::insertItem( uiListViewItem* itm )
    { mQthing().insertItem( mQitemFor(itm) ); }


void uiListViewItem::takeItem( uiListViewItem* itm )
    { mQthing().takeItem( mQitemFor(itm) ); }


void uiListViewItem::removeItem( uiListViewItem* itm)
    { mQthing().removeItem( mQitemFor(itm) ); }


int uiListViewItem::siblingIndex() const
{
    const mQListViewItem* thisp = &mQthing();
    const mQListViewItem* parentitem = mQthing().parent();
    const mQListView* parentlistview = mQthing().listView();
    if ( !parentitem && !parentlistview )
	return -1;
    
    mQListViewItem* curitem = parentitem ? parentitem->firstChild()  
					 : parentlistview->firstChild();
    int idx=0;
    while ( curitem && curitem!=thisp )
    {
	idx++;
	curitem = curitem->nextSibling();
    }

    if ( !curitem ) return -1;
    return idx;
}


/*! \brief depth in the tree
    returns parentItem ? parentItem->depth()+1 : -1; 
    with -1 == the hidden root
*/
int uiListViewItem::depth() const
    { return mQthing().depth(); }


void uiListViewItem::setText( const char* txt, int column )
{ 
    QString txt_(txt);
    mQListViewItem& itm = mQthing();

    itm.setText(column,txt_); 
}


const char* uiListViewItem::text( int column ) const
{ 
    rettxt = (const char*)mQthing().text( column );
    return rettxt;
}


void uiListViewItem::setPixmap( int column, const ioPixmap& pm )
{
    mQthing().setPixmap( column, pm.qpixmap() ? *pm.qpixmap() : QPixmap() );
}

/*!
    Creates a new ioPixmap, which becomes yours,
    so you have to delete it..
*/
const ioPixmap* uiListViewItem::pixmap( int column ) const
{ 
    const QPixmap* pm = mQthing().pixmap(column);
    return pm ? new ioPixmap( *pm ) : 0;
}


int uiListViewItem::childCount() const
    { return mQthing().childCount(); }


bool uiListViewItem::isOpen() const
    { return mQthing().isOpen(); }


void uiListViewItem::setOpen( bool yn )
    { mQthing().setOpen(yn); }


void uiListViewItem::setSelected( bool yn )
    { mQthing().setSelected(yn); }


bool uiListViewItem::isSelected() const
    { return mQthing().isSelected(); }


uiListViewItem* uiListViewItem::firstChild() const
    { return mItemFor( mQthing().firstChild() ); }


uiListViewItem* uiListViewItem::nextSibling() const
    { return mItemFor( mQthing().nextSibling() ); }


uiListViewItem* uiListViewItem::parent() const
    { return mItemFor( mQthing().parent() ); }


uiListViewItem* uiListViewItem::itemAbove()
    { return mItemFor( mQthing().itemAbove() ); }


uiListViewItem* uiListViewItem::itemBelow()
    { return mItemFor( mQthing().itemBelow() ); }


uiListView* uiListViewItem::listView() const
{
    mQListView* lv = mQthing().listView();
    uiListViewBody* lvb = dynamic_cast<uiListViewBody*>(lv);
    if ( !lvb ) return 0;

    return &lvb->lvhandle();
}

void uiListViewItem::setSelectable( bool yn )
    { mQthing().setSelectable(yn); }


bool uiListViewItem::isSelectable() const
    { return mQthing().isSelectable(); }


void uiListViewItem::setExpandable( bool yn )
    { mQthing().setExpandable(yn); }


bool uiListViewItem::isExpandable() const
    { return mQthing().isExpandable(); }


void uiListViewItem::moveItem( uiListViewItem* after )
    { mQthing().moveItem(mQitemFor(after)); }


void uiListViewItem::setDragEnabled( bool yn )
{ mQthing().setDragEnabled( yn ); }


void uiListViewItem::setDropEnabled( bool yn )
{ mQthing().setDropEnabled( yn ); }

bool uiListViewItem::dragEnabled() const
{ return mQthing().dragEnabled(); }

bool uiListViewItem::dropEnabled() const
{ return mQthing().dropEnabled(); }


void uiListViewItem::setVisible( bool yn )
    { mQthing().setVisible(yn); }


bool uiListViewItem::isVisible() const
    { return mQthing().isVisible(); }


void uiListViewItem::setRenameEnabled( int column, bool yn )
    { mQthing().setRenameEnabled(column,yn); }


bool uiListViewItem::renameEnabled( int column ) const
    { return mQthing().renameEnabled(column); }


void uiListViewItem::setEnabled( bool yn )
    { mQthing().setEnabled(yn); }


bool uiListViewItem::isEnabled() const
    { return mQthing().isEnabled(); }


void uiListViewItem::setMultiLinesEnabled( bool yn )
    { mQthing().setMultiLinesEnabled(yn); }


bool uiListViewItem::multiLinesEnabled() const
    { return mQthing().multiLinesEnabled(); }



mQListViewItem* uiListViewItem::qitemFor( uiListViewItem* itm )
{
    return &itm->body_->item();
}

const mQListViewItem*  uiListViewItem::qitemFor( const uiListViewItem* itm )
{
    return &itm->body_->item();
}


uiListViewItem*  uiListViewItem::itemFor( mQListViewItem* itm )
{
    uiQListViewItem* uiql = dynamic_cast<uiQListViewItem*>(itm);
    if( uiql )  return &uiql->uiItem();

    uiQCheckListItem* uiqcl = dynamic_cast<uiQCheckListItem*>(itm);
    if( uiqcl ) return &uiqcl->uiItem();

    return 0;
}

const uiListViewItem* uiListViewItem::itemFor( const mQListViewItem* itm )
{
    const uiQListViewItem* uiql = dynamic_cast<const uiQListViewItem*>(itm);
    if( uiql )  return &uiql->uiItem();

    const uiQCheckListItem* uiqcl = dynamic_cast<const uiQCheckListItem*>(itm);
    if( uiqcl ) return &uiqcl->uiItem();

    return 0;
}


#define mChkthing()	body()->chklstitem()

bool uiListViewItem::isCheckable() const
    { return mChkthing() ? true : false; }


void uiListViewItem::setChecked( bool yn, bool trigger )
{
    NotifyStopper ns( stateChanged );
    if ( trigger ) ns.restore();
    if ( mChkthing() ) mChkthing()->setOn( yn );
}


bool uiListViewItem::isChecked() const
    { return mChkthing() ? mChkthing()->isOn() : false; }
