#ifndef uiwindowfunctionsel_h
#define uiwindowfunctionsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.h,v 1.14 2012-08-03 13:01:16 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uitoolsmod.h"
#include "uigroup.h"
#include "multiid.h"

class WindowFunction;
class uiGenInput;
class uiPushButton;
class uiWindowFuncSelDlg;
class uiFreqTaperDlg;

/*!Selects a windowfunction and its eventual parameter. */

mClass(uiTools) uiWindowFunctionSel : public uiGroup
{
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


#endif


