#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;

namespace VolProc
{

class ProcessingStep;
class ThresholdStep;

mClass(VolProcTest) uiVolumeThresholder : public uiDialog
{
public:
    static void		initClass();

			uiVolumeThresholder(uiParent*,ThresholdStep*);

protected:
    static uiDialog*	create(uiParent*, ProcessingStep*);
    bool		acceptOK(CallBacker*);

    ThresholdStep*	thresholdstep_;
    uiGenInput*		thresholdfld_;
};


} // namespace VolProc
