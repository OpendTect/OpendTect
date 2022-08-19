#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"

class uiBatchJobDispatcherSel;
class uiGenInput;
class uiZAxisTransformSel;
class uiSeisSel;
class uiSeis2DSubSel;
class uiPosSubSel;

/*!Dialog to setup a time->depth conversion for volumes on disk. */

mClass(uiSeis) uiBatchTime2DepthSetup : public uiDialog
{ mODTextTranslationClass(uiBatchTime2DepthSetup);
public:
				uiBatchTime2DepthSetup(uiParent*,
						       bool is2d=false);
				~uiBatchTime2DepthSetup();
protected:

    bool			fillPar();
    bool			prepareProcessing();
    void			dirChangeCB(CallBacker*);
    void			objSelCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    uiGenInput*			directionsel_;

    uiZAxisTransformSel*	t2dfld_;
    uiZAxisTransformSel*	d2tfld_;

    uiSeisSel*			inputtimesel_;
    uiSeisSel*			inputdepthsel_;
    uiSeis2DSubSel*     subselfld_ = nullptr;

    uiPosSubSel*		possubsel_;

    uiSeisSel*			outputtimesel_;
    uiSeisSel*			outputdepthsel_;
    uiBatchJobDispatcherSel*	batchfld_;
};
