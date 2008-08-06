#ifndef uigmtcontour_h
#define uigmtcontour_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtcontour.h,v 1.1 2008-08-06 09:58:05 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class CtxtIOObj;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiPosSubSel;

namespace EM { class SurfaceIOData; class Horizon3D; }

class uiGMTContourGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    			~uiGMTContourGrp();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
protected:

    			uiGMTContourGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    CtxtIOObj&		ctio_;
    EM::Horizon3D*	hor_;
    EM::SurfaceIOData&	sd_;

    uiIOObjSel*		inpfld_;
    uiPosSubSel*	subselfld_;
    uiPushButton*	readbut_;
    uiGenInput*		rgfld_;
    uiPushButton*	resetbut_;
    uiGenInput*		nrcontourfld_;
    uiCheckBox*		linefld_;
    uiCheckBox*		fillfld_;

    void		readCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		rgChg(CallBacker*);
    void		selChg(CallBacker*);
    void		objSel(CallBacker*);

    bool		loadHor();
};

#endif
