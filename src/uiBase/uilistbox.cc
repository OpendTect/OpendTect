/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.4 2001-04-27 16:48:10 bert Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>
#include <uifont.h>
#include <uidset.h>

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
    int fieldWdt = 0;
    int nLines = 0;
    
    const uiListBox* ptClient = dynamic_cast<const uiListBox*> ( uiClient() );
    if( ptClient )
    {
	fieldWdt = ptClient->fieldWdt;
	nLines = ptClient->nLines;
    }

    const uiFont* mFont = uiClient()->font();
    if( !mFont ) { pErrMsg("uiClient has no font!"); return QSize(); }


    int totHeight= mFont->height() * nLines + 2*frameWidth();
    int totWidth = mFont->maxWidth() * fieldWdt + 2*frameWidth();

    if ( !(fieldWdt && nLines) )
    {
	int i = 0;
	QListBoxItem* itm = item (i);
	while (itm)
	{   
	    if ( !fieldWdt )
	    {
		const char* str = itm->text();
		if ( str )
		{
		    int w = mFont->width( str )
			+ 2 * mFont->maxWidth();
		    totWidth = QMAX ( totWidth, w);
		}
	    }
	    if ( !nLines ) 
	    {
		totHeight += mFont->height();
	    } 
	    i++;
	    itm = item (i);
	}
    }
    return QSize ( totWidth , totHeight );
}


uiListBox::uiListBox(  uiObject* parnt, const char* nm, bool isMultiSelect,
		       int preferredNrLines, int preferredFieldWidth )
	: uiWrapObj<i_QListBox>( new i_QListBox( *this, parnt, nm ), parnt, nm )
	, _messenger ( *new i_listMessenger( mQtThing(), this ))
	, fieldWdt( preferredFieldWidth )
	, nLines( preferredNrLines )
{
    if( isMultiSelect ) mQtThing()->setSelectionMode( QListBox::Extended );
    setStretch( 1, isSingleLine() ? 0 : 1 );
}


uiListBox::uiListBox( uiObject* p, const UserIDSet& uids,
			bool isMultiSelect,
			int preferredNrLines, int preferredFieldWidth )
	: uiWrapObj<i_QListBox>(
		new i_QListBox( *this,p,(const char*)uids.name() ),
				      p,(const char*)uids.name() )
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


void uiListBox::addItems( const UserIDSet& uids )
{
    for ( int idx=0; idx<uids.size(); idx++ )
	mQtThing()->insertItem( QString( uids[idx]->name() ), -1 );
}


void uiListBox::clear()
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
