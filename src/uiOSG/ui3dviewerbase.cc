/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ui3dviewerbase.h"


ui3DViewerBase::ui3DViewerBase( uiParent* parnt, const char* nm,
				uiObjectBody& body )
    : uiObject(parnt,nm,body)
    , viewmodechanged(this)
    , pageupdown(this)
{}


ui3DViewerBase::~ui3DViewerBase()
{}
