#ifndef SoTextureChannelSetElement_h
#define SoTextureChannelSetElement_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbLinear.h>
#include <Inventor/lists/SbList.h>

#include "soodbasic.h"

class SbImagei32;

/*!  Element that holds one image per texture unit.  */

mClass SoTextureChannelSetElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureChannelSetElement);
public:
    static void			set(SoState*,SoNode*,const SbImagei32*,
				    const int nrchannels,
				    const SbList<uint32_t>* addnodeids=0);
    				//!<The nodeid's will be added to the matchinfo

    static int			getNrChannels(SoState*);
    static const SbImagei32*	getChannels(SoState*);

    SbBool 			matches(const SoElement* element) const;
    SoElement* 			copyMatchInfo() const;
	
    static void			initClass();
private:
				~SoTextureChannelSetElement();
    void			setElt(const SbImagei32*, const int nrchannels);
    static SoElement*		getElement(SoState* const state,
				    const int stackIndex,
				    SoNode* const node,
				    const SbList<uint32_t>* additionalnodeids);


    const SbImagei32*		channels_;
    int				nrchannels_;
    SbList<uint32_t>		additionalnodeids_;
};

#endif

