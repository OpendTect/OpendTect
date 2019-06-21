#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"

#include "uivolprocstepdlg.h"
#include "wellloginterpolator.h"

class uiGridder2DSel;
class uiInterpolationLayerModel;
class uiMultiWellLogSel;

namespace VolProc
{

mExpClass(uiVolumeProcessing) uiWellLogInterpolator : public uiStepDialog
{ mODTextTranslationClass(uiWellLogInterpolator)
public:
    mDefaultFactoryInstantiationBase(
	WellLogInterpolator::sFactoryKeyword(),
	WellLogInterpolator::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )

protected:
				uiWellLogInterpolator(uiParent*,
						WellLogInterpolator&,bool is2d);
				~uiWellLogInterpolator();

    static VolProc::uiStepDialog* createInstance(uiParent*,VolProc::Step*,bool);

    void			finaliseCB(CallBacker*);
    bool			acceptOK();
    void			initWellLogSel();

    WellLogInterpolator&	hwinterpolator_;

    uiInterpolationLayerModel*	layermodelfld_;
    uiGridder2DSel*		algosel_;
    uiMultiWellLogSel*		welllogsel_;
};

} // namespace VolProc
