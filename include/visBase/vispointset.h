#ifndef vispointset_h
#define vispointset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: vispointset.h,v 1.5 2009/10/21 06:18:56 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "visshape.h"
#include "position.h"

class SoPointSet;
class Coord3;

namespace visBase
{

class DrawStyle;

mClass PointSet	: public VertexShape
{
public:
    static PointSet*	create()
			mCreateDataObj(PointSet);

    void		setPointSize(int);
    int			getPointSize() const;

protected:
    DrawStyle*		drawstyle_;
};

mClass IndexedPointSet : public IndexedShape
{
public:
    static IndexedPointSet*	create()
				mCreateDataObj(IndexedPointSet);
};

}; // Namespace


#endif
