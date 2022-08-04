#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		August 2002
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"


namespace osgGeo { class TabBoxDragger; }
namespace osg { class ShapeDrawable; }



namespace visBase
{

class BoxDraggerCallbackHandler;

mExpClass(visBase) BoxDragger : public VisualObjectImpl
{
public:
    friend class BoxDraggerCallbackHandler;

    static BoxDragger*		create()
				mCreateDataObj(BoxDragger);

    void			setCenter(const Coord3&);
    Coord3			center() const;

    void			setWidth(const Coord3&);
    Coord3			width() const;

    void			setBoxTransparency(float);
				//!<Between 0 and 1
    void			showScaleTabs(bool);

    void			setSpaceLimits(const Interval<float>&,
					       const Interval<float>&,
					       const Interval<float>&);

    void			setWidthLimits(const Interval<float>& x,
					       const Interval<float>& y,
					       const Interval<float>& z );

    void			showDraggerBorder(bool yn=true);
    bool			isDraggerBorderShown() const;

    bool			selectable() const override
				{ return selectable_; }
    void			setSelectable(bool yn) { selectable_ = yn; }

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setPlaneTransDragKeys(bool depth,int keys);
				/*!<\param depth tells whether the setting
				    for in-depth or in-plane translation
				    of the box planes should be changed.
				    \param keys combination of OD::ButtonState
				    \note only shift/ctrl/alt are used. */


    int				getPlaneTransDragKeys(bool depth) const;
				/*!<\param depth tells whether the setting
				    for in-depth or in-plane translation
				    of the box should be returned.
				    \returns combination of OD::ButtonState */

    void			useInDepthTranslationForResize(bool);
    bool			isInDepthTranslationUsedForResize() const;
    void			setDragCtrlSpacing(const StepInterval<float>&,
						   const StepInterval<float>&,
						   const StepInterval<float>&);


    Notifier<BoxDragger>	started;
    Notifier<BoxDragger>	motion;
    Notifier<BoxDragger>	changed;
    Notifier<BoxDragger>	finished;

protected:
					~BoxDragger();

    ConstRefMan<mVisTrans>		transform_;

    void				setOsgMatrix(const Coord3& worldscale,
						     const Coord3& worldtrans);

    osgGeo::TabBoxDragger*		osgboxdragger_;
    osg::ShapeDrawable*			osgdraggerbox_;
    BoxDraggerCallbackHandler*		osgcallbackhandler_;

    Interval<float>			widthranges_[3];
    Interval<float>			spaceranges_[3];

    StepInterval<float>			dragctrlspacing_[3];

    bool				selectable_;
    bool				useindepthtransforresize_;
};

};

