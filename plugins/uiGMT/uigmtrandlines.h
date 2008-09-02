#ifndef uigmtrandlines_h
#define uigmtrandlines_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtrandlines.h,v 1.1 2008-09-02 11:08:27 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class BufferStringSet;
class CtxtIOObj;
class uiCheckBox;
class uiGenInput;
class uiIOObjSel;
class uiSelLineStyle;
class uiSpinBox;

class uiGMTRandLinesGrp : public uiGMTOverlayGrp
{
public:

    			~uiGMTRandLinesGrp();

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    			uiGMTRandLinesGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;

    uiIOObjSel*		inpfld_;
    uiGenInput*		namefld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		labelfld_;
    uiSpinBox*		labelfontfld_;

    BufferStringSet&	linenms_;

    void		objSel(CallBacker*);
    void		labelSel(CallBacker*);
};

#endif
