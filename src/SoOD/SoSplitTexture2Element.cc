/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
 RCS:           $Id: SoSplitTexture2Element.cc,v 1.4 2008-09-22 13:35:51 cvskris Exp $
________________________________________________________________________

-*/

#include "SoSplitTexture2Element.h"

SO_ELEMENT_SOURCE( SoSplitTexture2Element );

void SoSplitTexture2Element::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoSplitTexture2Element, SoReplacedElement );
}


SoSplitTexture2Element::~SoSplitTexture2Element()
{}


void SoSplitTexture2Element::set( SoState* const state, SoNode* node,
				  int unit, const SbVec2s& size,
				  const int numcomponents,
				  const unsigned char* bytes,
       				  SoSplitTexture2::ForceTransparency ft)
{
    SoSplitTexture2Element * elem =
	(SoSplitTexture2Element *) getElement( state, classStackIndex, node );

    if ( elem )
	elem->setElt(unit, size, numcomponents, bytes, ft );
}


void SoSplitTexture2Element::setElt( int unit, const SbVec2s & size,
				  const int numcomponents,
				  const unsigned char* bytes,
       				   SoSplitTexture2::ForceTransparency ft )
{
    while ( numcomps_.getLength()<=unit )
    {
	numcomps_.append(0);
	bytes_.append(0);
	forcetrans_.append( SoSplitTexture2::DISABLE );
	sizes_.append( SbVec2s(0,0) );
    }

    numcomps_[unit] = numcomponents;
    bytes_[unit] = bytes;
    sizes_[unit] = size;
    forcetrans_[unit] = ft;
}


const unsigned char* SoSplitTexture2Element::get( SoState* state,
	int unit, SbVec2s& size, int& numcomponents,
        SoSplitTexture2::ForceTransparency& ft )
{
    const SoSplitTexture2Element* elem = (SoSplitTexture2Element*)
	getConstElement(state,classStackIndex);

    if ( unit>=elem->numcomps_.getLength() )
	return 0;

    size = elem->sizes_[unit];
    numcomponents = elem->numcomps_[unit];
    ft = elem->forcetrans_[unit];
    return elem->bytes_[unit];
}
