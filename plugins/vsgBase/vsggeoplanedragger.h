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
#include <vsg/nodes/DepthSorted.h>
#include <vsg/nodes/Group.h>

#include <vector>

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
    static int		nrCells();
			//!<Cells on each side of the plane's color grid
    void		setCellColors(const vsg::vec4*);
			/*!<nrCells() squared colors, row by row. Opaque cells
			    are drawn with the opaque objects. Cells with
			    transparency are blended in the transparent bin,
			    without writing depth. */
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

    static constexpr int cNrCells = 32;

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
    vsg::ref_ptr<vsg::vec3Array>	blendedverts_;
    vsg::ref_ptr<vsg::DepthSorted>	blendedplane_;
    std::vector<char>			cellopaque_;
};


VSGGEO_EXPORT vsg::ref_ptr<TabPlaneDragger> createTabPlaneDragger();

} // namespace vsgGeo
