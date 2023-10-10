#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class uiBatchJobDispatcherSel;
class uiVelSel;
class uiSeisSel;
class uiPosSubSel;
class uiLabeledComboBox;

/*!\brief Velocity*/

namespace Vel
{

/*!Dialog to setup a velocity conversion for volumes on disk. */

mExpClass(uiSeis) uiBatchVolumeConversion : public uiDialog
{ mODTextTranslationClass(uiBatchVolumeConversion);
public:
			uiBatchVolumeConversion(uiParent*);
			~uiBatchVolumeConversion();

protected:

    void		initDlgCB(CallBacker*);
    void		inputChangeCB(CallBacker*);
    bool		fillPar();
    bool		acceptOK(CallBacker*) override;

    uiVelSel*		input_;
    uiPosSubSel*	possubsel_;
    uiLabeledComboBox*	outputveltype_;
    uiSeisSel*		outputsel_;
    uiBatchJobDispatcherSel*	batchfld_;
};

} // namespace Vel
