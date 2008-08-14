#ifndef uigmtbasemap_h
#define uigmtbasemap_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtbasemap.h,v 1.3 2008-08-14 10:52:52 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiCheckBox;
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
    uiCheckBox*		gridlinesfld_;

    float		aspectratio_;

    void		resetCB(CallBacker*);
    void		xyrgChg(CallBacker*);
    void		dimChg(CallBacker*);
    void		updateFlds(bool);
};

#endif
