#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.10 2006-03-30 20:49:46 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include <Inventor/SbLinear.h>

class IOPar;
class SbColor;
class SoNode;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledComboBox;
class uiSoViewer;
class uiSpinBox;

class uiPrintSceneDlg : public uiDialog
{
public:
			uiPrintSceneDlg(uiParent*,const ObjectSet<uiSoViewer>&);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    uiLabeledComboBox*	scenefld;
    uiSpinBox*		heightfld;
    uiSpinBox*		widthfld;
    uiGenInput*		unitfld;
    uiCheckBox*		lockfld;
    uiGenInput*		dpifld;
    uiFileInput*	fileinputfld;

    void		fileSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    void		sceneSel(CallBacker*);
    void		unitChg(CallBacker*);
    void		lockChg(CallBacker*);
    void		sizeChg(CallBacker*);
    void		dpiChg(CallBacker*);
    bool		acceptOK(CallBacker*);

    const ObjectSet<uiSoViewer>& viewers;

    SbVec2f		sizepix;
    SbVec2f		sizeinch;
    SbVec2f		sizecm;
    float		aspectratio;	// width / height
    const float		screendpi;

    void		updateSizes();
    const char*		getExtension() const;

    static const char*	heightstr;
    static const char*	widthstr;
    static const char*	unitstr;
    static const char*	resstr;
};

#endif
