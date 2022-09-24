#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visdata.h"
#include "draw.h"
#include "visnodestate.h"

namespace osg {
    class Point;
    class LineStipple;
    class LineWidth;
};


namespace visBase
{
/*! \brief
*/


mExpClass(visBase) DrawStyle : public NodeState
{
public:
			DrawStyle();
    enum Style		{ Filled, Lines, Points, Invisible };

    void		setDrawStyle( Style );
    Style		getDrawStyle() const;

    void		setPointSize( float );
    float		getPointSize() const;

    void		setLineStyle(const OD::LineStyle&);
			/*!< Color in Linestyle is ignored, must be
			 set separately.
			 */

    void		setLineWidth(int);
    const OD::LineStyle& lineStyle() const		{ return linestyle_; }

    void		setPixelDensity(float) override;
    osg::LineStipple*	getLineStipple() { return linestippleattrib_; }


protected:
			~DrawStyle();

    void		updateLineStyle();

    OD::LineStyle	linestyle_;
    float		pointsize_;
    float		pixeldensity_;

    osg::Point*		pointsizeattrib_;
    osg::LineStipple*	linestippleattrib_;
    osg::LineWidth*	linewidthattrib_;
};

} // namespace visBase
