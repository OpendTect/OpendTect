#ifndef visdepthtabplanedragger_h
#define visdepthtabplanedragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visdepthtabplanedragger.h,v 1.6 2004-11-16 09:29:17 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

template <class T> class Interval;

class SoDepthTabPlaneDragger;
class SoDragger;
class Coord3;
class IOPar;

namespace visBase
{

/*!\brief

*/

class DepthTabPlaneDragger : public VisualObjectImpl
{
public:
    static DepthTabPlaneDragger*	create()
					mCreateDataObj(DepthTabPlaneDragger);
    void				removeScaleTabs();
    					/*!\note once removed, they cannot be
					    restored */

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

    void			setDisplayTransformation( Transformation* );
    Transformation*		getDisplayTransformation();

    void			setOwnShape( SoNode* );

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);


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

    static const char*		dimstr;
    static const char*		sizestr;
    static const char*		centerstr;
};

};

#endif

