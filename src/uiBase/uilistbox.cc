/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.2 2001-01-24 12:58:50 arend Exp $
________________________________________________________________________

-*/

#include <uilistbox.h>
#include <uifont.h>

#include <i_qlistbox.h>
#include <i_qobjwrap.h>

#include <qsize.h> 

typedef i_QObjWrapper<QListBox> i_QListBoxBase;

class i_QListBox : public i_QListBoxBase
//!< Derived QListBox, only to override sizeHint()
{
public:
          
			i_QListBox( uiObject& client,
				    uiObject* parnt=0, const char* name=0 )
			: i_QListBoxBase( client, parnt, name ) {}

        virtual QSize   sizeHint() const;
//        virtual QSize   minimumSize() const;
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

//------------------------------------------------------------------------------

uiListBox::uiListBox(  uiObject* parnt, const char* nm, bool isMultiSelect,
		       int preferredNrLines, int preferredFieldWidth )
: uiWrapObj<i_QListBox>( new i_QListBox( *this, parnt, nm ), parnt, nm )
, _messenger ( *new i_listMessenger( mQtThing(), this ))
, fieldWdt( preferredFieldWidth )
, nLines( preferredNrLines )
, cur_id( -1 )
{
    if( isMultiSelect ) mQtThing()->setSelectionMode( QListBox::Extended );
    setStretch( 1, isSingleLine() ? 0 : 1 );
}

uiListBox::~uiListBox() {} // mQtThing is deleted by Qt.

const QWidget* 	uiListBox::qWidget_() const 	{ return mQtThing(); } 

bool uiListBox::isSelected ( int index ) const
{
    return mQtThing()->isSelected( index );
}

int uiListBox::insertItem( const char* text ) 
//!< \return index of new inserted item. 
{ 
//    QListBoxItem* item = new QListBoxText ( QString( text ));
    int id = getNewId();
//    mQtThing->insertItem( item, id );
    mQtThing()->insertItem( QString( text ) , id );
    return id;
}


int uiListBox::insertItems( const char** textList ) 
//!< last element in textList array must be NULL pointer. 
//!< \return index of first inserted item. 
{   int first_id = 0;
    int i = 0;
    const char* pt_cur = textList[ 0 ];
    if( pt_cur )
    {  
        first_id = insertItem( pt_cur );
        pt_cur = textList[ ++i ];

	while( pt_cur ) 
	{
	    insertItem( pt_cur );
	    pt_cur = textList[ ++i ]; 
	}
    }
    return first_id;
}

uiSize uiListBox::minimumSize() const
{  
    if( !font() ) { pErrMsg("No font!"); return uiSize(); }

//    int fieldWdt = 3;
//    int nLines = 1;

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
