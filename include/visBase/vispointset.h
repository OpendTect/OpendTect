#ifndef vispointset_h
#define vispointset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: vispointset.h,v 1.2 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visshape.h"
#include "position.h"

class SoPointSet;

namespace visBase
{

mClass PointSet	: public VertexShape
{
public:
    static PointSet*	create()
			mCreateDataObj(PointSet);
};



}; // Namespace


#endif
