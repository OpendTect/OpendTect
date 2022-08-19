#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
