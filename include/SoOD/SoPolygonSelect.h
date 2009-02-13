#ifndef SoPolygonSelect_h
#define SoPolygonSelect_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoPolygonSelect.h,v 1.4 2009-02-13 10:47:31 cvsnanne Exp $
________________________________________________________________________


-*/

#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/SbLinear.h>
#include <Inventor/lists/SoCallbackList.h>

#include "soodbasic.h"

class SoGLRenderAction;
class SoHandleEventAction;
class SoCache;

/*!Paints a polygon or a rectangle just in front of near-clipping plane driven
   by mouse- movement. Once drawn, the polygon can be retrieved in 2D, and any
   point can be projected onto the 2D space and thus tested if it's inside
   or outside the polygon.

   The polygon is painted witht the current material and linestyle.
*/


mClass SoPolygonSelect : public SoNode
{
    SO_NODE_HEADER(SoPolygonSelect);
public:
				SoPolygonSelect(void);
    static void			initClass(void);

    enum			Modes { OFF, RECTANGLE, POLYGON };
    SoSFEnum			mode;

    const SbList<SbVec2f>&	getPolygon() const	{ return polygon_; }
    SbVec2f			projectPoint(const SbVec3f&) const;

    SoCallbackList		paintStart;
    SoCallbackList		paintStop;
    SoCallbackList		polygonChange;
    bool			isPainting() const 	{ return mousedown_; }

protected:
			~SoPolygonSelect();
    void		GLRender(SoGLRenderAction*);
    void		handleEvent(SoHandleEventAction*);

    SbList<SbVec2f>	polygon_;
    bool		mousedown_;
    bool		isgrabbing_;
    SbViewVolume	vv_;
    SbMatrix		modelmatrix_;

    SoCache*		dependencychecker_;
};

#endif
