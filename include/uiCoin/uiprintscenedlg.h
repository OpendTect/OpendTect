#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.21 2012-08-03 13:00:54 cvskris Exp $
________________________________________________________________________

-*/

#include "uicoinmod.h"
#include "uisaveimagedlg.h"

class IOPar;
class uiGenInput;
class uiLabeledComboBox;
class ui3DViewer;

mClass(uiCoin) uiPrintSceneDlg : public uiSaveImageDlg
{
public:
			uiPrintSceneDlg(uiParent*,const ObjectSet<ui3DViewer>&);
protected:

    uiLabeledComboBox*	scenefld_;
    uiGenInput*		dovrmlfld_;
    uiGenInput*		selfld_;

    const char*		getExtension();
    void		writeToSettings();
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters);

    void		setFldVals(CallBacker*);
    void		typeSel(CallBacker*);
    void		sceneSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    const ObjectSet<ui3DViewer>& viewers_;
};

#endif

