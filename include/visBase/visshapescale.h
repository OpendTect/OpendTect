#ifndef visshapescale_h
#define visshapescale_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshapescale.h,v 1.5 2004-01-05 09:43:47 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoShapeScale;

namespace visBase
{

/*!\brief
Makes an object have a constant size on the screen. The object that should have
a constant size is given to the object via setShape.

*/

class ShapeScale : public DataObject
{
public:
    static ShapeScale*	create()
			mCreateDataObj(ShapeScale);

    void		setShape( SoNode* );
    void		setShape( DataObject* );
    DataObject*		getShape() { return shape; }

    void		setSize( float );
    float		getSize() const;

    void		freeze(bool yn);
    bool		isFrozen() const;

    SoNode*		getInventorNode();
    int			usePar( const IOPar& iopar );
    void		fillPar( IOPar& iopar, TypeSet<int>& saveids ) const;
	
protected:
    static const char*	shapeidstr;

    DataObject*		shape;
    SoShapeScale*	shapescalekit;
private:
    virtual		~ShapeScale();
};

}; // Namespace


#endif
