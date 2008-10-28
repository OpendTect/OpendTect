#ifndef SoTextureComposerElement_h
#define SoTextureComposerElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTextureComposerElement.h,v 1.3 2008-10-28 13:03:17 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbLinear.h>
#include <Inventor/lists/SbList.h>
#include "SoTextureComposer.h"

class SbImage;

/* Element that holds one the active texture units for for SoTextureComposer
   and their transparency status. */

class SoTextureComposerElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureComposerElement);
public:
    static void			set(SoState*,SoNode*,const SbList<int>&,char);

    static const SbList<int>&	getUnits(SoState*);
    static char			getTransparencyInfo(SoState*);
	
    static void			initClass();
private:
				~SoTextureComposerElement();
    void			setElt(const SbList<int>&, char );

    SbList<int>			units_;
    char			ti_;
};

#endif
