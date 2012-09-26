/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiseparator.h"
#include "uiobjbody.h"

#include <QFrame>


class uiSeparatorBody : public uiObjBodyImpl<uiSeparator,mQtclass(QFrame)>
{
public:

uiSeparatorBody( uiSeparator& hndl, uiParent* p, const char* nm,
		 bool hor, bool raised )
    : uiObjBodyImpl<uiSeparator,mQtclass(QFrame)>(hndl,p,nm)
{
    setFrameStyle( (hor ? mQtclass(QFrame)::HLine : mQtclass(QFrame)::VLine)
		 | (raised ? mQtclass(QFrame)::Raised
		     	   : mQtclass(QFrame)::Sunken) );
    setLineWidth( 1 ); setMidLineWidth( 0 );
}

};


uiSeparator::uiSeparator( uiParent* p, const char* txt, bool hor, bool raised )
    : uiObject(p,txt,mkbody(p,txt,hor,raised))
{}


uiSeparatorBody& uiSeparator::mkbody( uiParent* p, const char* txt, 
				      bool hor, bool raised )
{ 
    body_= new uiSeparatorBody(*this,p,txt,hor,raised);
    return *body_; 
}


void uiSeparator::setRaised( bool yn )
{ 
    body_->setFrameShadow( yn ? mQtclass(QFrame)::Raised
	    		      : mQtclass(QFrame)::Sunken );
}
