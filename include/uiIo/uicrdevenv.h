#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiGenInput;
class uiFileInput;

mExpClass(uiIo) uiCrDevEnv : public uiDialog
{ mODTextTranslationClass(uiCrDevEnv);
public:

    static void		crDevEnv(uiParent*);		
    static bool		isOK(const char* dir=0); //!< default dir: $WORK

protected:
			uiCrDevEnv(uiParent*,const char*,const char*);

    uiGenInput*		workdirfld;
    uiFileInput*	basedirfld;

    bool		acceptOK(CallBacker*) override;
};
