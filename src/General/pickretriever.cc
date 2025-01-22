/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "pickretriever.h"


PickRetriever::PickRetriever()
    : buttonstate_(OD::NoButton)
{}


PickRetriever::~PickRetriever()
{}


const WeakPtr<PickRetriever>& PickRetriever::instance( PickRetriever* npr )
{
    static WeakPtr<PickRetriever> theinst;
    if ( npr )
	theinst = npr;

    return theinst;
}
