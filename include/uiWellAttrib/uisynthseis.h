#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiraytrace1d.h"
#include "uigroup.h"
#include "uistring.h"

class uiCheckBox;
class uiGenInput;
class uiLabeledSpinBox;
class uiPushButton;
class uiRayTracerSel;
class uiSeisWaveletSel;
class uiSynthCorrectionsGrp;

mExpClass(uiWellAttrib) uiSynthSeisGrp : public uiGroup
{ mODTextTranslationClass(uiSynthSeisGrp);
public:
				uiSynthSeisGrp(uiParent*,
					       const uiRayTracer1D::Setup&);
				~uiSynthSeisGrp();

    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;
    void			setWavelet(const char* wvltnm);
    const char*			getWaveletName() const;
    void			setRayTracerType(const char*);

    Notifier<uiSynthSeisGrp>	parsChanged;

private:

    uiSeisWaveletSel*		wvltfld_;
    uiRayTracerSel*		rtsel_;
    uiSynthCorrectionsGrp*	uisynthcorrgrp_;
    uiCheckBox*			internalmultiplebox_ = nullptr;
    uiLabeledSpinBox*		surfreflcoeffld_ = nullptr;

    void			parsChangedCB(CallBacker*);
    void			updateFieldDisplay();
};


class uiSynthCorrAdvancedDlg;

mExpClass(uiWellAttrib) uiSynthCorrectionsGrp : public uiGroup
{ mODTextTranslationClass(uiSynthCorrectionsGrp);
public:
				uiSynthCorrectionsGrp(uiParent*);
				~uiSynthCorrectionsGrp();

    bool			wantNMOCorr() const;
    float			getStrechtMutePerc() const;
    float			getMuteLength() const;
    void			setValues(bool,float mutelen,float stretchlim);

    Notifier<uiSynthCorrectionsGrp> nmoparsChanged;

protected:
    uiGenInput*			nmofld_;
    uiPushButton*		advbut_;
    uiSynthCorrAdvancedDlg*	uiscadvdlg_;

    void			initGrp(CallBacker*);
    void			getAdvancedPush(CallBacker*);
    void			nmoSelCB(CallBacker*);
    void			parsChanged(CallBacker*);
};

