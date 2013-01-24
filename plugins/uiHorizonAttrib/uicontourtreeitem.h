#ifndef uicontourtreeitem_h
#define uicontourtreeitem_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Feb 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uioddatatreeitem.h"

#include "cubesampling.h"
#include "color.h"
#include "externalattrib.h"

class BinID;
class visContourLabels;

template <class T> class Array2DImpl;
template <class T> class Array2D;

namespace Attrib { class SelSpec; }
namespace visSurvey { class SurveyObject; class HorizonDisplay; }
namespace visBase
{
    class DrawStyle;
    class IndexedPolyLine;
    class Material;
    class Text2;
}

/*!\brief Tree item for Contour display on 3D horizons */

mClass(uiHorizonAttrib) uiContourTreeItem : public uiODDataTreeItem
{
public:
    static void			initClass();
				uiContourTreeItem(const char* parenttype);
				~uiContourTreeItem();

    static uiODDataTreeItem*	create(const Attrib::SelSpec&,const char*);
    void			setupChangeCB(CallBacker*);
    void			setAttribName( const char* attrnm )
				{ attrnm_ = attrnm; }

    static const char*		sKeyContourDefString();
    static const char*		sKeyZValue();

protected:

    bool			hasTransparencyMenu() const { return false; }
    void			prepareForShutdown();
    bool			init();

    void			removeAll();
    void			removeLabels();
    void			checkCB(CallBacker*);
    void			intvChangeCB(CallBacker*);
    void			propChangeCB(CallBacker*);
    void			visClosingCB(CallBacker*);
    void			createMenu(MenuHandler*,bool istb);
    void			handleMenuCB(CallBacker*);

    void			createContours();
    bool			computeContours(const Array2D<float>&,
	    					const StepInterval<int>&,
						const StepInterval<int>&);
    void			updateContours(const StepInterval<float>&);
    Array2D<float>*		getDataSet(visSurvey::HorizonDisplay*);
    void			updateColumnText(int);
    void			createLines();
    void			addText(const Coord3&,const char*);
    void			updateZShift();

    Array2DImpl<int>*		arr_;
    TypeSet<BinID>		bids_;
    Interval<float>		rg_;
    StepInterval<float>		contourintv_;

    visBase::IndexedPolyLine*	lines_;
    visBase::DrawStyle*		drawstyle_;
    visBase::Material*		material_;
    ObjectSet<visBase::Text2>	labels_;
    visContourLabels*		labelgrp_;
    
    Color			color_;
    int				linewidth_;
    MenuItem			optionsmenuitem_;
    float			zshift_;
    BufferString		attrnm_;
    bool			showlabels_;

    BufferString		createDisplayName() const;
};

#endif
