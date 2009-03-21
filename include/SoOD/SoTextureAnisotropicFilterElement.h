#ifndef SoTextureAnisotropicFilterElement_h
#define SoTextureAnisotropicFilterElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		September 2008
 RCS:		$Id: SoTextureAnisotropicFilterElement.h,v 1.1 2009-03-21 02:05:05 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include "soodbasic.h"

class SbImage;

/* Element that sets the GL_TEXTURE_MAX_ANISOTROPY_EXT, which is between
   1 and maxAnisotropy(). For information what this does, do a web search.

   Element should be set prior to glTexture calls and is specific for each
   texture unit. */

mClass SoTextureAnisotropicFilterElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureAnisotropicFilterElement);
public:
    static void			set(SoState*,SoNode*,float maxanisotropy);
    static bool			isCapable();
    static float		maxAnisotropy();

    static void			initClass();
private:
				~SoTextureAnisotropicFilterElement();
    void			setElt(float);

    float			curmaxanisotropy_;
};

#endif
