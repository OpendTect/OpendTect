#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"

#include "uivolprocstepdlg.h"
#include "bufstringset.h"
#include "wellloginterpolator.h"

class uiGenInput;
class uiInterpolationLayerModel;
class uiMultiWellLogSel;

namespace VolProc
{

mExpClass(uiVolumeProcessing) uiWellLogInterpolator : public uiStepDialog
{ mODTextTranslationClass(uiWellLogInterpolator)
public:
    mDefaultFactoryInstanciationBase(
	WellLogInterpolator::sFactoryKeyword(),
	WellLogInterpolator::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )

				~uiWellLogInterpolator();

protected:
				uiWellLogInterpolator(uiParent*,
						WellLogInterpolator&,bool is2d);

    static VolProc::uiStepDialog* createInstance(uiParent*,VolProc::Step*,bool);

    void			finalizeCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    void			algoChg(CallBacker*);
    void			initWellLogSel();

    WellLogInterpolator&	hwinterpolator_;

    uiInterpolationLayerModel*	layermodelfld_;
    uiGenInput*			algosel_;
    uiGenInput*			radiusfld_;
    uiGenInput*			extensfld_;	//Will be removed after 6.2
    uiGenInput*			logextenfld_;	//Will be removed after 6.2
    uiMultiWellLogSel*		welllogsel_;
};

} // namespace VolProc
