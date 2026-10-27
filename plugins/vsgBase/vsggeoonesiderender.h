#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoexport.h"

#include <vsg/maths/vec3.h>
#include <vsg/nodes/Group.h>

#include <vector>

namespace vsgGeo
{

/*!\brief Group that draws a child only when the camera is on the positive
  side of the plane through the child's position, normal to its direction.*/

class VSGGEO_EXPORT OneSideRender
		: public vsg::Inherit<vsg::Group,OneSideRender>
{
public:
				OneSideRender();

    void			setLine(size_t idx,const vsg::dvec3& pos,
					const vsg::dvec3& dir);
    size_t			nrLines() const { return lines_.size(); }
    const vsg::dvec3&		linePos( size_t idx ) const
				{ return lines_[idx].pos_; }
    const vsg::dvec3&		lineDir( size_t idx ) const
				{ return lines_[idx].dir_; }

    bool			isDrawn(size_t idx,
					const vsg::dvec3& eye) const;
				/*!<eye in the coordinates of this node */

    void			accept(vsg::RecordTraversal&) const override;

protected:
    struct Line
    {
	vsg::dvec3		pos_;
	vsg::dvec3		dir_;
    };

    std::vector<Line>		lines_;
};

} // namespace vsgGeo
