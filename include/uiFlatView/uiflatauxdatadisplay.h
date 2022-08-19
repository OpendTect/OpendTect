#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "flatview.h"

class uiFlatViewer;
class uiGraphicsItemGroup;
class uiPolygonItem;
class uiPolyLineItem;
class uiMarkerItem;
class uiTextItem;

namespace FlatView
{

/*!
\brief Auxiliary data display of flatviewers.
*/

mExpClass(uiFlatView) uiAuxDataDisplay : public AuxData, public CallBacker
{
public:
				~uiAuxDataDisplay();

    AuxData*			clone() const override;
    void			setViewer( uiFlatViewer* fv ) { viewer_ = fv; }
    void			touch() override	{ updateCB(0); }

    uiGraphicsItemGroup*	getDisplay();
    void			removeDisplay();

protected:
    				friend class ::uiFlatViewer;
				uiAuxDataDisplay(const char* nm);
				uiAuxDataDisplay(const uiAuxDataDisplay&);

    void			removeItems();
    void			updateTransformCB(CallBacker*);
    void			updateCB(CallBacker*);

    uiGraphicsItemGroup*	display_ = nullptr;
    uiPolygonItem*		polygonitem_ = nullptr;
    uiPolyLineItem*		polylineitem_ = nullptr;
    ObjectSet<uiMarkerItem>	markeritems_;
    uiTextItem*			nameitem_ = nullptr;
    uiFlatViewer*		viewer_ = nullptr;
};

} // namespace FlatView
