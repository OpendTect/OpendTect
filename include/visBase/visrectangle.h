#ifndef visrectangle_h
#define visrectangle_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visrectangle.h,v 1.1 2002-02-06 22:30:19 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "vismanipobj.h"
#include "visselobj.h"
#include "ranges.h"

class SoScale;
class SoTranslation;
class SoSwitch;
class SoTabPlaneDragger;
class SoRotation;
class SoTranslate1Dragger;
class SoFaceSet;


namespace visBase
{

/*!\brief
    A Rectangle is a rectangle that can be positioned in 3d. It has
    manipulators that can be used to move it around. The rectangle
    is limited to be parallel with the space, i.e. it's normal
    must be parallel with either the x, y or z axis.
    
    The ranges of the movement can be set, and the positions can
    be snapped.
*/

class Rectangle : public VisualObject, public ManipObject,
		  public SelectableObject
{
public:

			Rectangle( bool xymanip, bool zmanip );
			~Rectangle();
    void		setOrigo( float, float, float );
    float		origo( int ) const;

    void		setWidth( float, float );
    float		width( int ) const;

    void		setRange(int dim, const StepInterval<float>& );
    void		setWidthRange( int dim, const Interval<float>&);
    enum Orientation	{ XY, XZ, YZ };
    void		setOrientation(Orientation);

    void		setSnapping(bool n) { snap = n; }
    bool		isSnapping() const { return snap; }

    void		displayDraggers(bool);

    bool		moveObjectToManipRect();
    void		resetManip();

protected:

    void		manipDataChanged()
			{
			    updateDraggers();
			    moveManipRectangletoDragger();
			}
    void		manipEnds( bool ) 
			{  moveDraggertoManipRect(); }


    void		select(SoPath*) { displayDraggers(true); }
    void		deSelect(SoPath*)
			{
			    displayDraggers(false);
			    moveObjectToManipRect();
			}

    void		moveManipRectangletoDragger();
    void		moveDraggertoManipRect();

    void		updateDraggers();

    float		snapPos(int dim,float pos) const;
    float		getWidth(int dim,float scale) const;
    float		getScale(int dim,float width) const;
    float		getStartPos(int dim,float centercrd,float scale) const;
    float		getStopPos(int dim,float centercoord,float scale) const;
    float		getCenterCoord(int dim,float startpos,float wdth) const;

    bool		snap;
    Orientation		orientation;

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
    SoFaceSet*		plane;

    // Manip objects:
    SoSwitch*		manipswitch;

    SoTranslate1Dragger* manipzdragger;
    SoTabPlaneDragger*	manipxydragger;
    SoScale*		manipxyscale;

    // Manip rectangle
    SoSwitch*		maniprectswitch;
    SoTranslation*	maniprecttrans;
    SoScale*		maniprectscale;

};

};
	
#endif
