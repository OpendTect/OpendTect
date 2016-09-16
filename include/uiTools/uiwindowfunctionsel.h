#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uitoolsmod.h"
#include "uigroup.h"
#include "dbkey.h"

class WindowFunction;
class uiGenInput;
class uiPushButton;
class uiWindowFuncSelDlg;
class uiFreqTaperDlg;

/*!Selects a windowfunction and its eventual parameter. */

mExpClass(uiTools) uiWindowFunctionSel : public uiGroup
{ mODTextTranslationClass(uiWindowFunctionSel);
public:

    mStruct(uiTools) Setup
    {
			Setup()
			    : onlytaper_(false)
			    , with2fldsinput_(false)
			    , winparam_(mUdf(float))
			    {}

	mDefSetupMemb(const char*,winname )
	mDefSetupMemb(const char*,label)
	mDefSetupMemb(BufferString,inpfldtxt)
	mDefSetupMemb(float,winparam)
	mDefSetupMemb(bool,onlytaper)
	mDefSetupMemb(bool,with2fldsinput)
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

    uiGenInput*			getVariableFld(int winidx);
    const uiGenInput*		getVariableFld(int winidx) const;

    BufferString		errmsg_;
    Interval<float>		annotrange_;

    bool			onlytaper_;
    uiGenInput*			windowtypefld_;
    ObjectSet<uiGenInput>	varinpflds_;
    uiPushButton*		viewbut_;
    uiWindowFuncSelDlg*		winfuncseldlg_;
    ObjectSet<WindowFunction>	windowfuncs_;

};
