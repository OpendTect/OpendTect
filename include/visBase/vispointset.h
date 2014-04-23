#ifndef vispointset_h
#define vispointset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "sets.h"
#include "visshape.h"
#include "position.h"

namespace visBase
{

class DrawStyle;

mExpClass(visBase) PointSet	: public VertexShape
{
public:
    static PointSet*	create()
			mCreateDataObj(PointSet);

    void		setPointSize(int);
    int			getPointSize() const;

    int			addPoint( const Coord3& pos );
    const Coord3	getPoint( int ) const;
    void		removeAllPoints();
    int			size() const ;

    void		setDisplayTransformation( const mVisTrans* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.  */

protected:
					~PointSet();
    DrawStyle*				drawstyle_;
};


}; // Namespace


#endif

