#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"
#include "bufstringset.h"
#include "mathfunc.h"
#include "uistring.h"

class Wavelet;
class uiCheckBox;
class uiLabeledComboBox;
class uiGenInput;
class uiIOObjSel;
class uiFuncSelDraw;


mExpClass(uiSeis) uiSeisWvltCreate : public uiDialog
{ mODTextTranslationClass(uiSeisWvltCreate);
public:
			uiSeisWvltCreate(uiParent*,uiDialog::Setup);
			~uiSeisWvltCreate();

    MultiID		storeKey() const;

protected:

    bool		putWvlt(const Wavelet&);

    uiIOObjSel*		wvltfld_;
};


mExpClass(uiSeis) uiSeisWvltGen : public uiSeisWvltCreate
{ mODTextTranslationClass(uiSeisWvltGen);
public:
			uiSeisWvltGen(uiParent*);
			~uiSeisWvltGen();

protected:

    uiGenInput*		isrickfld_;
    uiGenInput*		freqfld_;
    uiGenInput*		srfld_;
    uiGenInput*		peakamplfld_;

    bool		acceptOK(CallBacker*) override;
    void		initUI(CallBacker*);
    void		typeChgCB(CallBacker*);
};


mExpClass(uiSeis) uiSeisWvltMerge : public uiSeisWvltCreate
{ mODTextTranslationClass(uiSeisWvltMerge)
public:

    mExpClass(uiSeis) WvltMathFunction : public FloatMathFunction
    { mODTextTranslationClass(WvltMathFunction)
    public:
			WvltMathFunction(const Wavelet*);
			~WvltMathFunction();

	StepInterval<float> samppos_;
	int 		size_;
	const float*    samples_;
	float		getValue(float) const override;
	float 		getIntValue(float) const;
	virtual float 	getValue( const float* p ) const
	    		{ return getValue(*p); }
    };

			uiSeisWvltMerge(uiParent*,const char* curwvltnm=0);
			~uiSeisWvltMerge();

protected:

    BufferString 	curwvltnm_;
    int			maxwvltsize_;
    StepInterval<float> wvltsampling_;

    BufferStringSet	namelist_;
    Wavelet*		stackedwvlt_;
    ObjectSet<WvltMathFunction> wvltfuncset_;
    ObjectSet<uiFuncSelDraw> wvltdrawer_;
    ObjectSet<Wavelet>  wvltset_;

    uiCheckBox*		normalizefld_;
    uiCheckBox*		centerfld_;
    uiLabeledComboBox*	centerchoicefld_;

    void 		constructDrawer(bool);
    void		clearStackedWvlt(uiFuncSelDraw*);
    uiFuncSelDraw*	getCurrentDrawer();
    void		centerToMaxEnergyPos(Wavelet&);
    void		centerToMaxAmplPos(Wavelet&);
    void		makeStackedWvlt();
    void 		reloadWvlts();
    void 		reloadFunctions();

    bool		acceptOK(CallBacker*) override;
    void 		centerChged(CallBacker*);
    void		funcSelChg(CallBacker*);
    void 		reloadAll(CallBacker*);
};
