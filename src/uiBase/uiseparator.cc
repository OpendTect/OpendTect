/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include "uiseparator.h"
#include "uiobjbody.h"

#ifdef USEQT4
#define mQFrame Q3Frame
#include <Q3Frame>
#else
#define mQFrame QFrame
#include <qframe.h>
#endif


class uiSeparatorBody : public uiObjBodyImpl<uiSeparator,mQFrame>
{
public:
                        uiSeparatorBody(uiSeparator& handle, uiParent* p,
					const char* nm, bool hor, bool raised)
			    : uiObjBodyImpl<uiSeparator,mQFrame>(handle,p,nm)
			    {
				int style = hor ? mQFrame::HLine 
				    		: mQFrame::VLine;
				style    |= raised ? mQFrame::Raised 
						   : mQFrame::Sunken;

				setFrameStyle( style );
			    }
};


uiSeparator::uiSeparator( uiParent* p, const char* txt, bool hor, bool raised )
    : uiObject(p,txt, mkbody(p,txt,hor,raised) )
{}

uiSeparatorBody& uiSeparator::mkbody( uiParent* p, const char* txt, 
				      bool hor, bool raised )
{ 
    body_= new uiSeparatorBody(*this,p,txt,hor,raised);
    return *body_; 
}


void uiSeparator::setRaised( bool yn)
{ 
    body_->setFrameShadow( yn ? mQFrame::Raised : mQFrame::Sunken );
} 

