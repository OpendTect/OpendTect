/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uilistbox.cc,v 1.128 2012-07-30 10:09:15 cvsbruno Exp $";

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

#define mNoSelection QAbstractItemView::NoSelection
#define mSingle QAbstractItemView::SingleSelection
#define mMulti QAbstractItemView::MultiSelection
#define mExtended QAbstractItemView::ExtendedSelection
#define mContiguous QAbstractItemView::ContiguousSelection

static const int cIconSz = 16;
static const char* startmark = ":"; static const char* endmark = ":";
#define mGetMarkededBufferString(nm,yn,inpstr) \
    const BufferString nm( yn ? startmark : "", inpstr, yn ? endmark : "" )

const int uiListBox::cDefNrLines()	{ return 7; }


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

                        uiListBoxBody(uiListBox& hndle, 
				  uiParent* parnt=0, 
				  const char* nm="uiListBoxBody",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBoxBody()		{ delete &messenger_; }

    void		insertItem(int idx,const char* txt,bool mark,int id);
    void		addItem(const char* txt,bool mark,int id);
    void		removeAll();
    void		removeItem( int idx )
			{
			    items_.remove( idx );
			    delete takeItem(idx);
			}

    int			indexOf(uiListBoxItem*) const;
    void		setItemID(int idx,int id);
    int			getItemID(int idx) const;
    int			getItemIdx(int id) const;

    void		setItemAlignment(int idx,Alignment::HPos);

    int			maxNrOfSelections() const;

    void 		setNrLines( int prefNrLines )
			{ 
			    if( prefNrLines >= 0 )
				prefnrlines_=prefNrLines;
			    int hs = stretch(true,true);
			    setStretch( hs, (nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			{ return prefnrlines_ > 0 ? prefnrlines_
			    			  : uiListBox::cDefNrLines(); }

    int 		fieldwidth_;
    int 		prefnrlines_;

protected:
    void		mouseReleaseEvent(QMouseEvent*);
    void		keyPressEvent(QKeyEvent*);

private:

    i_listMessenger&    messenger_;
    ObjectSet<uiListBoxItem>	items_;

};


uiListBoxBody::uiListBoxBody( uiListBox& hndle, uiParent* parnt, 
			const char* nm, bool ismultiselect,
			int preferrednrlines, int preferredfieldwidth )
    : uiObjBodyImpl<uiListBox,QListWidget>( hndle, parnt, nm )
    , messenger_(*new i_listMessenger(this,&hndle))
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

    setMouseTracking( true );
}


static void createQString( QString& qs, const char* str, bool mark )
{
    if ( !str ) str = "";
    mGetMarkededBufferString( bs, mark, str );
    qs = bs.buf();
}


void uiListBoxBody::addItem( const char* txt, bool mark, int id )
{
    QString qs;
    createQString( qs, txt, mark );
    uiListBoxItem* itm = new uiListBoxItem( qs );
    itm->id_ = id;
    items_ += itm;
    QListWidget::addItem( itm );
}


void uiListBoxBody::insertItem( int idx, const char* txt, bool mark, int id )
{
    QString qs;
    createQString( qs, txt, mark );
    uiListBoxItem* itm = new uiListBoxItem( qs );
    itm->id_ = id;
    items_.insertAt( itm, idx );
    QListWidget::insertItem( idx, itm );
}


void uiListBoxBody::removeAll()
{
    QListWidget::clear();
    items_.erase();
}


void uiListBoxBody::setItemID( int idx, int id )
{ if ( items_.validIdx(idx) ) items_[idx]->id_ = id; }

int uiListBoxBody::getItemID( int idx ) const
{ return items_.validIdx(idx) ? items_[idx]->id_ : -1; }

int uiListBoxBody::getItemIdx( int id ) const
{
    for ( int idx=0; idx<items_.size(); idx++ )
	if ( items_[idx]->id_ == id )
	    return idx;
    return -1;
}


int uiListBoxBody::indexOf( uiListBoxItem* itm ) const
{ return items_.indexOf( itm ); }


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


void uiListBoxBody::mouseReleaseEvent( QMouseEvent* ev )
{
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

    QListWidget::keyPressEvent( qkeyev );
}


int uiListBoxBody::maxNrOfSelections() const
{ 
    if ( selectionMode() == mNoSelection )	return 0;
    if ( selectionMode() == mSingle )		return 1;
    return count();
}


// -------------- uiListBox ---------------

#define mStdInit \
    , buttonstate_(OD::NoButton) \
    , selectionChanged(this) \
    , doubleClicked(this) \
    , rightButtonClicked(this) \
    , leftButtonClicked(this) \
    , deleteButtonPressed(this) \
    , itemChecked(this) \
    , rightclickmnu_(*new uiPopupMenu(p)) \
    , itemscheckable_(false) \
    , alignment_(Alignment::Left) \
    , allowduplicates_(true)


uiListBox::uiListBox( uiParent* p, const char* nm, bool ms, int nl, int pfw )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,ms,nl,pfw) )
    mStdInit
{
    rightButtonClicked.notify( mCB(this,uiListBox,menuCB) );
}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& items, const char* nm,
		      bool ms, int nl, int pfw )
    : uiObject( p, nm?nm:"List Box", mkbody(p,nm,ms,nl,pfw))
    mStdInit
{
    addItems( items );
    setName( "Select Data" );
    rightButtonClicked.notify( mCB(this,uiListBox,menuCB) );
}


uiListBoxBody& uiListBox::mkbody( uiParent* p, const char* nm, bool ms,
				  int nl, int pfw )
{
    body_ = new uiListBoxBody(*this,p,nm,ms,nl,pfw);
    body_->setIconSize( QSize(cIconSz,cIconSz) );
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
    if ( res==0 || res==1 )
	setAllItemsChecked( res==0 );
}


void uiListBox::setNrLines( int prefnrlines )
{ body_->setNrLines( prefnrlines ); }

void uiListBox::setSelectionMode( SelectionMode mode )
{ body_->setSelectionMode( (QAbstractItemView::SelectionMode)mode ); }

void uiListBox::setNotSelectable()
{ setSelectionMode( No ); }

void uiListBox::setMultiSelect( bool yn )
{ setSelectionMode( yn ? Extended : Single ); }

int uiListBox::maxNrOfSelections() const
{ return body_->maxNrOfSelections(); }

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
    mBlockCmdRec;
    if ( validIndex(idx) )
	body_->item( idx )->setSelected( yn );
}


void uiListBox::selectAll( bool yn )
{
    mBlockCmdRec;
    if ( yn )
	body_->selectAll();
    else
	body_->clearSelection();
}


void uiListBox::setAllowDuplicates( bool yn )
{
    allowduplicates_ = yn;
    BufferStringSet allitems; getItems( allitems );
    while ( !allitems.isEmpty() )
    {
	const char* nm = allitems.remove(0)->buf(); 
	while ( allitems.isPresent( nm ) ) 
	{
	    const int itmidx = allitems.indexOf( nm );
	    allitems.remove( itmidx );
	    removeItem( itmidx + 1 );
	}
    }
}


void uiListBox::getItems( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( textOfItem( idx ) ); 
}


void uiListBox::addItem( const char* text, bool mark, int id ) 
{
    if ( !allowduplicates_ && isPresent( text ) )
	return;

    mBlockCmdRec;
    body_->addItem( text, mark, id );
    setItemCheckable( size()-1, false ); // Qt bug
    setItemCheckable( size()-1, itemscheckable_ );
    body_->setItemAlignment( size()-1, alignment_ );
}


void uiListBox::addItem( const char* text, const ioPixmap& pm, int id )
{
    addItem( text, false, id );
    setPixmap( size()-1, pm );
}


void uiListBox::addItem( const char* text, const Color& col, int id )
{
    ioPixmap pm( 64, 64); pm.fill( col );
    addItem( text, pm, id );
}


void uiListBox::addItems( const char** textList ) 
{
    int curidx = currentItem();
    const char** pt_cur = textList;
    while ( *pt_cur )
	addItem( *pt_cur++ );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::addItems( const BufferStringSet& strs )
{
    int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	addItem( strs.get(idx) );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::insertItem( const char* text, int index, bool mark, int id )
{
    mBlockCmdRec;
    if ( index<0 )
	addItem( text, mark );
    else
    {
	if ( !allowduplicates_ && isPresent( text ) )
	    return;

	body_->insertItem( index, text, mark, id );
	setItemCheckable( index<0 ? 0 : index, itemscheckable_ );
	body_->setItemAlignment( size()-1, alignment_ );
    }
}


void uiListBox::insertItem( const char* text, const ioPixmap& pm,
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


void uiListBox::insertItem( const char* text, const Color& col,
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
    if ( itm ) itm->setBackground( qcol );
//    body_->setFocus();
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

void uiListBox::clearSelection()
{
    mBlockCmdRec;
    body_->clearSelection();
}


void uiListBox::sortItems( bool asc )
{
    const int sz = size();
    if ( sz < 2 ) return;

    NotifyStopper nss( selectionChanged );
    NotifyStopper nsc( itemChecked );
    BoolTypeSet mrkd, selected;
    const BufferString cur( getText() );
    BufferStringSet nms;
    for ( int idx=0; idx<sz; idx++ )
    {
	mrkd += isMarked( idx );
	selected += isSelected( idx );
	nms.add( textOfItem(idx) );
    }
    int* sortidxs = nms.getSortIndexes(true,asc);
    nms.useIndexes( sortidxs );
    setEmpty(); addItems( nms );

    for ( int idx=0; idx<sz; idx++ )
    {
	const int newidx = sortidxs[idx];
	setMarked( newidx, selected[idx] );
	setSelected( newidx, selected[idx] );
    }
    delete [] sortidxs;
    if ( !cur.isEmpty() )
	setCurrentItem( cur );
}


void uiListBox::removeItem( int idx )
{
    mBlockCmdRec;
    body_->removeItem( idx );
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


const char* uiListBox::textOfItem( int idx ) const
{
    if ( !validIndex(idx) )
	return "";

    rettxt = (const char*)body_->item(idx)->text().toAscii();
    if ( rettxt[0] != *startmark )
	return rettxt;

    const int sz = rettxt.size();
    if ( rettxt[sz-1] != *endmark )
	return rettxt;

    rettxt[sz-1] = '\0';
    return ((const char*)rettxt) + 1;
}


bool uiListBox::isMarked( int idx ) const
{
    rettxt = (const char*)body_->item(idx)->text().toAscii();
    return rettxt.buf()[0] == *startmark
	&& rettxt.buf()[rettxt.size()-1] == *endmark;
}


void uiListBox::setMarked( int idx, bool yn )
{
    if ( isMarked(idx) == yn ) return;
    mGetMarkededBufferString( newitmtxt, yn, textOfItem(idx) );
    setItemText( idx, newitmtxt );

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
	if ( !strcmp(ptr,txt) )
	    { setCurrentItem( idx ); return; }
    }
}


void uiListBox::setCurrentItem( int idx )
{
    mBlockCmdRec;
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


void uiListBox::setAllItemsChecked( bool yn )
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( isItemCheckable( idx ) )
	    setItemChecked( idx, yn );
    }
}


void uiListBox::setItemChecked( int idx, bool yn )
{
    mBlockCmdRec;
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
    if ( !validIndex(idx) ) return;

    mGetMarkededBufferString( itmtxt, isMarked(idx), txt );
    body_->item(idx)->setText( QString(itmtxt.buf()) );
}


void uiListBox::setItemSelectable( int idx, bool yn )
{
    if ( !validIndex(idx) ) return;

    Qt::ItemFlags flags = body_->item(idx)->flags();
    const bool isselectable = flags.testFlag( Qt::ItemIsSelectable );
    if ( isselectable == yn )
	return;

    body_->item(idx)->setFlags( flags^Qt::ItemIsSelectable );
    setItemChecked( idx, false );
}


bool uiListBox::isItemSelectable( int idx ) const
{
    return validIndex(idx) &&
	body_->item(idx)->flags().testFlag( Qt::ItemIsSelectable );
}


void uiListBox::setSelectedItems( const BufferStringSet& itms )
{
    mBlockCmdRec;
    body_->setCurrentRow( -1 );
    for ( int idx=0; idx<size(); idx++ )
	setSelected( idx, itms.indexOf(textOfItem(idx))>=0 );
}


void uiListBox::setSelectedItems( const TypeSet<int>& itms )
{
    mBlockCmdRec;
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


void uiListBox::setItemID( int idx, int id )
{ body_->setItemID( idx, id ); }

int uiListBox::currentItemID() const
{ return getItemID(currentItem()); }

int uiListBox::getItemID( int idx ) const
{ return body_->getItemID( idx ); }

int uiListBox::getItemIdx( int id ) const
{ return body_->getItemIdx( id ); }


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


void uiListBox::handleCheckChange( QListWidgetItem* itm )
{
    mDynamicCastGet(uiListBoxItem*,lbitm,itm)
    if ( !lbitm ) return;

    const bool ischecked = itm->checkState() == 2;
    if ( lbitm->ischecked_ == ischecked )
	return;

    lbitm->ischecked_ = ischecked;
    const int itmidx = body_->indexOf( lbitm );
    itemChecked.trigger( itmidx );
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
