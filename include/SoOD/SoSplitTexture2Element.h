#ifndef SoSplitTexture2Element_h
#define SoSplitTexture2Element_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoSplitTexture2Element.h,v 1.11 2012-08-03 13:00:42 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SbLinear.h>
#include <SoSplitTexture2.h>

#include "soodbasic.h"

/*!  Element that holds one image per texture unit.  */

mClass(SoOD) SoSplitTexture2Element : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoSplitTexture2Element);
public:
    static void			set(SoState*,SoNode*,int unit,const SbVec2s&,
				    const int numcomponents,
				    const unsigned char* bytes, char ti);

    static const unsigned char*	get(SoState*, int unit, SbVec2s& size,
				    int& numcomponents, char& ti );
	
    static void			initClass();
private:
				~SoSplitTexture2Element();
    void			setElt(int unit,const SbVec2s&,
	    			       const int numcomponents,
				       const unsigned char* bytes, char ti);

    SbList<int>				numcomps_;
    SbList<const unsigned char*>	bytes_;
    SbList<SbVec2s>			sizes_;
    SbList<char>			ti_;
};

#endif


