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

mExpClass(uiFlatView) uiAuxDataDisplay : public AuxData, public CallBacker
{
public:
    AuxData*			clone() const;

    void			setViewer(uiFlatViewer* d) {viewer_=d;}

    void			touch() 	{ updateCB(0); }

				~uiAuxDataDisplay();
    uiGraphicsItemGroup*	getDisplay();
    void			removeDisplay() { display_ = 0; }


protected:
    				friend class ::uiFlatViewer;
				uiAuxDataDisplay(const char* nm);
				uiAuxDataDisplay(const uiAuxDataDisplay&);

    void			updateTransformCB(CallBacker*);

    void			updateCB(CallBacker*);

    uiGraphicsItemGroup*	display_;
    uiPolygonItem*		polygonitem_;
    uiPolyLineItem*		polylineitem_;
    ObjectSet<uiMarkerItem>	markeritems_;
    uiTextItem*			nameitem_;
    uiFlatViewer*		viewer_;
};

}; //namespace

#endif

