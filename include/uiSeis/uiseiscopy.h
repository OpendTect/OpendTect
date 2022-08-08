#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2014
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class IOObj;
class uiSeisSel;
class uiScaler;
class uiSeis2DMultiLineSel;
class uiLabeledComboBox;
class uiSeisTransfer;
class uiBatchJobDispatcherSel;


/*!\brief UI for copying cubes */

mExpClass(uiSeis) uiSeisCopyCube : public uiDialog
{ mODTextTranslationClass(uiSeisCopyCube);
public:

			uiSeisCopyCube(uiParent*,const IOObj*);

protected:

    uiSeisSel*		inpfld_;
    uiLabeledComboBox*	compfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		outfld_;
    uiBatchJobDispatcherSel* batchfld_;

    bool		ismc_;

    void		inpSel(CallBacker*);

    bool		acceptOK(CallBacker*) override;

};


/*!\brief UI for copying 2d Datasets */

mExpClass(uiSeis) uiSeisCopy2DDataSet : public uiDialog
{ mODTextTranslationClass(uiSeisCopy2DDataSet)
public:

			uiSeisCopy2DDataSet(uiParent*,const IOObj*,
					    const char* fixedouttransl=0);
protected:

    uiSeisSel*		inpfld_;
    uiSeis2DMultiLineSel* subselfld_;
    uiScaler*		scalefld_;
    uiSeisSel*		outpfld_;
    uiBatchJobDispatcherSel* batchfld_;

    void		inpSel(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};


