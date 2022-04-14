#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		Jan 2017
________________________________________________________________________

-*/

#include "uiwellattribmod.h"

#include "callback.h"
#include "uistring.h"

class uiParent;
class uiStratLayerModelManager;
class uiStratLayerModel;


mExpClass(uiWellAttrib) uiStratLayerModelManager : public CallBacker
{ mODTextTranslationClass(uiStratLayerModelManager);
public:

			uiStratLayerModelManager();
			~uiStratLayerModelManager();

    bool		haveExistingDlg();
    uiStratLayerModel*	getDlg();
    void		launchLayerModel(const char* modnm,int opt,
					 uiParent* =nullptr);

    void		addToTreeWin();

    static uiStratLayerModelManager& uislm_manager();
    static uiStratLayerModel* getUILayerModel();

    static void		initClass();
    static void		doBasicLayerModel();
    static void		doLayerModel(const char* modnm,int opt,
				     uiParent* =nullptr);

protected:

    void		survChg(CallBacker*);
    void		winClose(CallBacker*);
    void		startCB(CallBacker*);

    uiStratLayerModel*	dlg_ = nullptr;

};
