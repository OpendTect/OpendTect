#ifndef uisrchprocfiles_h
#define uisrchprocfiles_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Feb 2006
 RCS:           $Id: uisrchprocfiles.h,v 1.1 2006-02-28 16:33:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiIOObjSel;
class uiFileInput;
class CtxtIOObj;

/*!\brief Finds created objects in processing specification files */

class uiSrchProcFiles : public uiDialog
{
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
    bool		acceptOK(CallBacker*);
};


#endif
