/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/


#include "pickretriever.h"

RefMan<PickRetriever> PickRetriever::instance_ = 0;

PickRetriever::PickRetriever()	
    : buttonstate_(OD::NoButton)
{}

PickRetriever::~PickRetriever()	{}

void PickRetriever::setInstance( PickRetriever* npr )
{ instance_ = npr; }

PickRetriever* PickRetriever::getInstance()
{ return instance_; }
