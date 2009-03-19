#ifndef SoTextureMipMapBiasElement_h
#define SoTextureMipMapBiasElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		September 2008
 RCS:		$Id: SoTextureMipMapBiasElement.h,v 1.1 2009-03-19 14:46:30 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include "soodbasic.h"

class SbImage;

/* Element that sets the GL_TEXTURE_LOD_BIAS to alter which mipmap level that
   is used. Zero vales means no change, negative values means lower leve (higher
   resolution) an positive values yields lower resolutions.
   Element should be set prior to glTexture calls and is specific for each
   texture unit. */

mClass SoTextureMipMapBiasElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureMipMapBiasElement);
public:
    static void			set(SoState*,SoNode*,float bias);

    static void			initClass();
private:
				~SoTextureMipMapBiasElement();
    void			setElt(float);

    float			bias_;
};

#endif
