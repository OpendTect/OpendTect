#ifndef visrectangle_h
#define visrectangle_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visrectangle.h,v 1.5 2002-02-27 07:20:56 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "ranges.h"
#include "callback.h"

class SoScale;
class SoTranslation;
class SoSwitch;
class SoTabPlaneDragger;
class SoRotation;
class SoTranslate1Dragger;
class SoFaceSet;
class SoDragger;


namespace visBase
{

class RectangleDragger : public SceneObject, public CallBackClass
{
public:
    			RectangleDragger();
			~RectangleDragger();

    void		setCenter( float, float, float );
    float		center( int dim ) const;
    
    void		setScale( float, float );
    float		scale( int dim ) const;

    void		setDraggerSize( float w, float h, float d );

    Notifier<RectangleDragger>	started;
    Notifier<RectangleDragger>	motion;
    Notifier<RectangleDragger>	changed;
    Notifier<RectangleDragger>	finished;

    SoNode*		getData();

protected:
    void		syncronizeDraggers();
    void		draggerHasMoved( SoDragger* );
    

    SoSeparator*	root;
    SoTranslate1Dragger* manipzdraggertop;
    SoTranslate1Dragger* manipzdraggerright;
    SoTranslate1Dragger* manipzdraggerbottom;
    SoTranslate1Dragger* manipzdraggerleft;
    SoScale*		zdraggerscale;

    SoTabPlaneDragger*	manipxydragger;

    bool		allowcb;

    static void		startCB( void*, SoDragger* );
    static void		motionCB( void*, SoDragger* );
    static void		valueChangedCB(void*, SoDragger* );
    static void		finishCB( void*, SoDragger* );
};

/*!\brief
    A Rectangle is a rectangle that can be positioned in 3d. It has
    manipulators that can be used to move it around. The rectangle
    is limited to be parallel with the space, i.e. it's normal
    must be parallel with either the x, y or z axis.
    
    The ranges of the movement can be set, and the positions can
    be snapped.
*/

class Rectangle : public VisualObject, public CallBacker
{
public:

			Rectangle( Scene&, bool manip );
			~Rectangle();
    void		setOrigo( float, float, float );
    float		origo( int ) const;
    float		manipOrigo(int) const;

    void		setWidth( float, float );
    float		width( int ) const;

    void		setRange(int dim, const StepInterval<float>& );
    void		setWidthRange( int dim, const Interval<float>&);
    enum Orientation	{ XY, XZ, YZ };
    void		setOrientation(Orientation);
    Orientation		orientation() const { return orientation_; }

    void		setSnapping(bool n) { snap = n; }
    bool		isSnapping() const { return snap; }

    void		displayDraggers(bool);
    void		setDraggerSize( float w, float h, float d );


    void		moveObjectToManipRect(CallBacker* =0);
    void		resetManip();
    bool		isManipRectOnObject() const;

protected:

    void		select() { displayDraggers(true); }
    void		deSelect()
			{
			    displayDraggers(false);
			    moveObjectToManipRect();
			}

    void		moveManipRectangletoDragger(CallBacker* =0);
    void		moveDraggertoManipRect();

    float		snapPos(int dim,float pos) const;
    float		getWidth(int dim,float scale) const;
    float		getScale(int dim,float width) const;
    float		getStartPos(int dim,float centercrd,float scale) const;
    float		getStopPos(int dim,float centercoord,float scale) const;
    float		getCenterCoord(int dim,float startpos,float wdth) const;

    bool		snap;
    Orientation		orientation_;

    StepInterval<float>	xrange;
    StepInterval<float>	yrange;
    StepInterval<float>	zrange;

    Interval<float>	wxrange;
    Interval<float>	wyrange;

    SoTranslation*	origotrans;
    SoRotation*		orientationrot;
    SoTranslation*	localorigotrans;
    SoScale*		localscale;
    SoScale*		widthscale;
    SoSeparator*	planesep;

    SceneObjectWrapper	planewrapper;

    // Manip objects:
    SoSwitch*		manipswitch;
    RectangleDragger*	dragger;

    // Manip rectangle
    SoSwitch*		maniprectswitch;
    SoTranslation*	maniprecttrans;
    SoScale*		maniprectscale;

};

};
	
#endif
