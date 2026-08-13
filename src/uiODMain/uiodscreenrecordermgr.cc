/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodscreenrecordermgr.h"

#include "uiodmain.h"
#include "uiodscreenrecorderdlg.h"


uiODScreenRecorderMgr::uiODScreenRecorderMgr( uiODMain& appl )
    : appl_(appl)
{
}


uiODScreenRecorderMgr::~uiODScreenRecorderMgr()
{
    delete dialog_;
}


void uiODScreenRecorderMgr::toggleRecording()
{
    if ( !dialog_ )
	dialog_ = new uiODScreenRecorderDlg( appl_ );

    dialog_->show();
}
