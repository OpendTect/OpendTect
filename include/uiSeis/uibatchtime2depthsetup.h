#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
________________________________________________________________________

-*/

#include "uidialog.h"

class uiBatchJobDispatcherSel;
class uiGenInput;
class uiZAxisTransformSel;
class uiSeisSel;
class uiSeisSubSel;

/*!Dialog to setup a time->depth conversion for volumes on disk. */

mClass(uiSeis) uiBatchTime2DepthSetup : public uiDialog
{ mODTextTranslationClass(uiBatchTime2DepthSetup)
public:
				uiBatchTime2DepthSetup(uiParent*,bool is2d);
				~uiBatchTime2DepthSetup();

protected:

    bool			fillPar();
    bool			prepareProcessing();
    void			dirChangeCB(CallBacker*);
    void			objSelCB(CallBacker*);
    bool			acceptOK();

    uiGenInput*			directionsel_;

    uiZAxisTransformSel*	t2dfld_;
    uiZAxisTransformSel*	d2tfld_;

    uiSeisSel*			inputtimesel_;
    uiSeisSel*			inputdepthsel_;

    uiSeisSubSel*		subselfld_;

    uiSeisSel*			outputtimesel_;
    uiSeisSel*			outputdepthsel_;
    uiBatchJobDispatcherSel*	batchfld_;
};
