#ifndef uigmtwells_h
#define uigmtwells_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtwells.h,v 1.6 2009/07/22 16:01:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiListBox;
class uiSpinBox;
class uiGMTSymbolPars;

class uiGMTWellsGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTWellsGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiListBox*		welllistfld_;
    uiGenInput*		namefld_;
    uiGMTSymbolPars*	symbfld_;
    uiCheckBox*		lebelfld_;
    uiComboBox*		lebelalignfld_;
    uiSpinBox*		labelfontszfld_;

    void		choiceSel(CallBacker*);
    void		fillItems();
};

#endif
