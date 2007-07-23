#ifndef uiwindowfunctionsel_h
#define uiwindowfunctionsel_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2007
 RCS:           $Id: uiwindowfunctionsel.h,v 1.1 2007-07-23 16:51:20 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class WindowFunction;
class uiGenInput;

/*!Selects a windowfunction and its eventual parameter. */

class uiWindowFunctionSel : public uiGroup
{
public:
    			uiWindowFunctionSel(uiParent*,const char* label,
					    const char* curwinname = 0,
					    float curwinparam = mUdf(float) );
    			~uiWindowFunctionSel();

    NotifierAccess&	typeChange();

    const char*		windowName() const;
    float		windowParamValue() const;
    const char*		windowParamName() const;

    static const char*	sNone() { return "None"; }

protected:
    void			windowChangedCB(CallBacker*);

    BufferString		errmsg_;
    Interval<float>		annotrange_;

    uiGenInput*			windowtypefld_;
    ObjectSet<uiGenInput>	windowvariable_;
    ObjectSet<WindowFunction>	windowfuncs_;
};


#endif
