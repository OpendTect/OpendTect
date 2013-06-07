#ifndef uigmtclip_h
#define uigmtclip_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
 RCS:           $Id: uigmtclip.h,v 1.1 2011/05/10 03:53:52 cvsraman Exp $
________________________________________________________________________

-*/


#include "uigmtoverlay.h"

class uiGenInput;
class uiIOObjSel;

class uiGMTClipGrp : public uiGMTOverlayGrp
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
