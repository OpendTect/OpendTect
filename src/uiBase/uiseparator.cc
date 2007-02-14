/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:		$Id: uiseparator.cc,v 1.5 2007-02-14 12:38:00 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uiseparator.h"
#include "uiobjbody.h"

#ifdef USEQT3
# define mQFrame QFrame
# include <qframe.h>
#else
# define mQFrame Q3Frame
# include <Q3Frame>
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

