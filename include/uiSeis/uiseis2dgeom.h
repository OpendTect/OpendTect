#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
