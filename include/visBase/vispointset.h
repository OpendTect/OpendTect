#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
________________________________________________________________________


-*/

#include "visshape.h"
#include "sets.h"
#include "position.h"

namespace visBase
{

class DrawStyle;

mExpClass(visBase) PointSet : public VertexShape
{
public:
    static PointSet*	create()
			mCreateDataObj(PointSet);

    void		setPointSize(int);
    int			getPointSize() const;

    int			addPoint(const Coord3&);
    const Coord3	getPoint(int posidx,bool scenespace=false) const;
			/*!<\if scenespace is true, return display coordinates,
			false, return world coordinates.*/
    void		removeAllPoints();
    int			size() const ;

    void		setDisplayTransformation( const mVisTrans* );
			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.  */
    void		clear();

protected:
			~PointSet();

    DrawStyle*		drawstyle_;
};

} // namespace visBase
