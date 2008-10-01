#ifndef uigmtcontour_h
#define uigmtcontour_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtcontour.h,v 1.5 2008-10-01 05:20:28 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class CtxtIOObj;
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiPosSubSel;
class uiSelLineStyle;

namespace EM { class SurfaceIOData; class Horizon3D; }

class uiGMTContourGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    			~uiGMTContourGrp();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

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
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		fillfld_;
    uiComboBox*		colseqfld_;
    uiCheckBox*		flipfld_;

    void		readCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		rgChg(CallBacker*);
    void		selChg(CallBacker*);
    void		objSel(CallBacker*);
    void		drawSel(CallBacker*);

    void		fillColSeqs();
    bool		loadHor();
};

#endif
