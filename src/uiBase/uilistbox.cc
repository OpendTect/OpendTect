/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilistbox.h"
#include "i_qlistbox.h"

#include "uifont.h"
#include "uiicon.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uitoolbutton.h"

#include "bufstringset.h"
#include "color.h"

#include "q_uiimpl.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QStyledItemDelegate>

static const int cIconSz = 16;
int uiListBox::cDefNrLines()	{ return 7; }

mUseQtnamespace


class ListBoxPixmapDelegate : public QStyledItemDelegate
{
public:
ListBoxPixmapDelegate( QObject* qobj )
    : QStyledItemDelegate(qobj)
{}


void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    QStyleOptionViewItem myOpt(option);
    myOpt.decorationPosition = QStyleOptionViewItem::Right;
    QStyledItemDelegate::paint(painter, myOpt, index);
}

};


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


class uiListBoxBody : public uiObjBodyImpl<uiListBoxObj,QListWidget>
{
public:
			uiListBoxBody(uiListBoxObj&,uiParent*,
			      const char* nm,OD::ChoiceMode cm,
			      int prefnrlines=0,int preffieldwidth=0);

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
    bool		isItemMarked(int idx) const { return itemmarked_[idx]; }
    void		setItemMarked( int idx, bool yn )
			{ itemmarked_[idx] = yn; }
    bool		validItemIndex(int idx)
			{ return itemstrings_.validIdx( idx ); }

    void		updateText(int idx);

    int			indexOf(uiListBoxItem*) const;
    void		setItemID(int idx,int id);
    int			getItemID(int idx) const;
    int			getItemIdx(int id) const;

    void		setPixmap(int idx,const uiPixmap&,
				  bool placeright=false);
    void		removePixmap(int idx);
    bool		hasPixmap(int idx);
    void		setItemAlignment(int idx,Alignment::HPos);

    void		setNrLines( int prefNrLines )
			{
			    if( prefNrLines >= 0 )
				prefnrlines_=prefNrLines;
			    int hs = stretch(true,true);
			    setStretch( hs, (nrTxtLines()== 1) ? 0 : 2 );
			}

    uiSize		minimumSize() const override; //!< \reimp
    int			nrTxtLines() const override
			{ return prefnrlines_ > 0 ? prefnrlines_
						  : uiListBox::cDefNrLines(); }

    ObjectSet<uiListBoxItem> items_;
    BoolTypeSet		itemmarked_;
    int			fieldwidth_;
    int			prefnrlines_;

protected:

    void		mousePressEvent(QMouseEvent*) override;
    void		mouseMoveEvent(QMouseEvent*) override;
    void		mouseReleaseEvent(QMouseEvent*) override;
    void		keyPressEvent(QKeyEvent*) override;

    int			itemIdxAtEvPos(QMouseEvent&) const;
    void		handleSlideChange(int,bool);

private:

    i_listMessenger&	messenger_;
    uiStringSet		itemstrings_;
    Interval<int>	sliderg_;
    uiListBox*		lb_;
};


static bool doslidesel_ = true;


uiListBoxBody::uiListBoxBody( uiListBoxObj& hndle, uiParent* p,
			const char* nm, OD::ChoiceMode cm,
			int prefnrlines, int preffieldwidth )
    : uiObjBodyImpl<uiListBoxObj,QListWidget>( hndle, p, nm )
    , messenger_(*new i_listMessenger(this,(uiListBox*)p))
    , fieldwidth_(preffieldwidth)
    , prefnrlines_(prefnrlines)
    , sliderg_(-1,-1)
    , lb_((uiListBox*)p)
{
    setObjectName( nm );
    setEditTriggers( QAbstractItemView::NoEditTriggers );
    setDragDropMode( QAbstractItemView::NoDragDrop );
    setAcceptDrops( false ); setDragEnabled( false );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    setSelectionMode( cm == OD::ChooseNone ? QAbstractItemView::NoSelection
					: QAbstractItemView::SingleSelection );
    setAutoScroll( true );

    setStretch( 2, (nrTxtLines()== 1) ? 0 : 2 );
    setHSzPol( uiObject::Medium );

    setMouseTracking( true );

    setStyleSheet( "selection-background-color: lightblue;"
		   "selection-color: black" );
}


void uiListBoxBody::addItem( const uiString& txt, bool mark, int id )
{
    QString qs = toQString( txt );
    uiListBoxItem* itm = new uiListBoxItem( qs );
    itm->id_ = id;
    items_ += itm;
    itemstrings_ += txt;
    itemmarked_ += mark;
    QListWidget::addItem( itm );
}


void uiListBoxBody::insertItem( int idx, const uiString& txt, bool mark, int id)
{
    QString qs = toQString( txt );
    uiListBoxItem* itm = new uiListBoxItem( qs );
    itm->id_ = id;
    items_.insertAt( itm, idx );
    itemstrings_.insert( idx, txt );
    itemmarked_.insert( idx, mark );
    QListWidget::insertItem( idx, itm );
}


void uiListBoxBody::updateText( int idx )
{
    QString qs = toQString( itemstrings_[idx] );
    item(idx)->setText( qs );
}


void uiListBoxBody::removeAll()
{
    QListWidget::clear();
    items_.erase();
    itemstrings_.setEmpty();
    itemmarked_.setEmpty();
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


void uiListBoxBody::setPixmap( int idx, const uiPixmap& pm, bool placeright )
{
    if ( placeright )
	setItemDelegateForRow( idx, new ListBoxPixmapDelegate(this) );

    item(idx)->setIcon( *pm.qpixmap() );
}


void uiListBoxBody::removePixmap( int idx )
{
    item(idx)->setIcon( QIcon() );
}


bool uiListBoxBody::hasPixmap( int idx )
{
    return item(idx)->icon().isNull();
}


void uiListBoxBody::setItemAlignment( int idx, Alignment::HPos hpos )
{
    Alignment al( hpos, Alignment::VCenter );
    if ( item(idx) )
	item(idx)->setTextAlignment( (Qt::Alignment)al.uiValue() );
}


uiSize uiListBoxBody::minimumSize() const
{
    const int totHeight = fontHeight() * prefnrlines_;
    const int totWidth  = fontWidth( true ) * fieldwidth_;
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


static bool isShiftPressed( QInputEvent& ev )
{
    const Qt::KeyboardModifiers modif = ev.modifiers();
    return modif == Qt::ShiftModifier;
}


void uiListBoxBody::handleSlideChange( int newstop, bool isclear )
{
    sliderg_.include( newstop, false );

    Interval<int> rg = sliderg_;
    rg.sort();
    if ( rg.start<0 || rg.stop<0 )
	return;

    for ( int idx=rg.start; idx<=rg.stop; idx++ )
	item(idx)->setCheckState( !isclear ? Qt::Checked : Qt::Unchecked );
    lb_->updateCheckState();
    lb_->itemChosen.trigger( -1 );
}


void uiListBoxBody::mousePressEvent( QMouseEvent* ev )
{
    if ( ev && ev->button() == Qt::LeftButton && lb_->isMultiChoice() )
    {
	lb_->bulkcheckchg_ = true;
	if ( doslidesel_ )
	    sliderg_.start = sliderg_.stop = itemIdxAtEvPos( *ev );
	else
	    sliderg_.start = -1;

	const bool isshift = isShiftPressed(*ev);
	const bool isctrl = isCtrlPressed(*ev);
	if ( isshift || isctrl )
	{
	    sliderg_.start = currentRow();
	    sliderg_.stop = itemIdxAtEvPos( *ev );
	    sliderg_.sort();
	    handleSlideChange( sliderg_.stop, isctrl );
	}
    }

    QListWidget::mousePressEvent( ev );
}


void uiListBoxBody::mouseMoveEvent( QMouseEvent* ev )
{
    if ( ev && sliderg_.start >= 0 )
    {
	const int newstop = itemIdxAtEvPos( *ev );
	if ( newstop != sliderg_.stop )
	    handleSlideChange( newstop, isCtrlPressed(*ev) );
	sliderg_.start = sliderg_.stop = newstop;
    }

    QListWidget::mouseMoveEvent( ev );
}


void uiListBoxBody::mouseReleaseEvent( QMouseEvent* ev )
{
    lb_->bulkcheckchg_ = false;
    const int lbidx = itemIdxAtEvPos( *ev );

    const bool didslide = sliderg_.start>=0 && sliderg_.start != lbidx;
    if ( didslide )
    {
	handleSlideChange( lbidx, isCtrlPressed(*ev) );
	sliderg_.start = -1;
	const int refnr = lb_->beginCmdRecEvent( "selectionChanged" );
	lb_->selectionChanged.trigger();
	lb_->endCmdRecEvent( refnr, "selectionChanged" );

	lb_->buttonstate_ = OD::NoButton;
	if ( ev ) ev->accept();
	return;
    }
    sliderg_.start = -1;

    if ( !ev ) return;

    if ( ev->button() == Qt::RightButton )
	lb_->buttonstate_ = OD::RightButton;
    else if ( ev->button() == Qt::LeftButton )
	lb_->buttonstate_ = OD::LeftButton;
    else
	lb_->buttonstate_ = OD::NoButton;

    QListWidget::mouseReleaseEvent( ev );
    lb_->buttonstate_ = OD::NoButton;
}


void uiListBoxBody::keyPressEvent( QKeyEvent* qkeyev )
{
    if ( qkeyev )
    {
	if ( qkeyev->key() == Qt::Key_Delete )
	    lb_->deleteButtonPressed.trigger();
	else if ( isCtrlPressed(*qkeyev) )
	{
	    if ( qkeyev->key() == Qt::Key_A )
		lb_->usrChooseAll( true );
	    if ( qkeyev->key() == Qt::Key_Z )
		lb_->usrChooseAll( false );
	    if ( qkeyev->key() == Qt::Key_M )
		lb_->menuCB( 0 );
	}

	lb_->updateCheckState();
    }

    QListWidget::keyPressEvent( qkeyev );
}


// uiListBoxObj
uiListBoxObj::uiListBoxObj( uiParent* p, const char* nm, OD::ChoiceMode cm )
    : uiObject(p,nm,mkbody(p,nm,cm))
{
}

uiListBoxObj::~uiListBoxObj()
{ delete body_; }

uiListBoxBody& uiListBoxObj::mkbody( uiParent* p, const char* nm,
				     OD::ChoiceMode cm )
{
    body_ = new uiListBoxBody( *this, p, nm, cm );
    body_->setIconSize( QSize(cIconSz,cIconSz) );
    return *body_;
}


// uiListBox

#define mStdInit(cm) \
    , selectionChanged(this) \
    , itemChosen(this) \
    , doubleClicked(this) \
    , rightButtonClicked(this) \
    , leftButtonClicked(this) \
    , deleteButtonPressed(this) \
    , checkAllClicked(this) \
    , choicemode_(cm) \
    , rightclickmnu_(*new uiMenu(p))

#define mStdConstrEnd \
    lb_->setBackgroundColor( lb_->roBackgroundColor() ); \
    rightButtonClicked.notify( mCB(this,uiListBox,menuCB) )


uiListBox::uiListBox( uiParent* p, const char* nm, OD::ChoiceMode cm )
    : uiGroup(p,nm)
    mStdInit(cm)
{
    lb_ = new uiListBoxObj( this, nm, choicemode_ );
    mkCheckGroup();
    setHAlignObj( lb_ );

    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const Setup& setup, const char* nm )
    : uiGroup(p,nm)
    mStdInit(setup.cm_)
{
    lb_ = new uiListBoxObj( this, nm, choicemode_ );
    lb_->body().setNrLines( setup.prefnrlines_ );
    lb_->body().fieldwidth_ = setup.prefwidth_;

    mkCheckGroup();
    mkLabel( setup.lbl_, setup.lblpos_ );
    setHAlignObj( lb_ );

    mStdConstrEnd;
}


uiListBox::~uiListBox()
{
    delete &rightclickmnu_;
}


#define mListBoxBlockCmdRec	CmdRecStopper cmdrecstopper( lb_ );


void uiListBox::setLabelText( const uiString& txt, int nr )
{
    if ( nr >= lbls_.size() ) return;
    lbls_[nr]->setText( txt );
}


void uiListBox::addLabel( const uiString& txt, LblPos pos )
{
    mkLabel( txt, pos );
}


void uiListBox::mkCheckGroup()
{
    checkgrp_ = new uiGroup( this, "CheckGroup" );
    checkgrp_->attach( alignedAbove, lb_ );

    uiPushButton* pb = new uiPushButton( checkgrp_, uiStrings::sEmptyString(),
					 mCB(this,uiListBox,menuCB), true );
    pb->setName( "Selection menu" );
    pb->setIcon( "menu-arrow" );
    pb->setPrefWidth( 40 );
    pb->setMaximumWidth( 40 );
    pb->setFlat( true );
#ifdef __win__
    pb->setStyleSheet( ":pressed { background: transparent; }" );
#else
    pb->setStyleSheet( ":pressed { border: 0; background: transparent }" );
#endif
    cb_ = new uiCheckBox( checkgrp_, uiStrings::sEmptyString(),
			  mCB(this,uiListBox,checkCB) );
    cb_->setName( "Check-all box" );
    cb_->setMaximumWidth( 20 );
    mAttachCB( cb_->activated, uiListBox::checkAllClickedCB );

    openbut_ = new uiToolButton( checkgrp_, "open",
			tr("Read selection"), mCB(this,uiListBox,retrieveCB) );
    openbut_->setPrefHeight( cIconSz );
    openbut_->setPrefWidth( cIconSz );
    openbut_->attach( centeredRightOf, pb, 1 );
    openbut_->display( false );

    savebut_ = new uiToolButton( checkgrp_, "save",
			tr("Save selection"), mCB(this,uiListBox,saveCB) );
    savebut_->setPrefHeight( cIconSz );
    savebut_->setPrefWidth( cIconSz );
    savebut_->attach( rightTo, openbut_, 2 );
    savebut_->display( false );

    checkgrp_->display( isMultiChoice(), true );
}


void uiListBox::updateReadWriteButtonState()
{
    openbut_->setSensitive( !isEmpty() );
    savebut_->setSensitive( nrChecked() > 0 );
}


void uiListBox::offerReadWriteSelection( const CallBack& rcb,
					 const CallBack& wcb )
{
    opencb_ = rcb;
    savecb_ = wcb;
    openbut_->display( opencb_.willCall() );
    savebut_->display( savecb_.willCall() );
    updateReadWriteButtonState();
}


void uiListBox::retrieveCB( CallBacker* )
{
    const bool needretrieve = opencb_.willCall();
    if ( needretrieve )
	opencb_.doCall( this );
}


void uiListBox::saveCB( CallBacker* )
{
    const bool needsave = savecb_.willCall();
    if ( needsave )
	savecb_.doCall( this );
}


void uiListBox::checkCB( CallBacker* )
{
    const bool checkall = cb_->getCheckState()==OD::Checked;
    setAllItemsChecked( checkall );
    updateReadWriteButtonState();
}


void uiListBox::checkAllClickedCB( CallBacker* )
{
    checkAllClicked.trigger();
}


void uiListBox::updateCheckState()
{
    NotifyStopper ns( cb_->activated );
    CmdRecStopper cmdrecstopper( cb_ );

    const int nrchecked = nrChecked();
    if ( nrchecked==0 )
	cb_->setCheckState( OD::Unchecked );
    else if ( nrchecked==size() )
	cb_->setCheckState( OD::Checked );
    else
	cb_->setCheckState( OD::PartiallyChecked );

    updateReadWriteButtonState();
}


void uiListBox::mkLabel( const uiString& txt, LblPos pos )
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

    uiLabel* labl = new uiLabel( this, mToUiStringTodo(ptr) );
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
    if( isMultiChoice() )
    {
	labl->attach( rightOf, checkgrp_ );
	break;
    }
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
	uiLabel* cur = new uiLabel( this, mToUiStringTodo((last1st
		  ? txts[nrleft-1]->buf() : txts[txts.size()-nrleft]->buf())) );
	cur->attach( lblct, labl );
	lbls_ += cur;
	labl = cur;
	nrleft--;
    }

    txts.setEmpty();
}


void uiListBox::setChoiceMode( OD::ChoiceMode cm )
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

    if ( choicemode_ == OD::ChooseOnlyOne )
	choicemode_ = OD::ChooseAtLeastOne;
    else
	choicemode_ = OD::ChooseOnlyOne;

    updateFields2ChoiceMode();
}


void uiListBox::setAllowNoneChosen( bool yn )
{
    if ( choicemode_ == OD::ChooseOnlyOne )
	return;
    const bool nowallowed = choicemode_ == OD::ChooseZeroOrMore;
    if ( nowallowed == yn )
	return;

    choicemode_ = yn ? OD::ChooseZeroOrMore : OD::ChooseAtLeastOne;
    updateFields2ChoiceMode();
}


void uiListBox::setNotSelectable()
{
    choicemode_ = OD::ChooseNone;
    updateFields2ChoiceMode();
    lb_->body().setSelectionMode( QAbstractItemView::NoSelection );
}


#define mSetChecked(idx,yn) \
{ \
    QListWidgetItem* item = lb_->body().item( idx ); \
    if ( item ) \
	item->setCheckState( yn ? Qt::Checked : Qt::Unchecked ); \
}


void uiListBox::updateFields2ChoiceMode()
{
    const bool hascheck = isMultiChoice();
    for ( int idx=0; idx<size(); idx++ )
    {
	setItemCheckable( idx, hascheck );
	if ( hascheck )
	    mSetChecked( idx, false );
    }

    checkgrp_->display( hascheck );
}


void uiListBox::menuCB( CallBacker* )
{
    rightclickmnu_.clear();
    const int sz = size();
    if ( sz < 1 || !isMultiChoice() )
	return;

    const int nrchecked = nrChecked();
    if ( nrchecked < sz )
	rightclickmnu_.insertAction( new uiAction(tr("Check all (Ctrl-A)")), 0);
    if ( nrchecked > 0 )
	rightclickmnu_.insertAction(new uiAction(tr("Uncheck all (Ctrl-Z)")),1);
    rightclickmnu_.insertAction( new uiAction(tr("Invert selection")), 2 );

    if ( nrchecked > 0 )
    {
	rightclickmnu_.insertSeparator();
	if ( allshown_ )
	    rightclickmnu_.insertAction(
		    new uiAction(tr("Show checked only")), 5 );
	else
	    rightclickmnu_.insertAction(new uiAction(tr("Show all")), 6);
    }

    const bool needretrieve = opencb_.willCall();
    const bool needsave = savecb_.willCall() && nrchecked > 0;
    if ( needretrieve || needsave )
    {
	rightclickmnu_.insertSeparator();
	if ( needretrieve )
	    rightclickmnu_.insertAction( new uiAction(tr("Read selection")), 3);
	if ( needsave )
	    rightclickmnu_.insertAction( new uiAction(tr("Save selection")), 4);
    }

    const int res = rightclickmnu_.exec();
    if ( res==0 || res==1 )
	usrChooseAll( res==0 );
    else if ( res == 2 )
    {
	bulkcheckchg_ = true;
	const int selidx = currentItem();
	TypeSet<int> checked; getCheckedItems( checked );
	for ( int idx=0; idx<sz; idx++ )
	    setItemChecked( idx, !checked.isPresent(idx) );

	bulkcheckchg_ = false;
	setCurrentItem( selidx );
    }
    else if ( res == 3 )
	opencb_.doCall( this );
    else if ( res == 4 )
	savecb_.doCall( this );
    else if ( res == 5 || res == 6 )
    {
	allshown_ = !allshown_;
	const int selidx = currentItem();
	TypeSet<int> checked; getCheckedItems( checked );
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( allshown_ )
		displayItem( idx, true );
	    else
		displayItem( idx, checked.isPresent( idx ) );
	}
	setCurrentItem( selidx );
    }

    updateCheckState();
}


void uiListBox::handleCheckChange( QListWidgetItem* itm )
{
    mDynamicCastGet(uiListBoxItem*,lbitm,itm)
    if ( !lbitm ) return;

    const bool ischecked = itm->checkState() == 2;
    if ( lbitm->ischecked_ == ischecked )
	return;

    lbitm->ischecked_ = ischecked;
    if ( bulkcheckchg_ )
	return;

    const int itmidx = lb_->body().indexOf( lbitm );
    NotifyStopper nsic( itemChosen );
    mListBoxBlockCmdRec;
    lb_->body().setCurrentRow( itmidx );
    nsic.enableNotification();

    itemChosen.trigger( itmidx );
    updateCheckState();
}


void uiListBox::setNrLines( int prefnrlines )
{
    lb_->body().setNrLines( prefnrlines );
}


int uiListBox::maxNrOfChoices() const
{
    return isMultiChoice() ? size() : (choicemode_ == OD::ChooseNone ? 0 : 1);
}


int uiListBox::size() const
{
    return lb_->body().count();
}


bool uiListBox::validIdx( int idx ) const
{
    return lb_->body().validItemIndex( idx );
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

    lb_->body().setItemAlignment( newidx, alignment_ );
}


void uiListBox::addItem( const uiString& text, bool mark, int id )
{
    if ( !allowduplicates_ && isPresent( text.getFullString() ) )
	return;

    mListBoxBlockCmdRec;
    lb_->body().addItem( text, mark, id );
    const int newidx = size() - 1;
    setMarked( newidx, mark );
    lb_->body().setCurrentRow( newidx );
    initNewItem( newidx );
}


void uiListBox::addItem( const uiString& text, const uiPixmap& pm, int id )
{
    addItem( text, false, id );
    setPixmap( size()-1, pm );
}


void uiListBox::addItem( const uiString& text, const OD::Color& col, int id )
{
    uiPixmap pm( 64, 64);
    pm.fill( col );
    addItem( text, pm, id );
}


void uiListBox::addItemNoUpdate( const uiString& text, bool mark, int id )
{
    if ( !allowduplicates_ && isPresent(text.getFullString()) )
	return;

    mListBoxBlockCmdRec;
    lb_->body().addItem( text, mark, id );
    const int newidx = size() - 1;
    initNewItem( newidx );
    setMarked( newidx, mark );
}


void uiListBox::addItems( const char** textList )
{
    int curidx = currentItem();
    const char** pt_cur = textList;
    while ( *pt_cur )
	addItemNoUpdate( toUiString(*pt_cur++) );

    if ( choicemode_ != OD::ChooseNone && curidx < 0 )
	curidx = 0;

    setCurrentItem( curidx );
    updateReadWriteButtonState();
}


void uiListBox::addItems( const BufferStringSet& strs )
{
    int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	addItemNoUpdate( toUiString(strs.get(idx)) );

    if ( choicemode_ != OD::ChooseNone && curidx < 0 )
	curidx = 0;

    setCurrentItem( curidx );
    updateReadWriteButtonState();
}


void uiListBox::addItems( const uiStringSet& strs )
{
    int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	addItemNoUpdate( strs[idx] );

    if ( choicemode_ != OD::ChooseNone && curidx < 0 )
	curidx = 0;

    setCurrentItem( curidx );
    updateReadWriteButtonState();
}


void uiListBox::insertItem( const uiString& text, int index, bool mark, int id )
{
    mListBoxBlockCmdRec;
    if ( index<0 )
	addItem( text, mark );
    else
    {
	if ( !allowduplicates_ && isPresent( text.getFullString() ) )
	    return;

	lb_->body().insertItem( index, text, mark, id );
	initNewItem( index<0 ? 0 : index );
	setMarked( index, mark );
    }
}


void uiListBox::insertItem( const uiString& text, const uiPixmap& pm,
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


void uiListBox::insertItem( const uiString& text, const OD::Color& col,
			    int index, int id )
{
    uiPixmap pm( 64, 64 );
    pm.fill( col );
    insertItem( text, pm, index, id );
}


void uiListBox::displayItem( int index, bool yn )
{
    if ( index < 0 || index >= size() )
	return;

    QListWidgetItem* itm = lb_->body().item( index );
    itm->setHidden( !yn );
}


void uiListBox::setItemSelectable( int index, bool yn )
{
    if ( index < 0 || index >= size() )
	return;

    QListWidgetItem* itm = lb_->body().item( index );
    Qt::ItemFlags flags = itm->flags();
    bool issel = flags.testFlag( Qt::ItemIsEnabled );
    if ( issel == yn )
	return;

    if ( yn )
	flags &= Qt::ItemIsEnabled;
    else
	flags ^= Qt::ItemIsEnabled;

    itm->setFlags( flags );
}


void uiListBox::setPixmap( int index, const OD::Color& col,
			   DecorationPos decpos )
{
    if ( index<0 || index>=size() || !lb_->body().item(index) )
	return;

    const QSize sz = lb_->body().iconSize();
    uiPixmap pm( sz.width(), sz.height() );
    pm.fill( col );
    setPixmap( index, pm, decpos );
}


void uiListBox::setPixmap( int index, const uiPixmap& pm,
			   DecorationPos decpos )
{
    if ( index<0 || index>=size() ||
	 !lb_->body().item(index) || !pm.qpixmap() ) return;

    lb_->body().setPixmap( index, pm, decpos==Right );
}


void uiListBox::setIcon( int index, const char* iconnm )
{
    if ( index<0 || index>=lb_->body().count() )
	return;

    uiIcon icon( iconnm );
    lb_->body().item(index)->setIcon( icon.qicon() );
}



void uiListBox::setColor( int index, const OD::Color& col )
{
    QColor qcol( col.r(), col.g(), col.b() );
    QListWidgetItem* itm = lb_->body().item( index );
    if ( itm )
	itm->setBackground( qcol );
}


void uiListBox::setDefaultColor( int index )
{
    QListWidgetItem* itm = lb_->body().item( index );
    if ( itm )
	itm->setBackground( QBrush() );
}


OD::Color uiListBox::getColor( int index ) const
{
    QListWidgetItem* itm = lb_->body().item( index );
    if ( !itm )
	return OD::Color(255,255,255);

    const QColor qcol = itm->background().color();
    return OD::Color( qcol.red(), qcol.green(), qcol.blue() );
}


void uiListBox::setEmpty()
{
    mListBoxBlockCmdRec;
    lb_->body().removeAll();
    updateCheckState();
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
    setEmpty();
    addItems( nms );
    for ( int idx=0; idx<sz; idx++ )
    {
	const int newidx = sortidxs[idx];
	setMarked( newidx, mrkd[idx] );
	setChosen( newidx, chosen[idx] );
    }

    delete [] sortidxs;
    if ( !cur.isEmpty() )
	setCurrentItem( cur.buf() );
}


void uiListBox::removeItem( const char* txt )
{
    removeItem( indexOf(txt) );
}


void uiListBox::removeItem( int idx )
{
    mListBoxBlockCmdRec;
    lb_->body().removeItem( idx );
    updateCheckState();
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString itmtxt;
	if ( isMarked(idx) )
	    itmtxt = getUnmarkedText( idx );
	else
	    itmtxt = lb_->body().item(idx)->text();

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

    if ( isMarked(idx) )
    {
	rettxt_ = getUnmarkedText( idx );
	return rettxt_;
    }

    rettxt_ = lb_->body().getItemText(idx).getFullString();
    return rettxt_;
}


bool uiListBox::isMarked( int idx ) const
{
    return lb_->body().isItemMarked( idx );
}


void uiListBox::setMarked( int idx, DecorationType markdec )
{
    if ( !validIdx(idx) )
	return;

    BufferString texttobemarked = getUnmarkedText( idx );
    if ( markdec == uiListBox::None )
    {
	removePixmap( idx );
	setMarked( idx, false );
	setItemText( idx, texttobemarked );
	return;
    }
    else if ( markdec == Pixmap )
    {
	uiPixmap pm( "star" );
	setPixmap( idx, pm, Right );
    }
    else if ( markdec == Star )
    {
	removePixmap( idx );
	texttobemarked += " *";
	setItemText( idx, texttobemarked );
    }
    else if ( markdec == Legacy )
    {
	removePixmap( idx );
	BufferString text = "> ";
	text += textOfItem( idx );
	text += " <";
	setItemText( idx, text );
    }
    else if ( markdec == Invisible )
	setItemText( idx, texttobemarked );

    setMarked( idx, true );
}


void uiListBox::setMarked( int idx, bool yn )
{
    lb_->body().setItemMarked( idx, yn );
    lb_->body().updateText( idx );
}


BufferString uiListBox::getUnmarkedText( int idx ) const
{
    if ( !validIdx(idx) )
	return "";

    BufferString starchar = "*";
    BufferString legacychar = "><";
    BufferString text = lb_->body().getItemText(idx).getFullString();
    if ( text.last() == starchar.last() )
    {
	text.remove( starchar[0] );
	text.trimBlanks();
    }
    else if ( text.last() == legacychar.last()
	      && text.first() == legacychar.first() )
    {
	text.remove( legacychar.first() );
	text.remove( legacychar.last() );
	text.trimBlanks();
    }

    return text;
}


uiListBox::DecorationType uiListBox::getDecorationType( int idx ) const
{
    if ( !isMarked(idx) )
	return None;

    DecorationType dec = Pixmap;
    const char starchar = '*';
    const char defchar0 = '>';
    const char defchar1 = '<';
    BufferString markedtext = lb_->body().getItemText(idx).getFullString();
    if ( markedtext.last() == starchar )
	dec = Star;
    else if ( markedtext.last()==defchar0 && markedtext.first()==defchar1 )
	dec = Legacy;
    else if ( lb_->body().hasPixmap(idx) )
	dec = Invisible;

    return dec;
}


void uiListBox::removePixmap( int idx )
{
    lb_->body().removePixmap( idx );
}


int uiListBox::currentItem() const
{
    return lb_->body().currentRow();
}


void uiListBox::setCurrentItem( const char* txt )
{
    if ( !txt )
	return;

    const int sz = lb_->body().count();
    for ( int idx=0; idx<sz; idx++ )
    {
	const char* ptr = textOfItem( idx );
	mSkipBlanks(ptr);
	if ( StringView(ptr) == txt )
	{
	    setCurrentItem( idx );
	    return;
	}
    }
}


void uiListBox::setCurrentItem( int idx )
{
    if ( !validIdx(idx) )
    {
	const int curidx = lb_->body().currentRow();
	if ( validIdx(curidx) )
	    lb_->body().item( curidx )->setSelected( false );

	lb_->body().setCurrentRow( -1 );
	return;
    }

    mListBoxBlockCmdRec;

    const int curitmidx = currentItem();
    lb_->body().setCurrentRow( idx );
    if ( curitmidx == currentItem() )
	selectionChanged.trigger(); // Qt won't trigger
    if ( choicemode_ == OD::ChooseOnlyOne )
	lb_->body().item( idx )->setSelected( true );

    lb_->body().scrollToItem( lb_->body().item(idx),
			      QAbstractItemView::PositionAtCenter );
}


int uiListBox::indexOf( const char* txt ) const
{
    const StringView str( txt );
    for ( int idx=0; idx<size(); idx++ )
	if ( str == textOfItem(idx) )
	    return idx;

    return -1;
}


void uiListBox::setItemID( int idx, int id )
{
    lb_->body().setItemID( idx, id );
}


int uiListBox::currentItemID() const
{
    return getItemID( currentItem() );
}


int uiListBox::getItemID( int idx ) const
{
    return lb_->body().getItemID( idx );
}


int uiListBox::getItemIdx( int id ) const
{
    return lb_->body().getItemIdx( id );
}


void uiListBox::setItemText( int idx, const uiString& txt )
{
    if ( !validIdx(idx) )
	return;

    lb_->body().getItemText( idx ) = txt;
    lb_->body().updateText( idx );
    if ( !isMarked(idx) )
	return;

    setMarked( idx, getDecorationType(idx) );
}


void uiListBox::setFieldWidth( int fw )
{
    lb_->body().fieldwidth_ = fw;
}


void uiListBox::setHSzPol( uiObject::SzPolicy szpol )
{
    lb_->setHSzPol( szpol );
}


void uiListBox::setVSzPol( uiObject::SzPolicy szpol )
{
    lb_->setVSzPol( szpol );
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
	lb_->body().setItemAlignment( idx, al );
}


bool uiListBox::handleLongTabletPress()
{
    BufferString msg = "rightButtonClicked ";
    msg += currentItem();
    const int refnr = lb_->beginCmdRecEvent( msg );
    rightButtonClicked.trigger();
    lb_->endCmdRecEvent( refnr, msg );
    return true;
}


void uiListBox::disableRightClick( bool yn )
{
    if ( yn )
	rightButtonClicked.remove( mCB(this,uiListBox,menuCB) );
    else
	rightButtonClicked.notify( mCB(this,uiListBox,menuCB) );
}


void uiListBox::scrollToTop()
{ lb_->body().scrollToTop(); }

void uiListBox::scrollToBottom()
{ lb_->body().scrollToBottom(); }


void uiListBox::translateText()
{
    uiGroup::translateText();

    for ( int idx=0; idx<size(); idx++ )
	lb_->body().updateText( idx );
}


//---- 'Choose' functions ----

void uiListBox::setChoosable( int idx, bool yn )
{
    if ( !validIdx(idx) ) return;

    Qt::ItemFlags flags = lb_->body().item(idx)->flags();
    const bool isselectable = flags.testFlag( Qt::ItemIsSelectable );
    if ( isselectable == yn )
	return;

    lb_->body().item(idx)->setFlags( flags^Qt::ItemIsSelectable );
}


bool uiListBox::isChoosable( int idx ) const
{
    return validIdx(idx) &&
	lb_->body().item(idx)->flags().testFlag( Qt::ItemIsSelectable );
}


int uiListBox::nrChosen() const
{
    if ( !isMultiChoice() )
	return choicemode_ == OD::ChooseNone || currentItem() < 0 ? 0 : 1;

    int ret = nrChecked();
    if ( ret < 1 && choicemode_ == OD::ChooseAtLeastOne )
	ret = currentItem() < 0 ? 0 : 1;
    return ret;
}


bool uiListBox::isChosen( int lidx ) const
{
    if ( choicemode_ == OD::ChooseNone || !validIdx(lidx) )
	return false;
    else if ( choicemode_ == OD::ChooseOnlyOne )
	return lidx == currentItem();

    if ( isItemChecked(lidx) )
	return true;
    if ( choicemode_ == OD::ChooseZeroOrMore || nrChecked() > 0 )
	return false;

    return lidx == currentItem();
}


int uiListBox::firstChosen() const
{
    if ( !isMultiChoice() )
	return choicemode_ == OD::ChooseNone ? -1 : currentItem();

    return nextChosen( -1 );
}


int uiListBox::nextChosen( int prev ) const
{
    if ( !isMultiChoice() )
	return choicemode_ == OD::ChooseNone || prev >= 0 ? -1 : currentItem();

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
	if ( choicemode_ == OD::ChooseOnlyOne && rg.start >= 0 )
	    setCurrentItem( rg.start );
    }
    else
    {
	rg.sort();
	for ( int idx=rg.start; idx<=rg.stop; idx++ )
	    setItemChecked( idx, yn );
	setCurrentItem( rg.start );
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
	setCurrentItem( nms.get(0).buf() );
}


void uiListBox::usrChooseAll( bool yn )
{
    const int refnr = lb_->beginCmdRecEvent( "selectionChanged" );
    chooseAll( yn );
    lb_->endCmdRecEvent( refnr, "selectionChanged" );
}


void uiListBox::chooseAll( bool yn )
{
    if ( isMultiChoice() )
	setAllItemsChecked( yn );

    updateCheckState();
}


//---- 'Check' functions ----


void uiListBox::setItemCheckable( int idx, bool yn )
{
    if ( !validIdx(idx) )
	return;

    Qt::ItemFlags flags = lb_->body().item(idx)->flags();
    flags.setFlag( Qt::ItemIsUserTristate, false );
    flags.setFlag( Qt::ItemIsUserCheckable, yn );
    lb_->body().item(idx)->setFlags( flags );
}


void uiListBox::setItemChecked( int idx, bool yn )
{
    if ( isMultiChoice() && yn != isItemChecked(idx) )
    {
	mListBoxBlockCmdRec;
	mSetChecked( idx, yn );
    }
}


bool uiListBox::isItemChecked( int idx ) const
{
    return isMultiChoice() && validIdx(idx) ?
	   lb_->body().item(idx)->checkState() == Qt::Checked : false;
}


void uiListBox::setItemChecked( const char* nm, bool yn )
{
    if ( !isMultiChoice() )
	return;

    const int idxof = indexOf( nm );
    if ( idxof >= 0 )
	setItemChecked( indexOf(nm), yn );
}


void uiListBox::setAllItemsChecked( bool yn )
{
    if ( !isMultiChoice() )
	return;

    int lastchg = -1;
    for ( int idx=lb_->body().items_.size()-1; idx>-1; idx-- )
    {
	uiListBoxItem* itm = lb_->body().items_[idx];
	if ( itm->ischecked_ != yn )
	    { lastchg = idx; break; }
    }
    if ( lastchg < 0 )
	return;

    const bool blockstate = lb_->body().blockSignals( true );
    for ( int idx=0; idx<=lastchg; idx++ )
    {
	uiListBoxItem* itm = lb_->body().items_[idx];
	if ( itm->ischecked_ != yn )
	{
	    itm->setCheckState( yn ? Qt::Checked : Qt::Unchecked );
	    itm->ischecked_ = yn;
	}
    }

    lb_->body().blockSignals( blockstate );
    selectionChanged.trigger();
    itemChosen.trigger( -1 );
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


void uiListBox::setDragEnabled( bool yn )
{
    lb_->body().setDragEnabled( yn );
}


bool uiListBox::dragEnabled() const
{
    return lb_->body().dragEnabled();
}


void uiListBox::resizeToContents( int minw, int maxw, int minh, int maxh )
{
    resizeWidthToContents( minw, maxw );
    resizeHeightToContents( minh, maxh );
}


static int sMinWidth = 300;
static int sMaxWidth = 600;
static int sMinHeight = 150;
static int sMaxHeight = 400;

void uiListBox::resizeWidthToContents( int minw, int maxw )
{
    const int nritems = size();
    if ( nritems==0 )
	return;

    if ( minw < 0 ) minw = sMinWidth;
    if ( maxw < 0 ) maxw = sMaxWidth;

    int prefwidth = minw;
    for ( int idx=0; idx<nritems; idx++ )
    {
	const uiString& itmtxt = lb_->body().getItemText(idx);
	prefwidth = mMAX( prefwidth, lb_->body().fontWidthFor(itmtxt) );
    }

    prefwidth += 30; // Seems to be necessary in all cases

    if ( isMultiChoice() )
	prefwidth += 20;

    QListWidgetItem* item0 = lb_->body().item(0);
    if ( item0 && !item0->icon().isNull() )
	prefwidth += lb_->body().iconSize().width();

    if ( prefwidth > maxw )
	prefwidth = maxw;
    lb_->setPrefWidth( prefwidth );
}


void uiListBox::resizeHeightToContents( int minh, int maxh )
{
    const int nritems = size();
    if ( nritems==0 )
	return;

    if ( minh < 0 ) minh = sMinHeight;
    if ( maxh < 0 ) maxh = sMaxHeight;

    int iconheight = 0;
    QListWidgetItem* item0 = lb_->body().item(0);
    if ( item0 && !item0->icon().isNull() )
	iconheight = lb_->body().iconSize().height() + 2;

    int prefheight = nritems * mMAX(lb_->body().fontHeight(),iconheight);
    if ( prefheight < minh )
	prefheight = minh;
    else if ( prefheight > maxh )
	prefheight = maxh;
    lb_->setPrefHeight( prefheight );
}


// Deprecated. Old uiListBox constructors
uiListBox::uiListBox( uiParent* p, const BufferStringSet& itms, const char* nm )
    : uiGroup(p,nm)
    mStdInit(OD::ChooseOnlyOne)
{
    lb_ = new uiListBoxObj( this, nm, choicemode_ );
    mkCheckGroup();
    setHAlignObj( lb_ );
    addItems( itms );

    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& itms,
				  const char* nm, OD::ChoiceMode cm,
				  int prefnrlines, int prefw )
    : uiGroup(p,nm)
    mStdInit(cm)
{
    lb_ = new uiListBoxObj( this, nm, choicemode_ );
    mkCheckGroup();
    setHAlignObj( lb_ );

    addItems( itms );
    setNrLines( prefnrlines );
    setFieldWidth( prefw );

    mStdConstrEnd;
}


uiListBox::uiListBox( uiParent* p, const uiStringSet& itms, const char* nm,
		      OD::ChoiceMode cm, int prefnrlines, int prefw )
    : uiGroup(p,nm)
    mStdInit(cm)
{
    lb_ = new uiListBoxObj( this, nm, choicemode_ );
    mkCheckGroup();
    setHAlignObj( lb_ );

    addItems( itms );
    setNrLines( prefnrlines );
    setFieldWidth( prefw );

    mStdConstrEnd;
}

// Deprecated-------------- uiLabeledListBox ----------------


uiLabeledListBox::uiLabeledListBox( uiParent* p, const uiString& txt )
    : uiListBox(p,Setup(OD::ChooseOnlyOne,txt),"Labeled listbox")
{
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const uiString& txt,
		OD::ChoiceMode cm, uiLabeledListBox::LblPos pos )
    : uiListBox(p,Setup(cm,txt,(uiListBox::LblPos)pos),"Labeled listbox")
{
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const BufferStringSet& s,
    const uiString& txt, OD::ChoiceMode cm, uiLabeledListBox::LblPos pos)
    : uiListBox(p,Setup(cm,txt,(uiListBox::LblPos)pos),"Labeled listbox")
{
    addItems( s );
}
