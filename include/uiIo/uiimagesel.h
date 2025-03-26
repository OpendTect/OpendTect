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
    mExpClass(uiIo) Setup : public uiIOObjSel::Setup
    {
    public:
			Setup(const uiString& seltxt=uiString::empty());
			~Setup();

			mDefSetupMemb(bool,withimport)	//!<true
			mDefSetupMemb(bool,withedit)	//!<true
    };
			uiImageSel(uiParent*,bool forread,const Setup& ={});
			~uiImageSel();

private:

    void		doImportCB(CallBacker*);
    void		doEditCB(CallBacker*);
    void		importDoneCB(CallBacker*);
    void		editDoneCB(CallBacker*);
};


mExpClass(uiIo) uiImportImageDlg : public uiDialog
{ mODTextTranslationClass(uiImportImageDlg)
public:
			uiImportImageDlg(uiParent*);
			~uiImportImageDlg();

    CNotifier<uiImportImageDlg,const MultiID&> importDone;

private:

    void		finalizeCB(CallBacker*);
    void		fileSelectedCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiFileInput*	inputfld_;
    uiGenInput*		tlcrdfld_;
    uiGenInput*		brcrdfld_;
    uiImageSel*		outputfld_;
};


mExpClass(uiIo) uiImageCoordGrp : public uiGroup
{ mODTextTranslationClass(uiImageCoordGrp)
public:
			uiImageCoordGrp(uiParent*);
			~uiImageCoordGrp();

    void		fillCoords(const IOObj&);
    bool		saveCoords(const IOObj&);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

private:

    uiGenInput*		tlcrdfld_;
    uiGenInput*		brcrdfld_;
};


mExpClass(uiIo) uiEditImageDlg : public uiDialog
{ mODTextTranslationClass(uiEditImageDlg)
public:
			uiEditImageDlg(uiParent*,const IOObj&);
			~uiEditImageDlg();

    Notifier<uiEditImageDlg> editDone;

private:

    void		finalizeCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiImageCoordGrp*	imagegrp_;

    const IOObj&	ioobj_;
};
