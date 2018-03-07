#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "dbkey.h"

class IOObj;
class uiCheckBox;
class uiFileSel;
class uiGenInput;
class uiSeisSel;
class uiSeisTransfer;
class uiLabeledComboBox;

/*!\brief Imports or links to an OpendTect specific cube file */

mExpClass(uiSeis) uiSeisImportODCube : public uiDialog
{ mODTextTranslationClass(uiSeisImportODCube);
public:

			uiSeisImportODCube(uiParent*);
			~uiSeisImportODCube();

    const IOObj*	newIOObj() const	{ return outioobj_; }

protected:

    IOObj*		outioobj_;
    const DBKey		tmpid_;

    uiGenInput*		typefld_;
    uiGenInput*		modefld_;
    uiSeisTransfer*	transffld_;
    uiFileSel*		inpfld_;
    uiSeisSel*		outfld_;

    void		inpSel(CallBacker*);
    void		modeSel(CallBacker*);
    void		typeChg(CallBacker*);

    IOObj*		getInpIOObj(const char*) const;

    bool		acceptOK();

};
