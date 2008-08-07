#ifndef uigmtcoastline_h
#define uigmtcoastline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtcoastline.h,v 1.1 2008-08-07 12:38:39 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"


class uiGMTCoastlineGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
protected:

    			uiGMTCoastlineGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

};

#endif
