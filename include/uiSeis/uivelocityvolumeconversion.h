#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2008
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

protected:

    void		inputChangeCB(CallBacker*);
    bool		fillPar();
    bool		acceptOK(CallBacker*) override;

    uiVelSel*		input_;
    uiPosSubSel*	possubsel_;
    uiLabeledComboBox*	outputveltype_;
    uiSeisSel*		outputsel_;
    uiBatchJobDispatcherSel*	batchfld_;
};

}; //namespace



