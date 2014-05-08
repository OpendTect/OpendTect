/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uilistbox.h"

#include "uifont.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "bufstringset.h"
#include "color.h"
#include "pixmap.h"

#include "i_qlistbox.h"
#include <QKeyEvent>
#include <QMouseEvent>

static const int cIconSz = 16;
int uiListBox::cDefNrLines()	{ return 7; }


mUseQtnamespace


class uiListBoxItem : public QListWidgetItem
{
public:
uiListBoxItem( const QString& txt )
    : QListWidgetItem(txt)
    , ischeckable_(false)
    , ischecked_(false)
    , id_(-1)
{}

    bool		ischeckable_;
    bool		ischecked_;
    int			id_;
};


class uiListBoxBody : public uiObjBodyImpl<uiListBox,QListWidget>
{

public:

                        uiListBoxBody(uiListBox& hndle, uiParent* parnt,
			      const char* nm, uiListBox::ChoiceMode cm,
			      int preferredNrLines, int preferredFieldWidth);

    virtual		~uiListBoxBody()		{ delete &messenger_; }

    void		insertItem(int idx,const uiString& txt,
				   bool mark,int id);
    void		addItem(const uiString&,bool mark,int id);
    void		removeAll();
    void		removeItem( int idx )
			{
			    if ( !items_.validIdx(idx) )
				return;

			    delete takeItem(idx);
			    items_.removeSingle( idx );
			    itemstrings_.removeSingle( idx );
			    itemmarked_.removeSingle( idx );
			}
    uiString&		getItemText(int idx) { return itemstrings_[idx]; }
    BoolTypeSetType&	getItemMark(int idx) { return itemmarked_[idx]; }
    bool		validItemIndex(int idx)
			{ return itemstrings_.validIdx( idx ); }

    void		updateText(int idx);

    int			indexOf(uiListBoxItem*) const;
    void		setItemID(int idx,int id);
    int			getItemID(int idx) const;
    int			getItemIdx(int id) const;

    void		setItemAlignment(int idx,Alignment::HPos);

    void		setNrLines( int prefNrLines )
			{
			    if( prefNrLines >= 0 )
				prefnrlines_=prefNrLines;
			    int hs = stretch(true,true);
			    setStretch( hs, (nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int		nrTxtLines() const
			{ return prefnrlines_ > 0 ? prefnrlines_
						  : uiListBox::cDefNrLines(); }

    int			fieldwidth_;
    int			prefnrlines_;

protected:

    void		mousePressEvent(QMouseEvent*);
    void		mouseMoveEvent(QMouseEvent*);
    void		mouseReleaseEvent(QMouseEvent*);
    void		keyPressEvent(QKeyEvent*);

    int			itemIdxAtEvPos(QMouseEvent&) const;
    void		handleSlideChange(int,bool);

private:

    i_listMessenger&	messenger_;
    ObjectSet<uiListBoxItem> items_;
    TypeSet<uiString>	itemstrings_;
    BoolTypeSet		itemmarked_;
    Interval<int>	sliderg_;

};


static bool doslidesel_ = true;


uiListBoxBody::uiListBoxBody( uiListBox& hndle, uiParent* parnt,
			const char* nm, uiListBox::ChoiceMode cm,
			int preferrednrlines, int preferredfieldwidth )
    : uiObjBodyImpl<uiListBox,QListWidget>( hndle, parnt, nm )
    , messenger_(*new i_listMessenger(this,&hndle))
    , fieldwidth_(preferredfieldwidth)
    , prefnrlines_(preferrednrlines)
    , sliderg_(-1,-1)
{
    setObjectName( nm );
    setDragDropMode( QAbstractItemView::NoDragDrop );
    setAcceptDrops( false ); setDragEnabled( false );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setSelectionMode( cm == uiListBox::None ? QAbstractItemView::NoSelection
					: QAbstractItemView::SingleSelection );

    setStretch( 2, (nrTxtLines()== 1) ? 0 : 2 );
    setHSzPol( uiObject::Medium );

    setMouseTracking( true );
}


static void createQString( QString& qs, const uiString& str, bool mark )
{
    if ( !mark )
	qs = str.getQtString();
    else
    {
	const char* markstr = ":";
	qs = markstr;
	qs += str.getQtString();
	qs += markstr;
    }
}


void uiListBoxBody::addItem( const uiString& txt, bool mark, int id )
{
    QString qs;
    createQString( qs, txt, mark );
    uiListBoxItem* itm = new uiListBoxItem( qs );
    itm->id_ = id;
    items_ += itm;
    itemstrings_ += txt;
    itemmarked_ += mark;
    QListWidget::addItem( itm );
}


void uiListBoxBody::insertItem( int idx, const uiString& txt, bool mark, int id)
{
    QString qs;
    createQString( qs, txt, mark );
    uiListBoxItem* itm = new uiListBoxItem( qs );
    itm->id_ = id;
    items_.insertAt( itm, idx );
    itemstrings_.insert( idx, txt );
    itemmarked_.insert( idx, mark );
    QListWidget::insertItem( idx, itm );
}


void uiListBoxBody::updateText( int idx )
{
    QString qs;
    createQString( qs, itemstrings_[idx], itemmarked_[idx] );
    item(idx)->setText( qs );
}


void uiListBoxBody::removeAll()
{
    QListWidget::clear();
    items_.erase();
    itemstrings_.erase();
    itemmarked_.erase();
}


void uiListBoxBody::setItemID( int idx, int id )
{
    if ( items_.validIdx(idx) )
	items_[idx]->id_ = id;
}

int uiListBoxBody::getItemID( int idx ) const
{
    return items_.validIdx(idx) ? items_[idx]->id_ : -1;
}


int uiListBoxBody::getItemIdx( int id ) const
{
    for ( int idx=0; idx<items_.size(); idx++ )
	if ( items_[idx]->id_ == id )
	    return idx;
    return -1;
}


int uiListBoxBody::indexOf( uiListBoxItem* itm ) const
{
    return items_.indexOf( itm );
}


void uiListBoxBody::setItemAlignment( int idx, Alignment::HPos hpos )
{
    Alignment al( hpos, Alignment::VCenter );
    if ( item(idx) )
	item(idx)->setTextAlignment( (Qt::Alignment)al.uiValue() );
}


uiSize uiListBoxBody::minimumsize() const
{
    const int totHeight = fontHgt() * prefnrlines_;
    const int totWidth  = fontWdt( true ) * fieldwidth_;
    return uiSize( totWidth, totHeight );
}


int uiListBoxBody::itemIdxAtEvPos( QMouseEvent& ev ) const
{
    const QListWidgetItem* itm = itemAt( ev.pos() );
    return itm ? row( itm ) : -1;
}


static bool isCtrlPressed( QInputEvent& ev )
{
    const Qt::KeyboardModifiers modif = ev.modifiers();
    return modif == Qt::ControlModifier;
}


void uiListBoxBody::handleSlideChange( int newstop, bool isclear )
{
    sliderg_.include( newstop, false );
    handle_.setChosen( sliderg_, !isclear );
}


void uiListBoxBody::mousePressEvent( QMouseEvent* ev )
{
    if ( ev && doslidesel_ && ev->button() == Qt::LeftButton
	    && handle_.isMultiChoice() )
	sliderg_.start = sliderg_.stop = itemIdxAtEvPos( *ev );
    else
	sliderg_.start = -1;

    QListWidget::mousePressEvent( ev );
}


void uiListBoxBody::mouseMoveEvent( QMouseEvent* ev )
{
    if ( ev && sliderg_.start >= 0 )
    {
	const int newstop = itemIdxAtEvPos( *ev );
	if ( newstop != sliderg_.stop )
	    handleSlideChange( newstop, isCtrlPressed(*ev) );
    }

    QListWidget::mouseMoveEvent( ev );
}


void uiListBoxBody::mouseReleaseEvent( QMouseEvent* ev )
{
    sliderg_.start = -1;
    if ( !ev ) return;

    if ( ev->button() == Qt::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( ev->button() == Qt::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    QListWidget::mouseReleaseEvent( ev );
    handle_.buttonstate_ = OD::NoButton;
}


void uiListBoxBody::keyPressEvent( QKeyEvent* qkeyev )
{
    if ( qkeyev && qkeyev->key() == Qt::Key_Delete )
	handle_.deleteButtonPressed.trigger();
    if ( qkeyev && qkeyev->key() == Qt::Key_A && isCtrlPressed(*qkeyev) )
	handle_.chooseAll( true );
    if ( qkeyev && qkeyev->key() == Qt::Key_Z && isCtrlPressed(*qkeyev) )
	handle_.chooseAll( false );

    QListWidget::keyPressEvent( qkeyev );
}


// -------------- uiListBox ---------------

#define mStdInit(cm) \
    , choicemode_(cm) \
    , buttonstate_(OD::NoButton) \
    , selectionChanged(this) \
    , doubleClicked(this) \
    , rightButtonClicked(this) \
    , leftButtonClicked(this) \
    , deleteButtonPressed(this) \
    , itemChosen(this) \
    , rightclickmnu_(*new uiMenu(p)) \
    , alignment_(Alignment::Left) \
    , allowduplicates_(true)

#define mStdConstrEnd \
    setBackgroundColor( roBackgroundColor() ); \
    rightButtonClicked.notify( mCB(this,uiListBox,menuCB) )


uiListBox::uiListBox( uiParent* p, const char* nm )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,OnlyOne,0,0) )
    mStdInit(OnlyOne)
{
    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const char* nm, ChoiceMode cm,
			int nl, int pfw )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,cm,nl,pfw) )
    mStdInit(cm)
{
    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& items,
			const char* nm )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,uiListBox::OnlyOne,0,0))
    mStdInit(uiListBox::OnlyOne)
{
    addItems( items ); setName( "Select Data" );
    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& items, const char* nm,
		      ChoiceMode cm, int nl, int pfw )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,cm,nl,pfw))
    mStdInit(cm)
{
    addItems( items ); setName( "Select Data" );
    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const TypeSet<uiString>& items,
		     const char* nm, ChoiceMode cm, int nl, int pfw )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,cm,nl,pfw))
    mStdInit(cm)
{
    addItems( items ); setName( "Select Data" );
    mStdConstrEnd;
}


uiListBoxBody& uiListBox::mkbody( uiParent* p, const char* nm, ChoiceMode cm,
				  int nl, int pfw )
{
    body_ = new uiListBoxBody(*this,p,nm,cm,nl,pfw);
    body_->setIconSize( QSize(cIconSz,cIconSz) );
    return *body_;
}


uiListBox::~uiListBox()
{
    delete &rightclickmnu_;
}


void uiListBox::setChoiceMode( ChoiceMode cm )
{
    if ( choicemode_ == cm )
	return;

    choicemode_ = cm;
    updateFields2ChoiceMode();
}


void uiListBox::setMultiChoice( bool yn )
{
    if ( isMultiChoice() == yn )
	return;

    if ( choicemode_ == OnlyOne )
	choicemode_ = AtLeastOne;
    else
	choicemode_ = OnlyOne;

    updateFields2ChoiceMode();
}


void uiListBox::setAllowNoneChosen( bool yn )
{
    if ( choicemode_ == OnlyOne )
	return;
    const bool nowallowed = choicemode_ == ZeroOrMore;
    if ( nowallowed == yn )
	return;

    choicemode_ = yn ? ZeroOrMore : AtLeastOne;
    updateFields2ChoiceMode();
}


void uiListBox::setNotSelectable()
{
    choicemode_ = None;
    updateFields2ChoiceMode();
    body_->setSelectionMode( QAbstractItemView::NoSelection );
}


#define mSetChecked(idx,yn) \
    body_->item(idx)->setCheckState( yn ? Qt::Checked : Qt::Unchecked )


void uiListBox::updateFields2ChoiceMode()
{
    const bool hascheck = isMultiChoice();
    for ( int idx=0; idx<size(); idx++ )
    {
	setItemCheckable( idx, hascheck );
	if ( hascheck )
	    mSetChecked( idx, false );
    }
}


void uiListBox::menuCB( CallBacker* )
{
    if ( !isMultiChoice() ) return;

    rightclickmnu_.clear();
    rightclickmnu_.insertItem( new uiAction("&Check all items"), 0 );
    rightclickmnu_.insertItem( new uiAction("&Uncheck all items"), 1 );
    if ( nrChosen() > 1 )
	rightclickmnu_.insertItem( new uiAction("&Revert selection"), 2 );
    const int res = rightclickmnu_.exec();
    if ( res==0 || res==1 )
	setAllItemsChecked( res==0 );
    else if ( res == 2 )
    {
	const int selidx = currentItem();
	TypeSet<int> chosen; getChosen( chosen );
	for ( int idx=0; idx<size(); idx++ )
	    setChosen( idx, !chosen.isPresent(idx) );
	setCurrentItem( selidx );
    }
}


void uiListBox::handleCheckChange( QListWidgetItem* itm )
{
    mDynamicCastGet(uiListBoxItem*,lbitm,itm)
    if ( !lbitm ) return;

    const bool ischecked = itm->checkState() == 2;
    if ( lbitm->ischecked_ == ischecked )
	return;

    lbitm->ischecked_ = ischecked;
    const int itmidx = body_->indexOf( lbitm );

    NotifyStopper nsic( itemChosen );
    setCurrentItem( itmidx );
    nsic.restore();

    itemChosen.trigger( itmidx );
}


void uiListBox::setNrLines( int prefnrlines )
{
    body_->setNrLines( prefnrlines );
}


int uiListBox::maxNrOfSelections() const
{
    return isMultiChoice() ? size() : (choicemode_ == None ? 0 : 1);
}


int uiListBox::size() const
{
    return body_->count();
}


bool uiListBox::validIdx( int idx ) const
{
    return body_->validItemIndex( idx );
}


void uiListBox::setAllowDuplicates( bool yn )
{
    allowduplicates_ = yn;
    BufferStringSet allitems; getItems( allitems );
    while ( !allitems.isEmpty() )
    {
	BufferString nm = allitems[0]->buf();
	allitems.removeSingle(0);
	while ( allitems.isPresent( nm ) )
	{
	    const int itmidx = allitems.indexOf( nm );
	    allitems.removeSingle( itmidx );
	    removeItem( itmidx + 1 );
	}
    }
}


void uiListBox::getItems( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( textOfItem( idx ) );
}


void uiListBox::initNewItem( int newidx )
{
    const bool ismulti = isMultiChoice();
    setItemCheckable( newidx, ismulti );
    if ( ismulti )
	mSetChecked( newidx, false );
    body_->setItemAlignment( newidx, alignment_ );
}


void uiListBox::addItem( const uiString& text, bool mark, int id )
{
    if ( !allowduplicates_ && isPresent( text.getFullString() ) )
	return;

    mBlockCmdRec;
    body_->addItem( text, mark, id );
    initNewItem( size() - 1 );
}


void uiListBox::addItem( const uiString& text, const ioPixmap& pm, int id )
{
    addItem( text, false, id );
    setPixmap( size()-1, pm );
}


void uiListBox::addItem( const uiString& text, const Color& col, int id )
{
    ioPixmap pm( 64, 64); pm.fill( col );
    addItem( text, pm, id );
}


void uiListBox::addItems( const char** textList )
{
    const int curidx = currentItem();
    const char** pt_cur = textList;
    while ( *pt_cur )
	addItem( *pt_cur++ );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::addItems( const BufferStringSet& strs )
{
    const int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	addItem( strs.get(idx) );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::addItems( const TypeSet<uiString>& strs )
{
    const int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	addItem( strs[idx] );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::insertItem( const uiString& text, int index, bool mark, int id )
{
    mBlockCmdRec;
    if ( index<0 )
	addItem( text, mark );
    else
    {
	if ( !allowduplicates_ && isPresent( text.getFullString() ) )
	    return;

	body_->insertItem( index, text, mark, id );
	initNewItem( index<0 ? 0 : index );
    }
}


void uiListBox::insertItem( const uiString& text, const ioPixmap& pm,
			    int index, int id )
{
    if ( index < 0 )
	addItem( text, pm, id );
    else
    {
	insertItem( text, index, false, id );
	setPixmap( index, pm );
    }
}


void uiListBox::insertItem( const uiString& text, const Color& col,
			    int index, int id )
{
    ioPixmap pm( 64, 64 ); pm.fill( col );
    insertItem( text, pm, index, id );
}


void uiListBox::setPixmap( int index, const Color& col )
{
    if ( index<0 || index>=size() || !body_->item(index) )
	return;

    QSize sz = body_->iconSize();
    ioPixmap pm( sz.width(), sz.height() ); pm.fill( col );
    setPixmap( index, pm );
}


void uiListBox::setPixmap( int index, const ioPixmap& pm )
{
    if ( index<0 || index>=size() ||
	 !body_->item(index) || !pm.qpixmap() ) return;

    body_->item(index)->setIcon( *pm.qpixmap() );
}


ioPixmap uiListBox::pixmap( int index ) const
{
    if ( index<0 || index>=size() || !body_->item(index) )
	return ioPixmap();
    QIcon qicon = body_->item(index)->icon();
    return ioPixmap( qicon.pixmap(body_->iconSize()) );
}


void uiListBox::setColor( int index, const Color& col )
{
    QColor qcol( col.r(), col.g(), col.b() );
    QListWidgetItem* itm = body_->item( index );
    if ( itm )
	itm->setBackground( qcol );
}


Color uiListBox::getColor( int index ) const
{
    QListWidgetItem* itm = body_->item( index );
    if ( !itm ) return Color(255,255,255);

    const QColor qcol = itm->background().color();
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


void uiListBox::setEmpty()
{
    mBlockCmdRec;
    body_->removeAll();
}


void uiListBox::sortItems( bool asc )
{
    const int sz = size();
    if ( sz < 2 ) return;

    NotifyStopper nss( selectionChanged );
    NotifyStopper nsc( itemChosen );
    BoolTypeSet mrkd, chosen;
    const BufferString cur( getText() );
    BufferStringSet nms;
    for ( int idx=0; idx<sz; idx++ )
    {
	mrkd += isMarked( idx );
	chosen += isChosen( idx );
	nms.add( textOfItem(idx) );
    }
    int* sortidxs = nms.getSortIndexes(true,asc);
    nms.useIndexes( sortidxs );
    setEmpty(); addItems( nms );

    for ( int idx=0; idx<sz; idx++ )
    {
	const int newidx = sortidxs[idx];
	setMarked( newidx, chosen[idx] );
	setChosen( newidx, chosen[idx] );
    }
    delete [] sortidxs;
    if ( !cur.isEmpty() )
	setCurrentItem( cur );
}


void uiListBox::removeItem( const char* txt )
{
    removeItem( indexOf(txt) );
}


void uiListBox::removeItem( int idx )
{
    mBlockCmdRec;
    body_->removeItem( idx );
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString itmtxt( body_->item(idx)->text() );
	itmtxt.trimBlanks();
	if ( itmtxt == txt )
	    return true;
    }
    return false;
}


const char* uiListBox::textOfItem( int idx ) const
{
    if ( !validIdx(idx) )
	return "";

    rettxt_ = body_->getItemText(idx).getFullString();
    return rettxt_;
}


bool uiListBox::isMarked( int idx ) const
{
    return body_->getItemMark( idx );
}


void uiListBox::setMarked( int idx, bool yn )
{
    if ( isMarked(idx) == yn ) return;
    body_->getItemMark( idx ) = yn;
    body_->updateText( idx );
}


int uiListBox::currentItem() const
{
    return body_->currentRow();
}


void uiListBox::setCurrentItem( const char* txt )
{
    if ( !txt ) return;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	const char* ptr = textOfItem( idx );
	mSkipBlanks(ptr);
	if ( FixedString(ptr) == txt )
	    { setCurrentItem( idx ); return; }
    }
}


void uiListBox::setCurrentItem( int idx )
{
    if ( !validIdx(idx) )
	return;

    mBlockCmdRec;

    body_->setCurrentRow( idx );
    if ( choicemode_ == OnlyOne )
	body_->item( idx )->setSelected( true );
}


int uiListBox::indexOf( const char* txt ) const
{
    const FixedString str( txt );
    for ( int idx=0; idx<size(); idx++ )
	if ( str == textOfItem(idx) )
	    return idx;
    return -1;
}


void uiListBox::setItemID( int idx, int id )
{
    body_->setItemID( idx, id );
}


int uiListBox::currentItemID() const
{
    return getItemID( currentItem() );
}


int uiListBox::getItemID( int idx ) const
{
    return body_->getItemID( idx );
}


int uiListBox::getItemIdx( int id ) const
{
    return body_->getItemIdx( id );
}


void uiListBox::setItemText( int idx, const uiString& txt )
{
    if ( !validIdx(idx) ) return;

    body_->getItemText( idx ) = txt;
    body_->updateText( idx );
}


void uiListBox::setFieldWidth( int fw )
{
    body_->fieldwidth_ = fw;
}


int uiListBox::optimumFieldWidth( int minwdth, int maxwdth ) const
{
    const int sz = size();
    int len = minwdth;
    for ( int idx=0; idx<sz; idx++ )
    {
	int itlen = strlen( textOfItem(idx) );
	if ( itlen >= maxwdth )
	    { len = maxwdth; break; }
	else if ( itlen > len )
	    len = itlen;
    }
    return len + 1;
}


void uiListBox::setAlignment( Alignment::HPos al )
{
    alignment_ = al;
    for ( int idx=0; idx<size(); idx++ )
	body_->setItemAlignment( idx, al );
}


bool uiListBox::handleLongTabletPress()
{
    BufferString msg = "rightButtonClicked ";
    msg += currentItem();
    const int refnr = beginCmdRecEvent( msg );
    rightButtonClicked.trigger();
    endCmdRecEvent( refnr, msg );
    return true;
}


void uiListBox::disableRightClick( bool yn )
{
    if ( yn )
	rightButtonClicked.remove(  mCB(this,uiListBox,menuCB) );
    else
	rightButtonClicked.notify( mCB(this,uiListBox,menuCB) );
}


void uiListBox::scrollToTop()
{ body_->scrollToTop(); }

void uiListBox::scrollToBottom()
{ body_->scrollToBottom(); }


void uiListBox::translateText()
{
    uiObject::translateText();

    for ( int idx=0; idx<size(); idx++ )
	body_->updateText( idx );
}


//---- 'Choose' functions ----

void uiListBox::setChoosable( int idx, bool yn )
{
    if ( !validIdx(idx) ) return;

    Qt::ItemFlags flags = body_->item(idx)->flags();
    const bool isselectable = flags.testFlag( Qt::ItemIsSelectable );
    if ( isselectable == yn )
	return;

    body_->item(idx)->setFlags( flags^Qt::ItemIsSelectable );
}


bool uiListBox::isChoosable( int idx ) const
{
    return validIdx(idx) &&
	body_->item(idx)->flags().testFlag( Qt::ItemIsSelectable );
}


int uiListBox::nrChosen() const
{
    if ( !isMultiChoice() )
	return choicemode_ == None || currentItem() < 0 ? 0 : 1;

    int ret = nrChecked();
    if ( ret < 1 && choicemode_ == AtLeastOne )
	ret = currentItem() < 0 ? 0 : 1;
    return ret;
}


bool uiListBox::isChosen( int lidx ) const
{
    if ( choicemode_ == None || !validIdx(lidx) )
	return false;
    else if ( choicemode_ == OnlyOne )
	return lidx == currentItem();

    if ( isItemChecked(lidx) )
	return true;
    if ( choicemode_ == ZeroOrMore || nrChecked() > 0 )
	return false;

    return lidx == currentItem();
}


int uiListBox::firstChosen() const
{
    if ( !isMultiChoice() )
	return choicemode_ == None ? -1 : currentItem();

    return nextChosen( -1 );
}


int uiListBox::nextChosen( int prev ) const
{
    if ( !isMultiChoice() )
	return choicemode_ == None || prev >= 0 ? -1 : currentItem();

    if ( prev < -1 ) prev = -1;
    const int sz = size();
    for ( int idx=prev+1; idx<sz; idx++ )
	if ( isChosen(idx) )
	    return idx;

    return -1;
}


void uiListBox::getChosen( TypeSet<int>& items ) const
{
    items.setEmpty();
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isChosen(idx) )
	    items.add( idx );
}


void uiListBox::getChosen( BufferStringSet& nms ) const
{
    nms.setEmpty();
    TypeSet<int> items; getChosen( items );
    for ( int idx=0; idx<items.size(); idx++ )
	nms.add( textOfItem(items[idx]) );
}


void uiListBox::setChosen( int lidx, bool yn )
{
    if ( isMultiChoice() )
	setItemChecked( lidx, yn );

    if ( yn )
	setCurrentItem( lidx );
}


void uiListBox::setChosen( Interval<int> rg, bool yn )
{
    if ( rg.start < 0 || rg.stop < 0 )
	return;

    if ( !isMultiChoice() )
    {
	if ( choicemode_ == OnlyOne && rg.start >= 0 )
	    setCurrentItem( rg.start );
    }
    else
    {
	rg.sort();
	for ( int idx=rg.start; idx<=rg.stop; idx++ )
	    setItemChecked( idx, yn );
	setCurrentItem( rg.stop );
    }
}


void uiListBox::setChosen( const TypeSet<int>& itms )
{
    if ( isMultiChoice() )
	setCheckedItems( itms );
    else if ( !itms.isEmpty() )
	setCurrentItem( itms[0] );
}


void uiListBox::setChosen( const BufferStringSet& nms )
{
    if ( isMultiChoice() )
	setCheckedItems( nms );
    else if ( !nms.isEmpty() )
	setCurrentItem( nms.get(0) );
}


void uiListBox::chooseAll( bool yn )
{
    if ( isMultiChoice() )
	setAllItemsChecked( yn );
}


//---- 'Check' functions ----


void uiListBox::setItemCheckable( int idx, bool yn )
{
    if ( !validIdx(idx) )
	return;

    Qt::ItemFlags flags = body_->item(idx)->flags();
    flags |= Qt::ItemFlags(yn ? Qt::ItemIsUserCheckable
			      : ~Qt::ItemIsUserCheckable);
    body_->item(idx)->setFlags( flags );
}


void uiListBox::setItemChecked( int idx, bool yn )
{
    if ( isMultiChoice() && yn != isItemChecked(idx) )
    {
	mBlockCmdRec;
	mSetChecked( idx, yn );
    }
}


void uiListBox::setAllItemsChecked( bool yn )
{
    if ( !isMultiChoice() )
	return;

    for ( int idx=0; idx<size(); idx++ )
	setItemChecked( idx, yn );
}


bool uiListBox::isItemChecked( int idx ) const
{
    return isMultiChoice() && validIdx(idx) ?
	   body_->item(idx)->checkState() == Qt::Checked : false;
}


void uiListBox::setItemChecked( const char* nm, bool yn )
{
    if ( !isMultiChoice() )
	return;

    const int idxof = indexOf( nm );
    if ( idxof >= 0 )
	setItemChecked( indexOf( nm ), yn );
}


bool uiListBox::isItemChecked( const char* nm ) const
{
    if ( !isMultiChoice() )
	return false;

    const int idxof = indexOf( nm );
    return idxof < 0 ? false : isItemChecked( idxof );
}


int uiListBox::nrChecked() const
{
    if ( !isMultiChoice() )
	return 0;

    int res = 0;
    for ( int idx=0; idx<size(); idx++ )
	if ( isItemChecked(idx) )
	    res++;
    return res;
}


//---- 'Select' functions ----

/* QqQ deprecated

void uiListBox::setSelectionMode( SelectionMode mode )
{
    pErrMsg("Qt's SelectionMode no longer supported" );
}


void uiListBox::setItemSelectable( int idx, bool yn )
{
    setChoosable( idx, yn );
}


bool uiListBox::isItemSelectable( int idx ) const
{
    return isChoosable( idx );
}


void uiListBox::setSelectedItems( const BufferStringSet& itms )
{
    body_->setCurrentRow( -1 );
    for ( int idx=0; idx<size(); idx++ )
	setSelected( idx, itms.isPresent(textOfItem(idx)) );
}


void uiListBox::setSelectedItems( const TypeSet<int>& itms )
{
    body_->setCurrentRow( -1 );
    for ( int idx=0; idx<size(); idx++ )
	setSelected( idx, itms.isPresent(idx) );
}


bool uiListBox::isSelected ( int idx ) const
{
    return isChosen( idx );
}


int uiListBox::nrSelected() const
{
    return nrChosen();
}


void uiListBox::setSelected( int idx, bool yn )
{
    setChosen( idx, yn );
}


void uiListBox::selectAll( bool yn )
{
    mBlockCmdRec;
    chooseAll( yn );
}


void uiListBox::getSelectedItems( BufferStringSet& items ) const
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isSelected(idx) ) items.add( textOfItem(idx) );
}


void uiListBox::getSelectedItems( TypeSet<int>& items ) const
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isSelected(idx) ) items += idx;
}


QqQ end deprecated */


void uiListBox::setCheckedItems( const BufferStringSet& itms )
{
    for ( int idx=0; idx<size(); idx++ )
	setItemChecked( idx, itms.isPresent(textOfItem(idx)) );
}


void uiListBox::setCheckedItems( const TypeSet<int>& itms )
{
    for ( int idx=0; idx<size(); idx++ )
	setItemChecked( idx, itms.isPresent(idx) );
}


void uiListBox::getCheckedItems( BufferStringSet& items ) const
{
    items.setEmpty();
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isItemChecked(idx) )
	    items.add( textOfItem(idx) );
}


void uiListBox::getCheckedItems( TypeSet<int>& items ) const
{
    items.setEmpty();
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isItemChecked(idx) )
	    items += idx;
}


// -------------- uiLabeledListBox ----------------


uiLabeledListBox::uiLabeledListBox( uiParent* p, const uiString& txt )
    : uiGroup(p,"Labeled listbox")
{
    lb_ = new uiListBox( this, txt.getFullString(), uiListBox::OnlyOne );
    mkRest( txt, LeftMid );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const uiString& txt,
		uiListBox::ChoiceMode cm, uiLabeledListBox::LblPos pos )
    : uiGroup(p,"Labeled listbox")
{
    lb_ = new uiListBox( this, txt.getFullString(), cm );
    mkRest( txt, pos );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const BufferStringSet& s,
    const uiString& txt, uiListBox::ChoiceMode cm, uiLabeledListBox::LblPos pos)
    : uiGroup(p,"Labeled listbox")
{
    lb_ = new uiListBox( this, s, txt.getFullString(), cm );
    mkRest( txt, pos );
}


void uiLabeledListBox::setLabelText( const uiString& txt, int nr )
{
    if ( nr >= lbls_.size() ) return;
    lbls_[nr]->setText( txt );
}


void uiLabeledListBox::mkRest( const uiString& txt,
			       uiLabeledListBox::LblPos pos )
{
    setHAlignObj( lb_ );

    BufferStringSet txts;
    BufferString s( txt.getFullString() );
    char* ptr = s.getCStr();
    if( !ptr || !*ptr ) return;
    while ( 1 )
    {
	char* nlptr = firstOcc( ptr, '\n' );
	if ( nlptr ) *nlptr = '\0';
	txts += new BufferString( ptr );
	if ( !nlptr ) break;

	ptr = nlptr + 1;
    }
    if ( txts.size() < 1 ) return;

    bool last1st = pos > RightTop && pos < BelowLeft;
    ptr = last1st ? txts[txts.size()-1]->getCStr() : txts[0]->getCStr();

    uiLabel* labl = new uiLabel( this, ptr );
    lbls_ += labl;
    constraintType lblct = alignedBelow;
    switch ( pos )
    {
    case LeftTop:
	lb_->attach( rightOf, labl );		lblct = rightAlignedBelow;
    break;
    case RightTop:
	labl->attach( rightOf, lb_ );		lblct = alignedBelow;
    break;
    case LeftMid:
	labl->attach( centeredLeftOf, lb_ );	lblct = alignedBelow;
    break;
    case RightMid:
	labl->attach( centeredRightOf, lb_ );	lblct = alignedBelow;
    break;
    case AboveLeft:
	lb_->attach( alignedBelow, labl );	lblct = alignedAbove;
    break;
    case AboveMid:
	lb_->attach( centeredBelow, labl );	lblct = centeredAbove;
    break;
    case AboveRight:
	lb_->attach( rightAlignedBelow, labl ); lblct = rightAlignedAbove;
    break;
    case BelowLeft:
	labl->attach( alignedBelow, lb_ );	lblct = alignedBelow;
    break;
    case BelowMid:
	labl->attach( centeredBelow, lb_ );	lblct = centeredBelow;
    break;
    case BelowRight:
	labl->attach( rightAlignedBelow, lb_ ); lblct = rightAlignedBelow;
    break;
    }

    int nrleft = txts.size() - 1;
    while ( nrleft )
    {
	uiLabel* cur = new uiLabel( this, (last1st
			? txts[nrleft-1] : txts[txts.size()-nrleft])->buf() );
	cur->attach( lblct, labl );
	lbls_ += cur;
	labl = cur;
	nrleft--;
    }

    deepErase( txts );
}
