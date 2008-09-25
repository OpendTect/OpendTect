#ifndef uigmt2dlines_h
#define uigmt2dlines_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmt2dlines.h,v 1.3 2008-09-25 12:01:13 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class CtxtIOObj;
class uiCheckBox;
class uiGenInput;
class uiListBox;
class uiSeisSel;
class uiSelLineStyle;
class uiSpinBox;

class uiGMT2DLinesGrp : public uiGMTOverlayGrp
{
public:

    			~uiGMT2DLinesGrp();

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMT2DLinesGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;

    uiSeisSel*		inpfld_;
    uiGenInput*		namefld_;
    uiListBox*		linelistfld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		labelfld_;
    uiGenInput*		labelposfld_;
    uiSpinBox*		labelfontfld_;
    uiCheckBox*		trclabelfld_;
    uiGenInput*		trcstepfld_;

    void		objSel(CallBacker*);
    void		labelSel(CallBacker*);
};

#endif
