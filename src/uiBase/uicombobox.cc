/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.cc,v 1.32 2003-10-17 14:19:02 bert Exp $
________________________________________________________________________

-*/

#include "uicombobox.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "datainpspec.h"

#include "i_qcombobox.h"

#include "qsize.h"

#ifdef __msvc__
#include "qlabel.h"
#endif


class uiComboBoxBody : public uiObjBodyImpl<uiComboBox,QComboBox>
{

public:

			uiComboBoxBody( uiComboBox& handle, uiParent* p,
					const char* nm, bool ed)
			    : uiObjBodyImpl<uiComboBox,QComboBox>(handle,p,nm)
			    , messenger_( *new i_comboMessenger( this, &handle))
			    {
				setEditable( ed );
				setAutoCompletion( ed );
				setHSzPol( uiObject::medium) ;
			    }

    virtual		~uiComboBoxBody()
			    { delete &messenger_; }

    virtual int 	nrTxtLines() const		{ return 1; }

private:

    i_comboMessenger&    messenger_;

};


//------------------------------------------------------------------------------


uiComboBox::uiComboBox(  uiParent* parnt, const char* nm, bool ed )
						//false: no read/write
    : uiObject( parnt, nm, mkbody(parnt,nm,ed) )
    , selectionChanged( this )
{ if ( ed ) setStretch( 1, 0 );}

uiComboBox::uiComboBox(  uiParent* parnt, const BufferStringSet& uids,
			 const char* nm, bool ed )
    : uiObject( parnt, nm, mkbody(parnt,nm,ed) )
    , selectionChanged( this )
{ 
    if ( ed ) setStretch( 1, 0 );
    addItems( uids );
}


uiComboBox::~uiComboBox()
{}


uiComboBoxBody& uiComboBox::mkbody(uiParent* parnt, const char* nm, bool ed)
{
    body_= new uiComboBoxBody(*this,parnt,nm,ed);
    return *body_; 
}

int uiComboBox::currentItem() const
{
    return body_->currentItem();
}


void uiComboBox::empty()
{
    body_->QComboBox::clear();
}


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
{
    return body_->count();
}


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

    int cursel = spc->getIntValue();
    if ( cursel >= 0 && cursel < spc->strings().size() )
    {
	empty();
	addItems( spc->strings() );
	setCurrentItem( cursel );
	return true;
    }
    return false;
}


void uiComboBox::setReadOnly( bool yn )
{ 
    body_->setEditable( !yn );
}


bool uiComboBox::isReadOnly() const
{
    return !body_->editable();
}

void uiComboBox::addItem( const char* text ) 
{ 
    body_->insertItem( QString( text ), -1 );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const char* txt,
					const char* nm, bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, nm, ed );
    labl = new uiLabel( this, txt, cb );
    setHAlignObj( cb );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const BufferStringSet& s,
				      const char* nm, bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, s, nm, ed );
    labl = new uiLabel( this, nm, cb );
    setHAlignObj( cb );
}
