/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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

uiVolumeReader::uiVolumeReader( uiParent* p, VolumeReader* vr, bool is2d )
    : uiStepDialog( p, VolumeReader::sFactoryDisplayName(), vr, is2d )
    , volumereader_( vr )
{
    setHelpKey( mODHelpKey(mVolumeReaderHelpID) );

    Seis::GeomType seistype = is2d ? Seis::Line : Seis::Vol;
    seissel_ = new uiSeisSel( this, uiSeisSel::ioContext(seistype,true),
			      uiSeisSel::Setup(seistype) );
    if ( vr )
	seissel_->setInput( vr->getVolumeID() );
    seissel_->selectionDone.notify( mCB(this,uiVolumeReader,volSel) );

    addNameFld( seissel_ );
    postFinalize().notify( mCB(this,uiVolumeReader,volSel) );
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


uiStepDialog* uiVolumeReader::createInstance( uiParent* parent, Step* ps,
					      bool is2d )
{
    mDynamicCastGet( VolumeReader*, vr, ps );
    if ( !vr ) return 0;

    return new uiVolumeReader( parent, vr, is2d );
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

} // namespace VolProc
