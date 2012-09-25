#ifndef visshapehints_h
#define visshapehints_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		May 2007
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"

class SoShapeHints;

namespace visBase
{

/*!Controls the culling and rendering of an object. See Coin for details. */

mClass(visBase) ShapeHints : public DataObject
{
public:

    static ShapeHints*		create() mCreateDataObj(ShapeHints);

    enum VertexOrder		{ Unknown, ClockWise, CounterClockWise };
    void			setVertexOrder(VertexOrder);
    VertexOrder			getVertexOrder() const;

    void			setSolidShape(bool);
    bool			isSolidShape() const;

    void			touch();

protected:
    				~ShapeHints();

    SoShapeHints*		shapehints_;

    virtual SoNode*		gtInvntrNode();

};

}

#endif

