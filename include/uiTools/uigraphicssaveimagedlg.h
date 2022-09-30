#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uisaveimagedlg.h"

class uiGraphicsScene;

mExpClass(uiTools) uiGraphicsSaveImageDlg : public uiSaveImageDlg
{ mODTextTranslationClass(uiGraphicsSaveImageDlg);
public:
			uiGraphicsSaveImageDlg(uiParent*,uiGraphicsScene*);
			~uiGraphicsSaveImageDlg();

protected:
    uiGraphicsScene*	scene_;
  
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters) override;
    void		writeToSettings() override;
    void		setAspectRatio(CallBacker*);
    void		setFldVals(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;
};
