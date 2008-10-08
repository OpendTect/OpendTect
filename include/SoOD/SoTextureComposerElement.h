#ifndef SoTextureComposerElement_h
#define SoTextureComposerElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTextureComposerElement.h,v 1.1 2008-10-08 15:39:46 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbLinear.h>
#include <Inventor/lists/SbList.h>

class SbImage;

/*!  Element that holds one image per texture unit.  */

class SoTextureComposerElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureComposerElement);
public:
    static void			set(SoState*,SoNode*,const SbList<int>&);

    static const SbList<int>&	getUnits(SoState*);
	
    static void			initClass();
private:
				~SoTextureComposerElement();
    void			setElt(const SbList<int>&);

    SbList<int>			units_;
};

#endif

