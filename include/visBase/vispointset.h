#ifndef vispointset_h
#define vispointset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: vispointset.h,v 1.1 2006-04-19 22:20:13 cvskris Exp $
________________________________________________________________________


-*/

#include "visshape.h"
#include "position.h"

class SoPointSet;

namespace visBase
{

class PointSet	: public VertexShape
{
public:
    static PointSet*	create()
			mCreateDataObj(PointSet);
};



}; // Namespace


#endif
