#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.15 2008-04-18 04:40:58 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOPar;
class SbColor;
class SbVec2f;
class SoNode;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledSpinBox;
class uiSoViewer;

class uiPrintSceneDlg : public uiDialog
{
public:
			uiPrintSceneDlg(uiParent*,const ObjectSet<uiSoViewer>&);
			~uiPrintSceneDlg();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    uiLabeledComboBox*	scenefld_;
    uiGenInput*		dovrmlfld_;
    uiLabeledSpinBox*	heightfld_;
    uiLabeledSpinBox*	widthfld_;
    uiGenInput*		unitfld_;
    uiCheckBox*		lockfld_;
    uiGenInput*		dpifld_;
    uiFileInput*	fileinputfld_;

    void		write2settings();
    void		updateFilter();
    void		fileSel(CallBacker*);
    void		typeSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    void		sceneSel(CallBacker*);
    void		unitChg(CallBacker*);
    void		lockChg(CallBacker*);
    void		sizeChg(CallBacker*);
    void		dpiChg(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		surveyChanged(CallBacker*);

    const ObjectSet<uiSoViewer>& viewers_;

    SbVec2f&		sizepix_;
    SbVec2f&		sizeinch_;
    SbVec2f&		sizecm_;
    float		aspectratio_;	// width / height
    const float		screendpi_;

    static BufferString	dirname_;

    void		updateSizes();
    const char*		getExtension() const;

    static const char*	sVRMLExt()	{ return "wrl"; }
    static const char*	sKeyHeight()	{ return "Height"; }
    static const char*	sKeyWidth()	{ return "Width"; }
    static const char*	sKeyUnit()	{ return "Unit"; }
    static const char*	sKeyRes()	{ return "Resolution"; }
    static const char*	sKeyLockAR()	{ return "Lock aspect ratio"; }
    static const char*	sKeyFileType()	{ return "File type"; }
};

#endif
