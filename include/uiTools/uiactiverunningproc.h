#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "bufstring.h"
#include "callback.h"
#include "uistring.h"

class uiActiveRunningProcDlg;
class uiParent;
class ActiveRunningProcData;




mExpClass(uiTools) ActiveProcPrompter : public CallBacker
{ mODTextTranslationClass(NotClosedPrompter);
public:
					~ActiveProcPrompter();
    static  ActiveProcPrompter&		APP(); //gives instance

    bool				doTrigger(uiParent*);

protected:
					ActiveProcPrompter();
};
