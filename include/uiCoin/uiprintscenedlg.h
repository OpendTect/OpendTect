#ifndef uiprintscenedlg_h
#define uiprintscenedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id: uiprintscenedlg.h,v 1.6 2005-01-25 14:58:46 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include <Inventor/SbLinear.h>

class IOPar;
class SoNode;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiSpinBox;

class uiPrintSceneDlg : public uiDialog
{
public:
			uiPrintSceneDlg(uiParent*,SoNode*,const SbVec2s&);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    uiSpinBox*		heightfld;
    uiSpinBox*		widthfld;
    uiGenInput*		unitfld;
    uiCheckBox*		lockfld;
    uiGenInput*		dpifld;
    uiFileInput*	fileinputfld;

    SoNode*		scene;

    bool		filenameOK() const;
    bool		acceptOK(CallBacker*);
    void		unitChg(CallBacker*);
    void		lockChg(CallBacker*);
    void		sizeChg(CallBacker*);
    void		dpiChg(CallBacker*);

    SbVec2s		winsz;

    SbVec2f		sizepix;
    SbVec2f		sizeinch;
    SbVec2f		sizecm;
    float		aspectratio;	// width / height
    float		dpi;

    void		init();
    void		updateSizes();
    void		pixels2Inch(const SbVec2f&,SbVec2f&);
    void		inch2Pixels(const SbVec2f&,SbVec2f&);

    static const char*	heightstr;
    static const char*	widthstr;
    static const char*	unitstr;
    static const char*	resstr;

    static const char*	imageformats[];
    static const char*	filters[];

};

#endif
