/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoCameraInfoElement.cc,v 1.5 2009/07/30 18:17:49 cvsyuancheng Exp $";

#include "SoCameraInfoElement.h"

SO_ELEMENT_SOURCE(SoCameraInfoElement);


void SoCameraInfoElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(SoCameraInfoElement, SoInt32Element );
}


SoCameraInfoElement::~SoCameraInfoElement()
{}


void SoCameraInfoElement::init( SoState* state )
{
    SoInt32Element::init( state );
    data = getDefault();
}


void SoCameraInfoElement::set(SoState* state, SoNode* node,
			      COIN_INT32_T camerainfo )
{
    SoInt32Element::set(classStackIndex, state, node, camerainfo );
}


COIN_INT32_T SoCameraInfoElement::get(SoState* state)
{
    return SoInt32Element::get(classStackIndex, state );
}


COIN_INT32_T SoCameraInfoElement::getDefault()
{
    return 0;
}



