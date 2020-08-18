/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/

#include "uicombobox.h"
#include "i_qcombobox.h"

#include "uiicon.h"
#include "uilabel.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uivirtualkeyboard.h"

#include "datainpspec.h"
#include "mouseevent.h"

#include "q_uiimpl.h"

#include <QAbstractItemView>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QSize>

mUseQtnamespace

class uiComboBoxBody : public uiObjBodyImpl<uiComboBox,QComboBox>
{
public:
uiComboBoxBody( uiComboBox& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiComboBox,QComboBox>(hndle,p,nm)
    , messenger_( *new i_comboMessenger( this, &hndle))
{
    setEditable( false );
    setStretch( 1, 0 );
    setHSzPol( uiObject::Medium) ;
}

virtual	~uiComboBoxBody()
{ delete &messenger_; }

virtual int nrTxtLines() const
{ return 1; }

protected:

virtual void contextMenuEvent(QContextMenuEvent*);

private:

    i_comboMessenger&    messenger_;

};


void uiComboBoxBody::contextMenuEvent( QContextMenuEvent* ev )
{
    if ( uiVirtualKeyboard::isVirtualKeyboardEnabled() )
	handle_.popupVirtualKeyboard( ev->globalX(), ev->globalY() );
    else
	QComboBox::contextMenuEvent( ev );
}


//------------------------------------------------------------------------------


uiComboBox::uiComboBox( uiParent* parnt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
{
}


uiComboBox::uiComboBox( uiParent* parnt, const BufferStringSet& uids,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
{
    addItems( uids );
}


uiComboBox::uiComboBox( uiParent* parnt, const uiStringSet& strings,
		       const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
{
    addItems( strings );
}


uiComboBox::uiComboBox( uiParent* parnt, const EnumDef& enums, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(&enums)
{
    for ( int idx=0; idx<enums.size(); idx++ )
    {
	addItem( enums.getUiStringForIndex(idx), idx );
	if ( enums.getIconFileForIndex(idx) )
	{
	    setIcon( idx, enums.getIconFileForIndex(idx) );
	}
    }

    setReadOnly( true );
}


uiComboBox::~uiComboBox()
{}


uiComboBoxBody& uiComboBox::mkbody( uiParent* parnt, const char* nm )
{
    body_ = new uiComboBoxBody( *this, parnt, nm );
    return *body_;
}


void uiComboBox::adjustWidth( const uiString& txt )
{
    const uiFont& controlfont =
	uiFontList::getInst().get( FontData::key(FontData::Control) );
    const int txtwidth = controlfont.width( txt );

    curwidth_ = curwidth_ >= txtwidth ? curwidth_ : txtwidth;
    body_->view()->setMinimumWidth( curwidth_ + 50 );
}


int uiComboBox::currentItem() const
{ return body_->currentIndex(); }


int uiComboBox::indexOf( const char* str ) const
{
    const BufferString inputstr( str );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( inputstr == itemText(idx) )
	    return idx;
    }

    return -1;
}


int uiComboBox::indexOf( const uiString& str ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( itemstrings_[idx] == str )
	    return idx;
    }
    return -1;
}


void uiComboBox::setPixmap( int index, const uiPixmap& pixmap )
{
    if ( index >= 0 && index < body_->count() )
    {
	body_->setItemText( index, toQString(itemstrings_[index]) );
	body_->setItemIcon( index, *pixmap.qpixmap() );
    }
}


void uiComboBox::setIcon( int index, const char* iconnm )
{
    if ( index<0 || index>=body_->count() )
	return;

    uiIcon icon( iconnm );
    body_->setItemIcon( index, icon.qicon() );
}


void uiComboBox::setColorIcon( int index, const Color& col )
{
    if ( index<0 || index>=body_->count() )
	return;

    if ( col == Color::NoColor() )
	body_->setItemIcon( index, QIcon() );
    else
    {
	const auto maxsz = toolButtonSize();
	setPixmap( index, uiPixmap(maxsz,maxsz,col) );
    }
}


void uiComboBox::setEmpty()
{
    mBlockCmdRec;
    body_->QComboBox::clear();
    body_->clearEditText();
    itemids_.erase();
    itemstrings_.setEmpty();
}


const char* uiComboBox::text() const
{
    if ( isReadOnly() )
	rettxt_ = itemText( currentItem() );
    else
	rettxt_ = body_->currentText();

    return rettxt_.buf();
}


uiString uiComboBox::uiText() const
{
    const int idx = currentItem();
    return idx < 0 ? uiString::empty() : itemstrings_.get( idx );
}


void uiComboBox::setText( const char* txt )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);
    if ( isPresent(txt) )
	setCurrentItem(txt);
    else if ( body_->isEditable() )
	body_->setEditText( txt ? txt : "" );
}


void uiComboBox::setText( const uiString& txt )
{
    mBlockCmdRec;
    const int idx = indexOf( txt );
    if ( idx >= 0 )
	setCurrentItem( idx );
    else if ( body_->isEditable() )
	body_->setEditText( toQString(txt) );
}


bool uiComboBox::isPresent( const char* txt ) const
{
    return indexOf( txt ) >= 0;
}


bool uiComboBox::isPresent( const uiString& txt ) const
{
    return indexOf( txt ) >= 0;
}


uiString uiComboBox::textOfItem( int idx ) const
{
    if ( idx < 0 )
	return uiString::empty();

    const bool isreadonly = isReadOnly();
    if ( isreadonly && enumdef_ && idx<enumdef_->size() )
	return enumdef_->getUiStringForIndex( idx );

    bool useitmstr = false;
    if ( itemstrings_.validIdx(idx) )
    {
	useitmstr = isreadonly;
	if ( !useitmstr )
	    useitmstr = body_->itemText(idx) == toQString(itemstrings_[idx]);
    }

    return useitmstr ? itemstrings_[idx] : toUiString( itemText(idx) );
}


const char* uiComboBox::itemText( int idx ) const
{
    if ( idx < 0 || idx >= body_->count() )
	return OD::EmptyString();
    if ( isReadOnly() && enumdef_ && idx<enumdef_->size() )
	return enumdef_->getKeyForIndex( idx );

    rettxt_.set( body_->itemText(idx) );
    return rettxt_.buf();
}


int uiComboBox::size() const
{
    return body_->count();
}


void uiComboBox::setCurrentItem( const uiString& txt )
{
    mBlockCmdRec;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( itemstrings_.get(idx) == txt )
	{
	    NotifyStopper stopper(selectionChanged);
	    body_->setCurrentIndex( idx );
	    return;
	}
    }
}


void uiComboBox::setCurrentItem( const char* txt )
{
    mBlockCmdRec;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( FixedString(itemText(idx)) == txt )
	{
	    NotifyStopper stopper(selectionChanged);
	    body_->setCurrentIndex( idx );
	    return;
	}
    }
}


void uiComboBox::setCurrentItem( int idx )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);

    if ( idx>=0 && idx<body_->count() )
	body_->setCurrentIndex( idx );
}


void uiComboBox::setItemText( int idx, const uiString& txt )
{
    if ( idx >= 0 && idx < body_->count() )
    {
	adjustWidth( txt );
	body_->setItemText( idx, toQString(txt) );
	itemstrings_[idx] = txt;
    }
}


bool uiComboBox::update_( const DataInpSpec& spec )
{
    mDynamicCastGet(const StringListInpSpec*,spc,&spec)
    if ( !spc ) { return false; }

    setEmpty();
    int cursel = spc->getIntValue();
    if ( cursel >= 0 && cursel < spc->strings().size() )
    {
	addItems( spc->strings() );
	setCurrentItem( cursel );
	return true;
    }
    return false;
}


void uiComboBox::setReadOnly( bool yn )
{
    if ( !yn )
	body_->setEditable( true );

    if ( body_->lineEdit() )
	body_->lineEdit()->setReadOnly( yn );
}


bool uiComboBox::isReadOnly() const
{ return body_->lineEdit() ? body_->lineEdit()->isReadOnly() : true; }


void uiComboBox::setEditable( bool yn )
{ body_->setEditable( yn ); }


bool uiComboBox::isEditable() const
{ return body_->isEditable(); }


void uiComboBox::addItem( const uiString& str )
{
    addItem( str, -1 );
}


void uiComboBox::addItem( const uiString& txt, int id )
{
    mBlockCmdRec;
    adjustWidth( txt );
    body_->addItem( toQString(txt) );
    itemids_ += id;
    itemstrings_ += txt;
}


void uiComboBox::addItems( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	addItem( toUiString( bss.get(idx).str() ) );
}


void uiComboBox::addItems( const uiStringSet& items )
{
    for ( int idx=0; idx<items.size(); idx++ )
	addItem( items[idx] );
}


void uiComboBox::addSeparator()
{
    body_->insertSeparator( size() );
    itemids_ += -1;
    itemstrings_ += uiString::empty();
}


void uiComboBox::insertItem( const uiString& txt, int index, int id )
{
    mBlockCmdRec;
    adjustWidth( txt );
    body_->insertItem( index, toQString(txt) );
    itemids_.insert( index, id );
    itemstrings_.insert( index, txt );
}


void uiComboBox::insertItem( const uiPixmap& pm, const uiString& txt,
			     int index, int id )
{
    mBlockCmdRec;
    adjustWidth( txt );
    body_->insertItem( index, *pm.qpixmap(), toQString(txt) );
    itemids_.insert( index, id );
    itemstrings_.insert( index, txt );
}


void uiComboBox::setItemID( int index, int id )
{
    if ( itemids_.validIdx(index) )
	itemids_[index] = id;
}


int uiComboBox::currentItemID() const
{
    return getItemID(currentItem());
}


int uiComboBox::getItemID( int index ) const
{
    return itemids_.validIdx(index) ? itemids_[index] : -1;
}


int uiComboBox::getItemIndex( int id ) const
{
    return itemids_.indexOf( id );
}


void uiComboBox::setCurrentItemByID( int id )
{
    const int idx = itemids_.indexOf( id );
    if ( idx >= 0 )
	setCurrentItem( idx );
}


void uiComboBox::getItems( uiStringSet& nms ) const
{
    for ( int idx=0; idx<itemstrings_.size(); idx++ )
	nms.add( itemstrings_.get(idx) );
}


void uiComboBox::getItems( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( itemText( idx ) );
}


void uiComboBox::notifyHandler( bool selectionchanged )
{
    BufferString msg; msg.add( oldnritems_ );
    msg += " "; msg.add( oldcuritem_ );
    oldnritems_ = size();
    oldcuritem_ = currentItem();

    msg += selectionchanged ? " selectionChanged" : " editTextChanged";
    const int refnr = beginCmdRecEvent( msg );

    if ( selectionchanged )
	selectionChanged.trigger( this );
    else
	editTextChanged.trigger( this );

    endCmdRecEvent( refnr, msg );
}


bool uiComboBox::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x_, pos.y_ );
    return true;
}


void uiComboBox::popupVirtualKeyboard( int globalx, int globaly )
{
    if ( isReadOnly() || !hasFocus() )
	return;

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
    {
	const char* txt = text();
	if ( !isPresent(txt) )
	    addItem( toUiString(txt) );

	setCurrentItem( txt );
	selectionChanged.trigger();
    }
}


void uiComboBox::translateText()
{
    uiObject::translateText();

    if ( !isReadOnly() )
	return;

    for ( int idx=0; idx<size(); idx++ )
	body_->setItemText( idx, toQString(itemstrings_[idx]) );
}


//------------------------------------------------------------------------------

#define mGetBoxNm() \
    BufferString boxnm( nm ); \
    if ( boxnm.isEmpty() ) \
	boxnm = toString( txt )

uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const uiString& txt,
				      const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    mGetBoxNm();
    cb_ = new uiComboBox( this, boxnm );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


template <class DEF>
void uiLabeledComboBox::init( const DEF& itms, const uiString& txt,
			      const char* nm )
{
    mGetBoxNm();
    cb_ = new uiComboBox( this, itms, boxnm );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const BufferStringSet& itms,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    init( itms, txt, nm );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const uiStringSet& itms,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    init( itms, txt, nm );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const EnumDef& enumdef,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    init( enumdef, txt, nm );
}
