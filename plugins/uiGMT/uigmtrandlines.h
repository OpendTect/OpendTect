#ifndef uigmtrandlines_h
#define uigmtrandlines_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id$
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

mClass(uiGMT) uiGMTRandLinesGrp : public uiGMTOverlayGrp
{
public:

    			~uiGMTRandLinesGrp();

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

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
