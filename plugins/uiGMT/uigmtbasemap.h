#ifndef uigmtbasemap_h
#define uigmtbasemap_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtbasemap.h,v 1.2 2008-08-06 09:58:05 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiGenInput;
class uiPushButton;

class uiGMTBaseMapGrp : public uiDlgGroup
{
public:

    			uiGMTBaseMapGrp(uiParent*);

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    uiGenInput*		titlefld_;
    uiGenInput*		xdimfld_;
    uiGenInput*		ydimfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiPushButton*	resetbut_;
    uiGenInput*		lebelintvfld_;

    float		aspectratio_;

    void		resetCB(CallBacker*);
    void		xyrgChg(CallBacker*);
    void		dimChg(CallBacker*);
    void		updateFlds(bool);
};

#endif
