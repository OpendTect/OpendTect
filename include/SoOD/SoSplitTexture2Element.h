#ifndef SoSplitTexture2Element_h
#define SoSplitTexture2Element_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoSplitTexture2Element.h,v 1.1 2007-11-19 13:50:16 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoTextureImageElement.h>

/*!\brief

*/

class SoSplitTexture2Element : public SoTextureImageElement
{
    SO_ELEMENT_HEADER(SoSplitTexture2Element);
public:
    static void			set(SoState*,SoNode*, const SbVec2s&,
				    const int numComponents,
				    const unsigned char* bytes,
				    const Wrap wrapS, const Wrap wrapT,
				    const Model model,
				    const SbColor& blendColor);

    static const unsigned char*	getImage( const SoState*, SbVec2s& size,
					  int& numComponents);
	
    static void			initClass();
private:

    				~SoSplitTexture2Element();
};

#endif

