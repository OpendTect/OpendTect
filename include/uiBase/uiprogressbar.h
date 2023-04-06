#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiProgressBar();
			mOD_DisableCopy(uiProgressBar)

    void		setProgress(int);
    int			progress() const;
    void		setTotalSteps(int);
    int			totalSteps() const;

private:

    uiProgressBarBody*	body_;
    uiProgressBarBody&	mkbody(uiParent*,const char*);
};
