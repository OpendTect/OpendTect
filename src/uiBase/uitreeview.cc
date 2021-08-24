/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/01/2002
________________________________________________________________________

-*/

#include "uitreeview.h"

#include "uiicon.h"
#include "uimain.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uishortcutsmgr.h"
#include "uistrings.h"

#include "keystrs.h"
#include "odqtobjset.h"
#include "perthreadrepos.h"
#include "texttranslator.h"

#include "q_uiimpl.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPixmap>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

#include "i_qtreeview.h"

mUseQtnamespace

static ODQtObjectSet<uiTreeViewItem,QTreeWidgetItem> odqtobjects_;


#define mItemFor(itm)		uiTreeViewItem::itemFor(itm)

class uiTreeViewBody : public uiObjBodyImpl<uiTreeView,QTreeWidget>
{

friend class uiTreeView;

public:
			uiTreeViewBody(uiTreeView& hndle,uiParent* parnt,
				       const char* nm,int nrl);
    virtual		~uiTreeViewBody();

    void		setNrLines( int prefNrLines )
			{
			    if ( prefNrLines >= 0 )
				prefnrlines_ = prefNrLines;

			    int hs = stretch(true,true);
			    setStretch( hs, ( nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual int		nrTxtLines() const
			{ return prefnrlines_ ? prefnrlines_ : 7; }

    uiTreeView&		lvhandle()	{ return lvhandle_; }
    TypeSet<int>&	fixedColWidth()	{ return fixcolwidths_; }

protected:

    QPoint		mousepos_;
    int			prefnrlines_;

    void		resizeEvent(QResizeEvent *);
    void		keyPressEvent(QKeyEvent*);
    bool		moveItem(QKeyEvent*);
    void		mousePressEvent(QMouseEvent*);
    void		mouseReleaseEvent(QMouseEvent*);

    TypeSet<int>	fixcolwidths_;

private:

    i_treeVwMessenger&	messenger_;
    uiTreeView&		lvhandle_;
};



uiTreeViewBody::uiTreeViewBody( uiTreeView& hndle, uiParent* p,
				const char* nm, int nrl )
    : uiObjBodyImpl<uiTreeView,QTreeWidget>( hndle, p, nm )
    , messenger_( *new i_treeVwMessenger(*this,hndle) )
    , prefnrlines_( nrl )
    , lvhandle_(hndle)
    , mousepos_(0,0)
{
    setStretch( 1, (nrTxtLines()== 1) ? 0 : 1 );
    setHSzPol( uiObject::MedVar ) ;

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
    setSelectionBehavior( QTreeWidget::SelectItems );
    setExpandsOnDoubleClick( false );
    setMouseTracking( true );

    if ( header() )
    {
#if QT_VERSION < 0x050000
	header()->setResizeMode( QHeaderView::Interactive );
	header()->setMovable( false );
#else
	header()->setSectionResizeMode( QHeaderView::Interactive );
	header()->setSectionsMovable( false );
#endif
    }
}


uiTreeViewBody::~uiTreeViewBody()
{ delete &messenger_; }


void uiTreeViewBody::resizeEvent( QResizeEvent* ev )
{
    const int nrcols = columnCount();
    if ( nrcols != 2 )
	return QTreeWidget::resizeEvent( ev );

// hack for OpendTect scene tree
    if ( lvhandle_.columnWidthMode(1) == uiTreeView::Fixed )
    {
	const int fixedwidth = fixcolwidths_[ 1 ];
	if ( mIsUdf(fixedwidth) || fixedwidth==0 )
	    return QTreeWidget::resizeEvent( ev );
	QScrollBar* sb = verticalScrollBar();
	int sbwidth = sb && sb->isVisible() ? sb->width() : 0;
	setColumnWidth( 0, width()-fixedwidth-sbwidth-4 );
    }

    QTreeWidget::resizeEvent( ev );
}


static bool isCtrlPressed( QInputEvent& ev )
{
    const Qt::KeyboardModifiers modif = ev.modifiers();
    return (modif & Qt::ControlModifier);
}


static bool isShiftPressed( QInputEvent& ev )
{
    const Qt::KeyboardModifiers modif = ev.modifiers();
    return (modif & Qt::ShiftModifier);
}


void uiTreeViewBody::keyPressEvent( QKeyEvent* ev )
{
    if ( !ev || moveItem(ev) )
	return;

    if ( ev->key() == Qt::Key_Return )
    {
	lvhandle_.returnPressed.trigger();
	return;
    }

    if ( isCtrlPressed(*ev) )
    {
	if ( ev->key() == Qt::Key_A )
	{
	    if ( isShiftPressed(*ev) )
	    { lvhandle_.uncheckAll(); return; }
	    else
	    { lvhandle_.checkAll(); return; }
	}
    }

    uiTreeViewItem* currentitem = lvhandle_.currentItem();
    if ( !currentitem ) return;

    uiKeyDesc kd( ev );
    CBCapsule<uiKeyDesc> cbc( kd, this );
    currentitem->keyPressed.trigger( &cbc );
    if ( cbc.data.key() != 0 )
    {
	lvhandle_.unusedKey.trigger();
// Disabled for now. Probably need a 'handled' boolean somewhere.
	QTreeWidget::keyPressEvent( ev );
    }
}


void uiTreeViewBody::mouseReleaseEvent( QMouseEvent* ev )
{
    if ( !ev ) return;

    if ( ev->button() == Qt::RightButton )
	lvhandle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == Qt::LeftButton )
	lvhandle_.buttonstate_ = OD::LeftButton;
    else
	lvhandle_.buttonstate_ = OD::NoButton;

    QTreeWidget::mouseReleaseEvent( ev );
    lvhandle_.buttonstate_ = OD::NoButton;
}


void uiTreeViewBody::mousePressEvent( QMouseEvent* ev )
{
    if ( !ev ) return;

    mousepos_ = ev->pos();

    if ( ev->button() == Qt::RightButton )
	lvhandle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == Qt::LeftButton )
	lvhandle_.buttonstate_ = OD::LeftButton;
    else
	lvhandle_.buttonstate_ = OD::NoButton;

    QTreeWidget::mousePressEvent( ev );
    lvhandle_.buttonstate_ = OD::NoButton;
}


bool uiTreeViewBody::moveItem( QKeyEvent* ev )
{
    if ( ev->modifiers() != Qt::ShiftModifier )
	return false;

    QTreeWidgetItem* currentitem = currentItem();
    if ( !currentitem ) return false;

    QTreeWidgetItem* twpar = currentitem->parent();
    if ( !twpar ) return false;

    const int childidx = twpar->indexOfChild( currentitem );
    int newchildidx = -1;
    if ( ev->key() == Qt::Key_Up )
	newchildidx = childidx - 1;
    else if ( ev->key() == Qt::Key_Down )
	newchildidx = childidx + 1;

    if ( newchildidx<0 || newchildidx>=twpar->childCount() )
	return false;

    const bool isopen = currentitem->isExpanded();
    twpar->takeChild( childidx );
    twpar->insertChild( newchildidx, currentitem );
    currentitem->setExpanded( isopen );
    setCurrentItem( currentitem );

    return true;
}


uiTreeView::uiTreeView( uiParent* p, const char* nm, int nl, bool dec )
    : uiObject( p, nm, mkbody(p,nm,nl) )
    , selectionChanged(this)
    , currentChanged(this)
    , itemChanged(this)
    , itemRenamed(this)
    , returnPressed(this)
    , leftButtonClicked(this)
    , leftButtonPressed(this)
    , rightButtonClicked(this)
    , rightButtonPressed(this)
    , mouseButtonPressed(this)
    , mouseButtonClicked(this)
    , contextMenuRequested(this)
    , doubleClicked(this)
    , expanded(this)
    , collapsed(this)
    , unusedKey(this)
    , lastitemnotified_(0)
    , column_(0)
    , parent_(p)
{
    itemChanged.notify( mCB(this,uiTreeView,itemChangedCB) );
    mouseButtonClicked.notify( mCB(this,uiTreeView,cursorSelectionChanged) );
    setRootDecorated( dec );
    setBackgroundColor( roBackgroundColor() );
}


uiTreeView::~uiTreeView()
{
    for ( int idx=0; idx<nrItems(); idx++ )
	delete getItem( idx );
}


bool uiTreeView::isEmpty() const
{
    return body_->topLevelItemCount() < 1;
}


void uiTreeView::setEmpty()
{
    NotifyStopper ns( selectionChanged );
    for ( int idx=0; idx<nrItems(); idx++ )
	delete getItem( idx );

    body_->QTreeWidget::clear();
}


void uiTreeView::itemChangedCB( CallBacker* )
{
    updateCheckStatus( lastitemnotified_ );

    if ( lastitemnotified_ && lastitemnotified_->renameEnabled(0) &&
	 column_>=0 && column_<nrColumns() )
    {
	const BufferString oldnm = lastitemnotified_->text( column_ );
	const BufferString newnm =
		lastitemnotified_->qtreeitem_->text( column_ );
	if ( oldnm != newnm )
	{
	    lastitemnotified_->texts_[column_] = toUiString(newnm);
	    itemRenamed.trigger();
	}
    }
}


void uiTreeView::cursorSelectionChanged( CallBacker* )
{
    uiTreeViewItem* itm = selectedItem();
    if ( !itm ) return;

    updateCheckStatus( itm );
}


void uiTreeView::updateCheckStatus( uiTreeViewItem* itm )
{
    if ( !itm ) return;

    const bool needstrigger = itm->isChecked(false) != itm->isChecked(true);
    if ( needstrigger && itm->isCheckable() )
	itm->setChecked( itm->isChecked(true), true );
}


uiTreeViewBody& uiTreeView::mkbody( uiParent* p, const char* nm, int nl )
{
    body_ = new uiTreeViewBody( *this, p, nm, nl );
    return *body_;
}


void uiTreeView::setHScrollBarMode( ScrollMode mode )
{ body_->setHorizontalScrollBarPolicy( (Qt::ScrollBarPolicy)(int)mode ); }


void uiTreeView::setVScrollBarMode( ScrollMode mode )
{ body_->setVerticalScrollBarPolicy( (Qt::ScrollBarPolicy)(int)mode ); }


/*! \brief Set preferred number of lines.
    If set to 0, then it is determined by the number of items in tree.
    If set to 1, then the tree has a fixed height of 1 textline and
    therefore can not grow/shrink vertically.
*/
void uiTreeView::setNrLines( int prefNrLines )
{ body_->setNrLines(prefNrLines); }


bool uiTreeView::rootDecorated() const
{ return body_->rootIsDecorated(); }


void uiTreeView::setRootDecorated( bool yn )
{ body_->setRootIsDecorated(yn); }

void uiTreeView::showHeader( bool yn )
{ body_->setHeaderHidden( !yn ); }


/*! \brief insert an already existing item in this object's tree of children

    If you need to move an item from one place in the hierarchy to
    another you can use takeItem() to remove the item from the tree view
    and then insertItem() to put the item back in its new position.

    \sa uiTreeView::takeItem
*/
void uiTreeView::insertItem( int idx, uiTreeViewItem* itm )
{
    mBlockCmdRec;
    body_->insertTopLevelItem( idx, itm->qItem() );
}


void uiTreeView::takeItem( uiTreeViewItem* itm )
{
    mBlockCmdRec;
    const int childid = body_->indexOfTopLevelItem( itm->qItem() );
    body_->takeTopLevelItem( childid );
}


void uiTreeView::setNrColumns( int nrcols )
{
    uiStringSet strings;
    for ( int idx=0; idx<nrcols; idx++ )
	strings += uiString::emptyString();

    addColumns( strings );
}


void uiTreeView::addColumns( const BufferStringSet& lbls )
{
    uiStringSet strings;
    for ( int idx=0; idx<lbls.size(); idx++ )
    {
	strings += toUiString( lbls[idx]->buf() );
    }

    addColumns( strings );
}


void uiTreeView::addColumns( const uiStringSet& lbls )
{
    mBlockCmdRec;
    const int nrcol = nrColumns();
    for ( int idx=0; idx<nrcol; idx++ )
	body_->model()->removeColumn( idx, body_->currentIndex() );

    body_->fixedColWidth().setSize( lbls.size(),  0 );

    labels_ = lbls;
    updateHeaderLabels();
}


void uiTreeView::updateHeaderLabels()
{
    QStringList qlist;
    labels_.fill( qlist );
    body_->setHeaderLabels( qlist );
}


void uiTreeView::removeColumn( int col )
{
    mBlockCmdRec;
    body_->model()->removeColumn( col, body_->currentIndex() );
    labels_.removeSingle( col );
}


void uiTreeView::setColumnText( int col, const uiString& label )
{
    while ( labels_.size()<=col )
	labels_ += uiString();

    labels_[col] = label;
    body_->headerItem()->setText( col, toQString(label) );
}


uiString uiTreeView::getColumnText( int col ) const
{
    if ( !labels_.validIdx(col) ) return uiStrings::sEmptyString();

    return labels_[col];
}

const char* uiTreeView::columnText(int column) const
{ return getColumnText(column).getFullString(); }


void uiTreeView::setColumnWidth( int col, int w )
{ body_->setColumnWidth( col, w ); }


void uiTreeView::setFixedColumnWidth( int col, int w )
{
    body_->setColumnWidth( col, w );
    if ( body_->fixedColWidth().validIdx(col) )
	body_->fixedColWidth()[col] = w;
    setColumnWidthMode( col, uiTreeView::Fixed );
}


int uiTreeView::columnWidth( int col ) const
{ return body_->columnWidth( col ); }


int uiTreeView::nrColumns() const
{ return body_->columnCount(); }


void uiTreeView::setColumnWidthMode( WidthMode widthmode )
{
    const int nrcols = nrColumns();
    for ( int icol=0; icol<nrcols; icol++ )
	setColumnWidthMode( icol, widthmode );
}


void uiTreeView::setColumnWidthMode( int column, WidthMode widthmode )
{
#if QT_VERSION < 0x050000
    body_->header()->setResizeMode( column,
				(QHeaderView::ResizeMode)int(widthmode) );
#else
    body_->header()->setSectionResizeMode( column,
				(QHeaderView::ResizeMode)int(widthmode) );
#endif
}


uiTreeView::WidthMode uiTreeView::columnWidthMode( int column ) const
{
#if QT_VERSION < 0x050000
    return (uiTreeView::WidthMode)int(body_->header()->resizeMode(column));
#else
    return (uiTreeView::WidthMode)
	int(body_->header()->sectionResizeMode(column));
#endif
}


void uiTreeView::setColumnAlignment( Alignment::HPos hal )
{
    const int nrcols = nrColumns();
    for ( int icol=0; icol<nrcols; icol++ )
	setColumnAlignment( icol, hal );
}


void uiTreeView::setColumnAlignment( int col, Alignment::HPos hal )
{
    Alignment al( hal );
    body_->headerItem()->setTextAlignment( col, al.uiValue() );
}


Alignment::HPos uiTreeView::columnAlignment( int col ) const
{
    Alignment al;
    al.setUiValue( body_->headerItem()->textAlignment(col) );
    return al.hPos();
}


void uiTreeView::ensureItemVisible( const uiTreeViewItem* itm )
{
    body_->scrollToItem( itm->qItem() );
}


void uiTreeView::setSelectionMode( SelectionMode mod )
{ body_->setSelectionMode( (QTreeWidget::SelectionMode)int(mod) ); }

uiTreeView::SelectionMode uiTreeView::selectionMode() const
{ return (uiTreeView::SelectionMode)int(body_->selectionMode()); }

void uiTreeView::setSelectionBehavior( SelectionBehavior behavior )
{ body_->setSelectionBehavior( (QTreeWidget::SelectionBehavior)int(behavior)); }


uiTreeView::SelectionBehavior uiTreeView::selectionBehavior() const
{ return (uiTreeView::SelectionBehavior)int(body_->selectionBehavior()); }


void uiTreeView::setSelected( uiTreeViewItem* itm, bool yn )
{
    mBlockCmdRec;
    itm->qItem()->setSelected( yn );
}


bool uiTreeView::isSelected( const uiTreeViewItem* itm ) const
{ return  itm->qItem()->isSelected(); }


uiTreeViewItem* uiTreeView::selectedItem() const
{
    QList<QTreeWidgetItem*> qitms = body_->selectedItems();
    return qitms.isEmpty() ? 0 : mItemFor( qitms[0] );
}


int uiTreeView::nrSelected() const
{ return body_->selectedItems().size(); }


void uiTreeView::getSelectedItems( ObjectSet<uiTreeViewItem>& items ) const
{
    QList<QTreeWidgetItem*> qitms = body_->selectedItems();
    for ( int idx=0; idx<qitms.size(); idx++ )
	items += uiTreeViewItem::itemFor( qitms[idx] );
}


void uiTreeView::removeSelectedItems()
{
    ObjectSet<uiTreeViewItem> items;
    getSelectedItems( items );
    for ( int idx=0; idx<items.size(); idx++ )
	takeItem( items[idx] );

    deepErase( items );
}


void uiTreeView::setCurrentItem( uiTreeViewItem* itm, int column )
{
    mBlockCmdRec;
    body_->setCurrentItem( itm->qItem(), column );
}


uiTreeViewItem* uiTreeView::currentItem() const
{ return mItemFor( body_->currentItem() ); }


int uiTreeView::currentColumn() const
{ return body_->currentColumn(); }


uiTreeViewItem* uiTreeView::getItem( int idx ) const
{ return idx<0 || idx >=nrItems() ? 0 : mItemFor( body_->topLevelItem(idx) ); }


uiTreeViewItem* uiTreeView::firstItem() const
{ return getItem( 0 ); }


uiTreeViewItem* uiTreeView::lastItem() const
{ return getItem( nrItems()-1 ); }


int uiTreeView::nrItems() const
{ return body_->topLevelItemCount(); }


uiTreeViewItem* uiTreeView::findItem( const char* text, int column,
				      bool casesensitive ) const
{
    Qt::MatchFlags flags =
	casesensitive ? Qt::MatchFixedString | Qt::MatchCaseSensitive
		      : Qt::MatchFixedString;
    QList<QTreeWidgetItem*> items =
	lvbody()->findItems( QString(text), flags, column );

    if ( items.isEmpty() && !casesensitive )
    {
	uiTreeViewItem* nextitem = firstItem();
	while( nextitem )
	{
	    if ( !strcmp( nextitem->text( column ), text ) )
		return nextitem;

	    nextitem = nextitem->itemBelow();
	}
    }

    return items.isEmpty() ? 0 : mItemFor( items[0] );
}


int uiTreeView::indexOfItem( uiTreeViewItem* it ) const
{
    for ( int idx=0; idx<nrItems(); idx++ )
	if ( getItem(idx) == it )
	    return idx;

    return -1;
}

/*!
    Removes and deletes all the items in this tree view and triggers an
    update.
*/
void uiTreeView::clear()
{
    mBlockCmdRec;

    for ( int idx=0; idx<nrItems(); idx++ )
	delete getItem( idx );

    ((QTreeWidget*)body_)->clear();
}



void uiTreeView::selectAll()
{
    mBlockCmdRec;
    body_->selectAll();
    for ( int idx=0; idx<nrItems(); idx++ )
	getItem( idx )->checkAll( true, true );
}


void uiTreeView::deselectAll()
{
    mBlockCmdRec;
    body_->clearSelection();
}


void uiTreeView::clearSelection()
{
    mBlockCmdRec;
    body_->clearSelection();
    for ( int idx=0; idx<nrItems(); idx++ )
	getItem( idx )->checkAll( false, true );
}


void uiTreeView::checkAll()
{
    mBlockCmdRec;
    for ( int idx=0; idx<nrItems(); idx++ )
	getItem( idx )->checkAll( true, true );
}


void uiTreeView::uncheckAll()
{
    mBlockCmdRec;
    for ( int idx=0; idx<nrItems(); idx++ )
	getItem( idx )->checkAll( false, true );
}


void uiTreeView::expandAll()
{
    mBlockCmdRec;
    body_->expandAll();
}


void uiTreeView::expandTo( int dpth )
{
    mBlockCmdRec;
    body_->expandToDepth( dpth );
}


void uiTreeView::collapseAll()
{
    mBlockCmdRec;
    body_->collapseAll();
}


/*! \brief Triggers contents update.
    Triggers a size, geometry and content update during the next
    iteration of the event loop.  Ensures that there'll be
    just one update to avoid flicker.
*/
void uiTreeView::triggerUpdate()
{
    body_->updateGeometry();
    body_->repaint();
}


bool uiTreeView::handleLongTabletPress()
{
    BufferString msg = "rightButtonClicked";
    const int refnr = beginCmdRecEvent( msg );
    rightButtonClicked.trigger();
    endCmdRecEvent( refnr, msg );
    return true;
}


void uiTreeView::setNotifiedItem( QTreeWidgetItem* itm )
{ lastitemnotified_ = mItemFor( itm ); }


void uiTreeView::translateText()
{
    updateHeaderLabels();
    for ( int idx=0; idx<nrItems(); idx++ )
    {
	uiTreeViewItem* itm = getItem( idx );
	if ( itm ) itm->translateText();
    }

    for ( int idx=0; idx<nrColumns(); idx++ )
    {
	if ( idx<labels_.size() )
	{
	    body_->headerItem()->setText( idx, toQString(labels_[idx]) );
	}
    }
}


static int sDClickOffset = 70; // Can't find a function to get this information

bool uiTreeView::allowDoubleClick() const
{
// Needed to ignore double-clicks on checkboxes
    const bool hascheckbox =
	lastitemnotified_ && lastitemnotified_->isCheckable();
    if ( !hascheckbox || !lvbody() )
	return true;

    return lvbody()->mousepos_.x()>sDClickOffset;
}


// uiTreeViewItem
#define mTreeViewBlockCmdRec	CmdRecStopper cmdrecstopper(treeView());

#define mInitVars \
    , isselectable_(true), isenabled_(true) \
    , iseditable_(false), isdragenabled_(false), isdropenabled_(false) \
    , ischeckable_(false), checked_(false)

uiTreeViewItem::uiTreeViewItem( uiTreeView* p, const Setup& setup )
    : stateChanged(this)
    , keyPressed(this)
    mInitVars
{
    qtreeitem_ = new QTreeWidgetItem( p ? p->lvbody() : 0 );
    odqtobjects_.add( this, qtreeitem_ );
    init( setup );
}


uiTreeViewItem::uiTreeViewItem( uiTreeViewItem* p, const Setup& setup )
    : stateChanged(this)
    , keyPressed(this)
    mInitVars
{
    qtreeitem_ = new QTreeWidgetItem( p ? p->qItem() : 0 );
    odqtobjects_.add( this, qtreeitem_ );
    init( setup );
}


void uiTreeViewItem::init( const Setup& setup )
{
    ischeckable_ = setup.type_ == uiTreeViewItem::CheckBox;
    updateFlags();

    if ( ischeckable_ )
    {
	setChecked( setup.setcheck_ );
	checked_ = setup.setcheck_;
    }

    if ( setup.after_ )
	moveItem( setup.after_ );
    if ( !setup.iconname_.isEmpty() )
	setIcon( 0, setup.iconname_ );

    if ( setup.labels_.size() )
    {
	for( int idx=0; idx<setup.labels_.size() ; idx++ )
	{ setText( setup.labels_[idx], idx ); }
    }
}


uiTreeViewItem::~uiTreeViewItem()
{
    for ( int idx=0; idx<nrChildren(); idx++ )
	delete getChild( idx );

    odqtobjects_.remove( *this );
    //Deleting qtreeitem_ results in crashes.
    //delete qtreeitem_;
}


void uiTreeViewItem::setText( const uiString& txt, int column )
{
    mTreeViewBlockCmdRec;

    while ( texts_.size()<=column )
	texts_ += uiString();

    texts_[column] = txt;
    qtreeitem_->setText( column, toQString(txt) );
    setToolTip( column, txt );
}


void uiTreeViewItem::setBGColor( int column, const Color& color )
{
    qtreeitem_->setBackground( column,
			QBrush( QColor( color.r(), color.g(), color.b() ) ) );
}


const char* uiTreeViewItem::text( int column ) const
{
    return texts_.validIdx(column) ? texts_[column].getFullString().buf() : 0;
}


void uiTreeViewItem::translateText()
{
    for ( int idx=0; idx<texts_.size(); idx++ )
    {
	qtreeitem_->setText( idx, toQString(texts_[idx]) );
	updateToolTip( idx );
    }

    for ( int idx=0; idx<nrChildren(); idx++ )
	getChild(idx)->translateText();
}


void uiTreeViewItem::setIcon( int column, const char* iconname )
{
    mTreeViewBlockCmdRec;
    uiIcon icon( iconname );
    qItem()->setIcon( column, icon.qicon() );
}


void uiTreeViewItem::removeIcon( int column )
{
    qItem()->setIcon( column, QIcon() );
}


void uiTreeViewItem::setPixmap( int column, const uiPixmap& pm )
{
    mTreeViewBlockCmdRec;
    qItem()->setIcon( column, pm.qpixmap() ? *pm.qpixmap() : QPixmap() );
}


void uiTreeViewItem::setPixmap( int column, const Color& col,
				int width, int height )
{
    uiPixmap pm( width, height );
    pm.fill( col );
    setPixmap( column, pm );
}


void uiTreeViewItem::setPixmap( int column, const ColTab::Sequence& seq,
				int width, int height )
{
    uiPixmap pm( width, height );
    pm.fill( seq, true );
    setPixmap( column, pm );
}


int uiTreeViewItem::nrChildren() const
{ return qItem()->childCount(); }

bool uiTreeViewItem::isOpen() const
{ return qItem()->isExpanded(); }


void uiTreeViewItem::setOpen( bool yn )
{
    mTreeViewBlockCmdRec;
    qItem()->setExpanded( yn );
}


void uiTreeViewItem::setSelected( bool yn )
{
    mTreeViewBlockCmdRec;
    qItem()->setSelected( yn );
}


bool uiTreeViewItem::isSelected() const
{ return qItem()->isSelected(); }

uiTreeViewItem* uiTreeViewItem::getChild( int idx ) const
{ return idx<0 || idx>=nrChildren() ? 0 : mItemFor( qItem()->child( idx ) ); }

uiTreeViewItem* uiTreeViewItem::firstChild() const
{ return getChild(0); }

uiTreeViewItem* uiTreeViewItem::lastChild() const
{ return getChild( nrChildren()-1 ); }


int uiTreeViewItem::siblingIndex() const
{
    return qtreeitem_ && qtreeitem_->parent() ?
	qtreeitem_->parent()->indexOfChild(qtreeitem_) : -1;
}


uiTreeViewItem* uiTreeViewItem::nextSibling() const
{
    if ( !qtreeitem_ || !qtreeitem_->parent() ) return 0;

    return mItemFor( qtreeitem_->parent()->child( siblingIndex()+1 ) );
}


uiTreeViewItem* uiTreeViewItem::prevSibling() const
{
    if ( !qtreeitem_ || !qtreeitem_->parent() ) return 0;

    return mItemFor( qtreeitem_->parent()->child( siblingIndex()-1 ) );
}


uiTreeViewItem* uiTreeViewItem::parent() const
{ return mItemFor( qItem()->parent() ); }


uiTreeViewItem* uiTreeViewItem::itemAbove()
{ return mItemFor( qItem()->treeWidget()->itemAbove(qItem()) ); }


uiTreeViewItem* uiTreeViewItem::itemBelow()
{ return mItemFor( qItem()->treeWidget()->itemBelow(qItem()) ); }


uiTreeView* uiTreeViewItem::treeView() const
{
    QTreeWidget* lv = qtreeitem_->treeWidget();
    uiTreeViewBody* lvb = dynamic_cast<uiTreeViewBody*>(lv);
    return lvb ? &lvb->lvhandle() : 0;
}


void uiTreeViewItem::takeItem( uiTreeViewItem* itm )
{
    mTreeViewBlockCmdRec;
    const int childid = qItem()->indexOfChild( itm->qItem() );
    qItem()->takeChild( childid );
}


void uiTreeViewItem::insertItem( int idx, uiTreeViewItem* itm )
{
    mTreeViewBlockCmdRec;
    qItem()->insertChild( idx, itm->qItem() );
}


void uiTreeViewItem::removeItem( uiTreeViewItem* itm )
{
    mTreeViewBlockCmdRec;
    QTreeWidget* qtw = qItem()->treeWidget();
    if ( qtw && qtw->currentItem()==itm->qItem() )
	qtw->setCurrentItem( 0 );

    qItem()->removeChild( itm->qItem() );
}


void uiTreeViewItem::moveItem( uiTreeViewItem* after )
{
    mTreeViewBlockCmdRec;
    uiTreeViewItem* prnt = parent();
    if ( !prnt || !after ) return;

    const bool isopen = isOpen();
    const int afterid = prnt->qItem()->indexOfChild( after->qItem() );
    prnt->takeItem( this );
    prnt->insertItem( afterid, this );
    setOpen( isopen );
}


void uiTreeViewItem::setDragEnabled( bool yn )
{
    isdragenabled_ = yn;
    updateFlags();
}


void uiTreeViewItem::setDropEnabled( bool yn )
{
    isdropenabled_ = yn;
    updateFlags();
}


bool uiTreeViewItem::dragEnabled() const
{ return qItem()->flags().testFlag( Qt::ItemIsDragEnabled ); }

bool uiTreeViewItem::dropEnabled() const
{ return qItem()->flags().testFlag( Qt::ItemIsDropEnabled ); }


void uiTreeViewItem::setVisible( bool yn )
{ qItem()->setHidden( !yn ); }


bool uiTreeViewItem::isVisible() const
{ return !qItem()->isHidden(); }


void uiTreeViewItem::setRenameEnabled( int column, bool yn )
{
    iseditable_ = yn;
    updateFlags();
}


bool uiTreeViewItem::renameEnabled( int column ) const
{ return qItem()->flags().testFlag( Qt::ItemIsEditable ); }


void uiTreeViewItem::setEnabled( bool yn )
{ qItem()->setDisabled( !yn ); }


bool uiTreeViewItem::isEnabled() const
{ return !qItem()->isDisabled(); }


void uiTreeViewItem::setSelectable( bool yn )
{
    isselectable_ = yn;
    updateFlags();
}


bool uiTreeViewItem::isSelectable() const
{ return qItem()->flags().testFlag( Qt::ItemIsSelectable ); }


void uiTreeViewItem::edit( int col )
{
    if ( !qtreeitem_ && !qtreeitem_->treeWidget() ) return;

    qtreeitem_->treeWidget()->editItem( qtreeitem_, col );
}


void uiTreeViewItem::setCheckable( bool yn )
{
    ischeckable_ = yn;
    updateFlags();
}


bool uiTreeViewItem::isCheckable() const
{
    return qItem()->flags().testFlag( Qt::ItemIsUserCheckable );
}


bool uiTreeViewItem::isChecked( bool qtstatus ) const
{
    return qtstatus ? qtreeitem_->checkState(0) == Qt::Checked
		    : checked_;
}


void uiTreeViewItem::setChecked( bool yn, bool trigger )
{
    if ( !ischeckable_ )
	return;

    mTreeViewBlockCmdRec;
    NotifyStopper ns( stateChanged );
    if ( trigger ) ns.enableNotification();
    qItem()->setCheckState( 0, yn ? Qt::Checked : Qt::Unchecked );
    checked_ = yn;
    stateChanged.trigger();
}


void uiTreeViewItem::checkAll( bool yn, bool trigger )
{
    setChecked( yn, trigger );
    for ( int idx=0; idx<nrChildren(); idx++ )
	getChild(idx)->checkAll( yn, trigger );

}


void uiTreeViewItem::setToolTip( int column, const uiString& txt )
{
    if ( column < 0 )
	return;

    while ( column >= tooltips_.size() )
	tooltips_ += uiString();

    tooltips_[column] = txt;

    updateToolTip( column );
}


bool uiTreeViewItem::updateToolTip( int column )
{
    if ( !tooltips_.validIdx( column ) )
	return false;

    if ( uiMain::isNameToolTipUsed() )
	qtreeitem_->setToolTip( column, "" ); // no name-tooltip for tree items

    qtreeitem_->setToolTip( column, toQString(tooltips_[column]) );

    return true;
}


void uiTreeViewItem::updateToolTips()
{
    for ( int idx=odqtobjects_.size()-1; idx>=0; idx-- )
    {
	int column = 0;
	while ( odqtobjects_.getODObject(idx)->updateToolTip(column++) )
	{}
    }
}

void uiTreeViewItem::updateFlags()
{
    mTreeViewBlockCmdRec;

    Qt::ItemFlags itmflags;
    if ( isselectable_ )
	itmflags |= Qt::ItemIsSelectable;
    if ( iseditable_ )
	itmflags |= Qt::ItemIsEditable;
    if ( isdragenabled_ )
	itmflags |= Qt::ItemIsDragEnabled;
    if ( isdropenabled_ )
	itmflags |= Qt::ItemIsDropEnabled;
    if ( ischeckable_ )
	itmflags |= Qt::ItemIsUserCheckable;
    if ( isenabled_ )
	itmflags |= Qt::ItemIsEnabled;

    qtreeitem_->setFlags( itmflags );
}


uiTreeViewItem* uiTreeViewItem::itemFor( QTreeWidgetItem* itm )
{ return odqtobjects_.getODObject( *itm ); }

const uiTreeViewItem* uiTreeViewItem::itemFor( const QTreeWidgetItem* itm )
{ return odqtobjects_.getODObject( *itm ); }


// uiTreeViewItemIterator
uiTreeViewItemIterator::uiTreeViewItemIterator( uiTreeView& view )
    : view_(view)
{
    iter_ = new QTreeWidgetItemIterator( view_.lvbody() );
}


uiTreeViewItemIterator::~uiTreeViewItemIterator()
{
    delete iter_;
}


uiTreeViewItem* uiTreeViewItemIterator::next()
{
    QTreeWidgetItem* qitm = *(*iter_);
    if ( !qitm )
	return 0;

    uiTreeViewItem* itm = uiTreeViewItem::itemFor( qitm );
    ++(*iter_);
    return itm;
}

