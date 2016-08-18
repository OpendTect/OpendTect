/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________

-*/

#include "odscenepresentationmgr.h"

ScenePresentationMgr::ScenePresentationMgr()
    : ODVwrTypePresentationMgr()
{
}


void ScenePresentationMgr::initClass()
{
    ScenePresentationMgr* scenemgr = new ScenePresentationMgr;
    ODPrMan().addViewerTypeManager( scenemgr );
}
