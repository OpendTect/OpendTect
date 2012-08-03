#ifndef visrectangle_h
#define visrectangle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visrectangle.h,v 1.38 2012-08-03 13:01:26 cvskris Exp $
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "ranges.h"
#include "position.h"

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

mClass(visBase) RectangleDragger : public DataObject
{
public:
    static RectangleDragger*	create()
				mCreateDataObj(RectangleDragger);

    void			setCenter( const Coord3& );
    Coord3			center() const;
    
    void			setScale( float, float );
    float			scale( int dim ) const;

    void			setDraggerSize( float w, float h, float d );
    Coord3			getDraggerSize() const;

    void			showTabs(bool);
    bool			tabsShown() const;

    Notifier<RectangleDragger>	started;
    Notifier<RectangleDragger>	motion;
    Notifier<RectangleDragger>	changed;
    Notifier<RectangleDragger>	finished;

protected:
				~RectangleDragger();

    void			setOwnShapeHints();
    void			syncronizeDraggers();
    void			draggerHasMoved( SoDragger* );
    

    SoSeparator*		root;
    SoTranslate1Dragger* 	manipzdraggertop;
    SoTranslate1Dragger* 	manipzdraggerright;
    SoTranslate1Dragger* 	manipzdraggerbottom;
    SoTranslate1Dragger* 	manipzdraggerleft;
    SoScale*			zdraggerscale;

    SoTabPlaneDragger*		manipxydragger0;
    SoTabPlaneDragger*		manipxydragger1;
    SoSwitch*			tabswitch;

    bool			allowcb;

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    virtual SoNode*		gtInvntrNode();

};

/*!\brief
    A Rectangle is a rectangle that can be positioned in 3d. It has
    manipulators that can be used to move it around. The rectangle
    is limited to be parallel with the space, i.e. it's normal
    must be parallel with either the x, y or z axis.
    
    The ranges of the movement can be set, and the positions can
    be snapped.
*/

mClass(visBase) Rectangle : public VisualObjectImpl
{
public:
    static Rectangle*	create()
			mCreateDataObj(Rectangle);

    void		setOrigo( const Coord3& );
    Coord3		origo() const;
    Coord3		manipOrigo() const;

    void		setWidth(float,float);
    float		width(int,bool manip=true) const;

    void		setRanges(const StepInterval<float>&,
	    			  const StepInterval<float>&,
				  const StepInterval<float>&,bool);
    void		setRange(int dim, const StepInterval<float>& );
    void		setWidthRange( int dim, const Interval<float>&);
    enum Orientation	{ XY, XZ, YZ };
    void		setOrientation(Orientation);
    Orientation		orientation() const { return orientation_; }

    void		setSnapping(bool n) { snap = n; }
    bool		isSnapping() const { return snap; }

    void		showDraggers(bool);
    bool		draggersShown() const;
    void		showTabs(bool);
    void		setDraggerSize( float w, float h, float d );
    Coord3		getDraggerSize() const;


    void		moveObjectToManipRect();
    void		resetManip();
    bool		isManipRectOnObject() const;

    NotifierAccess*	manipStarts();
    NotifierAccess*	manipChanges();
    NotifierAccess*	manipEnds();

    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

protected:
			~Rectangle();
    void		moveManipRectangletoDragger(CallBacker* =0);
    void		moveDraggertoManipRect();

    float		snapPos(int dim,float pos) const;
    float		getWidth(int dim,float scale) const;
    float		getScale(int dim,float width) const;
    float		getStartPos(int dim,float centercrd,float scale) const;
    float		getStopPos(int dim,float centercoord,float scale) const;
    float		getCenterCoord(int dim,float startpos,float wdth) const;

    const SoNode*	getSelObj() const { return (const SoNode*) plane; }

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

    SoFaceSet*		plane;

    // Manip objects:
    SoSwitch*		manipswitch;
    RectangleDragger*	dragger;

    // Manip rectangle
    SoSwitch*		maniprectswitch;
    SoTranslation*	maniprecttrans;
    SoScale*		maniprectscale;

    static const char*	orientationstr();
    static const char*	origostr();
    static const char*	widthstr();
    static const char*	xrangestr();
    static const char*	yrangestr();
    static const char*	zrangestr();
    static const char*	xwidhtrange();
    static const char*	ywidhtrange();
    static const char*	draggersizestr();
    static const char*	snappingstr();
};

};
	
#endif

