#ifndef uiwindowfunctionsel_h
#define uiwindowfunctionsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.h,v 1.9 2009-11-16 17:08:49 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class WindowFunction;
class uiGenInput;
class uiPushButton;
class uiWindowFuncSelDlg;
class uiFreqTaperDlg;

/*!Selects a windowfunction and its eventual parameter. */

mClass uiWindowFunctionSel : public uiGroup
{
public:

    mStruct Setup
    {
			Setup() 
			    : onlytaper_(false)		      	
			    , ismaxfreq_(false)		
			    , isminfreq_(false)		
			    , winparam_(mUdf(float))  
			    {}

	mDefSetupMemb(const char*,winname )
	mDefSetupMemb(const char*,label)
	mDefSetupMemb(BufferString,inpfldtxt)
	mDefSetupMemb(float,winparam)
	mDefSetupMemb(bool,ismaxfreq)
	mDefSetupMemb(bool,isminfreq)
	mDefSetupMemb(bool,onlytaper)
    };

    				uiWindowFunctionSel(uiParent*,const Setup&);
    				~uiWindowFunctionSel();

    NotifierAccess&		typeChange();

    void			setWindowName(const char*);
    void			setWindowParamValue(float,int fldnr=0);

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

    int				taperidx_;
    bool			onlytaper_;
    uiGenInput*			windowtypefld_;
    uiGenInput*			varinpfld_;
    uiPushButton*		viewbut_;
    uiWindowFuncSelDlg*		winfuncseldlg_;
    ObjectSet<WindowFunction>	windowfuncs_;
};


mClass uiFreqTaperSel : public uiWindowFunctionSel
{
public:
    				uiFreqTaperSel(uiParent*,const Setup&);

    void 			setIsMinMaxFreq(bool,bool);
    void 			setFreqValue(float,int);
    Interval<float> 		freqValues() const;
    void 			setFreqSel(Interval<float>); 

protected :

    Interval<float> 		freqrg_;
    Interval<float> 		selfreqrg_;
    bool			isminfreq_;
    bool			ismaxfreq_;
    uiFreqTaperDlg*		freqtaperdlg_;

    void			winfuncseldlgCB(CallBacker*);
    void			windowClosed(CallBacker*);
    void			setFreqsSel(CallBacker*);
};


#endif
