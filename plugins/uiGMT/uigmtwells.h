#ifndef uigmtwells_h
#define uigmtwells_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtwells.h,v 1.3 2008-08-20 05:26:14 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiSpinBox;

class uiGMTWellsGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
protected:

    			uiGMTWellsGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiListBox*		welllistfld_;
    uiGenInput*		namefld_;
    uiComboBox*		shapefld_;
    uiGenInput*		sizefld_;
    uiCheckBox*		fillfld_;
    uiColorInput*	outcolfld_;
    uiColorInput*	fillcolfld_;
    uiCheckBox*		lebelfld_;
    uiComboBox*		lebelalignfld_;
    uiSpinBox*		labelfontszfld_;

    void		choiceSel(CallBacker*);
    void		fillItems();
};

#endif
