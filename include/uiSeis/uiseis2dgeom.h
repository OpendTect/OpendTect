#ifndef uiseis2dgeom_h
#define uiseis2dgeom_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uiseis2dgeom.h,v 1.2 2004-12-10 16:57:41 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOObj;
class uiSeisSel;
class CtxtIOObj;
class uiGenInput;
class uiFileInput;


class uiSeisDump2DGeom : public uiDialog
{
public:
                        uiSeisDump2DGeom(uiParent*,const IOObj* ioobj=0);
			~uiSeisDump2DGeom();

protected:

    uiSeisSel*		seisfld;
    uiGenInput*		lnmsfld;
    uiGenInput*		incnrfld;
    uiGenInput*		zfld;
    uiFileInput*	outfld;

    CtxtIOObj&		ctio;

    virtual bool	acceptOK(CallBacker*);

    void		seisSel(CallBacker*);
};

#endif
