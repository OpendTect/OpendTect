#ifndef uiprogressbar_H
#define uiprogressbar_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id: uiprogressbar.h,v 1.6 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>

class uiProgressBarBody;

class uiProgressBar : public uiObject
{
public:

                        uiProgressBar( uiParent*,const char* nm="ProgressBar", 
				       int totalSteps=100, int progress=0);

    void		setProgress(int);
    int			Progress() const;
    void		setTotalSteps(int);
    int			totalSteps() const;

private:

    uiProgressBarBody*	body_;
    uiProgressBarBody&	mkbody(uiParent*, const char*);
};

#endif
