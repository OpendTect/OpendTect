#ifndef uiseis2dgeom_h
#define uiseis2dgeom_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id: uiseis2dgeom.h,v 1.6 2012-08-03 13:01:07 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
class IOObj;
class uiSeisSel;
class CtxtIOObj;
class uiGenInput;
class uiFileInput;


mClass(uiSeis) uiSeisDump2DGeom : public uiDialog
{
public:
                        uiSeisDump2DGeom(uiParent*,const IOObj* ioobj=0);
			~uiSeisDump2DGeom();

protected:

    uiSeisSel*		seisfld;
    uiGenInput*		lnmsfld;
    uiFileInput*	outfld;

    CtxtIOObj&		ctio;

    virtual bool	acceptOK(CallBacker*);

    void		seisSel(CallBacker*);
};

#endif

