#ifndef uigmtcoastline_h
#define uigmtcoastline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtcoastline.h,v 1.5 2009-07-22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiSelLineStyle;
class uiSpinBox;

class uiGMTCoastlineGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTCoastlineGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiSpinBox*		utmfld_;
    uiSpinBox*		cmfld_;
    uiGenInput*		ewfld_;
    uiComboBox*		resolutionfld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		fillwetfld_;
    uiCheckBox*		filldryfld_;
    uiColorInput*	wetcolfld_;
    uiColorInput*	drycolfld_;

    void		fillSel(CallBacker*);
    void		utmSel(CallBacker*);
};

#endif
