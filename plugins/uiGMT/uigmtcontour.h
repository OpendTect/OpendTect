#ifndef uigmtcontour_h
#define uigmtcontour_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id$
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
    Interval<float>	valrg_;

    uiIOObjSel*		inpfld_;
    uiPosSubSel*	subselfld_;
    uiComboBox*		attribfld_;
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
