#ifndef SoSplitTexture2Element_h
#define SoSplitTexture2Element_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoSplitTexture2Element.h,v 1.2 2007-11-19 22:47:20 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SbLinear.h>

/*!\brief

*/

class SoSplitTexture2Element : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoSplitTexture2Element);
public:
    static void			set(SoState*,SoNode*,int unit,const SbVec2s&,
				    const int numComponents,
				    const unsigned char* bytes);

    static const unsigned char*	get(const SoState*, int unit, SbVec2s& size,
				    int& numComponents);
	
    static void			initClass();
private:
				~SoSplitTexture2Element();
    void			setElt(int unit,const SbVec2s&,
	    			       const int numcomponents,
				       const unsigned char* bytes);

    SbList<int>				numcomps_;
    SbList<const unsigned char*>	bytes_;
    SbList<SbVec2s>			sizes_;
};

#endif

