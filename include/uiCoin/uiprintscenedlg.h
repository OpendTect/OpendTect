#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.5 2004-05-27 13:53:44 kristofer Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOPar;
class SoNode;
class uiFileInput;
class uiComboBox;
class uiGenInput;

class uiPrintSceneDlg : public uiDialog
{
public:
			uiPrintSceneDlg(uiParent*,SoNode*);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

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

    static const char*	horwidthfldstr;
    static const char*	vertwidthfldstr;
    static const char*	widthunitfldstr;
    static const char*	resfldstr;
    static const char*	filetypefldstr;
    static const char*	filenamefldstr;

};

#endif
