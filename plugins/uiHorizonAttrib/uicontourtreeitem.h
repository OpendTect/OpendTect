#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Feb 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uioddatatreeitem.h"

#include "trckeyzsampling.h"
#include "color.h"
#include "externalattrib.h"
#include "polygon.h"
#include "visdata.h"
#include "indexedshape.h"

class visContourLabels;

template <class T> class Array2DImpl;
template <class T> class Array2D;

namespace Attrib { class SelSpec; }
namespace visSurvey { class SurveyObject; class HorizonDisplay; }
namespace visBase
{
    class DrawStyle;
    class PolyLine;
    class Material;
    class Text2;
}
namespace EM { class Horizon3D; }

class IsoContourTracer;
class ZAxisTransform;
class uiContourParsDlg;
class uiContourTreeItemContourData;

/*!\brief Tree item for Contour display on 3D horizons */

mClass(uiHorizonAttrib) uiContourTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(uiContourTreeItem);
public:

    static void			initClass();
				uiContourTreeItem(const char* parenttype);
				~uiContourTreeItem();

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			setAttribName( const char* attrnm )
				{ attrnm_ = attrnm; }

    static const char*		sKeyContourDefString();
    static const char*		sKeyZValue();
    int				getNrContours() const;

private:

    virtual bool		init();
    virtual bool		hasTransparencyMenu() const { return false; }
    virtual uiString		createDisplayName() const;
    virtual void		checkCB(CallBacker*);
    virtual bool		doubleClick(uiTreeViewItem*);
    virtual void		createMenu(MenuHandler*,bool istb);
    virtual void		handleMenuCB(CallBacker*);
    void			saveAreasAsCB(CallBacker*);

    void			prepareForShutdown();
    void			removeAll();
    void			removeLabels();
    void			removeOldUICContoursFromScene();

    void			intvChangeCB(CallBacker*);
    void			propChangeCB(CallBacker*);
    void			visClosingCB(CallBacker*);

    void			startCreateUICContours();
    bool			createPolyLines();
    bool			setLabels(visBase::Text2*);
    bool			computeUICContourSteps(const Array2D<float>&);
    void			getZVSAreaValues(TypeSet<float>& zvals,
						 TypeSet<float>& areas) const;

    void			showPropertyDlg();
    void			updateUICContours(const StepInterval<float>&);
    void			updateColumnText(int);
    void			updateZShift();

    Array2D<float>*		getDataSet(visSurvey::HorizonDisplay*);
    visSurvey::HorizonDisplay*	getHorDisp();

				// Specified from outside
    BufferString		attrnm_;
    Color			color_;
    float			zshift_;
    bool			showlabels_;
    int				linewidth_;
				// objects for contours
    visBase::PolyLine*		lines_;
    visBase::DrawStyle*		drawstyle_;
    visBase::Material*		material_;
    visBase::Text2*		labels_;
				//
    Interval<float>		contoursteprange_;
    TypeSet<double>		areas_; //empty if no non-udf
    StepInterval<float>		contourintv_;
    MenuItem			optionsmenuitem_;
    MenuItem			areamenuitm_;
    uiContourParsDlg*		propdlg_;

    friend class	uiContourTreeItemContourGenerator;

};

