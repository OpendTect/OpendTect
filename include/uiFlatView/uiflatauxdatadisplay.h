#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
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

/*!\brief Auxiliary data display of flatviewers. */

mExpClass(uiFlatView) uiAuxDataDisplay : public AuxData, public CallBacker
{
public:
				~uiAuxDataDisplay();

    AuxData*			clone() const;
    void			setViewer( uiFlatViewer* fv ) { viewer_ = fv; }
    void			touch()	{ updateCB(0); }

    uiGraphicsItemGroup*	getDisplay();
    void			removeDisplay();

protected:

				friend class ::uiFlatViewer;
				uiAuxDataDisplay(const uiString&);
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
