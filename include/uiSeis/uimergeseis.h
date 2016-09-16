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
#include "dbkey.h"

class IOObj;
class uiGenInput;
class uiIOObjSelGrp;
class uiSeisTransfer;
class uiSeisSel;


mExpClass(uiSeis) uiMergeSeis : public uiDialog
{ mODTextTranslationClass(uiMergeSeis);
public:
                        uiMergeSeis(uiParent*);

    void		setInputIds(const DBKeySet& mids);

protected:

    uiIOObjSelGrp*	inpfld_;
    uiGenInput*		stackfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		outfld_;

    virtual bool	acceptOK();
    bool		getInput(ObjectSet<IOPar>&,IOPar&);
};
