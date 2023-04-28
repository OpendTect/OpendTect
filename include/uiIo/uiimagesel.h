#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"

#include "uidialog.h"
#include "uiioobjsel.h"

class uiFileInput;
class uiGenInput;


mExpClass(uiIo) uiImageSel : public uiIOObjSel
{ mODTextTranslationClass(uiImageSel)
public:
			uiImageSel(uiParent*,bool forread,const Setup& ={});
			~uiImageSel();

private:
    void		importCB(CallBacker*);
};


mExpClass(uiIo) uiImportImageDlg : public uiDialog
{ mODTextTranslationClass(uiImportImageDlg)
public:
			uiImportImageDlg(uiParent*);
			~uiImportImageDlg();

    MultiID		getKey() const;

private:
    void		finalizeCB(CallBacker*);
    void		fileSelectedCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiFileInput*	inputfld_;
    uiGenInput*		tlcrdfld_;
    uiGenInput*		brcrdfld_;
    uiImageSel*		outputfld_;
};
