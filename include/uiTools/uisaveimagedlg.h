#ifndef uisaveimagedlg_h
#define uisaveimagedlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki Maitra
 Date:          February 2009
 RCS:           $Id: uisaveimagedlg.h,v 1.2 2009-02-20 09:21:39 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "geometry.h"
#include "bufstringset.h"

class IOPar;
class uiCheckBox;
class uiFileInput;
class uiGenInput;
class uiLabeledComboBox;
class uiLabeledSpinBox;

mClass uiSaveImageDlg : public uiDialog
{
public:
			uiSaveImageDlg(uiParent*,const uiDialog::Setup&);

    Notifier<uiSaveImageDlg>	sizesChanged;

    void		sPixels2Inch(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&,float);
    void		sInch2Pixels(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&,float);
    void		sCm2Inch(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&);
    void		sInch2Cm(const Geom::Size2D<float>&,
	    			     Geom::Size2D<float>&);
    void		createGeomInpFlds(uiParent*);
    void                fillPar(IOPar&);
    bool                usePar(const IOPar&);

protected:

    uiLabeledSpinBox*	heightfld_;
    uiLabeledSpinBox*	widthfld_;
    uiGenInput*		unitfld_;
    uiCheckBox*		lockfld_;
    uiGenInput*		dpifld_;
    uiGenInput*		useparsfld_;
    uiFileInput*	fileinputfld_;
    
    BufferString	filters_;
    BufferString	selfilter_;

    void		updateFilter();
    virtual void	getSupportedFormats(const char** imgfrmt,
	    				    const char** frmtdesc,
					    BufferString& filter)	=0;
    void		fileSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    void		unitChg(CallBacker*);
    void		lockChg(CallBacker*);
    void		sizeChg(CallBacker*);
    void		dpiChg(CallBacker*);
    void		surveyChanged(CallBacker*);
    virtual void	setFldVals(CallBacker*);


    Geom::Size2D<float> sizepix_;
    Geom::Size2D<float> sizeinch_;
    Geom::Size2D<float> sizecm_;
    float		aspectratio_;	// width / height
    float		screendpi_;

    static BufferString	dirname_;

    void		updateSizes();
    virtual const char*	getExtension();

    static const char*  sKeyType()	{ return "Type"; }
    static const char*  sKeyHeight()    { return "Height"; }
    static const char*  sKeyWidth()     { return "Width"; }
    static const char*  sKeyUnit()      { return "Unit"; }
    static const char*  sKeyRes()       { return "Resolution"; }
    static const char*  sKeyLockAR()    { return "Lock aspect ratio"; }
    static const char*  sKeyFileType()  { return "File type"; }
};

#endif
