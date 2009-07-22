#ifndef vispointset_h
#define vispointset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: vispointset.h,v 1.4 2009-07-22 16:01:24 cvsbert Exp $
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
