#ifndef SoShapeScale_h
#define SoShapeScale_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoShapeScale.h,v 1.1 2002-07-12 07:31:38 kristofer Exp $
________________________________________________________________________


-*/

#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFFloat.h>


/*!\brief
The SoShapeScale class is used for scaling a shape based on projected size.

This nodekit can be inserted in your scene graph to add for instance
3D markers that will be of a constant projected size.

The marker shape is stored in the "shape" part. Any kind of node
can be used, even group nodes with several shapes, but the marker
shape should be approximately of unit size, and with a center 
position in (0, 0, 0).
	      
*/

class SoShapeScale : public SoBaseKit
{
    typedef SoBaseKit inherited;

			SO_KIT_HEADER(SoShapeScale);
			SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
			SO_KIT_CATALOG_ENTRY_HEADER(scale);
			SO_KIT_CATALOG_ENTRY_HEADER(shape);

public:
			SoShapeScale(void);
    static void		initClass(void);


    SoSFFloat		active;
    SoSFFloat		projectedSize;

protected:
    virtual void	GLRender(SoGLRenderAction * action);
    virtual		~SoShapeScale();
};

#endif
