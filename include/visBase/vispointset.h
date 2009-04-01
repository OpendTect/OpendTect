#ifndef vispointset_h
#define vispointset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: vispointset.h,v 1.3 2009-04-01 07:02:03 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "sets.h"
#include "visshape.h"
#include "position.h"

class SoPointSet;
class Coord3;

namespace visBase
{

mClass PointSet	: public VertexShape
{
public:
    static PointSet*	create()
			mCreateDataObj(PointSet);
};

mClass IndexedPointSet : public IndexedShape
{
public:
    static IndexedPointSet*	create()
				mCreateDataObj(IndexedPointSet);
};

}; // Namespace


#endif
