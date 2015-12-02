#ifndef uiflatauxdataeditor_h
#define uiflatauxdataeditor_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id$
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

    AuxData*			clone() const;
    void			setViewer( uiFlatViewer* fv ) { viewer_ = fv; }
    void			touch() 	{ updateCB(0); }

    uiGraphicsItemGroup*	getDisplay();
    void			removeDisplay();

protected:
    				friend class ::uiFlatViewer;
				uiAuxDataDisplay(const char* nm);
				uiAuxDataDisplay(const uiAuxDataDisplay&);

    void			removeItems();
    void			updateTransformCB(CallBacker*);
    void			updateCB(CallBacker*);

    uiGraphicsItemGroup*	display_;
    uiPolygonItem*		polygonitem_;
    uiPolyLineItem*		polylineitem_;
    ObjectSet<uiMarkerItem>	markeritems_;
    uiTextItem*			nameitem_;
    uiFlatViewer*		viewer_;
};

} // namespace FlatView

#endif
