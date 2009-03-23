/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uivolprocvolreader.cc,v 1.2 2009-03-23 11:02:00 cvsbert Exp $";

#include "uivolprocvolreader.h"
#include "uimsg.h"
#include "volprocvolreader.h"

#include "ctxtioobj.h"
#include "seistrctr.h"
#include "mousecursor.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uivolprocchain.h"


namespace VolProc
{


void uiVolumeReader::initClass()
{
    uiChain::factory().addCreator(create, VolumeReader::sKeyType() );
}    


uiVolumeReader::uiVolumeReader( uiParent* p, VolumeReader* vr )
    : uiStepDialog( p, VolumeReader::sUserName(), vr )
    , volumereader_( vr )
    , ctio_(uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
{
    if ( vr )
	ctio_->setObj( vr->getVolumeID() );
    else
	ctio_->setObj( 0 );

    seissel_ = new uiSeisSel( this, *ctio_, uiSeisSel::Setup(false,false) );
    seissel_->selectiondone.notify( mCB(this,uiVolumeReader,volSel) );

    addNameFld( seissel_ );
    finaliseDone.notify( mCB(this,uiVolumeReader,volSel) );
}


uiVolumeReader::~uiVolumeReader()
{
    delete ctio_->ioobj; delete ctio_;
}


void uiVolumeReader::volSel( CallBacker* )
{
    if ( !*namefld_->text() )
    {
	seissel_->processInput();
	const IOObj* ioobj = seissel_->ctxtIOObj().ioobj;
	if ( ioobj )
	    namefld_->setText( ioobj->name() );
    }
}


uiStepDialog* uiVolumeReader::create( uiParent* parent, Step* ps )
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

    if ( !seissel_->existingTyped() )
    {
	uiMSG().error("Non-existing volume selected");
	return false;
    }

    if ( !volumereader_->setVolumeID( seissel_->ctxtIOObj().ioobj->key() ) )
    {
	uiMSG().error("Cannot use selected volume" );
	return false;
    }

    return true;
}


};//namespace

