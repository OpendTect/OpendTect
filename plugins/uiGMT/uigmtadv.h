#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		September 2008
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiLineEdit;

mClass(uiGMT) uiGMTAdvGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTAdvGrp);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();
protected:

    			uiGMTAdvGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiLineEdit*		inpfld_;
};
