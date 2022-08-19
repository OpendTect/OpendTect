/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
