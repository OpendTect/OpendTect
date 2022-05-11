#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2022
________________________________________________________________________

-*/

#include "uidialog.h"
#include "multiid.h"

class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSeisSubSel;


/*! \brief Create flattened cube from horizon */

mClass(uiEMAttrib) uiFlatUnflatCube : public uiDialog
{
mODTextTranslationClass(uiFlatUnflatCube)
public:
			uiFlatUnflatCube(uiParent*);
			~uiFlatUnflatCube();

    void		setHorizon(const MultiID&);

protected:

    void		finalizeCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		inpSelCB(CallBacker*);
    void		horSelCB(CallBacker*);

    uiGenInput*		modefld_;
    uiSeisSel*		seisinfld_;
    uiIOObjSel*		horfld_;
    uiGenInput*		flatvalfld_;
    uiSeisSubSel*	rgfld_;
    uiSeisSel*		seisoutfld_;
};
