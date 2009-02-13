#ifndef SoTextureComposerElement_h
#define SoTextureComposerElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		September 2008
 RCS:		$Id: SoTextureComposerElement.h,v 1.8 2009-02-13 10:47:31 cvsnanne Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbLinear.h>
#include <Inventor/lists/SbList.h>
#include "SoTextureComposer.h"
#include "soodbasic.h"

class SbImage;

/* Element that holds one the active texture units for for SoTextureComposer
   and their transparency status. */

mClass SoTextureComposerElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureComposerElement);
public:
    static void			set(SoState*,SoNode*,const SbList<int>&,
	    			    char ti);

    static const SbList<int>&	getUnits(SoState*);
    static char			getTransparencyInfo(SoState*);
	
    static void			initClass();
private:
				~SoTextureComposerElement();
    void			setElt(const SbList<int>&,char ti);

    SbList<int>			units_;
    char			ti_;
};

#endif
