#ifndef uigainanalysiswin_h
#define uigainanalysiswin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          Feb 2011
 RCS:           $Id: uigainanalysisdlg.h,v 1.1 2011/03/01 10:17:55 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "arraynd.h"
#include "uidialog.h"

class SeisTrcBuf;

class uiFunctionDisplay;
class uiGenInput;
class uiLabeledSpinBox;

mClass uiGainAnalysisDlg : public uiDialog
{
public:
				uiGainAnalysisDlg(uiParent*,const SeisTrcBuf&,
				      TypeSet<float>& zvals,
				      TypeSet<float>& scalefac);
				~uiGainAnalysisDlg();

    const TypeSet<float>&	zVals() const		{ return zvals_; }
    const TypeSet<float>&	scaleFactors() const	{ return scalefactors_;}
protected:

    uiFunctionDisplay*		funcdisp_;
    uiGenInput*			rangefld_;
    uiGenInput*			ampscaletypefld_;
    uiLabeledSpinBox*		stepfld_;

    TypeSet<float>&		zvals_;
    TypeSet<float>&		scalefactors_;
    
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			dispRangeChgd(CallBacker*);
    void			amplScaleTypeChanged(CallBacker*);
    void			setData(bool sety=false);
    void			convertZTo(bool msec);
    
    const SeisTrcBuf&		trcbuf_;

};


#endif
