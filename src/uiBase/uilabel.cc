/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/


#include <uilabel.h>
#include <qlabel.h> 
#include <i_qobjwrap.h>


uiLabel::uiLabel( uiObject* p, const char* txt, uiObject* buddy )
	: uiWrapObj<i_QLabel>(new i_QLabel( *this, p, "Label" ), p,txt)
{
    mQtThing()->setText( txt );
    if ( buddy ) 
    {
	mQtThing()->setBuddy( &buddy->qWidget() );
	buddy->attach( rightOf, this );
    }
    setStretch( 0, 0 );
}

const QWidget* 	uiLabel::qWidget_() const 	{ return mQtThing(); } 
