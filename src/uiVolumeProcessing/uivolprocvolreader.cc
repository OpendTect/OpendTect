/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocvolreader.h"
#include "uimsg.h"
#include "volprocvolreader.h"

#include "ctxtioobj.h"
#include "seistrctr.h"
#include "mousecursor.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uivolprocchain.h"
#include "od_helpids.h"


namespace VolProc
{


uiVolumeReader::uiVolumeReader( uiParent* p, VolumeReader* vr )
    : uiStepDialog( p, VolumeReader::sFactoryDisplayName(), vr )
    , volumereader_( vr )
{
    setHelpKey( mODHelpKey(mVolumeReaderHelpID) );

    seissel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true),
				uiSeisSel::Setup(Seis::Vol) );
    if ( vr )
	seissel_->setInput( vr->getVolumeID() );
    seissel_->selectionDone.notify( mCB(this,uiVolumeReader,volSel) );

    addNameFld( seissel_ );
    postFinalise().notify( mCB(this,uiVolumeReader,volSel) );
}


uiVolumeReader::~uiVolumeReader()
{
}


void uiVolumeReader::volSel( CallBacker* )
{
    const IOObj* ioobj = seissel_->ioobj( true );
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


uiStepDialog* uiVolumeReader::createInstance( uiParent* parent, Step* ps )
{
    mDynamicCastGet( VolumeReader*, vr, ps );
    if ( !vr ) return 0;

    return new uiVolumeReader( parent, vr );
}


bool uiVolumeReader::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    const IOObj* ioobj = seissel_->ioobj();
    if ( !ioobj )
	return false;

    if ( !volumereader_->setVolumeID( ioobj->key() ) )
	{ uiMSG().error(tr("Cannot use selected volume") ); return false; }

    return true;
}


};//namespace
