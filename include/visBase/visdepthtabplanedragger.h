#ifndef visdepthtabplanedragger_h
#define visdepthtabplanedragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "ranges.h"
#include "visobject.h"


namespace osgManipulator { class TabPlaneDragger; }
namespace osg { class Switch; }


class SoDepthTabPlaneDragger;
class SoDragger;
class Coord3;
class IOPar;

namespace visBase
{

class PlaneDraggerCallbackHandler;

/*!\brief

*/

mExpClass(visBase) DepthTabPlaneDragger : public VisualObjectImpl
{
    friend class PlaneDraggerCallbackHandler;

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

    void			setDisplayTransformation( const mVisTrans* );
    const mVisTrans*		getDisplayTransformation() const;

    void			setOwnShape( SoNode* );

    void			showDraggerBorder(bool yn=true);
    bool			isDraggerBorderShown() const;

    void			showPlane(bool yn=true);
    bool			isPlaneShown() const;

    void			setTransDragKeys(bool depth,int keys);
    				/*!<\param depth specifies wheter the depth or
				    		 the plane setting should be
						 changed.
				   \param keys	 combination of OD::ButtonState
				   \note only shift/ctrl/alt are used. */
    int				getTransDragKeys(bool depth) const;
    				/*!<\param depth specifies wheter the depth or
				    		the plane setting should be
						returned.
				    \returns	combination of OD::ButtonState*/

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

    void			initOsgDragger();

    SoDepthTabPlaneDragger*	dragger_;

    int				dim_;
    Transformation*		rotation_;
    TypeSet<Coord3>		centers_;
    TypeSet<Coord3>		sizes_;

    const mVisTrans*		transform_;

    osgManipulator::TabPlaneDragger*	osgdragger_;
    osg::Switch*			osgdraggerplane_;
    PlaneDraggerCallbackHandler*	osgcallbackhandler_;

    Interval<float>		widthranges_[3];
    Interval<float>		spaceranges_[3];

private:
    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    static const char*		dimstr();
    static const char*		sizestr();
    static const char*		centerstr();
};

};

#endif


