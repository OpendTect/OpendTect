#ifndef uiflatauxdataeditor_h
#define uiflatauxdataeditor_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdatadisplay.h,v 1.1 2012-04-05 12:09:48 cvskris Exp $
________________________________________________________________________

-*/

#include "flatview.h"

class uiFlatViewer;
class uiGraphicsItemGroup;
class uiPolygonItem;
class uiPolyLineItem;
class uiMarkerItem;
class uiTextItem;

namespace FlatView
{

mClass uiAuxDataDisplay : public AuxData, public CallBacker
{
public:
    AuxData*			clone() const;

    void			setViewer(uiFlatViewer* d) {viewer_=d;}

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
