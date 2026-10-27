#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/core/Array.h>
#include <vsg/maths/vec3.h>
#include <vsg/maths/vec4.h>
#include <vsg/nodes/Group.h>

namespace vsgGeo
{

/*!\brief Axis-aligned plane with corner and edge handles.

  Coordinates are the caller's local space. Mouse buttons are not
  known here: the caller decides 1D or in-plane translation and passes
  a point already on the plane.
*/

class VSGGEO_EXPORT TabPlaneDragger : public vsg::Inherit<vsg::Group,
							   TabPlaneDragger>
{
public:
    enum Part	{ None, Plane, Handle };

    struct Hit
    {
	Part		part = None;
	int		handle = -1;
	vsg::dvec3	point;
    };

			TabPlaneDragger();

    void		setCenter(const vsg::dvec3&);
    const vsg::dvec3&	center() const		{ return center_; }
    void		setSize(const vsg::dvec3&);
    const vsg::dvec3&	size() const		{ return size_; }
    void		setNormalAxis(int);
    int			normalAxis() const	{ return axis_; }

    void		setSpaceLimits(int axis,double minv,double maxv);
    void		setSizeLimits(int axis,double minv,double maxv);
    void		removeScaleTabs();
    void		setShowPlane(bool);
    void		setShowBorder(bool);
    void		setColor(const vsg::vec4&);
    void		setTranslation1D(bool yn)	{ translate1d_ = yn; }
    bool		translation1D() const		{ return translate1d_; }

    Hit			hit(const vsg::dvec3& origin,
			    const vsg::dvec3& direction) const;
    void		begin(const Hit&);
    void		move(const vsg::dvec3& point);
    void		end();

protected:
			~TabPlaneDragger();

private:

    void		rebuild();
    void		clampCenter();
    vsg::dvec3		axisNormal() const;

    vsg::dvec3		center_ = { 0., 0., 0. };
    vsg::dvec3		size_ = { 1., 1., 1. };
    int			axis_ = 2;
    bool		handles_ = true;
    bool		showplane_ = true;
    bool		showborder_ = true;
    bool		translate1d_ = false;
    bool		dragging_ = false;
    Part		dragpart_ = None;
    int			draghandle_ = -1;
    vsg::dvec3		beginpoint_;
    vsg::dvec3		begincenter_;
    vsg::dvec3		beginsize_;
    double		spacemin_[3] = { -1e30, -1e30, -1e30 };
    double		spacemax_[3] = { 1e30, 1e30, 1e30 };
    double		sizemin_[3] = { 0., 0., 0. };
    double		sizemax_[3] = { 1e30, 1e30, 1e30 };

    vsg::ref_ptr<vsg::vec3Array>	planeverts_;
    vsg::ref_ptr<vsg::vec3Array>	lineverts_;
    vsg::ref_ptr<vsg::vec4Array>	planecolor_;
};


VSGGEO_EXPORT vsg::ref_ptr<TabPlaneDragger> createTabPlaneDragger();

} // namespace vsgGeo
