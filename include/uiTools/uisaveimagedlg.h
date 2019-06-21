#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          February 2009
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "geometry.h"
#include "ptrman.h"

class Settings;
class uiCheckBox;
class uiFileSel;
class uiGenInput;
class uiLabel;
class uiLabeledSpinBox;

mExpClass(uiTools) uiSaveImageDlg : public uiDialog
{ mODTextTranslationClass(uiSaveImageDlg);
public:
    Notifier<uiSaveImageDlg>	sizesChanged;

    void		sPixels2Inch(const Geom::Size2D<float>&,
				     Geom::Size2D<float>&,float);
    void		sInch2Pixels(const Geom::Size2D<float>&,
				     Geom::Size2D<float>&,float);
    void		sCm2Inch(const Geom::Size2D<float>&,
				     Geom::Size2D<float>&);
    void		sInch2Cm(const Geom::Size2D<float>&,
				     Geom::Size2D<float>&);
    void		createGeomInpFlds(uiObject*);

    void		fillPar(IOPar&,bool is2d);
    bool		usePar(const IOPar&);

    BufferString	getExtension() const;

protected:
			uiSaveImageDlg(uiParent*,bool withclipbrd=true,
				       bool withuseparsfld=true);

    void		setDirName(const char*);

    uiLabeledSpinBox*	pixheightfld_;
    uiLabeledSpinBox*	pixwidthfld_;
    uiLabeledSpinBox*	heightfld_;
    uiLabeledSpinBox*	widthfld_;
    uiLabeledSpinBox*	dpifld_;
    uiLabel*		pixlabel_;
    uiGenInput*		unitfld_;
    uiCheckBox*		lockfld_;
    uiGenInput*		useparsfld_;
    uiFileSel*		inpfilefld_;
    uiCheckBox*		cliboardselfld_;

    BufferString	filters_;
    BufferString	selfilter_;
    Settings&		settings_;
    Interval<float>	fldranges_;
    bool		withuseparsfld_;

    void		getSettingsPar(PtrMan<IOPar>&,BufferString);
    void		setSizeInPix(int width, int height);
    void		updateFilter();
    virtual bool	supportPrintFormats() const			= 0;
    void		fileSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    void		unitChg(CallBacker*);
    void		lockChg(CallBacker*);
    void		sizeChg(CallBacker*);
    void		dpiChg(CallBacker*);
    void		surveyChanged(CallBacker*);
    virtual void	setFldVals(CallBacker*)			{}
    void		copyToClipBoardClicked(CallBacker*);


    Geom::Size2D<float> sizepix_;
    Geom::Size2D<float> sizeinch_;
    Geom::Size2D<float> sizecm_;
    float		aspectratio_;	// width / height
    float		screendpi_;

    static BufferString	dirname_;

    void		updateSizes();
    void		setNotifiers(bool enable);
    virtual void	writeToSettings()		{}

    static const char*  sKeyType()	{ return "Type"; }
    static const char*  sKeyHeight()    { return "Height"; }
    static const char*  sKeyWidth()     { return "Width"; }
    static const char*  sKeyUnit()      { return "Unit"; }
    static const char*  sKeyRes()       { return "Resolution"; }
    static const char*  sKeyFileType()  { return "File type"; }
};


mExpClass(uiTools) uiSaveWinImageDlg : public uiSaveImageDlg
{
public:
			uiSaveWinImageDlg(uiParent*);

protected:
    bool		supportPrintFormats() const	{ return true; }

    void		setFldVals(CallBacker*);
    bool		acceptOK();
};
