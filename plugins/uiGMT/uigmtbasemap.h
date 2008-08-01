#ifndef uigmtbasemap_h
#define uigmtbasemap_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		July 2008
 RCS:		$Id: uigmtbasemap.h,v 1.1 2008-08-01 08:31:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class uiGenInput;

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
    uiGenInput*		lebelintvfld_;

    float		aspectratio_;

    void		xyrgChg(CallBacker*);
    void		dimChg(CallBacker*);
    void		updateFlds(bool);
};

#endif
