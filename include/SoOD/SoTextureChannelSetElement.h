#ifndef SoTextureChannelSetElement_h
#define SoTextureChannelSetElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTextureChannelSetElement.h,v 1.1 2008-09-16 16:17:01 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbLinear.h>

class SbImage;

/*!  Element that holds one image per texture unit.  */

class SoTextureChannelSetElement : public SoReplacedElement
{
    SO_ELEMENT_HEADER(SoTextureChannelSetElement);
public:
    static void			set(SoState*,SoNode*,const SbImage*,
				    const int nrchannels);

    static int			getNrChannels(SoState*);
    static const SbImage*	getChannels(SoState*);
	
    static void			initClass();
private:
				~SoTextureChannelSetElement();
    void			setElt(const SbImage*, const int nrchannels);

    const SbImage*			channels_;
    int					nrchannels_;
};

#endif

