/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.cc,v 1.2 2001-01-24 12:58:47 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
#include <qwidget.h>
#include <i_qobjwrap.h>
#include <uimainwin.h>

#include <iostream.h>
#include "errh.h"

uiGroup::uiGroup( uiObject* parnt, const char* nm, int border, int spacing)
: uiWrapObj<i_QWidget>( new i_QWidget ( *this, parnt, nm ), parnt, nm, true )
, loMngr( *new i_LayoutMngr( this,  border, spacing )) 
, hAlignObj( 0 ), hCentreObj( 0 )
{}

/*!
     Avoids referring to uiMainWin or uiDialog's mCentralWidget 
     trough calling of clientQWidget() on prnt in construction of i_QWidget.
*/
uiGroup::uiGroup( const char* nm, uiObject* parnt, int spacing, int border )
: uiWrapObj<i_QWidget>( new i_QWidget(*this,parnt?&parnt->qWidget():0,nm), 
	  parnt, nm, false )
, loMngr( *new i_LayoutMngr( this , border, spacing )) 
, hAlignObj( 0 ), hCentreObj( 0 )
{}


uiGroup::~uiGroup( )
{
       delete &loMngr;
       //delete &mQtThing; done by Qt!!
}


const QWidget* 	uiGroup::qWidget_() const 	{ return mQtThing(); } 

void uiGroup::setSpacing( int space )
{ 
    loMngr.setSpacing( space ); 
}


void uiGroup::setBorder( int b )
{ 
    loMngr.setMargin( b ); 
}


void uiGroup::forceRedraw_( bool deep )
{  
    uiObject::forceRedraw_( deep ); // calls qWidget().update()
    loMngr.forceChildrenRedraw( this, deep ); 
}


int uiGroup::horCentre() const
{
    mChkmLayout();
    int offs = mLayoutItm->loMngr().pos().left() + mLayoutItm->pos().left();
    int border = loMngr.borderSpace();

    if( hCentreObj ) return hCentreObj->horCentre() + offs + border;

    return (mLayoutItm->loMngr().pos().left() + 
            mLayoutItm->loMngr().pos().right()  ) / 2;
}

int uiGroup::horAlign() const
{
    mChkmLayout();
    int offs = mLayoutItm->loMngr().pos().left() + mLayoutItm->pos().left();
    int border = loMngr.borderSpace();

    if( hAlignObj ) return hAlignObj->horAlign() + offs + border;
    return offs;
}
