#ifndef vistristripset_h
#define vistristripset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistristripset.h,v 1.2 2002-03-11 10:46:12 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "callback.h"

class SoCoordinate3;
class SoIndexedTriangleStripSet;
class CallBacker;

namespace Geometry { class TriangleStripSet; };


namespace visBase
{

/*!\brief


*/

class TriangleStripSet : public VisualObjectImpl
{
public:
    static TriangleStripSet*	create()
				mCreateDataObj0arg( TriangleStripSet );

    void		setStrips( Geometry::TriangleStripSet*, bool connect );
    			/*!< If connect is true, the object will use the
			     stripsets callbacks to keep itself updated.
			     stripset must then stay alive.
			*/

protected:
    			TriangleStripSet();
			~TriangleStripSet();
    void		updateCoords(CallBacker* =0);
    void		updateIndexes(CallBacker* =0);

    SoCoordinate3*		coords;
    SoIndexedTriangleStripSet*	strips;

    Geometry::TriangleStripSet*	stripset;
};


};


#endif
