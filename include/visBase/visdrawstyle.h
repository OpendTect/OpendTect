#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
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

    void			updateLineStyle();

    OD::LineStyle		linestyle_;
    float			pointsize_;
    float			pixeldensity_;

    osg::Point*		pointsizeattrib_;
    osg::LineStipple*		linestippleattrib_;
    osg::LineWidth*		linewidthattrib_;
};

} // namespace visBase


