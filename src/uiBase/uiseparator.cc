/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include <uiseparator.h>
#include <qframe.h> 
#include <i_qobjwrap.h>


uiSeparator::uiSeparator( uiObject* p, const char* txt, bool hor, bool raised )
	: uiWrapObj<i_QFrame>(new i_QFrame( *this, p, txt ), p,txt)
{
    int style = hor ? QFrame::HLine : QFrame::VLine;
    style    |= raised ? QFrame::Raised : QFrame::Sunken;

//    mQtThing()->setFrameShape( hor ? QFrame::HLine : QFrame::VLine );
//    mQtThing()->setFrameShadow( raised ? QFrame::Raised : QFrame::Sunken );
    mQtThing()->setFrameStyle( style );
}

const QWidget* 	uiSeparator::qWidget_() const 	{ return mQtThing(); } 

void uiSeparator::setRaised( bool yn)
{ 
    mQtThing()-> setFrameShadow( yn ? QFrame::Raised : QFrame::Sunken );
} 

