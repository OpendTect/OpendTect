#ifndef uigmtlocations_h
#define uigmtlocations_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtlocations.h,v 1.1 2008-08-01 08:31:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class CtxtIOObj;
class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;

class uiGMTLocationsGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
protected:

    			uiGMTLocationsGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;

    uiIOObjSel*		inpfld_;
    uiGenInput*		namefld_;
    uiComboBox*		shapefld_;
    uiGenInput*		sizefld_;
    uiCheckBox*		fillfld_;
    uiColorInput*	outcolfld_;
    uiColorInput*	fillcolfld_;

    void		fillSel(CallBacker*);
    void		objSel(CallBacker*);
    void		fillShapes();
};

#endif
