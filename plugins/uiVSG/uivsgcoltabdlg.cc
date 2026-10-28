/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgcoltabdlg.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "mousecursor.h"
#include "notify.h"
#include "uicolortable.h"


uiVSGColTabDlg::uiVSGColTabDlg( uiParent* p, dGBVisSurvey::Scene& scene )
    : uiDialog(p,Setup(tr("VSG Color Table"),mNoHelpKey).modal(false))
    , scene_(&scene)
{
    setCtrlStyle( CloseOnly );

    coltab_ = new uiColorTableGroup( this, ColTab::Sequence(""),
				     OD::Vertical, false );
    bind();
    mAttachCB( coltab_->seqChanged, uiVSGColTabDlg::seqChgCB );
    mAttachCB( coltab_->scaleChanged, uiVSGColTabDlg::scaleChgCB );
    mAttachCB( scene_->testchannelschanged, uiVSGColTabDlg::channelsCB );
}


void uiVSGColTabDlg::channelsCB( CallBacker* )
{
    bind();
}


void uiVSGColTabDlg::bind()
{
    channels_ = scene_->testTextureChannels();
    if ( !channels_ )
	return;

    NotifyStopper seqstopper( coltab_->seqChanged );
    NotifyStopper scalestopper( coltab_->scaleChanged );
    coltab_->setSequence( &channels_->getColTabSequence(), true, false );
    coltab_->setMapperSetup( &channels_->getColTabMapperSetup(0,0), false );
    coltab_->setHistogram( channels_->getHistogram(0) );
}


uiVSGColTabDlg::~uiVSGColTabDlg()
{
    detachAllNotifiers();
}


void uiVSGColTabDlg::seqChgCB( CallBacker* )
{
    if ( channels_ )
	channels_->setColTabSequence( coltab_->colTabSeq() );
}


void uiVSGColTabDlg::scaleChgCB( CallBacker* )
{
    if ( !channels_ )
	return;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    channels_->setColTabMapperSetup( 0, coltab_->colTabMapperSetup() );
    channels_->reMapData( 0, false, nullptr );
    NotifyStopper scalestopper( coltab_->scaleChanged );
    coltab_->setMapperSetup( &channels_->getColTabMapperSetup(0,0), false );
    coltab_->setHistogram( channels_->getHistogram(0) );
}
