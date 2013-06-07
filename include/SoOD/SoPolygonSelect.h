#ifndef SoPolygonSelect_h
#define SoPolygonSelect_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
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
class SoSceneManager;

/*!Paints a polygon or a rectangle just in front of near-clipping plane driven
   by mouse- movement. Once drawn, the polygon can be retrieved in 2D, and any
   point can be projected onto the 2D space and thus tested if it's inside
   or outside the polygon.

   The polygon is painted witht the current material and linestyle.
*/


mClass SoPolygonSelect : public SoNode
{
    friend class		SoTabletEventFilter;
    friend class		uiSoViewerBody;

    SO_NODE_HEADER(SoPolygonSelect);

public:
				SoPolygonSelect(void);
    static void			initClass(void);

    enum			Modes { OFF, RECTANGLE, POLYGON };
    SoSFEnum			mode;

    const SbList<SbVec2f>&	getPolygon() const	{ return polygon_; }
    SbVec2f			projectPointToScreen(const SbVec3f&) const;
    bool			projectPointFromScreen(const SbVec2f&,
						       SbLine&) const;

    SoCallbackList		paintStart;
    SoCallbackList		paintStop;
    SoCallbackList		polygonChange;
    void			clear();
    bool			isPainting() const 	{ return mousedown_; }

    const SoPath*		rayPickThrough(const SbVec3f& displaypos,
					       int depthidx=0) const;

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

    SbList<SbVec2f>	polybuf_;
    SbList<SbVec2f>	outdir_;
    SbVec2f		prevpos_;

    void		initRub( const SbVec2f& pt );
    void		rub( const SbVec2f& pt );

			// Not for casual use
    static void		setTabletPressure(float);
    static void		setActiveSceneManager(SoSceneManager*);
};

#endif
