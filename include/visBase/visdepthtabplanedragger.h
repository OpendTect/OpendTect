#ifndef visdepthtabplanedragger_h
#define visdepthtabplanedragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdepthtabplanedragger.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________


-*/

#include "visobject.h"

template <class T> class Interval;

class SoDepthTabPlaneDragger;
class Coord3;
class SoDragger;

namespace visBase
{

/*!\brief

*/

class DepthTabPlaneDragger : public VisualObjectImpl
{
public:
    static DepthTabPlaneDragger*	create()
					mCreateDataObj(DepthTabPlaneDragger);

    void			setCenter( const Coord3&, bool alldims = true );
    				/*!< \param alldims if true, it updates the
				            internal cache, so the new position
					    is valid through a setDim() 
				*/
    Coord3			center() const;

    void			setSize( const Coord3&, bool alldims=true );
    				/*!< \param alldims if true, it updates the
				            internal cache, so the new position
					    is valid through a setDim() 
				*/
    Coord3			size() const;

    void			setDim(int dim);
    				/*!< Sets the dim of the plane's normal
				    \param dim=0 x-axis
				    \param dim=1 y-axis
				    \param dim=2 z-axis
				*/
    int				getDim() const;

    void			setSpaceLimits( const Interval<float>& x,
	    					const Interval<float>& y,
						const Interval<float>& z );
    void			getSpaceLimits( Interval<float>& x,
	    					Interval<float>& y,
						Interval<float>& z ) const;

    void			setWidthLimits( const Interval<float>& x,
	    					const Interval<float>& y,
						const Interval<float>& z );
    void			getWidthLimits( Interval<float>& x,
	    					Interval<float>& y,
						Interval<float>& z ) const;

    void			setTransformation( Transformation* );
    Transformation*		getTransformation();

    void			setOwnShape( SoNode* );


    Notifier<DepthTabPlaneDragger>  started;
    Notifier<DepthTabPlaneDragger>  motion;
    Notifier<DepthTabPlaneDragger>  changed;
    Notifier<DepthTabPlaneDragger>  finished;

protected:
    				~DepthTabPlaneDragger();
    Coord3			world2Dragger( const Coord3&, bool pos) const;
    Coord3			dragger2World( const Coord3&, bool pos) const;

    SoDepthTabPlaneDragger*	dragger;
    SoSeparator*		ownshape;

    int				dim;
    visBase::Transformation*	rotation;
    TypeSet<Coord3>		centers;
    TypeSet<Coord3>		sizes;

    visBase::Transformation*	transform;

private:
    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

};

};

#endif

