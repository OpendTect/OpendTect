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
#include "synthseisgenparams.h"

class uiButton;
class uiComboBox;
class uiGenInput;
class uiRayTracerSel;
class uiWaveletIOObjSel;
class uiSynthCorrectionsGrp;
class uiSpinBox;
namespace StratSynth { class DataMgr; }


mExpClass(uiWellAttrib) uiSynthGenParams : public uiGroup
{ mODTextTranslationClass(uiSynthGenParams);
public:

    typedef SynthSeis::GenParams	GenParams;
    typedef SynthSeis::SyntheticType	SynthType;
    typedef StratSynth::DataMgr		DataMgr;

				uiSynthGenParams(uiParent*,const DataMgr&);
				~uiSynthGenParams();

    void			get(GenParams&) const;
    void			set(const GenParams&);
    void			setByName(const char*);

    Notifier<uiSynthGenParams>	nameChanged;
    BufferString		getName() const;

protected:

    const DataMgr&		mgr_;
    TypeSet<SynthType>		synthtypes_;

    uiGroup*			zeroofssgrp_;
    uiGroup*			prestackgrp_;
    uiGroup*			pspostprocgrp_;

    uiComboBox*			typefld_;
    uiGenInput*			namefld_;
    uiWaveletIOObjSel*		wvltfld_;
    uiButton*			wvltscalebut_;
    uiGenInput*			dointernalmultiplesfld_;
    uiSpinBox*			surfreflcoeffld_;
    uiRayTracerSel*		rtsel_;
    uiSynthCorrectionsGrp*	uisynthcorrgrp_;
    uiComboBox*			psinpfld_;
    uiGenInput*			angleinpfld_;

    uiGroup*			createGroups();
    void			fillTypeFld();
    void			updUi();
    void			updWvltScaleFldDisp();
    SynthType			typeFromFld() const;
    void			typeToFld(SynthType);

    void			initWin(CallBacker*);
    void			typeChgCB(CallBacker*);
    void			nameChgCB(CallBacker*);
    void			waveletSelCB(CallBacker*);
    void			scaleWvltCB(CallBacker*);

};
