#ifndef uiwindowfunctionsel_h
#define uiwindowfunctionsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.h,v 1.7 2009-10-15 15:27:40 cvsbruno Exp $
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

    int				taperidx_;
    bool			isfreq_;
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

    Interval<float> 		freqrg_;
    void 			setIsMinMaxFreq(bool,bool);

protected :

    bool			isminfreq_;
    bool			ismaxfreq_;
    uiFreqTaperDlg*		freqtaperdlg_;

    void			setWindowParamValues(Interval<float>);
    Interval<float>		windowParamValues() const;
    void			winfuncseldlgCB(CallBacker*);
    void			windowClosed(CallBacker*);
};


#endif
