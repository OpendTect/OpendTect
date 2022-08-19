#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "ranges.h"
#include "visobject.h"


namespace osgGeo { class TabPlaneDragger; }
namespace osg { class Switch; }


class Coord3;

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
				    \param dim =0 x-axis
				    dim=1 y-axis
				    dim=2 z-axis
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

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

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
    void			setDragCtrlSpacing(const StepInterval<float>&,
						   const StepInterval<float>&,
						   const StepInterval<float>&);

    Notifier<DepthTabPlaneDragger>  started;
    Notifier<DepthTabPlaneDragger>  motion;
    Notifier<DepthTabPlaneDragger>  changed;
    Notifier<DepthTabPlaneDragger>  finished;

protected:
				~DepthTabPlaneDragger();

    void			setOsgMatrix(const Coord3& worldscale,
					     const Coord3& worldtrans);

    void			initOsgDragger();

    int				dim_;
    TypeSet<Coord3>		centers_;
    TypeSet<Coord3>		sizes_;

    ConstRefMan<mVisTrans>	transform_;

    osgGeo::TabPlaneDragger*	osgdragger_;
    osg::Switch*			osgdraggerplane_;
    PlaneDraggerCallbackHandler*	osgcallbackhandler_;

    Interval<float>		widthranges_[3];
    Interval<float>		spaceranges_[3];
    StepInterval<float>		dragctrlspacing_[3];
};

} // namespace visBase
