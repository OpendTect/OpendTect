/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilistbox.cc,v 1.98 2009-05-28 09:08:50 cvsjaap Exp $";

#include "uilistbox.h"

#include "uifont.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "bufstringset.h"
#include "color.h"
#include "pixmap.h"

#include "i_qlistbox.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>

#define mNoSelection QAbstractItemView::NoSelection
#define mExtended QAbstractItemView::ExtendedSelection
#define mSingle QAbstractItemView::SingleSelection


class uiListBoxBody : public uiObjBodyImpl<uiListBox,QListWidget>
{

public:

                        uiListBoxBody(uiListBox& handle, 
				  uiParent* parnt=0, 
				  const char* nm="uiListBoxBody",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBoxBody()		{ delete &messenger_; }

    int			maxSelectable() const;

    void		activateClick(int idx,bool leftclick,bool doubleclick);
    void		activateButton(int idx);
    void		activateSelect(const TypeSet<int>&);
    bool		event(QEvent*);

    void 		setLines( int prefNrLines, bool adaptVStretch )
			{ 
			    if( prefNrLines >= 0 ) prefnrlines_=prefNrLines;

			    if( adaptVStretch )
			    {
				int hs = stretch(true,true);
				setStretch( hs, (nrTxtLines()== 1) ? 0 : 2 );
			    }
			}

    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			    { return prefnrlines_ ? prefnrlines_ : 7; }

    int 		fieldwidth_;
    int 		prefnrlines_;

protected:
    void		mouseReleaseEvent(QMouseEvent*);
    void		keyPressEvent(QKeyEvent*);

    int			actidx_;
    bool		actleftclick_;
    bool		actdoubleclick_;
    const TypeSet<int>* actselset_;

private:

    i_listMessenger&    messenger_;

};


uiListBoxBody::uiListBoxBody( uiListBox& handle, uiParent* parnt, 
			const char* nm, bool ismultiselect,
			int preferrednrlines, int preferredfieldwidth )
    : uiObjBodyImpl<uiListBox,QListWidget>( handle, parnt, nm )
    , messenger_(*new i_listMessenger(this,&handle))
    , fieldwidth_(preferredfieldwidth)
    , prefnrlines_(preferrednrlines)
{
    setObjectName( nm );
    setDragDropMode( QAbstractItemView::NoDragDrop );
    setAcceptDrops( false ); setDragEnabled( false );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    if ( ismultiselect ) setSelectionMode( mExtended );

    setStretch( 2, (nrTxtLines()== 1) ? 0 : 2 );

    setHSzPol( uiObject::Medium );
}


uiSize uiListBoxBody::minimumsize() const
{ 
    const int totHeight = fontHgt() * prefnrlines_;
    const int totWidth  = fontWdt( true ) * fieldwidth_;
    return uiSize( totWidth, totHeight );
}


void uiListBoxBody::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !event ) return;

    if ( event->button() == Qt::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( event->button() == Qt::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    QListWidget::mouseReleaseEvent( event );
    handle_.buttonstate_ = OD::NoButton;
}


void uiListBoxBody::keyPressEvent( QKeyEvent* qkeyev )
{
    if ( qkeyev && qkeyev->key() == Qt::Key_Delete )
	handle_.deleteButtonPressed.trigger();

    QListWidget::keyPressEvent( qkeyev );
}


int uiListBoxBody::maxSelectable() const
{ 
    if ( selectionMode() == mNoSelection )	return 0;
    if ( selectionMode() == mSingle )		return 1;
    return count();
}


static const QEvent::Type sQEventActClick  = (QEvent::Type) (QEvent::User+0);
static const QEvent::Type sQEventActButton = (QEvent::Type) (QEvent::User+1);
static const QEvent::Type sQEventActSelect = (QEvent::Type) (QEvent::User+2);


void uiListBoxBody::activateClick( int idx, bool leftclick, bool doubleclick )
{
    actidx_ = idx;
    actleftclick_ = leftclick;
    actdoubleclick_ = doubleclick;
    QEvent* actevent = new QEvent( sQEventActClick );
    QApplication::postEvent( this, actevent );
}


void uiListBoxBody::activateButton( int idx )
{
    actidx_ = idx;
    QEvent* actevent = new QEvent( sQEventActButton );
    QApplication::postEvent( this, actevent );
}


void uiListBoxBody::activateSelect( const TypeSet<int>& selectset )
{
    actselset_ = &selectset;
    QEvent* actevent = new QEvent( sQEventActSelect );
    QApplication::postEvent( this, actevent );
}


#define mSetCurListItem() \
    if ( maxSelectable()>0 ) \
    { \
	handle_.selectionChanged.disable(); \
	clearSelection(); \
	handle_.selectionChanged.enable(); \
	setCurrentRow( actidx_ ); \
    }

bool uiListBoxBody::event( QEvent* ev )
{
    if ( ev->type() == sQEventActClick ) 
    {
	if ( actidx_>=0 && actidx_<count() )
	{
	    mSetCurListItem();
	    if ( actdoubleclick_ )
		handle_.doubleClicked.trigger();
	    else if ( actleftclick_ )
		handle_.leftButtonClicked.trigger();
	    else
		handle_.rightButtonClicked.trigger();
	}
    }
    else if ( ev->type() == sQEventActButton )
    {
	if ( handle_.isItemCheckable(actidx_) )
	{
	    handle_.setItemChecked( actidx_,
				    !handle_.isItemChecked(actidx_) );
	    mSetCurListItem();
	}
    }
    else if ( ev->type() == sQEventActSelect )
    {
	if ( maxSelectable()>0 && actselset_->size()<=maxSelectable() )
	{
	    handle_.selectionChanged.disable();
	    clearSelection();
	    for ( int idx=0; idx<actselset_->size(); idx++ )
	    {
		if ( (*actselset_)[idx]>=0 && (*actselset_)[idx]<count() )
		    item( (*actselset_)[idx] )->setSelected( true );
	    }
	    handle_.selectionChanged.enable();
	    handle_.selectionChanged.trigger();
	}
    }
    else
	return QListWidget::event( ev );

    handle_.activatedone.trigger();
    return true;
}


// -------------- uiListBox ---------------

#define mStdInit \
    , buttonstate_(OD::NoButton) \
    , selectionChanged(this) \
    , doubleClicked(this) \
    , rightButtonClicked(this) \
    , leftButtonClicked(this) \
    , deleteButtonPressed(this) \
    , activatedone(this) \
    , rightclickmnu_(*new uiPopupMenu(p)) \
    , itemscheckable_(false) \
    , curitemwaschecked_(false) \
    , oldnrselected_(0)


uiListBox::uiListBox( uiParent* p, const char* nm, bool ms, int nl, int pfw )
    : uiObject( p, nm, mkbody(p,nm,ms,nl,pfw) )
    mStdInit
{
    rightButtonClicked.notify( mCB(this,uiListBox,menuCB) );
}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& items,
		      const char* txt, bool ms, int nl, int pfw )
    : uiObject( p, txt, mkbody(p,txt,ms,nl,pfw))
    mStdInit
{
    addItems( items );
    setName( "Select Data" );
    rightButtonClicked.notify( mCB(this,uiListBox,menuCB) );
}


static const int sIconSz = 16;

uiListBoxBody& uiListBox::mkbody( uiParent* p, const char* nm, bool ms,
				  int nl, int pfw )
{
    body_ = new uiListBoxBody(*this,p,nm,ms,nl,pfw);
    body_->setIconSize( QSize(sIconSz,sIconSz) );
    return *body_;
}


uiListBox::~uiListBox()
{
    delete &rightclickmnu_;
}


void uiListBox::menuCB( CallBacker* )
{
    if ( !itemscheckable_ ) return;

    rightclickmnu_.clear();
    rightclickmnu_.insertItem( new uiMenuItem("Check all items"), 0 );
    rightclickmnu_.insertItem( new uiMenuItem("Uncheck all items"), 1 );
    const int res = rightclickmnu_.exec();
    setItemsChecked( res==0 );
}


void uiListBox::setLines( int prefnrlines, bool adaptvstretch )
{ body_->setLines( prefnrlines, adaptvstretch ); }

void uiListBox::setNotSelectable()
{ body_->setSelectionMode( mNoSelection ); }

void uiListBox::setMultiSelect( bool yn )
{ body_->setSelectionMode( yn ? mExtended : mSingle ); }

int uiListBox::maxSelectable() const
{ return body_->maxSelectable(); }

int uiListBox::size() const
{ return body_->count(); }

bool uiListBox::validIndex( int idx ) const
{ return idx>=0 && idx<body_->count(); }


bool uiListBox::isSelected ( int idx ) const
{
    if ( !validIndex(idx) ) return false;

    QListWidgetItem* itm = body_->item( idx );
    return itm ? itm->isSelected() : false;
}


int uiListBox::nrSelected() const
{
    int res = 0;
    for ( int idx=0; idx<size(); idx++ )
	{ if ( isSelected(idx) ) res++; }
    return res;
}


void uiListBox::setSelected( int idx, bool yn )
{
    if ( validIndex(idx) )
	body_->item( idx )->setSelected( yn );
}


void uiListBox::selectAll( bool yn )
{
    if ( yn && body_->selectionMode()!=mExtended ) return;

    if ( yn )
	body_->selectAll();
    else
	body_->clearSelection();
}


void uiListBox::createQString( QString& qs, const char* str, bool embed ) const
{
    if ( !str ) str = "";
    BufferString bs( str );
    if ( embed ) { bs = "["; bs += str; bs += "]"; }
    qs = bs.buf();
}


void uiListBox::addItem( const char* text, bool embed ) 
{
    QString qs;
    createQString( qs, text, embed );
    body_->addItem( qs );
    setItemCheckable( size()-1, false ); // Qt bug
    setItemCheckable( size()-1, itemscheckable_ );
}


void uiListBox::addItem( const char* text, const ioPixmap& pm )
{
    addItem( text, false );
    setPixmap( size()-1, pm );
}


void uiListBox::addItem( const char* text, const Color& col )
{
    ioPixmap pm( 64, 64); pm.fill( col );
    addItem( text, pm );
}


void uiListBox::addItems( const char** textList ) 
{
    int curidx = currentItem();
    const char* pt_cur = *textList;
    while ( pt_cur )
        addItem( pt_cur++ );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::addItems( const BufferStringSet& strs )
{
    int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
    {
	body_->addItem( QString(strs.get(idx)) );
	setItemCheckable( size()-1,  false ); // Qt bug
	setItemCheckable( size()-1, itemscheckable_ );
    }
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::insertItem( const char* text, int index, bool embed )
{
    QString qs;
    createQString( qs, text, embed );
    if ( index<0 )
	body_->addItem( qs );
    else
	body_->insertItem( index, qs );

    setItemCheckable( index<0 ? 0 : index, itemscheckable_ );
}


void uiListBox::insertItem( const char* text, const ioPixmap& pm, int index )
{
    if ( index < 0 )
	addItem( text, pm );
    else
    {
	insertItem( text, index, false );
	setPixmap( index, pm );
    }
}


void uiListBox::insertItem( const char* text, const Color& col, int index )
{
    ioPixmap pm( 64, 64 ); pm.fill( col );
    insertItem( text, pm, index );
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


void uiListBox::empty()
{
    body_->QListWidget::clear();
}


void uiListBox::clear()
{
    body_->clearSelection();
}


void uiListBox::sort( bool asc )
{
    if ( !asc )
	pErrMsg("Descending sort not possible");
    body_->setSortingEnabled( true );
}


void uiListBox::removeItem( int idx )
{
    delete body_->takeItem( idx );
}


int uiListBox::nextSelected( int prev ) const
{
    if ( prev<0 ) prev = -1;
    const int sz = size();
    for ( int idx=prev+1; idx<sz; idx++ )
    {
	if ( isSelected(idx) )
	    return idx;
    }

    return -1;
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
	BufferString itmtxt( body_->item(idx)->text().toAscii().data() );
	char* ptr = itmtxt.buf();
	mSkipBlanks( ptr );
	if ( !strcmp(txt,ptr) ) return true;
    }
    return false;
}


const char* uiListBox::textOfItem( int idx, bool disembed ) const
{
    if ( !validIndex(idx) )
	return "";

    rettxt = (const char*)body_->item(idx)->text().toAscii();
    if ( !disembed || rettxt[0] != '[' )
	return rettxt;

    const int sz = rettxt.size();
    if ( rettxt[sz-1] != ']' )
	return rettxt;

    rettxt[sz-1] = '\0';
    return ((const char*)rettxt) + 1;
}


bool uiListBox::isEmbedded( int idx ) const
{
    rettxt = (const char*)body_->item(idx)->text().toAscii();
    return rettxt.buf()[0] == '[' && rettxt.buf()[rettxt.size()-1] == ']';
}


int uiListBox::currentItem() const
{
    QListWidgetItem* itm1 = body_->currentItem();
    QListWidgetItem* itm2 = body_->item( body_->currentRow() );
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
	if ( !strcmp(ptr,txt) )
	    { setCurrentItem( idx ); return; }
    }
}


void uiListBox::setCurrentItem( int idx )
{
    if ( !validIndex(idx) )
	return;

    body_->setCurrentRow( idx );
    if ( body_->selectionMode() != mExtended )
	setSelected( idx );
}


int uiListBox::indexOf( const char* txt ) const
{
    BufferString str( txt );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( str == textOfItem(idx) )
	    return idx;
    }

    return -1;
}


void uiListBox::setItemsCheckable( bool yn )
{
    itemscheckable_ = yn;
    for ( int idx=0; idx<size(); idx++ )
	setItemCheckable( idx, yn );
}


void uiListBox::setItemCheckable( int idx, bool yn )
{
    if ( !validIndex(idx) ) return;

    Qt::ItemFlags flags = body_->item(idx)->flags();
    const bool ischeckable = flags.testFlag( Qt::ItemIsUserCheckable );
    if ( ischeckable == yn )
	return;

    body_->item(idx)->setFlags( flags^Qt::ItemIsUserCheckable );
    setItemChecked( idx, false );
}


bool uiListBox::isItemCheckable( int idx ) const
{
    return validIndex(idx) &&
	body_->item(idx)->flags().testFlag( Qt::ItemIsUserCheckable );
}


void uiListBox::setItemsChecked( bool yn )
{
    for ( int idx=0; idx<size(); idx++ )
	setItemChecked( idx, yn );
}


void uiListBox::setItemChecked( int idx, bool yn )
{
    if ( isItemCheckable(idx) )
	body_->item(idx)->setCheckState( yn ? Qt::Checked : Qt::Unchecked );
}


bool uiListBox::isItemChecked( int idx ) const
{
    return validIndex(idx) && body_->item(idx)->checkState()==Qt::Checked;
}


int uiListBox::nrChecked() const
{
    int res = 0;
    for ( int idx=0; idx<size(); idx++ )
	if ( isItemChecked(idx) )
	    res++;
    return res;
}


void uiListBox::setItemText( int idx, const char* txt )
{
    if ( validIndex(idx) )
	body_->item(idx)->setText( QString(txt) );
}


void uiListBox::setSelectedItems( const BufferStringSet& itms )
{
    body_->setCurrentRow( -1 );
    for ( int idx=0; idx<size(); idx++ )
	setSelected( idx, itms.indexOf(textOfItem(idx))>=0 );
}


void uiListBox::setSelectedItems( const TypeSet<int>& itms )
{
    body_->setCurrentRow( -1 );
    for ( int idx=0; idx<size(); idx++ )
	setSelected( idx, itms.indexOf(idx)>=0 );
}


void uiListBox::setCheckedItems( const BufferStringSet& itms )
{
    for ( int idx=0; idx<size(); idx++ )
	setItemChecked( idx, itms.indexOf(textOfItem(idx))>=0 );
}


void uiListBox::setCheckedItems( const TypeSet<int>& itms )
{
    for ( int idx=0; idx<size(); idx++ )
	setItemChecked( idx, itms.indexOf(idx)>=0 );
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


void uiListBox::getCheckedItems( BufferStringSet& items ) const
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isItemChecked(idx) ) items.add( textOfItem(idx) );
}


void uiListBox::getCheckedItems( TypeSet<int>& items ) const
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isItemChecked(idx) ) items += idx;
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


void uiListBox::activateClick( int idx, bool leftclick, bool doubleclick )
{ body_->activateClick( idx, leftclick, doubleclick ); }


void uiListBox::activateButton( int idx )
{ body_->activateButton( idx ); }


void uiListBox::activateSelect( const TypeSet<int>& selection )
{ body_->activateSelect( selection ); }


#define mTriggerIf( notifiername, notifier ) \
    if ( !strcmp(notifiername, #notifier) ) \
	notifier.trigger( this );

void uiListBox::notifyHandler( const char* notifiername )
{
    BufferString msg = notifiername; msg += " "; msg += oldnrselected_;
    if ( curitemwaschecked_ )
	msg += " CurItemWasChecked";
    curitemwaschecked_ = isItemChecked( currentItem() );

    const int refnr = beginCmdRecEvent( msg );
    mTriggerIf( notifiername, selectionChanged );
    mTriggerIf( notifiername, doubleClicked );
    mTriggerIf( notifiername, leftButtonClicked );
    mTriggerIf( notifiername, rightButtonClicked );
    endCmdRecEvent( refnr, msg );
}


// -------------- uiLabeledListBox ----------------
uiLabeledListBox::uiLabeledListBox( uiParent* p, const char* txt, bool multisel,
				    uiLabeledListBox::LblPos pos )
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, txt, multisel );
    mkRest( txt, pos );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const BufferStringSet& s,
				    const char* txt,
				    bool multisel, uiLabeledListBox::LblPos pos)
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, s, txt, multisel );
    mkRest( txt, pos );
}


void uiLabeledListBox::setLabelText( const char* txt, int nr )
{
    if ( nr >= lbls.size() ) return;
    lbls[nr]->setText( txt );
}


const char* uiLabeledListBox::labelText( int nr ) const
{
    if ( nr >= lbls.size() ) return "";
    return lbls[nr]->text();
}


void uiLabeledListBox::mkRest( const char* txt, uiLabeledListBox::LblPos pos )
{
    setHAlignObj( lb );

    BufferStringSet txts;
    BufferString s( txt );
    char* ptr = s.buf();
    if( !ptr || !*ptr ) return;
    while ( 1 )
    {
	char* nlptr = strchr( ptr, '\n' );
	if ( nlptr ) *nlptr = '\0';
	txts += new BufferString( ptr );
	if ( !nlptr ) break;

	ptr = nlptr + 1;
    }
    if ( txts.size() < 1 ) return;

    bool last1st = pos > RightTop && pos < BelowLeft;
    ptr = last1st ? txts[txts.size()-1]->buf() : txts[0]->buf();

    uiLabel* labl = new uiLabel( this, ptr );
    lbls += labl;
    constraintType lblct = alignedBelow;
    switch ( pos )
    {
    case LeftTop:
	lb->attach( rightOf, labl );		lblct = rightAlignedBelow;
    break;
    case RightTop:
	labl->attach( rightOf, lb );		lblct = alignedBelow;
    break;
    case LeftMid:
	labl->attach( centeredLeftOf, lb );	lblct = alignedBelow;
    break;
    case RightMid:
	labl->attach( centeredRightOf, lb );	lblct = alignedBelow;
    break;
    case AboveLeft:
	lb->attach( alignedBelow, labl );	lblct = alignedAbove;
    break;
    case AboveMid:
	lb->attach( centeredBelow, labl );	lblct = centeredAbove;
    break;
    case AboveRight:
	lb->attach( rightAlignedBelow, labl );	lblct = rightAlignedAbove;
    break;
    case BelowLeft:
	labl->attach( alignedBelow, lb );	lblct = alignedBelow;
    break;
    case BelowMid:
	labl->attach( centeredBelow, lb );	lblct = centeredBelow;
    break;
    case BelowRight:
	labl->attach( rightAlignedBelow, lb );	lblct = rightAlignedBelow;
    break;
    }

    int nrleft = txts.size() - 1;
    while ( nrleft )
    {
	uiLabel* cur = new uiLabel( this, (last1st
			? txts[nrleft-1] : txts[txts.size()-nrleft])->buf() );
	cur->attach( lblct, labl );
	lbls += cur;
	labl = cur;
	nrleft--;
    }

    deepErase( txts );
}
