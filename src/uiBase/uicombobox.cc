/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.cc,v 1.46 2008-05-07 05:39:21 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uicombobox.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "pixmap.h"
#include "datainpspec.h"

#include "i_qcombobox.h"

#include <QApplication>
#include <QEvent>
#include <QSize>


class uiComboBoxBody : public uiObjBodyImpl<uiComboBox,QComboBox>
{

public:

			uiComboBoxBody( uiComboBox& handle, uiParent* p,
					const char* nm )
			    : uiObjBodyImpl<uiComboBox,QComboBox>(handle,p,nm)
			    , messenger_( *new i_comboMessenger( this, &handle))
			{
			    setEditable( false );
			    setAutoCompletion( false );
			    setHSzPol( uiObject::Medium) ;
			}

    virtual		~uiComboBoxBody()
			    { delete &messenger_; }

    virtual int 	nrTxtLines() const		{ return 1; }

    void		activate(int idx);
    bool		event(QEvent*);

protected:
    int			activateidx_;

private:

    i_comboMessenger&    messenger_;

};


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User+0);

void uiComboBoxBody::activate( int idx )
{
    activateidx_ = idx;
    QEvent* actevent = new QEvent( sQEventActivate );
    QApplication::postEvent( this, actevent );
}


bool uiComboBoxBody::event( QEvent* ev )
{
    if ( ev->type() != sQEventActivate )
	return QComboBox::event( ev );
    
    if ( activateidx_>=0 && activateidx_<handle_.size() )
    {
	handle_.setCurrentItem( activateidx_ );
	handle_.selectionChanged.trigger();
    }

    handle_.activatedone.trigger();
    return true;
}


//------------------------------------------------------------------------------


uiComboBox::uiComboBox( uiParent* parnt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this ), activatedone( this )
{
}


uiComboBox::uiComboBox( uiParent* parnt, const BufferStringSet& uids,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this ), activatedone( this )
{ 
    addItems( uids );
}


uiComboBox::uiComboBox( uiParent* parnt, const char** uids, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selectionChanged( this ), activatedone( this )
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
{ return body_->currentItem(); }


void uiComboBox::setPixmap( const ioPixmap& pixmap, int index )
{
    const char* txt = textOfItem( index );
    if ( index >= 0 && index < body_->count() )
	body_->changeItem( *pixmap.qpixmap(), QString(txt), index );
}


void uiComboBox::empty()
{ body_->QComboBox::clear(); }


const char* uiComboBox::text() const
{
    static QString ret;
    ret = body_->currentText();
    return ret;
}


void uiComboBox::setText( const char* txt )
{
    NotifyStopper stopper(selectionChanged);
    if ( isPresent(txt) )
	setCurrentItem(txt);
    else
    {
	bool iseditable = body_->editable();
	if ( !iseditable ) body_->setEditable( true );
	body_->setEditText( txt ? txt : "" );
	if ( !iseditable ) body_->setEditable( false );
    }
}


bool uiComboBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	if ( body_->text(idx) == txt ) return true;
    return false;
}


const char* uiComboBox::textOfItem( int idx ) const
{
    if ( idx < 0 || idx >= body_->count() ) return "";
    const_cast<uiComboBox*>(this)->rettxt = (const char*)body_->text(idx);
    return (const char*)rettxt;
}


int uiComboBox::size() const
{ return body_->count(); }


void uiComboBox::setCurrentItem( const char* txt )
{
    NotifyStopper stopper(selectionChanged);

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( body_->text(idx) == txt )
	    { body_->setCurrentItem( idx ); return; }
    }
}


void uiComboBox::setCurrentItem( int idx )
{
    NotifyStopper stopper(selectionChanged);

    if ( idx >= 0 && idx < body_->count() )
	body_->setCurrentItem( idx );
}


void uiComboBox::setItemText( int idx, const char* txt )
{
    if ( idx >= 0 && idx < body_->count() )
	body_->changeItem( QString(txt), idx );
}


bool uiComboBox::update_( const DataInpSpec& spec )
{
    mDynamicCastGet(const StringListInpSpec*,spc,&spec)
    if ( !spc ) { return false; }

    empty();
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
{ return !body_->editable(); }


void uiComboBox::addItem( const char* text ) 
{ insertItem( text, -1 ); }


void uiComboBox::addItems( const BufferStringSet& bss )
{
    for ( int idx=0; idx<bss.size(); idx++ )
	insertItem( bss.get(idx), -1 );
}


void uiComboBox::insertItem( const char* text, int index )
{
    body_->insertItem( QString(text), index );
}


void uiComboBox::insertItem( const ioPixmap& pm, const char* text , int index )
{
    body_->insertItem( *pm.qpixmap(), QString(text), index );
}


void uiComboBox::activate( int idx )
{ body_->activate( idx ); }



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
