#ifndef uigmtwells_h
#define uigmtwells_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtwells.h,v 1.2 2008-08-14 10:52:52 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiListBox;

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

    void		choiceSel(CallBacker*);
    void		fillItems();
};

#endif
