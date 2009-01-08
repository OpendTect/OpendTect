#ifndef visshapehints_h
#define visshapehints_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kris Tingdahl
 Date:		May 2007
 RCS:		$Id: visshapehints.h,v 1.2 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoShapeHints;

namespace visBase
{

/*!Controls the culling and rendering of an object. See Coin for details. */

mClass ShapeHints : public DataObject
{
public:
    static ShapeHints*		create() mCreateDataObj(ShapeHints);

    enum VertexOrder		{ Unknown, ClockWise, CounterClockWise };
    void			setVertexOrder(VertexOrder);
    VertexOrder			getVertexOrder() const;

    void			setSolidShape(bool);
    bool			isSolidShape() const;

    SoNode*			getInventorNode();

protected:
    				~ShapeHints();

    SoShapeHints*		shapehints_;
};

};

#endif
