/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uicombobox.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "uivirtualkeyboard.h"
#include "pixmap.h"
#include "datainpspec.h"
#include "mouseevent.h"

#include "i_qcombobox.h"

#include <QContextMenuEvent>
#include <QSize>

mUseQtnamespace

class uiComboBoxBody : public uiObjBodyImpl<uiComboBox,QComboBox>
{

public:

			uiComboBoxBody( uiComboBox& hndle, uiParent* p,
					const char* nm )
		     : uiObjBodyImpl<uiComboBox,QComboBox>(hndle,p,nm)
		     , messenger_( *new i_comboMessenger( this, &hndle))
		     {
			    setEditable( false );
			    setAutoCompletion( false );
			    setStretch( 1, 0 );
			    setHSzPol( uiObject::Medium) ;
		      }

    virtual		~uiComboBoxBody()
			    { delete &messenger_; }

    virtual int 	nrTxtLines() const		{ return 1; }

protected:

    virtual void	contextMenuEvent(QContextMenuEvent*);

private:

    i_comboMessenger&    messenger_;

};


void uiComboBoxBody::contextMenuEvent( QContextMenuEvent* ev )
{ handle().popupVirtualKeyboard( ev->globalX(), ev->globalY() ); }


//------------------------------------------------------------------------------


uiComboBox::uiComboBox( uiParent* parnt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))  
{
}


uiComboBox::uiComboBox( uiParent* parnt, const BufferStringSet& uids,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))  
{ 
    addItems( uids );
}


uiComboBox::uiComboBox( uiParent* parnt, const char** uids, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this )
    , editTextChanged( this )
    , oldnritems_(mUdf(int)), oldcuritem_(mUdf(int))  
{ 
    addItems( uids );
}


uiComboBox::~uiComboBox()
{}


uiComboBoxBody& uiComboBox::mkbody( uiParent* parnt, const char* nm )
{
    body_ = new uiComboBoxBody( *this, parnt, nm );
    return *body_; 
}


int uiComboBox::currentItem() const
{ return body_->currentIndex(); }


int uiComboBox::indexOf( const char* str ) const
{
    const FixedString inputstr( str );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( textOfItem(idx)==inputstr )
	    return idx;
    }

    return -1;
}


void uiComboBox::setPixmap( const ioPixmap& pixmap, int index )
{
    const char* txt = textOfItem( index );
    if ( index >= 0 && index < body_->count() )
    {
	body_->setItemText( index, QString(txt) );
	body_->setItemIcon( index, *pixmap.qpixmap() );
    }
}


void uiComboBox::setEmpty()
{
    mBlockCmdRec;
    body_->QComboBox::clear();
    itemids_.erase();
}


const char* uiComboBox::text() const
{
    rettxt_ = mQStringToConstChar( body_->currentText() );
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
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	if ( body_->itemText(idx) == txt )
	    return true;

    return false;
}


const char* uiComboBox::textOfItem( int idx ) const
{
    if ( idx < 0 || idx >= body_->count() ) return "";
    rettxt_ = mQStringToConstChar( body_->itemText(idx) );
    return rettxt_.buf();
}


int uiComboBox::size() const
{ return body_->count(); }


void uiComboBox::setCurrentItem( const char* txt )
{
    mBlockCmdRec;
    NotifyStopper stopper(selectionChanged);

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( body_->itemText(idx) == txt )
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


void uiComboBox::setItemText( int idx, const char* txt )
{
    if ( idx >= 0 && idx < body_->count() )
	body_->setItemText( idx, QString(txt) );
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
{ body_->setEditable( !yn ); }


bool uiComboBox::isReadOnly() const
{ return !body_->isEditable(); }


void uiComboBox::addItem( const wchar_t* txt, int id )
{
    mBlockCmdRec;
#ifdef __win__
    const int wsz = wcslen(txt) + 1;
    char* buf = new char [wsz];
    int rsz = wcstombs(buf,txt,wsz);
    buf[wsz-1] = '\0';
    QString itmtxt = QString::fromUtf8( buf );
#else
    QString itmtxt = QString::fromWCharArray( txt );
#endif
    body_->addItem( itmtxt );
    itemids_ += id;
}


void uiComboBox::addItem( const char* txt )
{ addItem( txt, -1 ); }


void uiComboBox::addItem( const char* txt, int id ) 
{
    mBlockCmdRec;
    body_->addItem( QString(txt) );
    itemids_ += id;
}


void uiComboBox::addItems( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	addItem( bss.get(idx) );
}


void uiComboBox::addSeparator()
{ body_->insertSeparator( size() ); }


void uiComboBox::insertItem( const char* txt, int index, int id )
{
    mBlockCmdRec;
    body_->insertItem( index, QString(txt) );
    itemids_.insert( index, id );
}


void uiComboBox::insertItem( const ioPixmap& pm, const char* txt,
			     int index, int id )
{
    mBlockCmdRec;
    body_->insertItem( index, *pm.qpixmap(), QString(txt) );
    itemids_.insert( index, id );
}


void uiComboBox::setItemID( int index, int id )
{ if ( itemids_.validIdx(index) ) itemids_[index] = id; }

int uiComboBox::currentItemID() const
{ return getItemID(currentItem()); }

int uiComboBox::getItemID( int index ) const
{ return itemids_.validIdx(index) ? itemids_[index] : -1; }

int uiComboBox::getItemIndex( int id ) const
{ return itemids_.indexOf( id ); }


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
	    addItem( txt );

	setCurrentItem( txt );
	selectionChanged.trigger();
    }
}


//------------------------------------------------------------------------------


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const char* txt,
				      const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, nm && *nm ? nm : txt );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const BufferStringSet& strs,
				      const char* txt, const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm ? nm : txt );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const char** strs,
				      const char* txt, const char* nm )
	: uiGroup(p,"Labeled combobox")
{
    cb_ = new uiComboBox( this, strs, nm && *nm ? nm : txt );
    labl_ = new uiLabel( this, txt, cb_ );
    setHAlignObj( cb_ );
}
