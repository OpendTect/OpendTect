/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.10 2001-05-08 08:20:12 bert Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>
#include <uifont.h>
#include <uidobjset.h>

#include <i_qlistbox.h>
#include <i_qobjwrap.h>

#include <qsize.h> 

typedef i_QObjWrapper<QListBox> i_QListBoxBase;


/*!\brief Derived QListBox, only to override sizeHint() */

class i_QListBox : public i_QListBoxBase
{
public:
          
			i_QListBox( uiObject& client,
				    uiObject* parnt=0, const char* name=0 )
			: i_QListBoxBase( client, parnt, name ) {}

        virtual QSize   sizeHint() const;

};


QSize i_QListBox::sizeHint() const
{  
    const uiListBox* ptClient = dynamic_cast<const uiListBox*> ( uiClient() );
    if ( !ptClient ) { pErrMsg("uiClient is not a listbox!"); return QSize(); }
    const uiFont* mFont = ptClient->font();
    if( !mFont ) { pErrMsg("uiClient has no font!"); return QSize(); }

    // initialize to requested size or reasonable default size
    // reasonable sizes are 3 <= nrlines <= 7 , 20 <= nrchars <= 40.
    const int sz = ptClient->size();
    int nrchars = ptClient->fieldWdt ? ptClient->fieldWdt : 20;
    int nrlines = ptClient->nLines ? ptClient->nLines
		: sz > 7 ? 7 : (sz < 3 ? 3 : sz);

    // if biggest string is over 20 chars, grow box to max 40 chars.
    const int fontmaxpixwidth = mFont->maxWidth();
    const int maxwdth = 40 * fontmaxpixwidth;
    int pixwidth = nrchars * fontmaxpixwidth;
    if ( !ptClient->fieldWdt )
    {
	QListBoxItem* itm = item( 0 );
	for ( int idx=0; itm; itm = item(++idx) )
	{
	    const int pixw = mFont->width( itm->text() ) + 2 * fontmaxpixwidth;
	    if ( pixw > pixwidth )
		pixwidth = pixw > maxwdth ? maxwdth : pixw;
	}
    }

    const int extrasz = 2 * frameWidth();
    const int pixheight = mFont->height() * nrlines;
    return QSize ( pixwidth+extrasz, pixheight+extrasz );
}


uiListBox::uiListBox(  uiObject* parnt, const char* nm, bool isMultiSelect,
		       int preferredNrLines, int preferredFieldWidth )
	: uiWrapObj<i_QListBox>(new i_QListBox(*this,parnt,nm),parnt,nm)
	, _messenger (*new i_listMessenger(mQtThing(),this))
	, fieldWdt(preferredFieldWidth)
	, nLines(preferredNrLines)
{
    if( isMultiSelect ) mQtThing()->setSelectionMode( QListBox::Extended );
    setStretch( 1, isSingleLine() ? 0 : 1 );
}


uiListBox::uiListBox( uiObject* p, const PtrUserIDObjectSet& uids,
			bool isMultiSelect,
			int preferredNrLines, int preferredFieldWidth )
	: uiWrapObj<i_QListBox>(
		new i_QListBox( *this,p,(const char*)uids->name() ),
				      p,(const char*)uids->name() )
	, _messenger(*new i_listMessenger(mQtThing(),this))
	, fieldWdt(preferredFieldWidth)
	, nLines(preferredNrLines)
{
    if( isMultiSelect ) mQtThing()->setSelectionMode( QListBox::Extended );
    setStretch( 1, isSingleLine() ? 0 : 1 );
    addItems( uids );
}


uiListBox::~uiListBox()
{
}


const QWidget* 	uiListBox::qWidget_() const 	{ return mQtThing(); } 


int uiListBox::size() const
{
    return mQtThing()->numRows();
}


bool uiListBox::isSelected ( int index ) const
{
    return mQtThing()->isSelected( index );
}

void uiListBox::addItem( const char* text ) 
{ 
    mQtThing()->insertItem( QString( text ) , -1 );
}


void uiListBox::addItems( const char** textList ) 
{
    const char* pt_cur = *textList;
    while ( pt_cur )
        addItem( pt_cur++ );
}


void uiListBox::addItems( const PtrUserIDObjectSet& uids )
{
    int curidx = currentItem();
    if ( uids.currentIndex() >= 0 ) curidx = size() + uids.currentIndex() - 1;
    for ( int idx=0; idx<uids.size(); idx++ )
	mQtThing()->insertItem( QString( uids[idx]->name() ), -1 );
    setCurrentItem( curidx );
}


void uiListBox::empty()
{
    mQtThing()->clear();
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
	if ( mQtThing()->text(idx) == txt ) return true;
    return false;
}


const char* uiListBox::textOfItem( int idx ) const
{
    if ( idx < 0 ) return "";
    const_cast<uiListBox*>(this)->rettxt = (const char*)mQtThing()->text(idx);
    return (const char*)rettxt;
}


int uiListBox::currentItem() const
{
    return mQtThing()->currentItem();
}


void uiListBox::setCurrentItem( const char* txt )
{
    const int sz = mQtThing()->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( mQtThing()->text(idx) == txt )
	    { mQtThing()->setCurrentItem( idx ); return; }
    }
}

void uiListBox::setCurrentItem( int index )
{
    mQtThing()->setCurrentItem( index );
}


void uiListBox::setItemText( int idx, const char* txt )
{
    mQtThing()->changeItem( QString(txt), idx );
}


uiSize uiListBox::minimumSize() const
{  
    if( !font() ) { pErrMsg("No font!"); return uiSize(); }

    int totHeight = font()->height() * nLines;
    int totWidth  = font()->maxWidth() * fieldWdt;

    return uiSize ( totWidth , totHeight );
}

//-----------------------------------------------------------------------------
//  Documentation 
//-----------------------------------------------------------------------------

/*!
    \fn int uiListBox::lastSelected() 

    \return index of last selected item.
    \retval  0 when no selection has taken place yet.
    \retval -1 when the selection set of a multi ListBox was changed.
*/


/*! \fn QSize i_QListBox::sizeHint() const
    \reimp
    Computes preferred size by looking at the items in the list.
    If the uiClient object has a fieldWdt and/or a nLines value set, 
    then that overrides.
*/
