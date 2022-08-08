#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Feb 2006
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class CtxtIOObj;

/*!\brief Finds created objects in processing specification files */

mExpClass(uiIo) uiSrchProcFiles : public uiDialog
{ mODTextTranslationClass(uiSrchProcFiles);
public:
			uiSrchProcFiles(uiParent*,CtxtIOObj&,
					const char* iopar_key);

    const char*		fileName() const;

protected:

    uiFileInput*	dirfld;
    uiGenInput*		maskfld;
    uiGenInput*		fnamefld;
    uiIOObjSel*		objfld;

    BufferString	iopkey_;
    CtxtIOObj&		ctio_;

    void		srchDir(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};


