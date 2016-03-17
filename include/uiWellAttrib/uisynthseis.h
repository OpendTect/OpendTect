#ifndef uisynthseis_h
#define uisynthseis_h

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

    void			usePar(const IOPar&);
    void			fillPar(IOPar&) const;
    void			setWavelet(const char* wvltnm);
    const char*			getWaveletName() const;
    void			updateDisplayForPSBased();
    void			setRayTracerType(const char*);
    void			updateFieldDisplay();
    Notifier<uiSynthSeisGrp>	parsChanged;

protected:

    uiSeisWaveletSel*		wvltfld_;
    uiRayTracerSel*		rtsel_;
    uiSynthCorrectionsGrp*	uisynthcorrgrp_;
    uiCheckBox*			internalmultiplebox_;
    uiLabeledSpinBox*		surfreflcoeffld_;

    void			parsChangedCB(CallBacker*);
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

    Notifier<uiSynthCorrectionsGrp> nmoparsChanged_;

protected:
    uiGenInput*			nmofld_;
    uiPushButton*		advbut_;
    uiSynthCorrAdvancedDlg*	uiscadvdlg_;

    void			getAdvancedPush(CallBacker*);
    void			parsChanged(CallBacker*);
};

#endif
