/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          November 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoSplitTexture2Element.cc,v 1.7 2009/07/22 16:01:35 cvsbert Exp $";

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
				  const unsigned char* bytes, char ti)
{
    SoSplitTexture2Element * elem =
	(SoSplitTexture2Element *) getElement( state, classStackIndex, node );

    if ( elem )
	elem->setElt(unit, size, numcomponents, bytes, ti );
}


void SoSplitTexture2Element::setElt( int unit, const SbVec2s & size,
				  const int numcomponents,
				  const unsigned char* bytes, char ti )
{
    while ( numcomps_.getLength()<=unit )
    {
	numcomps_.append(0);
	bytes_.append(0);
	ti_.append( 0 );
	sizes_.append( SbVec2s(0,0) );
    }

    numcomps_[unit] = numcomponents;
    bytes_[unit] = bytes;
    sizes_[unit] = size;
    ti_[unit] = ti;
}


const unsigned char* SoSplitTexture2Element::get( SoState* state,
	int unit, SbVec2s& size, int& numcomponents, char& ti )
{
    const SoSplitTexture2Element* elem = (SoSplitTexture2Element*)
	getConstElement(state,classStackIndex);

    if ( unit>=elem->numcomps_.getLength() )
	return 0;

    size = elem->sizes_[unit];
    numcomponents = elem->numcomps_[unit];
    ti = elem->ti_[unit];
    return elem->bytes_[unit];
}
