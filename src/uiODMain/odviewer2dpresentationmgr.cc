/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		August 2016
________________________________________________________________________

-*/

#include "odviewer2dpresentationmgr.h"

Viewer2DPresentationMgr::Viewer2DPresentationMgr()
    : ODVwrTypePresentationMgr()
{
}


void Viewer2DPresentationMgr::initClass()
{
    Viewer2DPresentationMgr* vwr2dmgr = new Viewer2DPresentationMgr;
    ODPrMan().addViewerTypeManager( vwr2dmgr );
}
