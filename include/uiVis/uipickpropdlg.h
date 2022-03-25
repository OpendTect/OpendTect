#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
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

    void			doFinalize(CallBacker*);
    void			sliderMove(CallBacker*);
    void			typeSel(CallBacker*);
    void			colSel(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			fillColorChangeCB(CallBacker*);
    void			linePropertyChanged(CallBacker*);
    void			useThresholdCB(CallBacker*);
    void			thresholdChangeCB(CallBacker*);
    void			initDlg(CallBacker*);

    uiCheckBox*			usethresholdfld_;
    uiGenInput*			thresholdfld_;
    uiSelLineStyle*		lsfld_		= nullptr;
    uiColorInput*		fillcolfld_	= nullptr;

    Pick::Set&			set_;
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
