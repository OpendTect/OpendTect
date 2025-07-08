#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uioddatatreeitem.h"

#include "color.h"
#include "emobject.h"
#include "externalattrib.h"
#include "indexedshape.h"
#include "polygon.h"
#include "trckeyzsampling.h"
#include "visdata.h"
#include "visdrawstyle.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"


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

mClass(uiODMain) uiContourTreeItem : public uiODDataTreeItem
{ mODTextTranslationClass(uiContourTreeItem)
public:

    static void			initClass();
				uiContourTreeItem(const char* parenttype);
				~uiContourTreeItem();

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			setAttribName( const char* attrnm )
				{ attrnm_ = attrnm; }
    void			show(bool yn) override;

    static const char*		sKeyContourDefString();
    static const char*		sKeyZValue();
    int				getNrContours() const;

    static BufferString		selectAttribute(uiParent*,const MultiID&);

private:

    bool			init() override;
    bool			hasTransparencyMenu() const override
				{ return false; }

    uiString			createDisplayName() const override;
    void			checkCB(CallBacker*) override;
    bool			doubleClick(uiTreeViewItem*) override;
    void			createMenu(MenuHandler*,bool istb) override;
    void			handleMenuCB(CallBacker*) override;
    void			saveAreasAsCB(CallBacker*);

    void			prepareForShutdown() override;
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
    void			updateColumnText(int) override;
    void			updateZShift();

    Array2D<float>*		getDataSet(visSurvey::HorizonDisplay*);
    visSurvey::HorizonDisplay*	getHorDisp();

				// Specified from outside
    BufferString		attrnm_;
    OD::Color			color_;
    float			zshift_;
    bool			showlabels_;
    int				linewidth_;

				// objects for contours
    RefMan<visBase::PolyLine>	lines_;
    RefMan<visBase::DrawStyle>	drawstyle_;
    RefMan<visBase::Material>	material_;
    RefMan<visBase::Text2>	labels_;

    Interval<float>		contoursteprange_;
    TypeSet<double>		areas_; //empty if no non-udf
    StepInterval<float>		contourintv_;
    MenuItem			optionsmenuitem_;
    MenuItem			areamenuitm_;
    uiContourParsDlg*		propdlg_;

    friend class	uiContourTreeItemContourGenerator;
};
