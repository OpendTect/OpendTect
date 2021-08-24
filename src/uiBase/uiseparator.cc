/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include "uiseparator.h"
#include "uiobjbodyimpl.h"

#include <QFrame>

mUseQtnamespace

class uiSeparatorBody : public uiObjBodyImpl<uiSeparator,QFrame>
{
public:

uiSeparatorBody( uiSeparator& hndl, uiParent* p, const char* nm,
		 bool hor, bool raised )
    : uiObjBodyImpl<uiSeparator,QFrame>(hndl,p,nm)
{
    setFrameStyle( (hor ? QFrame::HLine : QFrame::VLine)
		 | (raised ? QFrame::Raised
			   : QFrame::Sunken) );
    setLineWidth( 1 ); setMidLineWidth( 0 );
}

};


uiSeparator::uiSeparator( uiParent* p, const char* txt,
			OD::Orientation ori, bool raised )
    : uiObject(p,txt,mkbody(p,txt,ori==OD::Horizontal,raised))
{
}


uiSeparatorBody& uiSeparator::mkbody( uiParent* p, const char* txt,
				      bool hor, bool raised )
{
    body_= new uiSeparatorBody(*this,p,txt,hor,raised);
    return *body_;
}


void uiSeparator::setRaised( bool yn )
{
    body_->setFrameShadow( yn ? QFrame::Raised
			      : QFrame::Sunken );
}
