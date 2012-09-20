#ifndef uigmtcoastline_h
#define uigmtcoastline_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

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
    uiColorInput*	wetcolfld_;
    uiColorInput*	drycolfld_;

    void		utmSel(CallBacker*);
};

#endif
