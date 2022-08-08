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
#include "multiid.h"

class IOObj;
class uiGenInput;
class uiIOObjSelGrp;
class uiSeisTransfer;
class uiSeisSel;


mExpClass(uiSeis) uiMergeSeis : public uiDialog
{ mODTextTranslationClass(uiMergeSeis);
public:
                        uiMergeSeis(uiParent*);

    void		setInputIds(const TypeSet<MultiID>& mids);

protected:

    uiIOObjSelGrp*	inpfld_;
    uiGenInput*		stackfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		outfld_;

    bool		acceptOK(CallBacker*) override;
    bool		getInput(ObjectSet<IOPar>&,IOPar&);
};


