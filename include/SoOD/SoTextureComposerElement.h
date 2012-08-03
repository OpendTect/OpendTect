#ifndef SoTextureComposerElement_h
#define SoTextureComposerElement_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		September 2008
 RCS:		$Id: SoTextureComposerElement.h,v 1.10 2012-08-03 13:00:42 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbLinear.h>
#include <Inventor/lists/SbList.h>
#include "SoTextureComposer.h"
#include "soodbasic.h"

class SbImage;

/* Element that holds one the active texture units for for SoTextureComposer
   and their transparency status. */

mClass(SoOD) SoTextureComposerElement : public SoReplacedElement
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

