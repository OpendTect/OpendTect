#ifndef uiprogressbar_h
#define uiprogressbar_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiprogressbar.h,v 1.7 2007-05-14 06:55:01 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiProgressBarBody;

class uiProgressBar : public uiObject
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
