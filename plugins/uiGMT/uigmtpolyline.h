#ifndef uigmtpolyline_h
#define uigmtpolyline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtpolyline.h,v 1.3 2009-07-22 16:01:28 cvsbert Exp $
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
    void		reset();

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
