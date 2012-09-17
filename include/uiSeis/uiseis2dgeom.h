#ifndef uiseis2dgeom_h
#define uiseis2dgeom_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uiseis2dgeom.h,v 1.4 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class IOObj;
class uiSeisSel;
class CtxtIOObj;
class uiGenInput;
class uiFileInput;


mClass uiSeisDump2DGeom : public uiDialog
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
