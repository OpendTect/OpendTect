/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.cc,v 1.3 2001-04-30 14:57:59 bert Exp $
________________________________________________________________________

-*/

#include <uicombobox.h>
#include <uilabel.h>
#include <uidset.h>

#include <i_qcombobox.h>
#include <i_qobjwrap.h>

#include <qsize.h> 

typedef i_QObjWrapper<QComboBox> i_QComboBox;

//------------------------------------------------------------------------------

uiComboBox::uiComboBox(  uiObject* parnt, const char* nm, bool ed )
	: uiWrapObj<i_QComboBox>(new i_QComboBox(*this,nm,parnt,false),parnt,nm)
						//false: no read/write
	, _messenger ( *new i_comboMessenger( mQtThing(), this ))
{
    mQtThing()->setEditable( ed );
    mQtThing()->setAutoCompletion( ed );
}

uiComboBox::uiComboBox(  uiObject* parnt, const UserIDSet& uids, bool ed )
	: uiWrapObj<i_QComboBox>(new i_QComboBox(*this,(const char*)uids.name(),
			  parnt,false), parnt, (const char*)uids.name() )
						//false: no read/write
	, _messenger ( *new i_comboMessenger( mQtThing(), this ))
{
    mQtThing()->setEditable( ed );
    mQtThing()->setAutoCompletion( ed );
    addItems( uids );
}

uiComboBox::~uiComboBox()
{}

const QWidget* 	uiComboBox::qWidget_() const 	{ return mQtThing(); } 

int uiComboBox::currentItem() const
{
    return mQtThing()->currentItem();
}


void uiComboBox::clear()
{
    mQtThing()->clear();
}


const char* uiComboBox::getText() const
{
    return mQtThing()->currentText();
}


bool uiComboBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	if ( mQtThing()->text(idx) == txt ) return true;
    return false;
}


const char* uiComboBox::textOfItem( int idx ) const
{
    const_cast<uiComboBox*>(this)->rettxt = (const char*)mQtThing()->text(idx);
    return (const char*)rettxt;
}


int uiComboBox::size() const
{
    return mQtThing()->count();
}


void uiComboBox::setCurrentItem( const char* txt )
{
    const int sz = mQtThing()->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( mQtThing()->text(idx) == txt )
	    { mQtThing()->setCurrentItem( idx ); return; }
    }
}

void uiComboBox::setCurrentItem( int index )
{
    mQtThing()->setCurrentItem( index );
}


void uiComboBox::setItemText( int idx, const char* txt )
{
    mQtThing()->changeItem( QString(txt), idx );
}


void uiComboBox::addItem( const char* text ) 
{ 
    mQtThing()->insertItem( QString( text ), -1 );
}


void uiComboBox::addItems( const char** textList ) 
{
    const char* pt_cur = *textList;
    while ( pt_cur )
	addItem( pt_cur++ );
}


void uiComboBox::addItems( const UserIDSet& uids )
{
    for ( int idx=0; idx<uids.size(); idx++ )
	mQtThing()->insertItem( QString( uids[idx]->name() ), -1 );
}


uiLabeledComboBox::uiLabeledComboBox( uiObject* p, const char* txt,
					const char* nm, bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, nm, ed );
    labl = new uiLabel( this, txt, cb );
    setHAlignObj( cb );
}


uiLabeledComboBox::uiLabeledComboBox( uiObject* p, const UserIDSet& s, bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, s, ed );
    labl = new uiLabel( this, s.name(), cb );
    setHAlignObj( cb );
}
