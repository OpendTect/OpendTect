#ifndef uiprogressbar_h
#define uiprogressbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiprogressbar.h,v 1.9 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiProgressBarBody;

mClass uiProgressBar : public uiObject
{
public:

                        uiProgressBar(uiParent*,const char* nm="ProgressBar", 
				      int totalSteps=100,int progress=0);

    void		setProgress(int);
    int			progress() const;
    void		setTotalSteps(int);
    int			totalSteps() const;

private:

    uiProgressBarBody*	body_;
    uiProgressBarBody&	mkbody(uiParent*,const char*);
};

#endif
