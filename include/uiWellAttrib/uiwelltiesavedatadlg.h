#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          January 2009
RCS:           $Id: uiwellwelltiesavedatadlg.h,v 1.1 2009-09-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uicreatelogcubedlg.h"
#include "uidialog.h"
#include "uigroup.h"


class uiCheckBox;
class uiCheckList;
class uiGenInput;
class uiWaveletIOObjSel;


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
    uiWaveletIOObjSel*		initwvltsel_;
    uiWaveletIOObjSel*		estimatedwvltsel_;
    Server&			dataserver_;

    bool			acceptOK();
    bool			saveLogs();
    bool			saveWvlt(bool isestimated=true);
    void			changeLogUIOutput(CallBacker*);
    void			saveLogsSelCB(CallBacker*);
    void			saveWvltSelCB(CallBacker*);
};

} // namespace WellTie
