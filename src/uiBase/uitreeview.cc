/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/01/2002
 RCS:           $Id: uitreeview.cc,v 1.36 2008-07-07 09:35:15 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uilistview.h"
#include "uiobjbody.h"
#include "uishortcutsmgr.h"

#include "odqtobjset.h"
#include "pixmap.h"

#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QTreeWidgetItem>

#include "i_qlistview.h"

static ODQtObjectSet<uiListViewItem,QTreeWidgetItem> odqtobjects_;


#define mQitemFor(itm)		uiListViewItem::qitemFor(itm)
#define mItemFor(itm)		uiListViewItem::itemFor(itm)

class uiListViewBody : public uiObjBodyImplNoQtNm<uiListView,QTreeWidget>
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

    void		activateClick(uiListViewItem&);
    void		activateButton(uiListViewItem&,bool expand);
    void		activateMenu(uiListViewItem&);
    bool		event(QEvent*);

protected:

    int 		prefnrlines;

    void		keyPressEvent(QKeyEvent*);
    bool		moveItem(QKeyEvent*);
    void		mousePressEvent(QMouseEvent*);
    void		mouseReleaseEvent(QMouseEvent*);
    void 		setSorting(int,bool increasing = true );

    uiListViewItem*	actitem_;
    bool		actexpand_;

private:

    i_listVwMessenger&	messenger_;
    uiListView&		lvhandle_;
};



uiListViewBody::uiListViewBody( uiListView& handle, uiParent* parent, 
    const char* nm, int nrl )
    : uiObjBodyImplNoQtNm<uiListView,QTreeWidget>( handle, parent, nm )
    , messenger_ ( *new i_listVwMessenger(*this,handle) )
    , prefnrlines( nrl )
    , lvhandle_(handle)
    , actitem_( 0 )
    , actexpand_( true )
{
    setStretch( 1, (nrTxtLines()== 1) ? 0 : 1 );
    setHSzPol( uiObject::MedVar ) ;

    setSorting( -1 );
    setAcceptDrops( TRUE );
    viewport()->setAcceptDrops( TRUE );
    setSelectionBehavior( QTreeWidget::SelectItems );
    setMouseTracking( true );

    if ( header() )
	header()->setResizeMode( QHeaderView::Interactive );
}


uiListViewBody::~uiListViewBody()
{ delete &messenger_; }


void uiListViewBody::setSorting( int column, bool increasing )
{ sortByColumn(column,increasing ? Qt::AscendingOrder : Qt::DescendingOrder); }


void uiListViewBody::keyPressEvent( QKeyEvent* event )
{
    if ( moveItem(event) ) return;

    if ( event->key() == Qt::Key_Return )
    {
	lvhandle_.returnPressed.trigger();
	return;
    }

    uiListViewItem* currentitem = lvhandle_.currentItem();
    uiKeyDesc kd( event );
    CBCapsule<uiKeyDesc> cbc( kd, this );
    currentitem->keyPressed.trigger( &cbc );
    if ( cbc.data.key() != 0 )
    {
	lvhandle_.unusedKey.trigger();
	QTreeWidget::keyPressEvent( event );
    }
}


void uiListViewBody::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !event ) return;

    if ( event->button() == Qt::RightButton )
	lvhandle_.buttonstate_ = OD::RightButton;
    else if ( event->button() == Qt::LeftButton )
	lvhandle_.buttonstate_ = OD::LeftButton;
    else
	lvhandle_.buttonstate_ = OD::NoButton;

    QTreeWidget::mouseReleaseEvent( event );
    lvhandle_.buttonstate_ = OD::NoButton;
}


void uiListViewBody::mousePressEvent( QMouseEvent* event )
{
    if ( !event ) return;

    if ( event->button() == Qt::RightButton )
    {
	lvhandle_.rightButtonPressed.trigger();
	lvhandle_.buttonstate_ = OD::RightButton;
    }
    else if ( event->button() == Qt::LeftButton )
    {
	lvhandle_.mouseButtonPressed.trigger();
	lvhandle_.buttonstate_ = OD::LeftButton;
    }
    else
	lvhandle_.buttonstate_ = OD::NoButton;

    QTreeWidget::mousePressEvent( event );
    lvhandle_.buttonstate_ = OD::NoButton;
}


bool uiListViewBody::moveItem( QKeyEvent* event )
{
    if ( event->state() != Qt::ShiftButton )
	return false;

    QTreeWidgetItem* currentitem = currentItem();
    if ( !currentitem ) return false;

    QTreeWidgetItem* parent = currentitem->parent();
    if ( !parent ) return false;

    QTreeWidget* treewidget = currentitem->treeWidget();
    QTreeWidgetItem* moveafteritem = 0;
    if ( event->key() == Qt::Key_Up )
    {
	QTreeWidgetItem* itmabove = treewidget->itemAbove( currentitem );
	moveafteritem = itmabove ? treewidget->itemAbove( itmabove ) : 0;
    }
    else if ( event->key() == Qt::Key_Down )
	moveafteritem = currentitem->treeWidget()->itemBelow( currentitem );

    if ( !moveafteritem )
	return false;

    const int moveafteritemid = moveafteritem->indexOfChild( moveafteritem );
    const int currentitemid = moveafteritem->indexOfChild( currentitem );
    parent->takeChild( currentitemid );
    parent->insertChild( moveafteritemid, currentitem );
    setCurrentItem( currentitem );
    
    return true;
}


static const QEvent::Type sQEventActClick  = (QEvent::Type) (QEvent::User+0);
static const QEvent::Type sQEventActButton = (QEvent::Type) (QEvent::User+1);
static const QEvent::Type sQEventActMenu   = (QEvent::Type) (QEvent::User+2);


void uiListViewBody::activateClick( uiListViewItem& uilvwitm )
{
    actitem_ = &uilvwitm;
    QEvent* actevent = new QEvent( sQEventActClick );
    QApplication::postEvent( this, actevent );
}


void uiListViewBody::activateButton( uiListViewItem& uilvwitm, bool expand )
{
    actitem_ = &uilvwitm;
    actexpand_ = expand;
    QEvent* actevent = new QEvent( sQEventActButton );
    QApplication::postEvent( this, actevent );
}


void uiListViewBody::activateMenu( uiListViewItem& uilvwitm )
{
    actitem_ = &uilvwitm;
    QEvent* actevent = new QEvent( sQEventActMenu );
    QApplication::postEvent( this, actevent );
}


bool uiListViewBody::event( QEvent* ev )
{
    if ( ev->type() == sQEventActClick )
    {
	if ( actitem_ && actitem_->listView()==&lvhandle_ )
	{
	    lvhandle_.lastitemnotified_ = actitem_;
	    lvhandle_.mouseButtonClicked.trigger();
	}
    }
    else if ( ev->type() == sQEventActButton )
    {
	if ( actitem_ && actitem_->listView()==&lvhandle_ )
	{
	    if ( actexpand_ )
	    {
		actitem_->setOpen( !actitem_->isOpen() );
	    }
	    else
	    {
		lvhandle_.lastitemnotified_ = actitem_;
		lvhandle_.mouseButtonClicked.trigger();
		actitem_->setChecked( !actitem_->isChecked(), true );
	    }
	}
    }
    else if ( actitem_ && ev->type() == sQEventActMenu )
    {
	if ( actitem_->listView()==&lvhandle_ )
	{
	    lvhandle_.lastitemnotified_ = actitem_;
	    lvhandle_.rightButtonClicked.trigger();
	}
    }
    else
	return QTreeWidget::event( ev );

    lvhandle_.activatedone.trigger();
    return true;
}




uiListView::uiListView( uiParent* p, const char* nm, int nl, bool dec )
    : uiObject( p, nm, mkbody(p,nm,nl) )
    , selectionChanged(this)
    , currentChanged(this)
    , itemChanged(this)
    , clicked(this)
    , pressed(this)
    , returnPressed(this)
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
    , lastitemnotified_(0) 
    , activatedone(this)
    , parent_(p)
{
    itemChanged.notify( mCB(this,uiListView,cursorSelectionChanged) );
    setRootDecorated( dec );
}


void uiListView::cursorSelectionChanged( CallBacker* )
{
    if ( lastitemnotified_->isCheckable() )
    {
	lastitemnotified_->setChecked( 
	    lastitemnotified_->qItem()->checkState(0) == Qt::Checked, true );
    }
}

   
uiListViewBody& uiListView::mkbody( uiParent* p, const char* nm, int nl )
{
    body_ = new uiListViewBody( *this, p, nm, nl );
    return *body_;
}


void uiListView::setHScrollBarMode( ScrollMode mode )
{ body_->setHorizontalScrollBarPolicy( (Qt::ScrollBarPolicy)(int)mode ); }


void uiListView::setVScrollBarMode( ScrollMode mode )
{ body_->setVerticalScrollBarPolicy( (Qt::ScrollBarPolicy)(int)mode ); }


/*! \brief Set preferred number of lines. 
    If set to 0, then it is determined by the number of items in list.
    If set to 1, then the list has a fixed height of 1 textline and 
    therefore can not grow/shrink vertically.
*/
void uiListView::setLines( int prefNrLines )
{ body_->setLines(prefNrLines); }


void uiListView::setTreeStepSize( int stepsize )
{//TODO
}

void uiListView::setColumnWidthMode( int column, uiListView::WidthMode )
{//TODO
}

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
void uiListView::insertItem(int item, uiListViewItem* itm )
{ itm->qItem()->insertChild( item, itm->qItem() ); }


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
{ 
     const int childid = itm->qItem()->indexOfChild( itm->qItem() );
     itm->qItem()->takeChild( childid );
}


/*! \brief Adds a column
    Adds a column with the column header label to this uiListView, 
    and returns the index of the new column.
    All columns apart from the first one are inserted to the right of the
    existing ones.
    \return index of the new column
*/


void uiListView::addColumns( const BufferStringSet& lbls )
{
    QStringList qlist;
    for ( int idx=0; idx<body_->columnCount(); idx++ )
	body_->model()->removeColumn( idx, body_->currentIndex() );
    for ( int idx=0; idx<lbls.size(); idx++ )
	qlist.append( QString(lbls.get(idx).buf()) );
    for ( int idx=0; idx<qlist.size(); idx++ )
    {
	if ( idx != 0 )
	    body_->model()->insertColumn( idx );
	body_->headerItem()->setText( idx, qlist[idx] ); 
    }
}


void uiListView::removeColumn( int idx )
{ body_->model()->removeColumn( idx, body_->currentIndex() ); }


void uiListView::setColumnText( int column, const char* label )
{
    QString qlabel = QString( label );
    selectedItem()->qItem()->setText( column, label );
}


const char* uiListView::columnText( int idx ) const
{
    if ( idx < 0  ) return "";

    QString qlabel = selectedItem()->qItem()->text(idx);
    return qlabel.toAscii().data();
}


void uiListView::setColumnWidth( int column, int width )
{ body_->setColumnWidth ( column, width ); }


int uiListView::columnWidth( int column ) const
{ return body_->columnWidth(column); }


int uiListView::columns() const			
{ return body_->model()->columnCount(); }


void uiListView::setColumnAlignment( int idx , int al )
{ selectedItem()->qItem()->setTextAlignment(idx, al); }


int uiListView::columnAlignment( int idx) const 
{ return selectedItem()->qItem()->textAlignment(idx); }


void uiListView::ensureItemVisible( const uiListViewItem* itm )
{
// TODO: Causes a Bus error
//    body_->scrollToItem( itm->qItem() );
}


void uiListView::setMultiSelection( bool yn )
{
    if ( yn )
	body_->setSelectionMode( QTreeWidget::MultiSelection ); 
}


bool uiListView::isMultiSelection() const
{ return body_->selectionMode() == QTreeWidget::MultiSelection; }


void uiListView::setSelectionMode( SelectionMode mod )
{ body_->setSelectionMode( (QTreeWidget::SelectionMode)int(mod) ); }


void uiListView::setSelectionBehavior( SelectionBehavior behavior )
{ body_->setSelectionBehavior( (QTreeWidget::SelectionBehavior)int(behavior)); }


uiListView::SelectionMode uiListView::selectionMode() const
{ return (uiListView::SelectionMode)int(body_->selectionMode()); }


uiListView::SelectionBehavior uiListView::selectionBehavior() const
{ return (uiListView::SelectionBehavior)int(body_->selectionBehavior()); }


void uiListView::clearSelection()
{ body_->clearSelection(); }


void uiListView::setSelected( uiListViewItem* itm, bool yn )
{ itm->qItem()->setSelected( yn ); }


bool uiListView::isSelected( const uiListViewItem* itm ) const
{ return  itm->qItem()->isSelected(); }


uiListViewItem* uiListView::selectedItem() const
{ return mItemFor( body_->currentItem() ); }


void uiListView::setOpen( uiListViewItem* itm, bool yn )
{ body_->expandItem( itm->qItem() ); }


bool uiListView::isOpen( const uiListViewItem* itm ) const
{ return body_->isExpanded( qtreeWidget()->currentIndex() ); }


void uiListView::setCurrentItem( uiListViewItem* itm )
{ body_->setCurrentItem( itm->qItem() ); }


uiListViewItem* uiListView::currentItem() const
{ return mItemFor( body_->currentItem() ); }


uiListViewItem* uiListView::firstChild() const
{ return currentItem() ? mItemFor( currentItem()->qItem()->child(0) ) : 0; }


uiListViewItem* uiListView::lastItem() const
{ return currentItem() ? mItemFor(currentItem()->qItem()->child(childCount()-1)) 		       : 0; }


int uiListView::childCount() const
{ return selectedItem()->qItem()->childCount(); }


void uiListView::setSorting( int column, bool increasing )
{ body_->sortByColumn( column, (Qt::SortOrder)int(increasing) ); }


void uiListView::sort()
{ body_->sortColumn(); }


uiListViewItem* uiListView::findItem( const char* text, int column,
				      bool casesensitive ) const
{
    Qt::MatchFlags flags =
	casesensitive ? Qt::MatchFixedString | Qt::MatchCaseSensitive
		      : Qt::MatchFixedString;
    QList<QTreeWidgetItem*> items =
	lvbody()->findItems( QString(text), flags, column );

    return items.isEmpty() ? 0 : mItemFor( items[0] );
}


/*!
    Removes and deletes all the items in this list view and triggers an
    update.
*/
void uiListView::clear()
{ body_->QTreeWidget::clear(); }

/*void uiListView::invertSelection()
    { body_->invertSelection(); }*/

void uiListView::selectAll( bool yn )
{ body_->selectAll(); }

/*! \brief Triggers contents update.
    Triggers a size, geometry and content update during the next
    iteration of the event loop.  Ensures that there'll be
    just one update to avoid flicker.
*/
void uiListView::triggerUpdate()
{ body_->updateGeometry(); }

void uiListView::setNotifiedItem( QTreeWidgetItem* itm)
{ lastitemnotified_ = mItemFor( itm ); }


void uiListView::activateClick( uiListViewItem& uilvwitm )
{ body_->activateClick( uilvwitm ); }


void uiListView::activateButton( uiListViewItem& uilvwitm, bool expand )
{ body_->activateButton( uilvwitm, expand ); }


void uiListView::activateMenu( uiListViewItem& uilvwitm )
{ body_->activateMenu( uilvwitm ); }



#define mQthing()		body()->item()


uiListViewItem::uiListViewItem( uiListView* parent, const Setup& setup )
    : stateChanged( this )
    , keyPressed( this )
{ 
    qtreeitem_ = new QTreeWidgetItem( parent ? parent->lvbody() : 0 );
    odqtobjects_.add( this, qtreeitem_ );
    init( setup );
}


uiListViewItem::uiListViewItem( uiListViewItem* parent, const Setup& setup )
    : stateChanged( this )
    , keyPressed( this )
{ 
    qtreeitem_ = new QTreeWidgetItem( parent ? parent->qItem() : 0 );
    odqtobjects_.add( this, qtreeitem_ );
    init( setup );
}


void uiListViewItem::init( const Setup& setup )
{
    if ( setup.after_ )         moveItem( setup.after_ );
    if ( setup.pixmap_ )        setPixmap( 0, *setup.pixmap_ );
    if ( setup.type_ == uiListViewItem::CheckBox )
	qtreeitem_->setCheckState( 0, setup.setcheck_ ? Qt::Checked :
							Qt::Unchecked );
    else
	qtreeitem_->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );

    if ( setup.labels_.size() )
    {
	for( int idx=0; idx < setup.labels_.size() ; idx++ )
	{ setText( *setup.labels_[idx], idx ); }
    }
}


uiListViewItem::~uiListViewItem()
{
    odqtobjects_.remove( *this );
    delete qtreeitem_;
}

/*
while ( curitem && curitem!=thisp )
    {
	idx++;
	curitem = curitem->child(curitem->indexOfChild(qtreeitem_)+1);
    }

    if ( !curitem ) return -1;
    return idx;
}*/


/*! \brief depth in the tree
    returns parentItem ? parentItem->depth()+1 : -1; 
    with -1 == the hidden root
*/
int uiListViewItem::depth() const
{ return qItem()->treeWidget()->depth(); }

void uiListViewItem::setText( const char* txt, int column )
{ 
    qItem()->setText( column, QString(txt) );
    qtreeitem_->setToolTip( column, QString(txt) );
}


const char* uiListViewItem::text( int column ) const
{ 
    rettxt = (const char*)qItem()->text( column );
    return rettxt;
}


void uiListViewItem::setPixmap( int column, const ioPixmap& pm )
{
    qItem()->setIcon( column, pm.qpixmap() ? *pm.qpixmap() : QPixmap() );
}


int uiListViewItem::childCount() const
{ return qItem()->childCount(); }


bool uiListViewItem::isOpen() const
{ return qItem()->isExpanded(); }


void uiListViewItem::setOpen( bool yn )
{ qItem()->setExpanded(yn); }


void uiListViewItem::setSelected( bool yn )
{ qItem()->setSelected(yn); }


bool uiListViewItem::isSelected() const
{ return qItem()->isSelected(); }


uiListViewItem* uiListViewItem::firstChild() const
{ return mItemFor( qItem()->child(0) ); }


int uiListViewItem::siblingIndex() const
{
    QTreeWidgetItem* itm = const_cast<QTreeWidgetItem*> (qItem());
    return itm && itm->parent() ? itm->parent()->indexOfChild(itm) : -1;
}


uiListViewItem* uiListViewItem::nextSibling() const
{ 
    QTreeWidgetItem* itm = const_cast<QTreeWidgetItem*> (qItem());
    return mItemFor( itm->child(itm->indexOfChild(itm)+1) ); 
}


uiListViewItem* uiListViewItem::parent() const
{ return mItemFor( qItem()->parent() ); }

 
uiListViewItem* uiListViewItem::itemAbove()
{ return mItemFor( qItem()->treeWidget()->itemAbove(qItem()) ); }


uiListViewItem* uiListViewItem::itemBelow()
{ return mItemFor( qItem()->treeWidget()->itemBelow(qItem()) ); }


uiListView* uiListViewItem::listView() const
{
    return &( listView()->lvbody()->lvhandle() );
}


void uiListViewItem::setSelectable( bool yn )
{
    qItem()->treeWidget()->setSelectionMode(yn ? QTreeWidget::SingleSelection
					     : QTreeWidget::NoSelection);
    qItem()->treeWidget()->setSelectionBehavior( QTreeWidget::SelectItems );
}


bool uiListViewItem::isSelectable() const
{ return qItem()->treeWidget()->selectionMode() != QTreeWidget::NoSelection ; }


void uiListViewItem::setExpandable( bool yn )
{ qItem()->setExpanded(yn); }


bool uiListViewItem::isExpandable() const
{ return qItem()->isExpanded(); }


void uiListViewItem::takeItem( uiListViewItem* itm )
{ 
     const int childid = itm->qItem()->indexOfChild( itm->qItem() );
     itm->qItem()->takeChild( childid );
}


void uiListViewItem::insertItem(int item, uiListViewItem* itm )
{ itm->qItem()->insertChild( item, itm->qItem() ); }


void uiListViewItem::removeItem( uiListViewItem* itm )
{
    const int idx = qItem()->indexOfChild( itm->qItem() );
    qItem()->removeChild( itm->qItem() );
}


void uiListViewItem::moveItem( uiListViewItem* after )
{
    parent()->takeItem(this);
    const int afterid = after->qItem()->indexOfChild( after->qItem() );
    parent()->insertItem( afterid, this );
}


void uiListViewItem::setDragEnabled( bool yn )
{
    if ( yn )
	qItem()->treeWidget()->setDragDropMode( QTreeWidget::DragOnly ); 
}


void uiListViewItem::setDropEnabled( bool yn )
{
    if ( yn )
	qItem()->treeWidget()->setDragDropMode( QTreeWidget::DropOnly ); 
}

bool uiListViewItem::dragEnabled() const
{
    return qItem()->treeWidget()->dragDropMode() == QTreeWidget::DragOnly || 
						    QTreeWidget::DragDrop;
}

bool uiListViewItem::dropEnabled() const
{
    return qItem()->treeWidget()->dragDropMode() == QTreeWidget::DropOnly || 
						    QTreeWidget::DragDrop;
}


void uiListViewItem::setVisible( bool yn )
{ qItem()->setHidden(!yn); }


bool uiListViewItem::isVisible() const
{ return !qItem()->isHidden(); }


void uiListViewItem::setRenameEnabled( int column, bool yn )
{ qItem()->setFlags( yn ? Qt::ItemIsEditable : Qt::ItemIsSelectable ); }


bool uiListViewItem::renameEnabled( int column ) const
{ return qItem()->flags() == Qt::ItemIsEditable; }


void uiListViewItem::setEnabled( bool yn )
{ qItem()->setDisabled(!yn); }


bool uiListViewItem::isEnabled() const
{ return !qItem()->isDisabled(); }


uiListViewItem* uiListViewItem::itemFor( QTreeWidgetItem* itm )
{
    return odqtobjects_.getODObject( *itm );
}

const uiListViewItem* uiListViewItem::itemFor( const QTreeWidgetItem* itm )
{
    return odqtobjects_.getODObject( *itm );
}


bool uiListViewItem::isCheckable() const
{
    return qItem()->flags().testFlag( Qt::ItemIsUserCheckable );
}


void uiListViewItem::setChecked( bool yn, bool trigger )
{
    NotifyStopper ns( stateChanged );
    if ( trigger ) ns.restore();
    qItem()->setCheckState( qItem()->indexOfChild(qItem()), 
	    		    yn ? Qt::Checked : Qt::Unchecked );
    stateChanged.trigger();
}


bool uiListViewItem::isChecked() const
{
    return qItem()->checkState( 0 ) == Qt::Checked;
}
