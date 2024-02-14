#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "filepath.h"
#include "uidialog.h"

class uiGenInput;
class uiFileInput;

mExpClass(uiIo) uiCrDevEnv : public uiDialog
{ mODTextTranslationClass(uiCrDevEnv)
public:
			~uiCrDevEnv();

    FilePath		getWorkDir() const;

private:
			uiCrDevEnv(uiParent*,const FilePath&);

    uiGenInput*		workdirfld_;
    uiFileInput*	basedirfld_;

    bool		acceptOK(CallBacker*) override;

    static uiRetVal	copyEnv(const char* swdir,const char* envdir);

public:

    static uiRetVal	canCreateDevEnv();
    static void		crDevEnv(uiParent*);
};
