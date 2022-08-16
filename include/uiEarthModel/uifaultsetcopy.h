#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2008
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "multiid.h"

class uiIOObjSel;
class uiIOObjSelGrp;

mExpClass(uiEarthModel) uiFault2FaultSet : public uiDialog
{
mODTextTranslationClass(uiFault2FaultSet)
public:
			uiFault2FaultSet(uiParent*);
			~uiFault2FaultSet();

    void		setInput(const TypeSet<MultiID>&);
    MultiID		key() const;

protected:
    bool		acceptOK(CallBacker*) override;

    uiIOObjSelGrp*	infld_;
    uiIOObjSel*		outfld_;

};
