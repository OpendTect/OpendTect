#ifndef uigmtadv_h
#define uigmtadv_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		September 2008
 RCS:		$Id: uigmtadv.h,v 1.2 2008-09-25 12:01:13 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiLineEdit;

class uiGMTAdvGrp : public uiGMTOverlayGrp
{
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

#endif
