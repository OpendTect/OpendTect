/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/05/2000
 RCS:           $Id: uicombobox.cc,v 1.15 2001-06-03 15:43:51 bert Exp $
________________________________________________________________________

-*/

#include <uicombobox.h>
#include <uilabel.h>
#include <uidobjset.h>

#include <i_qcombobox.h>
#include <i_qobjwrap.h>

#include <qsize.h> 

#ifdef __msvc__
#include <qlabel.h>
#endif

typedef i_QObjWrapper<QComboBox> i_QComboBox;

//------------------------------------------------------------------------------

uiComboBox::uiComboBox(  uiObject* parnt, const char* nm, bool ed )
	: uiWrapObj<i_QComboBox>(new i_QComboBox(*this,nm,parnt,false),parnt,nm)
						//false: no read/write
	, _messenger ( *new i_comboMessenger( mQtThing(), this ))
	, selectionchanged( this )
{
    mQtThing()->setEditable( ed );
    mQtThing()->setAutoCompletion( ed );
    setStretch( 1, 0 );
}

uiComboBox::uiComboBox(  uiObject* parnt, const PtrUserIDObjectSet& uids,
			 bool ed )
	: uiWrapObj<i_QComboBox>(
		new i_QComboBox(*this,(const char*)uids->name(),parnt,false),
				parnt, (const char*)uids->name() )
	, _messenger ( *new i_comboMessenger( mQtThing(), this ))
	, selectionchanged( this )
{
    mQtThing()->setEditable( ed );
    mQtThing()->setAutoCompletion( ed );
    setStretch( 1, 0 );
    addItems( uids );
}

uiComboBox::~uiComboBox()
{}

const QWidget* 	uiComboBox::qWidget_() const 	{ return mQtThing(); } 

int uiComboBox::currentItem() const
{
    return mQtThing()->currentItem();
}


void uiComboBox::empty()
{
    mQtThing()->clear();
}


const char* uiComboBox::getText() const
{
    return mQtThing()->currentText();
}


void uiComboBox::setText( const char* txt )
{
    NotifyStopper<uiComboBox> stopper(selectionchanged);
    if ( isPresent(txt) )
	setCurrentItem(txt);
    else
    {
	bool iseditable = mQtThing()->editable();
	if ( !iseditable ) mQtThing()->setEditable( true );
	mQtThing()->setEditText( txt ? txt : "" );
	if ( !iseditable ) mQtThing()->setEditable( false );
    }
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
    if ( idx < 0 || idx >= mQtThing()->count() ) return "";
    const_cast<uiComboBox*>(this)->rettxt = (const char*)mQtThing()->text(idx);
    return (const char*)rettxt;
}


int uiComboBox::size() const
{
    return mQtThing()->count();
}


void uiComboBox::setCurrentItem( const char* txt )
{
    NotifyStopper<uiComboBox> stopper(selectionchanged);

    const int sz = mQtThing()->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( mQtThing()->text(idx) == txt )
	    { mQtThing()->setCurrentItem( idx ); return; }
    }
}

void uiComboBox::setCurrentItem( int idx )
{
    NotifyStopper<uiComboBox> stopper(selectionchanged);

    if ( idx >= 0 && idx < mQtThing()->count() )
	mQtThing()->setCurrentItem( idx );
}


void uiComboBox::setItemText( int idx, const char* txt )
{
    if ( idx >= 0 && idx < mQtThing()->count() )
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


void uiComboBox::addItems( const PtrUserIDObjectSet& uids )
{
    int curidx = currentItem();
    if ( uids.currentIndex() >= 0 ) curidx = size() + uids.currentIndex() - 1;
    for ( int idx=0; idx<uids.size(); idx++ )
	mQtThing()->insertItem( QString( uids[idx]->name() ), -1 );
    setCurrentItem( curidx );
}


void uiComboBox::addItems( const ObjectSet<BufferString>& strs )
{
    for ( int idx=0; idx<strs.size(); idx++ )
	mQtThing()->insertItem( QString( *strs[idx] ), -1 );
}


uiLabeledComboBox::uiLabeledComboBox( uiObject* p, const char* txt,
					const char* nm, bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, nm, ed );
    labl = new uiLabel( this, txt, cb );
    setHAlignObj( cb );
}


uiLabeledComboBox::uiLabeledComboBox( uiObject* p, const PtrUserIDObjectSet& s,
				      bool ed )
	: uiGroup(p,"Labeled combobox")
{
    cb = new uiComboBox( this, s, ed );
    labl = new uiLabel( this, s->name(), cb );
    setHAlignObj( cb );
}
