#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"

#include "uidirectfiledatadlg.h"

class IOPar;
class uiTable;

mExpClass(uiSeis) uiSeisDirectFileDataDlg : public uiEditDirectFileDataDlg
{ mODTextTranslationClass(uiSeis)
public:
			uiSeisDirectFileDataDlg(uiParent*,const IOObj&);
			~uiSeisDirectFileDataDlg();

private:

    uiTable*		filetable_			= nullptr;

    void		fillFileTable();
    void		updateFileTable(int);
    void		doDirSel() override;
    void		editCB(CallBacker*);
    void		fileSelCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};
