#ifndef uigmtclip_h
#define uigmtclip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uigmtoverlay.h"

class uiGenInput;
class uiIOObjSel;

mClass(uiGMT) uiGMTClipGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

    static bool		getTerminatingPars(IOPar&);

protected:

    			uiGMTClipGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    void		actionSel(CallBacker*);

    uiGenInput*		actionfld_;
    uiIOObjSel*		polygonfld_;
    uiGenInput*		optionfld_;
};


#endif
