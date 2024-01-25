#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "uidialog.h"

class IOObj;
class uiBatchJobDispatcherSel;
class uiLabeledComboBox;
class uiMultiZSeisSubSel;
class uiScaler;
class uiSeisSel;
class uiSeisTransfer;


/*!\brief UI for copying cubes */

mExpClass(uiSeis) uiSeisCopyCube : public uiDialog
{ mODTextTranslationClass(uiSeisCopyCube);
public:

			uiSeisCopyCube(uiParent*,const IOObj*);
			~uiSeisCopyCube();

protected:

    uiSeisSel*		inpfld_;
    uiLabeledComboBox*	compfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		outfld_;
    uiBatchJobDispatcherSel* batchfld_;

    bool		ismc_;

    void		initDlgCB(CallBacker*);
    void		inpSelCB(CallBacker*);
    void		compSel(CallBacker*);

    bool		acceptOK(CallBacker*) override;

};


/*!\brief UI for copying 2d Datasets */

mExpClass(uiSeis) uiSeisCopy2DDataSet : public uiDialog
{ mODTextTranslationClass(uiSeisCopy2DDataSet)
public:
			uiSeisCopy2DDataSet(uiParent*,const IOObj*,
					    const char* fixedouttransl=nullptr);
			~uiSeisCopy2DDataSet();
protected:

    uiSeisSel*		inpfld_;
    uiMultiZSeisSubSel* subselfld_;
    uiScaler*		scalefld_;
    uiSeisSel*		outpfld_;
    uiBatchJobDispatcherSel* batchfld_;

    void		initDlgCB(CallBacker*);
    void		inpSelCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
