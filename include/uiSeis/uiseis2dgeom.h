#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
class IOObj;
class uiSeisSel;
class CtxtIOObj;
class uiGenInput;
class uiFileInput;


mExpClass(uiSeis) uiSeisDump2DGeom : public uiDialog
{ mODTextTranslationClass(uiSeisDump2DGeom);
public:

                        uiSeisDump2DGeom(uiParent*,const IOObj* ioobj=0);

protected:

    uiSeisSel*		seisfld_;
    uiGenInput*		lnmsfld_;
    uiFileInput*	outfld_;

    bool		acceptOK(CallBacker*) override;

    void		seisSel(CallBacker*);
};


