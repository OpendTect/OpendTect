/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoCameraInfoElement.cc,v 1.1 2003-09-02 12:36:03 kristofer Exp $";

#include "SoCameraInfoElement.h"

SO_ELEMENT_SOURCE(SoCameraInfoElement);


void SoCameraInfoElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(SoCameraInfoElement, SoInt32Element );
}


SoCameraInfoElement::~SoCameraInfoElement()
{}


void SoCameraInfoElement::init(SoState*)
{
    data = getDefault();
}


void SoCameraInfoElement::set(SoState* state, SoNode* node, int32_t camerainfo )
{
    SoInt32Element::set(classStackIndex, state, node, camerainfo );
}


int32_t SoCameraInfoElement::get(SoState* state)
{
    return SoInt32Element::get(classStackIndex, state );
}


int32_t SoCameraInfoElement::getDefault()
{
    return 0;
}



