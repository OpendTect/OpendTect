#ifndef uigmt2dlines_h
#define uigmt2dlines_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmt2dlines.h,v 1.1 2008-08-18 11:24:25 cvsraman Exp $
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

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

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
    uiSpinBox*		labelfontfld_;

    void		objSel(CallBacker*);
    void		labelSel(CallBacker*);
};

#endif
