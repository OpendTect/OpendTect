/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/01/2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uitreeview.cc,v 1.80 2012-08-29 18:59:34 cvsnanne Exp $";

#include "uilistview.h"
#include "uiobjbody.h"
#include "uishortcutsmgr.h"

#include "hiddenparam.h"
#include "texttranslator.h"
#include "odqtobjset.h"
#include "pixmap.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QPixmap>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QTreeWidgetItem>

#include "i_qlistview.h"

static ODQtObjectSet<uiListViewItem,mQtclass(QTreeWidgetItem)> odqtobjects_;


#define mQitemFor(itm)		uiListViewItem::qitemFor(itm)
#define mItemFor(itm)		uiListViewItem::itemFor(itm)

class uiListViewBody : public uiObjBodyImpl<uiListView,mQtclass(QTreeWidget)>
{

public:
                        uiListViewBody(uiListView& hndle,uiParent* parnt, 
				       const char* nm,int nrl);
    virtual 		~uiListViewBody();

    void 		setNrLines( int prefNrLines )
			{ 
			    if ( prefNrLines >= 0 )
				prefnrlines_ = prefNrLines;

			    int hs = stretch(true,true);
			    setStretch( hs, ( nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual int 	nrTxtLines() const
			    { return prefnrlines_ ? prefnrlines_ : 7; }

    uiListView&		lvhandle()	{ return lvhandle_; }
    TypeSet<int>&	fixedColWidth()	{ return fixcolwidths_; }

protected:

    int 		prefnrlines_;

    void		resizeEvent(mQtclass(QResizeEvent) *);
    void		keyPressEvent(mQtclass(QKeyEvent*));
    bool		moveItem(mQtclass(QKeyEvent*));
    void		mousePressEvent(mQtclass(QMouseEvent*));
    void		mouseReleaseEvent(mQtclass(QMouseEvent*));

    TypeSet<int>	fixcolwidths_;

private:

    i_listVwMessenger&	messenger_;
    uiListView&		lvhandle_;
};



uiListViewBody::uiListViewBody( uiListView& hndle, uiParent* p, 
				const char* nm, int nrl )
    : uiObjBodyImpl<uiListView,mQtclass(QTreeWidget)>( hndle, p, nm )
    , messenger_( *new i_listVwMessenger(*this,hndle) )
    , prefnrlines_( nrl )
    , lvhandle_(hndle)
{
    setStretch( 1, (nrTxtLines()== 1) ? 0 : 1 );
    setHSzPol( uiObject::MedVar ) ;

    setAcceptDrops( true );
    viewport()->setAcceptDrops( true );
    setSelectionBehavior( mQtclass(QTreeWidget)::SelectItems );
    setMouseTracking( true );

    if ( header() )
    {
	header()->setResizeMode( mQtclass(QHeaderView)::Interactive );
	header()->setMovable( false );
    }
}


uiListViewBody::~uiListViewBody()
{ delete &messenger_; }


void uiListViewBody::resizeEvent( mQtclass(QResizeEvent*) ev )
{
    const int nrcols = columnCount();
    if ( nrcols != 2 )
	return mQtclass(QTreeWidget)::resizeEvent( ev );

// hack for OpendTect scene tree
    if ( lvhandle_.columnWidthMode(1) == uiListView::Fixed )
    {
	const int fixedwidth = fixcolwidths_[ 1 ];
	if ( mIsUdf(fixedwidth) || fixedwidth==0 )
	    return mQtclass(QTreeWidget)::resizeEvent( ev );
	mQtclass(QScrollBar*) sb = verticalScrollBar();
	int sbwidth = sb && sb->isVisible() ? sb->width() : 0;
	setColumnWidth( 0, width()-fixedwidth-sbwidth-4 );
    }

    mQtclass(QTreeWidget)::resizeEvent( ev );
}


void uiListViewBody::keyPressEvent( mQtclass(QKeyEvent*) ev )
{
    if ( moveItem(ev) ) return;

    if ( ev->key() == mQtclass(Qt)::Key_Return )
    {
	lvhandle_.returnPressed.trigger();
	return;
    }

    uiListViewItem* currentitem = lvhandle_.currentItem();
    if ( !currentitem ) return;

    uiKeyDesc kd( ev );
    CBCapsule<uiKeyDesc> cbc( kd, this );
    currentitem->keyPressed.trigger( &cbc );
    if ( cbc.data.key() != 0 )
    {
	lvhandle_.unusedKey.trigger();
	mQtclass(QTreeWidget)::keyPressEvent( ev );
    }
}


void uiListViewBody::mouseReleaseEvent( mQtclass(QMouseEvent*) ev )
{
    if ( !ev ) return;

    if ( ev->button() == mQtclass(Qt)::RightButton )
	lvhandle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == mQtclass(Qt)::LeftButton )
	lvhandle_.buttonstate_ = OD::LeftButton;
    else
	lvhandle_.buttonstate_ = OD::NoButton;

    mQtclass(QTreeWidget)::mouseReleaseEvent( ev );
    lvhandle_.buttonstate_ = OD::NoButton;
}


void uiListViewBody::mousePressEvent( mQtclass(QMouseEvent*) ev )
{
    if ( !ev ) return;

    if ( ev->button() == mQtclass(Qt)::RightButton )
    {
	lvhandle_.rightButtonPressed.trigger();
	lvhandle_.buttonstate_ = OD::RightButton;
    }
    else if ( ev->button() == mQtclass(Qt)::LeftButton )
    {
	lvhandle_.mouseButtonPressed.trigger();
	lvhandle_.buttonstate_ = OD::LeftButton;
    }
    else
	lvhandle_.buttonstate_ = OD::NoButton;

    mQtclass(QTreeWidget)::mousePressEvent( ev );
    lvhandle_.buttonstate_ = OD::NoButton;
}


bool uiListViewBody::moveItem( mQtclass(QKeyEvent*) ev )
{
    if ( ev->modifiers() != mQtclass(Qt)::ShiftModifier )
	return false;

    mQtclass(QTreeWidgetItem*) currentitem = currentItem();
    if ( !currentitem ) return false;

    mQtclass(QTreeWidgetItem*) twpar = currentitem->parent();
    if ( !twpar ) return false;

    const int childidx = twpar->indexOfChild( currentitem );
    int newchildidx = -1;
    if ( ev->key() == mQtclass(Qt)::Key_Up )
	newchildidx = childidx - 1;
    else if ( ev->key() == mQtclass(Qt)::Key_Down )
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


uiListView::uiListView( uiParent* p, const char* nm, int nl, bool dec )
    : uiObject( p, nm, mkbody(p,nm,nl) )
    , selectionChanged(this)
    , currentChanged(this)
    , itemChanged(this)
    , returnPressed(this)
    , leftButtonClicked(this)
    , leftButtonPressed(this)
    , rightButtonClicked(this)
    , rightButtonPressed(this)
    , mouseButtonPressed(this)
    , mouseButtonClicked(this)
    , contextMenuRequested(this)
    , doubleClicked(this)
    , itemRenamed(this)
    , expanded(this)
    , collapsed(this)
    , unusedKey(this)
    , lastitemnotified_(0)
    , column_(0)
    , parent_(p)
{
    itemChanged.notify( mCB(this,uiListView,itemChangedCB) );
    mouseButtonClicked.notify( mCB(this,uiListView,cursorSelectionChanged) );
    setRootDecorated( dec );
}


uiListView::~uiListView()
{
    for ( int idx=0; idx<nrItems(); idx++ )
	delete getItem( idx );
}


void uiListView::itemChangedCB( CallBacker* )
{
    updateCheckStatus( lastitemnotified_ );
}


void uiListView::cursorSelectionChanged( CallBacker* )
{
    uiListViewItem* itm = selectedItem();
    if ( !itm ) return;

    updateCheckStatus( itm );
}


void uiListView::updateCheckStatus( uiListViewItem* itm )
{
    const bool needstrigger = itm->isChecked(false) != itm->isChecked(true);
    if ( needstrigger && itm->isCheckable() )
        itm->setChecked( itm->isChecked(true), true );
}

   
uiListViewBody& uiListView::mkbody( uiParent* p, const char* nm, int nl )
{
    body_ = new uiListViewBody( *this, p, nm, nl );
    return *body_;
}


void uiListView::setHScrollBarMode( ScrollMode mode )
{ body_->setHorizontalScrollBarPolicy(
				(mQtclass(Qt)::ScrollBarPolicy)(int)mode ); }


void uiListView::setVScrollBarMode( ScrollMode mode )
{ body_->setVerticalScrollBarPolicy(
				(mQtclass(Qt)::ScrollBarPolicy)(int)mode ); }


/*! \brief Set preferred number of lines. 
    If set to 0, then it is determined by the number of items in list.
    If set to 1, then the list has a fixed height of 1 textline and 
    therefore can not grow/shrink vertically.
*/
void uiListView::setNrLines( int prefNrLines )
{ body_->setNrLines(prefNrLines); }


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
void uiListView::insertItem( int idx, uiListViewItem* itm )
{
    mBlockCmdRec;
    body_->insertTopLevelItem( idx, itm->qItem() );
}


void uiListView::takeItem( uiListViewItem* itm )
{ 
    mBlockCmdRec;
    const int childid = body_->indexOfTopLevelItem( itm->qItem() );
    body_->takeTopLevelItem( childid );
}


void uiListView::addColumns( const BufferStringSet& lbls )
{
    mBlockCmdRec;
    const int nrcol = nrColumns();
    for ( int idx=0; idx<nrcol; idx++ )
	body_->model()->removeColumn( idx, body_->currentIndex() );

    mQtclass(QStringList) qlist;
    for ( int idx=0; idx<lbls.size(); idx++ )
    {
	body_->fixedColWidth() += 0;
	qlist.append( mQtclass(QString)(lbls.get(idx).buf()) );
    }

    body_->setHeaderLabels( qlist );
}


void uiListView::removeColumn( int col )
{
    mBlockCmdRec;
    body_->model()->removeColumn( col, body_->currentIndex() );
}


void uiListView::setColumnText( int col, const char* label )
{ body_->headerItem()->setText( col, mQtclass(QString)(label) ); }


const char* uiListView::columnText( int col ) const
{
    if ( col < 0  ) return "";
    mQtclass(QString) qlabel = body_->headerItem()->text( col );
    return qlabel.toAscii().data();
}


void uiListView::setColumnWidth( int col, int w )
{ body_->setColumnWidth( col, w ); }


void uiListView::setFixedColumnWidth( int col, int w )
{
    body_->setColumnWidth( col, w );
    if ( body_->fixedColWidth().validIdx(col) )
	body_->fixedColWidth()[col] = w;
    setColumnWidthMode( col, uiListView::Fixed );
}


int uiListView::columnWidth( int col ) const
{ return body_->columnWidth( col ); }


int uiListView::nrColumns() const
{ return body_->columnCount(); }


void uiListView::setColumnWidthMode( int column, WidthMode widthmode )
{
    body_->header()->setResizeMode( column,
	    		(mQtclass(QHeaderView)::ResizeMode)int(widthmode) ); 
}


uiListView::WidthMode uiListView::columnWidthMode( int column ) const
{
    return (uiListView::WidthMode)int(body_->header()->resizeMode(column));
}


void uiListView::setColumnAlignment( int col, Alignment::HPos hal )
{
    Alignment al( hal );
    body_->headerItem()->setTextAlignment( col, al.uiValue() );
}


Alignment::HPos uiListView::columnAlignment( int col ) const 
{
    Alignment al;
    al.setUiValue( body_->headerItem()->textAlignment(col) );
    return al.hPos();
}


void uiListView::ensureItemVisible( const uiListViewItem* itm )
{
    body_->scrollToItem( itm->qItem() );
}


void uiListView::setSelectionMode( SelectionMode mod )
{ body_->setSelectionMode( (mQtclass(QTreeWidget)::SelectionMode)int(mod) ); }

uiListView::SelectionMode uiListView::selectionMode() const
{ return (uiListView::SelectionMode)int(body_->selectionMode()); }

void uiListView::setSelectionBehavior( SelectionBehavior behavior )
{ body_->setSelectionBehavior(
		   (mQtclass(QTreeWidget)::SelectionBehavior)int(behavior)); }


uiListView::SelectionBehavior uiListView::selectionBehavior() const
{ return (uiListView::SelectionBehavior)int(body_->selectionBehavior()); }


void uiListView::clearSelection()
{
    mBlockCmdRec;
    body_->clearSelection();
}


void uiListView::setSelected( uiListViewItem* itm, bool yn )
{
    mBlockCmdRec;
    itm->qItem()->setSelected( yn );
}


bool uiListView::isSelected( const uiListViewItem* itm ) const
{ return  itm->qItem()->isSelected(); }


uiListViewItem* uiListView::selectedItem() const
{ return mItemFor( body_->currentItem() ); }


void uiListView::setCurrentItem( uiListViewItem* itm, int column )
{
    mBlockCmdRec;
    body_->setCurrentItem( itm->qItem(), column );
}


uiListViewItem* uiListView::currentItem() const
{ return mItemFor( body_->currentItem() ); }


int uiListView::currentColumn() const
{ return body_->currentColumn(); }


uiListViewItem* uiListView::getItem( int idx ) const
{ return idx<0 || idx >=nrItems() ? 0 : mItemFor( body_->topLevelItem(idx) ); }


uiListViewItem* uiListView::firstItem() const
{ return getItem( 0 ); }


uiListViewItem* uiListView::lastItem() const
{ return getItem( nrItems()-1 ); }


int uiListView::nrItems() const
{ return body_->topLevelItemCount(); }


uiListViewItem* uiListView::findItem( const char* text, int column,
				      bool casesensitive ) const
{
    mQtclass(Qt)::MatchFlags flags =
	casesensitive ? mQtclass(Qt)::MatchFixedString |
						mQtclass(Qt)::MatchCaseSensitive
		      : mQtclass(Qt)::MatchFixedString;
    mQtclass(QList)<mQtclass(QTreeWidgetItem*)> items =
	lvbody()->findItems( mQtclass(QString)(text), flags, column );

    if ( items.isEmpty() && !casesensitive )
    {
	uiListViewItem* nextitem = firstItem();
	while( nextitem )
	{
	    if ( !strcmp( nextitem->text( column ), text ) )
		return nextitem;

	    nextitem = nextitem->itemBelow();
	}
    }

    return items.isEmpty() ? 0 : mItemFor( items[0] );
}


int uiListView::indexOfItem( uiListViewItem* it ) const
{
    for ( int idx=0; idx<nrItems(); idx++ )
	if ( getItem(idx) == it )
	    return idx;

    return -1;
}

/*!
    Removes and deletes all the items in this list view and triggers an
    update.
*/
void uiListView::clear()
{
    mBlockCmdRec;
    ((mQtclass(QTreeWidget*))body_)->clear();
}

void uiListView::selectAll()
{
    mBlockCmdRec;
    body_->selectAll();
}

void uiListView::expandAll()
{
    mBlockCmdRec;
    body_->expandAll();
}

void uiListView::expandTo( int dpth )
{
    mBlockCmdRec;
    body_->expandToDepth( dpth );
}


void uiListView::collapseAll()
{
    mBlockCmdRec;
    body_->collapseAll();
}


/*! \brief Triggers contents update.
    Triggers a size, geometry and content update during the next
    iteration of the event loop.  Ensures that there'll be
    just one update to avoid flicker.
*/
void uiListView::triggerUpdate()
{ body_->updateGeometry(); }


bool uiListView::handleLongTabletPress()
{
    BufferString msg = "rightButtonClicked";
    const int refnr = beginCmdRecEvent( msg );
    rightButtonClicked.trigger();
    endCmdRecEvent( refnr, msg );
    return true;
}


void uiListView::setNotifiedItem( mQtclass(QTreeWidgetItem*) itm)
{ lastitemnotified_ = mItemFor( itm ); }


void uiListView::translate()
{
    for ( int idx=0; idx<nrItems(); idx++ )
    {
	uiListViewItem* itm = getItem( idx );
	if ( itm ) itm->translate( 0 );
    }
}


#define mListViewBlockCmdRec	CmdRecStopper cmdrecstopper(listView());

uiListViewItem::uiListViewItem( uiListView* p, const Setup& setup )
    : stateChanged(this)
    , keyPressed(this)
    , translateid_(-1)
{ 
    qtreeitem_ = new mQtclass(QTreeWidgetItem)( p ? p->lvbody() : 0 );
    odqtobjects_.add( this, qtreeitem_ );
    init( setup );
}


uiListViewItem::uiListViewItem( uiListViewItem* p, const Setup& setup )
    : stateChanged(this)
    , keyPressed(this)
    , translateid_(-1)
{ 
    qtreeitem_ = new mQtclass(QTreeWidgetItem)( p ? p->qItem() : 0 );
    odqtobjects_.add( this, qtreeitem_ );
    init( setup );
}


void uiListViewItem::init( const Setup& setup )
{
    if ( setup.after_ )
	moveItem( setup.after_ );
    if ( setup.pixmap_ )
	setPixmap( 0, *setup.pixmap_ );

    isselectable_ = isenabled_ = true;
    iseditable_ = isdragenabled_ = isdropenabled_ = false;
    ischeckable_ = setup.type_ == uiListViewItem::CheckBox;
    updateFlags();

    checked_ = false;
    if ( ischeckable_ )
    {
	setChecked( setup.setcheck_ );
	checked_ = setup.setcheck_;
    }

    if ( setup.labels_.size() )
    {
	for( int idx=0; idx<setup.labels_.size() ; idx++ )
	{ setText( *setup.labels_[idx], idx ); }
    }
}


uiListViewItem::~uiListViewItem()
{
    for ( int idx=0; idx<nrChildren(); idx++ )
	delete getChild( idx );

    odqtobjects_.remove( *this );
//  Not sure whether the qtreeitem_ should be delete here.
//  When enabled od crashes, so commented for now
//    delete qtreeitem_;
}


void uiListViewItem::setText( const char* txt, int column )
{ 
    qtreeitem_->setText( column, mQtclass(QString)(txt) );
    qtreeitem_->setToolTip( column, mQtclass(QString)(txt) );
}


void uiListViewItem::setBGColor( int column, const Color& color )
{
    qtreeitem_->setBackground( column,
	    mQtclass(QBrush)( mQtclass(QColor)( color.r(), color.g(),
		    				color.b() ) ) );
}


const char* uiListViewItem::text( int column ) const
{
    rettxt = mQStringToConstChar( qItem()->text(column) );
    return rettxt;
}


void uiListViewItem::translate( int column )
{
    if ( !TrMgr().tr() ) return;

    TrMgr().tr()->ready.notify( mCB(this,uiListViewItem,trlReady) );
    BufferString txt = text( column );
    translateid_ = TrMgr().tr()->translate( txt );
}


void uiListViewItem::trlReady( CallBacker* cb )
{
    mCBCapsuleUnpack(int,id,cb);
    if ( id != translateid_ )
	return;

    const wchar_t* translation = TrMgr().tr()->get();
    mQtclass(QString) txt = mQtclass(QString)::fromWCharArray( translation );
    mQtclass(QString) tt( text(0) ); tt += "\n\n"; tt += txt;
    qtreeitem_->setToolTip( 0, tt );
    TrMgr().tr()->ready.remove( mCB(this,uiListViewItem,trlReady) );
}


void uiListViewItem::setPixmap( int column, const ioPixmap& pm )
{ qItem()->setIcon( column, pm.qpixmap() ? *pm.qpixmap()
					 : mQtclass(QPixmap)() ); }

int uiListViewItem::nrChildren() const
{ return qItem()->childCount(); }

bool uiListViewItem::isOpen() const
{ return qItem()->isExpanded(); }

void uiListViewItem::setOpen( bool yn )
{
    mListViewBlockCmdRec;
    qItem()->setExpanded( yn );
}

void uiListViewItem::setSelected( bool yn )
{
    mListViewBlockCmdRec;
    qItem()->setSelected( yn );
}

bool uiListViewItem::isSelected() const
{ return qItem()->isSelected(); }

uiListViewItem* uiListViewItem::getChild( int idx ) const
{ return idx<0 || idx>=nrChildren() ? 0 : mItemFor( qItem()->child( idx ) ); }

uiListViewItem* uiListViewItem::firstChild() const
{ return getChild(0); }

uiListViewItem* uiListViewItem::lastChild() const
{ return getChild( nrChildren()-1 ); }


int uiListViewItem::siblingIndex() const
{
    return qtreeitem_ && qtreeitem_->parent() ?
	qtreeitem_->parent()->indexOfChild(qtreeitem_) : -1;
}


uiListViewItem* uiListViewItem::nextSibling() const
{
    if ( !qtreeitem_ || !qtreeitem_->parent() ) return 0;

    return mItemFor( qtreeitem_->parent()->child( siblingIndex()+1 ) );
}


uiListViewItem* uiListViewItem::prevSibling() const
{
    if ( !qtreeitem_ || !qtreeitem_->parent() ) return 0;

    return mItemFor( qtreeitem_->parent()->child( siblingIndex()-1 ) );
}


uiListViewItem* uiListViewItem::parent() const
{ return mItemFor( qItem()->parent() ); }


uiListViewItem* uiListViewItem::itemAbove()
{ return mItemFor( qItem()->treeWidget()->itemAbove(qItem()) ); }


uiListViewItem* uiListViewItem::itemBelow()
{ return mItemFor( qItem()->treeWidget()->itemBelow(qItem()) ); }


uiListView* uiListViewItem::listView() const
{
    mQtclass(QTreeWidget*) lv = qtreeitem_->treeWidget();
    uiListViewBody* lvb = dynamic_cast<uiListViewBody*>(lv);
    return lvb ? &lvb->lvhandle() : 0;
}


void uiListViewItem::takeItem( uiListViewItem* itm )
{ 
    mListViewBlockCmdRec;
    const int childid = qItem()->indexOfChild( itm->qItem() );
    qItem()->takeChild( childid );
}


void uiListViewItem::insertItem( int idx, uiListViewItem* itm )
{
    mListViewBlockCmdRec;
    qItem()->insertChild( idx, itm->qItem() );
}


void uiListViewItem::removeItem( uiListViewItem* itm )
{
    mListViewBlockCmdRec;
    mQtclass(QTreeWidget*) qtw = qItem()->treeWidget();
    if ( qtw && qtw->currentItem()==itm->qItem() )
	qtw->setCurrentItem( 0 );

    qItem()->removeChild( itm->qItem() );
}


void uiListViewItem::moveItem( uiListViewItem* after )
{
    mListViewBlockCmdRec;
    uiListViewItem* prnt = parent();
    if ( !prnt || !after ) return;

    const bool isopen = isOpen();
    const int afterid = prnt->qItem()->indexOfChild( after->qItem() );
    prnt->takeItem( this );
    prnt->insertItem( afterid, this );
    setOpen( isopen );
}


void uiListViewItem::setDragEnabled( bool yn )
{
    isdragenabled_ = yn;
    updateFlags();
}


void uiListViewItem::setDropEnabled( bool yn )
{
    isdropenabled_ = yn;
    updateFlags();
}


bool uiListViewItem::dragEnabled() const
{ return qItem()->flags().testFlag( mQtclass(Qt)::ItemIsDragEnabled ); }

bool uiListViewItem::dropEnabled() const
{ return qItem()->flags().testFlag( mQtclass(Qt)::ItemIsDropEnabled ); }


void uiListViewItem::setVisible( bool yn )
{ qItem()->setHidden( !yn ); }


bool uiListViewItem::isVisible() const
{ return !qItem()->isHidden(); }


void uiListViewItem::setRenameEnabled( int column, bool yn )
{
    iseditable_ = yn;
    updateFlags();
}


bool uiListViewItem::renameEnabled( int column ) const
{ return qItem()->flags().testFlag( mQtclass(Qt)::ItemIsEditable ); }


void uiListViewItem::setEnabled( bool yn )
{ qItem()->setDisabled( !yn ); }


bool uiListViewItem::isEnabled() const
{ return !qItem()->isDisabled(); }


void uiListViewItem::setSelectable( bool yn )
{
    isselectable_ = yn;
    updateFlags();
}


bool uiListViewItem::isSelectable() const
{ return qItem()->flags().testFlag( mQtclass(Qt)::ItemIsSelectable ); }


void uiListViewItem::setCheckable( bool yn )
{
    ischeckable_ = yn;
    updateFlags();
}


bool uiListViewItem::isCheckable() const
{ return qItem()->flags().testFlag( mQtclass(Qt)::ItemIsUserCheckable ); }


void uiListViewItem::setChecked( bool yn, bool trigger )
{
    mListViewBlockCmdRec;
    NotifyStopper ns( stateChanged );
    if ( trigger ) ns.restore();
    qItem()->setCheckState( 0, yn ? mQtclass(Qt)::Checked
	    			  : mQtclass(Qt)::Unchecked );
    checked_ = yn;
    stateChanged.trigger();
}


bool uiListViewItem::isChecked( bool qtstatus ) const
{
    return qtstatus ? qtreeitem_->checkState(0) == mQtclass(Qt)::Checked
		    : checked_;
}


void uiListViewItem::setToolTip( int column, const char*  txt )
{ qtreeitem_->setToolTip( column, mQtclass(QString)(txt) ); }


void uiListViewItem::updateFlags()
{
    mQtclass(Qt)::ItemFlags itmflags;
    if ( isselectable_ )
	itmflags |= mQtclass(Qt)::ItemIsSelectable;
    if ( iseditable_ )
	itmflags |= mQtclass(Qt)::ItemIsEditable;
    if ( isdragenabled_ )
	itmflags |= mQtclass(Qt)::ItemIsDragEnabled;
    if ( isdropenabled_ )
	itmflags |= mQtclass(Qt)::ItemIsDropEnabled;
    if ( ischeckable_ )
	itmflags |= mQtclass(Qt)::ItemIsUserCheckable;
    if ( isenabled_ )
	itmflags |= mQtclass(Qt)::ItemIsEnabled;

    qtreeitem_->setFlags( itmflags );
}


uiListViewItem* uiListViewItem::itemFor( mQtclass(QTreeWidgetItem*) itm )
{ return odqtobjects_.getODObject( *itm ); }

const uiListViewItem* uiListViewItem::itemFor(
					const mQtclass(QTreeWidgetItem*) itm )
{ return odqtobjects_.getODObject( *itm ); }


