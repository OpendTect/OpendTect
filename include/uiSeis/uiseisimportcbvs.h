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
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiSeisSel;
class uiSeisTransfer;
class uiLabeledComboBox;

/*!\brief Imports or links to a CBVS file */

mExpClass(uiSeis) uiSeisImportCBVS : public uiDialog
{ mODTextTranslationClass(uiSeisImportCBVS);
public:

			uiSeisImportCBVS(uiParent*);
			~uiSeisImportCBVS();

    const IOObj*	newIOObj() const	{ return outioobj_; }

protected:

    IOObj*		outioobj_;
    const BufferString	tmpid_;

    uiGenInput*		typefld_;
    uiGenInput*		modefld_;
    uiSeisTransfer*	transffld_;
    uiFileInput*	inpfld_;
    uiSeisSel*		outfld_;

    void		inpSel(CallBacker*);
    void		modeSel(CallBacker*);
    void		typeChg(CallBacker*);

    IOObj*		getInpIOObj(const char*) const;

    bool		acceptOK(CallBacker*) override;

};
