#ifndef uiprogressbar_h
#define uiprogressbar_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"

class uiProgressBarBody;

mExpClass(uiBase) uiProgressBar : public uiObject
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

