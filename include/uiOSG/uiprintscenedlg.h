#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiosgmod.h"
#include "uisaveimagedlg.h"

class uiGenInput;
class uiLabeledComboBox;
class ui3DViewer;

namespace osgViewer
{ class View; }
namespace osg
{ class Image; }

/*!
\brief Print scene dialog box.
*/

mExpClass(uiOSG) ui3DViewer2Image
{ mODTextTranslationClass(ui3DViewer2Image)
public:
			ui3DViewer2Image(ui3DViewer&,const char* imgfnm,
					 uiSize imgsz=uiSize(),int dpi=-1);

    bool		create();

protected:

    enum		{InvalidImages=0, OnlyMainViewImage, MainAndHudImages };
    osg::Image*		offScreenRenderViewToImage(osgViewer::View*,
						   unsigned char transparency);
			/*! The returned image is not referenced yet. */
    int			validateImages(const osg::Image*,const osg::Image*);
    bool		hasImageValidFormat(const osg::Image*);
    void		flipImageVertical(osg::Image*);
    bool		saveImages(const osg::Image*,const osg::Image*);

    ui3DViewer&		vwr_;
    BufferString	imgfnm_;
    uiSize		sizepix_;
    float		dpi_;
    uiString		errmsg_;
};


mExpClass(uiOSG) uiPrintSceneDlg : public uiSaveImageDlg
{ mODTextTranslationClass(uiPrintSceneDlg);
public:
			uiPrintSceneDlg(uiParent*,const ObjectSet<ui3DViewer>&);
protected:

    uiLabeledComboBox*	scenefld_;

    const char*		getExtension() override;
    void		writeToSettings() override;
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters) override;

    enum		{InvalidImages=0, OnlyMainViewImage, MainAndHudImages };
    void		setFldVals(CallBacker*) override;
    void		sceneSel(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    osg::Image*		offScreenRenderViewToImage(osgViewer::View*,
						   unsigned char transparency);
			/*! The returned image is not referenced yet. */
    int			validateImages(const osg::Image*,const osg::Image*);
    bool		hasImageValidFormat(const osg::Image*);
    void		flipImageVertical(osg::Image*);
    bool		saveImages(const osg::Image*,const osg::Image*);

    const ObjectSet<ui3DViewer>& viewers_;
};
