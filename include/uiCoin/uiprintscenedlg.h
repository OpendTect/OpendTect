#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.2 2002-10-17 05:44:54 kristofer Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class SoNode;
class uiFileInput;
class uiComboBox;
class uiGenInput;

class uiPrintSceneDlg : public uiDialog
{
public:
			uiPrintSceneDlg(uiParent*,SoNode*);

protected:

    uiFileInput*	fileinputfld;
    uiComboBox*		filetypesfld;
    uiGenInput*		horwidthfld;
    uiComboBox*		widthunitfld;
    uiGenInput*		vertwidthfld;
    uiGenInput*		resolutionfld;


    SoNode*		scene;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
};

#endif
