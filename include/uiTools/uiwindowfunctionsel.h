#ifndef uiwindowfunctionsel_h
#define uiwindowfunctionsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.h,v 1.6 2009-10-14 14:37:32 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class WindowFunction;
class uiGenInput;
class uiWindowFuncSelDlg;
class uiFreqTaperDlg;

/*!Selects a windowfunction and its eventual parameter. */

mClass uiWindowFunctionSel : public uiGroup
{
public:

    mStruct Setup
    {
			Setup() 
			    : isminfreq_(false)   
			    , ismaxfreq_(false)   
			    , winparam_(mUdf(float))  
			    {}

	mDefSetupMemb(const char*,winname )
	mDefSetupMemb(const char*,label)
	mDefSetupMemb(float,winparam)
	mDefSetupMemb(bool,ismaxfreq)
	mDefSetupMemb(bool,isminfreq)
    };

    				uiWindowFunctionSel(uiParent*,const Setup&);
    				~uiWindowFunctionSel();

    NotifierAccess&		typeChange();

    void			setWindowName(const char*);
    void			setWindowParamValue(float);

    const char*			windowName() const;
    float			windowParamValue() const;
    const char*			windowParamName() const;

    static const char*		sNone() { return "None"; }

protected:

    void			windowChangedCB(CallBacker*);
    virtual void		winfuncseldlgCB(CallBacker*);
    void			windowClosed(CallBacker*);

    BufferString		errmsg_;
    Interval<float>		annotrange_;

    uiGenInput*			windowtypefld_;
    uiWindowFuncSelDlg*		winfuncseldlg_;
    ObjectSet<uiGenInput>	windowvariable_;
    ObjectSet<WindowFunction>	windowfuncs_;
};


mClass uiFreqTaperSel : public uiWindowFunctionSel
{
public:
    				uiFreqTaperSel(uiParent*,const Setup&);

    Interval<float> 		freqrg_;
    void 			setIsMinMaxFreq(bool min, bool max)
    				{ isminfreq_ = min; ismaxfreq_ = max; }

protected :

    bool			isminfreq_;
    bool			ismaxfreq_;
    uiFreqTaperDlg*		freqtaperdlg_;

    void			winfuncseldlgCB(CallBacker*);
    void			windowClosed(CallBacker*);
};


#endif
