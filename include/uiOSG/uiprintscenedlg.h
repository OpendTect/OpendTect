#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          July 2002
 RCS:           $Id$
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

    const char*		getExtension();
    void		writeToSettings();
    void		getSupportedFormats(const char** imagefrmt,
					    const char** frmtdesc,
					    BufferString& filters);

    enum		{InvalidImages=0, OnlyMainViewImage, MainAndHudImages };
    void		setFldVals(CallBacker*);
    void		sceneSel(CallBacker*);
    bool		acceptOK(CallBacker*);
    osg::Image*		offScreenRenderViewToImage(osgViewer::View*,
						   unsigned char transparency);
			/*! The returned image is not referenced yet. */
    int			validateImages(const osg::Image*,const osg::Image*);
    bool		hasImageValidFormat(const osg::Image*);
    void		flipImageVertical(osg::Image*);
    bool		saveImages(const osg::Image*,const osg::Image*);

    const ObjectSet<ui3DViewer>& viewers_;
};

