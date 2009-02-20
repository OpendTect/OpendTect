#ifndef uigraphicssaveimagedlg_h
#define uigraphicssaveimagedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          February 2009
 RCS:           $Id: uigraphicssaveimagedlg.h,v 1.2 2009-02-20 09:21:39 cvssatyaki Exp $
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
  
    float 		initaspectratio_;

    const char*		getExtension();
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters);
    void		write2Dsettings();
    void		setAspectRatio(CallBacker*);
    void		setFldVals(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
