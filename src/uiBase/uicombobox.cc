/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.cc,v 1.19 2001-09-20 14:28:52 arend Exp $
________________________________________________________________________

-*/

#include <uicombobox.h>
#include <uilabel.h>
#include <uidobjset.h>
#include <uiobjbody.h>

#include <i_qcombobox.h>

#include <qsize.h> 

#ifdef __msvc__
#include <qlabel.h>
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
			    }

    virtual bool        isSingleLine() const { return true; }

private:

    i_comboMessenger&    messenger_;

};


//------------------------------------------------------------------------------


uiComboBox::uiComboBox(  uiParent* parnt, const char* nm, bool ed )
						//false: no read/write
    : uiObject( parnt, nm, mkbody(parnt,nm,ed) )
    , selectionChanged( this )
{}

uiComboBox::uiComboBox(  uiParent* parnt, const PtrUserIDObjectSet& uids,
			 bool ed )
    : uiObject( parnt, (const char*)uids->name(), 
		mkbody(parnt,(const char*)uids->name(),ed) )
    , selectionChanged( this )
{
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


const char* uiComboBox::getText() const
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


void uiComboBox::addItem( const char* text ) 
{ 
    body_->insertItem( QString( text ), -1 );
}


void uiComboBox::addItems( const char** textList ) 
{
    const char* pt_cur = *textList;
    while ( pt_cur )
	addItem( pt_cur++ );
}


void uiComboBox::addItems( const PtrUserIDObjectSet& uids )
{
    int curidx = currentItem();
    if ( uids.currentIndex() >= 0 ) curidx = size() + uids.currentIndex() - 1;
    for ( int idx=0; idx<uids.size(); idx++ )
	body_->insertItem( QString( uids[idx]->name() ), -1 );
    setCurrentItem( curidx );
}


void uiComboBox::addItems( const ObjectSet<BufferString>& strs )
{
    for ( int idx=0; idx<strs.size(); idx++ )
	body_->insertItem( QString( *strs[idx] ), -1 );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const char* txt,
					const char* nm, bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, nm, ed );
    labl = new uiLabel( this, txt, cb );
    setHAlignObj( cb );
}


uiLabeledComboBox::uiLabeledComboBox( uiParent* p, const PtrUserIDObjectSet& s,
				      bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, s, ed );
    labl = new uiLabel( this, s->name(), cb );
    setHAlignObj( cb );
}
