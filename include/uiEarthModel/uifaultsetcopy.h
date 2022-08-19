#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
