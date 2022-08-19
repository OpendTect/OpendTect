#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"

class IOObj;
class uiIOObjSel;
class uiPosSubSel;
class uiGenInput;


/*!\brief Group for output when copying PS data stores */

mExpClass(uiSeis) uiPreStackOutputGroup : public uiGroup
{ mODTextTranslationClass(uiPreStackOutputGroup);
public:

			uiPreStackOutputGroup(uiParent*);
			~uiPreStackOutputGroup();

    uiPosSubSel*	subselFld()		{ return subselfld_; }
    uiIOObjSel*		outputFld()		{ return outpfld_; }

    void		setInput(const IOObj&);
    bool		go();

protected:

    uiPosSubSel*	subselfld_;
    uiGenInput*		offsrgfld_;
    uiIOObjSel*		outpfld_;

    IOObj*		inpioobj_;

};


/*!\brief Dialog for copying PS data stores */

mExpClass(uiSeis) uiPreStackCopyDlg : public uiDialog
{ mODTextTranslationClass(uiPreStackCopyDlg);
public:

			uiPreStackCopyDlg(uiParent*,const MultiID&);

    uiPreStackOutputGroup* outputGroup()		{ return outgrp_; }

protected:

    uiIOObjSel*		inpfld_;
    uiPreStackOutputGroup* outgrp_;

    void		objSel(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
