#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uimarkerstyledlg.h"
#include "uisellinest.h"
#include "uicolor.h"

class uiCheckBox;
class uiGenInput;
class uiSlider;

namespace Pick { class Set; };
namespace visSurvey
{ class PickSetDisplay; class LocationDisplay; class SeedPainter; };


mExpClass(uiVis) uiPickPropDlg : public uiMarkerStyleDlg
{ mODTextTranslationClass(uiPickPropDlg);
public:
				uiPickPropDlg(uiParent* p,
					      Pick::Set& set,
					      visSurvey::PickSetDisplay* psd);
				~uiPickPropDlg();

protected:

    void			doFinalize(CallBacker*) override;
    void			sliderMove(CallBacker*) override;
    void			typeSel(CallBacker*) override;
    void			colSel(CallBacker*) override;
    bool			acceptOK(CallBacker*) override;
    void			fillColorChangeCB(CallBacker*);
    void			linePropertyChanged(CallBacker*);
    void			useThresholdCB(CallBacker*);
    void			thresholdChangeCB(CallBacker*);
    void			initDlg(CallBacker*);

    uiCheckBox*			usethresholdfld_;
    uiGenInput*			thresholdfld_;
    uiSelLineStyle*		lsfld_		= nullptr;
    uiColorInput*		fillcolfld_	= nullptr;

    RefMan<Pick::Set>		set_;
    visSurvey::PickSetDisplay*	psd_;
};


mExpClass(uiVis) uiSeedPainterDlg : public uiDialog
{ mODTextTranslationClass(uiSeedPainterDlg);
public:
				uiSeedPainterDlg(uiParent* p,
						 visSurvey::LocationDisplay*);
				~uiSeedPainterDlg();

    void			refresh();

protected:

    void			sizeCB(CallBacker*);
    void			densCB(CallBacker*);

    visSurvey::SeedPainter*	seedpainter_;

    uiSlider*			szfld_;
    uiSlider*			densfld_;

};
