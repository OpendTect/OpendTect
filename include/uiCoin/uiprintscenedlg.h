#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.1 2002-10-16 07:34:21 kristofer Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class SoNode;
class uiFileInput;
class uiListBox;

class uiPrintSceneDlg : public uiDialog
{
public:
			uiPrintSceneDlg(uiParent*,SoNode*);

protected:

    uiFileInput*	fileinputfld;
    uiListBox*		filetypesfld;

    SoNode*		scene;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);

};

#endif
