#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2011
________________________________________________________________________

-*/


#include "uiattributesmod.h"
#include "arraynd.h"
#include "uidialog.h"

class SeisTrcBuf;

class uiFuncDispBase;
class uiGenInput;
class uiLabeledSpinBox;

mExpClass(uiAttributes) uiGainAnalysisDlg : public uiDialog
{ mODTextTranslationClass(uiFrequencyAttrib);
public:
				uiGainAnalysisDlg(uiParent*,const SeisTrcBuf&,
				      TypeSet<float>& zvals,
				      TypeSet<float>& scalefac);
				~uiGainAnalysisDlg();

    const TypeSet<float>&	zVals() const		{ return zvals_; }
    const TypeSet<float>&	scaleFactors() const	{ return scalefactors_;}
protected:

    uiFuncDispBase*		funcdisp_;
    uiGenInput*			rangefld_;
    uiGenInput*			ampscaletypefld_;
    uiLabeledSpinBox*		stepfld_;

    TypeSet<float>&		zvals_;
    TypeSet<float>&		scalefactors_;

    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			dispRangeChgd(CallBacker*);
    void			amplScaleTypeChanged(CallBacker*);
    void			setData(bool sety=false);
    void			convertZtoDisplay();
    void			convertZfromDisplay();

    const SeisTrcBuf&		trcbuf_;

};


