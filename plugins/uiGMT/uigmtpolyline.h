#ifndef uigmtpolyline_h
#define uigmtpolyline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtpolyline.h,v 1.1 2008-08-01 08:31:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class CtxtIOObj;
class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiIOObjSel;
class uiSelLineStyle;

class uiGMTPolylineGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    			uiGMTPolylineGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;

    uiIOObjSel*		inpfld_;
    uiGenInput*		namefld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		fillfld_;
    uiColorInput*	fillcolfld_;

    void		fillSel(CallBacker*);
    void		objSel(CallBacker*);
};

#endif
