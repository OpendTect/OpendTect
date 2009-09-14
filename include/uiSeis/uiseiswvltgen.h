#ifndef uiseiswvltgen_h
#define uiseiswvltgen_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiseiswvltgen.h,v 1.4 2009-09-14 15:06:12 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"
#include "mathfunc.h"

class CtxtIOObj;
class Wavelet;
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

	int 		size_;
	const float*    samples_;
	float 		getValue() const;    
	
	float 		getValue(float) const;
    };

			uiSeisWvltMerge(uiParent*,const char* curwvltnm=0);
			~uiSeisWvltMerge();

protected:

    int 		maxwvltsize_;			
    uiFuncSelDraw* 	wvltdrawer_;
    Wavelet*		stackedwvlt_;
    ObjectSet<WvltMathFunction>  wvltfuncset_;
    ObjectSet<Wavelet>  wvltset_;

    void 		clearStackedWvlt();    
    void 		stackWvlts();    
    bool		acceptOK(CallBacker*);
    void 		funcSelChg(CallBacker*);    
};



#endif
