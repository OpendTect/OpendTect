#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uicreatelogcubedlg.h"
#include "uidialog.h"
#include "uigroup.h"

#include "bufstringset.h"

class uiCheckBox;
class uiCheckList;
class uiGenInput;
class uiIOObjSel;


namespace WellTie
{

class Server;

mExpClass(uiWellAttrib) uiSaveDataDlg : public uiDialog
{ mODTextTranslationClass(uiSaveDataDlg);
public:
				uiSaveDataDlg(uiParent*,Server&);

protected :

    uiCheckBox*			logchk_;
    uiCheckList*		logsfld_;
    uiCreateLogCubeOutputSel*	outputgrp_;
    uiGenInput*			saveasfld_;
    uiCheckBox*			wvltchk_;
    uiGenInput*			samplefld_;
    uiIOObjSel*			initwvltsel_;
    uiIOObjSel*			estimatedwvltsel_;
    Server&			dataserver_;

    bool			acceptOK(CallBacker*) override;
    bool			saveLogs();
    bool			saveWvlt(bool isestimated=true);
    void			changeLogUIOutput(CallBacker*);
    void			saveLogsSelCB(CallBacker*);
    void			saveWvltSelCB(CallBacker*);
};

} // namespace WellTie
