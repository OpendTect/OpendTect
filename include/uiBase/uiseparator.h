#ifndef uiseparator_H
#define uiseparator_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiseparator.h,v 1.6 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

//class QFrame;

class uiSeparatorBody;

mClass uiSeparator : public uiObject
{
public:

                        uiSeparator(uiParent*,const char* nm="Separator", 
				    bool hor=true, bool raised=true);


    void		setRaised(bool);

private:

    uiSeparatorBody*	body_;
    uiSeparatorBody&	mkbody(uiParent*, const char*, bool, bool);

};

#endif
