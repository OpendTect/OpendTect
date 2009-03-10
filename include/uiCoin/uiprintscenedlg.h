#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.18 2009-03-10 06:35:42 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uisaveimagedlg.h"

class IOPar;
class uiGenInput;
class uiLabeledComboBox;
class uiSoViewer;

mClass uiPrintSceneDlg : public uiSaveImageDlg
{
public:
			uiPrintSceneDlg(uiParent*,const ObjectSet<uiSoViewer>&);
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

    const ObjectSet<uiSoViewer>& viewers_;
};

#endif
