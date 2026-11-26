#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 Width/height/destination-file picker for uiVSGViewer::takeScreenshot(), from od's uiSaveImageDlg.

-*/

#include "uivsgmod.h"

#include "uisaveimagedlg.h"


mExpClass(uiVSG) uiVSGScreenshotDlg : public uiSaveImageDlg
{ mODTextTranslationClass(uiVSGScreenshotDlg);
public:
			uiVSGScreenshotDlg(uiParent*,int curwidth,
					   int curheight);

    int			width() const;
    int			height() const;
    BufferString	filename() const;
    float		dpi() const;

protected:
    void		getSupportedFormats(const char** imgfrmt,
					    const char** frmtdesc,
					    BufferString& filters) override;
    void		setFldVals(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;
    void		writeToSettings() override;

    const int		curwidth_;
    const int		curheight_;
};
