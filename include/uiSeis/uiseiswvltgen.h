#ifndef uiseiswvltgen_h
#define uiseiswvltgen_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltgen.h,v 1.6 2009-09-18 16:10:09 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "bufstringset.h"
#include "mathfunc.h"

class CtxtIOObj;
class Wavelet;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiFuncSelDraw;


mClass uiSeisWvltCreate : public uiDialog
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


mClass uiSeisWvltGen : public uiSeisWvltCreate
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


mClass uiSeisWvltMerge : public uiSeisWvltCreate
{
public:

    mClass WvltMathFunction : public FloatMathFunction
    {
    public:
			WvltMathFunction(const Wavelet*);

	StepInterval<float> samppos_;
	int 		size_;
	const float*    samples_;
	float 		getValue(float) const;
	float 		getIntValue(float) const;
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
    uiComboBox*		centerchoicefld_;

    void 		constructDrawer(bool);
    void 		clearStackedWvlt(uiFuncSelDraw*);   
    uiFuncSelDraw* 	getCurrentDrawer(); 
    void 		makeStackedWvlt();    
    void 		reloadWvlts();
    void 		reloadFunctions();

    bool		acceptOK(CallBacker*);
    void 		funcSelChg(CallBacker*);    
    void 		reloadAll(CallBacker*);
};



#endif
