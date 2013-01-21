#ifndef uiseiswvltgen_h
#define uiseiswvltgen_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"
#include "bufstringset.h"
#include "mathfunc.h"

class CtxtIOObj;
class Wavelet;
class uiCheckBox;
class uiLabeledComboBox;
class uiGenInput;
class uiIOObjSel;
class uiFuncSelDraw;


mExpClass(uiSeis) uiSeisWvltCreate : public uiDialog
{
public:
			uiSeisWvltCreate(uiParent*,uiDialog::Setup);
			~uiSeisWvltCreate();

    MultiID		storeKey() const;

protected:

    bool		putWvlt(const Wavelet&);

    CtxtIOObj&		ctio_;
    uiIOObjSel*		wvltfld_;
};


mExpClass(uiSeis) uiSeisWvltGen : public uiSeisWvltCreate
{
public:
			uiSeisWvltGen(uiParent*);
			~uiSeisWvltGen(){};

protected:

    uiGenInput*		isrickfld_;
    uiGenInput*		freqfld_;
    uiGenInput*		srfld_;
    uiGenInput*		peakamplfld_;
    
    bool		acceptOK(CallBacker*);
};


mExpClass(uiSeis) uiSeisWvltMerge : public uiSeisWvltCreate
{
public:

    mExpClass(uiSeis) WvltMathFunction : public FloatMathFunction
    {
    public:
			WvltMathFunction(const Wavelet*);

	StepInterval<float> samppos_;
	int 		size_;
	const float*    samples_;
	float 		getValue(float) const;
	float 		getIntValue(float) const;
	virtual float 	getValue( const float* p ) const
	    		{ return getValue(*p); }
    };

			uiSeisWvltMerge(uiParent*,const char* curwvltnm=0);
			~uiSeisWvltMerge();

protected:

    BufferString 	curwvltnm_;
    int 		maxwvltsize_;			
    BufferStringSet	namelist_;
    Wavelet*		stackedwvlt_;
    ObjectSet<WvltMathFunction> wvltfuncset_;
    ObjectSet<uiFuncSelDraw> wvltdrawer_;
    ObjectSet<Wavelet>  wvltset_;

    uiCheckBox*		normalizefld_;
    uiCheckBox*		centerfld_;
    uiLabeledComboBox*	centerchoicefld_;

    void 		constructDrawer(bool);
    void 		clearStackedWvlt(uiFuncSelDraw*);   
    uiFuncSelDraw* 	getCurrentDrawer(); 
    void 		centerToMaxEnergyPos(Wavelet&); 
    void 		centerToMaxAmplPos(Wavelet&); 
    void 		makeStackedWvlt();    
    void 		reloadWvlts();
    void 		reloadFunctions();

    bool		acceptOK(CallBacker*);
    void 		centerChged(CallBacker*);
    void 		funcSelChg(CallBacker*);    
    void 		reloadAll(CallBacker*);
};



#endif

