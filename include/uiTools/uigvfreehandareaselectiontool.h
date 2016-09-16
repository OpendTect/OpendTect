#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Nov 2015
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "notify.h"

/*! \brief Tool that handles freehand selection of areas on graphics views.

  Enabling the tool will make sure that when the user starts dragging a polygon
  can be constructed by the user. When dragging starts, while dragging, and
  when it stops you can get notifications.

 */

class uiGraphicsView;
class uiPolygonItem;
template <class T> class ODPolygon;

mExpClass(uiTools) uiGVFreehandAreaSelectionTool : public CallBacker
{
public:

			uiGVFreehandAreaSelectionTool(uiGraphicsView&);
			~uiGVFreehandAreaSelectionTool();

    void		enable();
    void		disable();
    bool		isEnabled() const { return enabled_; }

    void		setPolygonMode(bool);
    bool		isPolygonMode() const;

    const ODPolygon<int>& selection() const { return odpolygon_; }

    Notifier<uiGVFreehandAreaSelectionTool>	started;
    Notifier<uiGVFreehandAreaSelectionTool>	pointAdded;
    Notifier<uiGVFreehandAreaSelectionTool>	stopped;

protected:

    uiGraphicsView&	gv_;
    uiPolygonItem*	polygonselitem_;
    ODPolygon<int>&	odpolygon_;


    bool		ispolygonmode_;
    bool		enabled_;

    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};
