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
#include "uilineedit.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uivirtualkeyboard.h"

#include "datainpspec.h"
#include "mouseevent.h"

#include "q_uiimpl.h"

#include <QAbstractItemView>
#include <QContextMenuEvent>
#include <QLineEdit>
#include <QRegExpValidator>
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
{}


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


uiComboBox::uiComboBox( uiParent* parnt, const char** uids, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
{
    addItems( BufferStringSet(uids) );
}


uiComboBox::uiComboBox( uiParent* parnt, const uiString* strings,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))
    , curwidth_(0)
    , enumdef_(0)
{
    for ( int idx=0; !strings[idx].isEmpty(); idx++ )
	addItem( strings[idx] );
}


uiComboBox::uiComboBox( uiParent* parnt, const EnumDef& enums,
			const char* nm )
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


void uiComboBox::setValidator( const BufferString& regex )
{
    if ( regex.isEmpty() )
	return;

    QRegExpValidator* textvl = new QRegExpValidator();
    QRegExp regexp( regex.buf() );
    textvl->setRegExp(regexp);

    body_->setValidator(textvl);
}


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
    const FixedString inputstr( str );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( inputstr == textOfItem(idx) )
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


void uiComboBox::setColorIcon( int index, const Color& col )
{
    if ( index<0 || index>=body_->count() )
	return;

    if ( col == Color::NoColor() )
	body_->setItemIcon( index, QIcon() );
    else
    {
	uiPixmap pm( 15, 10 );
	pm.fill( col );
	setPixmap( index, pm );
    }
}


void uiComboBox::setIcon( int index, const char* iconnm )
{
    if ( index<0 || index>=body_->count() )
	return;

    uiIcon icon( iconnm );
    body_->setItemIcon( index, icon.qicon() );
}


void uiComboBox::setToolTips( const uiStringSet& tooltips )
{
    for ( int idx=0; idx<body_->count(); idx++ )
	body_->setItemData( idx, toQString(tooltips.get(idx)),
			    Qt::ToolTipRole );
}


void uiComboBox::setEmpty()
{
    mBlockCmdRec;
    body_->QComboBox::clear();
    body_->clearEditText();
    itemids_.erase();
    itemstrings_.erase();
}


const char* uiComboBox::text() const
{
    if ( isReadOnly() )
	rettxt_ = textOfItem( currentItem() );
    else
	rettxt_ = body_->currentText();

    return rettxt_.buf();
}


void uiComboBox::setText( const char* txt )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);
    if ( isPresent(txt) )
	setCurrentItem(txt);
    else
    {
	bool iseditable = body_->isEditable();
	if ( !iseditable ) body_->setEditable( true );
	body_->setEditText( txt ? txt : "" );
	if ( !iseditable ) body_->setEditable( false );
    }
}


bool uiComboBox::isPresent( const char* txt ) const
{
    return indexOf( txt ) >= 0;
}


const char* uiComboBox::textOfItem( int idx ) const
{
    if ( idx < 0 || idx >= body_->count() ) return sKey::EmptyString();

    if ( isReadOnly() && enumdef_ && idx<enumdef_->size() )
	return enumdef_->getKeyForIndex( idx );

    if ( itemstrings_.validIdx(idx) && (isReadOnly() ||
	 body_->itemText(idx)==toQString(itemstrings_[idx])) )
    {
	rettxt_ = itemstrings_[idx].getFullString();
    }
    else
    {
	rettxt_ = body_->itemText(idx);
    }

    return rettxt_.buf();
}


int uiComboBox::size() const
{ return body_->count(); }


void uiComboBox::setCurrentItem( const char* txt )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);
    if ( !txt )
    {
	body_->setCurrentIndex( 0 );
	return;
    }

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( FixedString(textOfItem(idx)) == txt )
	    { body_->setCurrentIndex( idx ); return; }
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
{ addItem( str, -1 ); }


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


void uiComboBox::addItems( const char* arr[] )
{
    const BufferStringSet bss( arr );
    addItems( bss );
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
    itemstrings_ += uiString::emptyString();
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
{ if ( itemids_.validIdx(index) ) itemids_[index] = id; }

int uiComboBox::currentItemID() const
{ return getItemID(currentItem()); }

int uiComboBox::getItemID( int index ) const
{ return itemids_.validIdx(index) ? itemids_[index] : -1; }

int uiComboBox::getItemIndex( int id ) const
{ return itemids_.indexOf( id ); }


void uiComboBox::getItems( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<size(); idx++ )
	nms.add( textOfItem( idx ) );
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
    popupVirtualKeyboard( pos.x, pos.y );
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
    {
	body_->setItemText( idx, toQString(itemstrings_[idx]) );
    }
}


//------------------------------------------------------------------------------


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const uiString& txt,
				      const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, nm && *nm ? nm : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const BufferStringSet& strs,
				      const uiString& txt, const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
	    ? nm
	    : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const char** strs,
				      const uiString& txt, const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
	    ? nm
	    : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const uiStringSet& strs,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
			 ? nm
			 : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const EnumDef& strs,
				     const uiString& txt, const char* nm )
    : uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm
			 ? nm
			 : txt.getFullString().buf() );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}
