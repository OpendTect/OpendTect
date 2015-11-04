#ifndef uigvfreehandareaselectiontool_h
#define uigvfreehandareaselectiontool_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          Nov 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "callback.h"

/*! \brief Tool that handles freehand selection of areas on graphics views.

  Enabling the tool will make sure that when the user starts dragging a polygon
  can be constructed by the user. When dragging starts, while dragging, and
  when it stops you can get notifications.

 */

class uiGraphicsView;
class uiPolygonItem;
namespace OD{ template <class T> class Polygon; };

mExpClass(uiTools) uiGVFreehandAreaSelectionTool : public CallBacker
{
public:

			uiGVFreehandAreaSelectionTool(uiGraphicsView&);
			~uiGVFreehandAreaSelectionTool();

    void		enable();
    void		disable();
    bool		isEnabled() const;

    void		setPolygonMode(bool);
    bool		isPolygonMode() const;

    const OD::Polygon<int>& selection();

    Notifier<uiGVFreehandAreaSelectionTool>	started;
    Notifier<uiGVFreehandAreaSelectionTool>	pointAdded;
    Notifier<uiGVFreehandAreaSelectionTool>	stopped;

protected:

    uiGraphicsView&	gv_;
    uiPolygonItem*	polygonselitem_;
    
    bool		ispolygonmode_;
    bool		enabled_;

    void		mousePressCB(CallBacker*);
    void		mouseReleaseCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};


#endif

