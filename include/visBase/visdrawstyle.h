#ifndef visdrawstyle_h
#define visdrawstyle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id$
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
    			DeclareEnumUtils(Style);

    void		setDrawStyle( Style );
    Style		getDrawStyle() const;

    void		setPointSize( float );
    float		getPointSize() const;

    void		setLineStyle( const LineStyle& );
			/*!< Color in Linestyle is ignored, must be
			 set separately.
			 */

    void 		setLineWidth(int);
    const LineStyle&	lineStyle() const 		{ return linestyle_; }

    int			usePar( const IOPar& );
    void		fillPar( IOPar& ) const;

protected:

    void			updateLineStyle();

    LineStyle			linestyle_;

    osg::Point*			pointsize_;
    osg::LineStipple*		linestipple_;
    osg::LineWidth*		linewidth_;

    static const char*	linestylestr();
    static const char*	drawstylestr();
    static const char*	pointsizestr();
};

};


#endif

