#ifndef visshapescale_h
#define visshapescale_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshapescale.h,v 1.9 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoShapeScale;
class SoSeparator;

namespace visBase
{

/*!\brief
Makes an object have a constant size on the screen. The object that should have
a constant size is given to the object via setShape.

*/

mClass ShapeScale : public DataObject
{
public:
    static ShapeScale*	create()
			mCreateDataObj(ShapeScale);

    void		setShape( SoNode* );
    void		setShape( DataObject* );
    DataObject*		getShape() { return shape; }

    void		setScreenSize( float );
    			/*!<If size is set to zero, the size won't be changed */
    float		getScreenSize() const;
    void		setMinScale( float );
    float		getMinScale() const;
    void		setMaxScale( float );
    float		getMaxScale() const;

    void		restoreProportions(bool yn);
    bool		restoresProportions() const;

    SoNode*		getInventorNode();
    int			usePar( const IOPar& iopar );
    void		fillPar( IOPar& iopar, TypeSet<int>& saveids ) const;
	
protected:
    static const char*	shapeidstr;

    SoSeparator*	root;
    SoNode*		node;
    DataObject*		shape;
    SoShapeScale*	shapescalekit;
private:
    virtual		~ShapeScale();
};

}; // Namespace


#endif
