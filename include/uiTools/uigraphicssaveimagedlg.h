#ifndef uigraphicssaveimagedlg_h
#define uigraphicssaveimagedlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          February 2009
 RCS:           $Id: uigraphicssaveimagedlg.h,v 1.5 2009/07/22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisaveimagedlg.h"

class uiGraphicsScene;

mClass uiGraphicsSaveImageDlg : public uiSaveImageDlg
{
public:
			uiGraphicsSaveImageDlg(uiParent*,uiGraphicsScene*);
protected:
    uiGraphicsScene*	scene_;
  
    const char*		getExtension();
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters);
    void		writeToSettings();
    void		setAspectRatio(CallBacker*);
    void		setFldVals(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
