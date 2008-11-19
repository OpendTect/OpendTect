/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID = "$Id: uivolprocvolreader.cc,v 1.1 2008-11-19 15:01:57 cvskris Exp $";

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
    VolProc::uiChain::factory().addCreator(create, VolumeReader::sKeyType() );
}    


uiVolumeReader::uiVolumeReader( uiParent* p, VolumeReader* vr )
    : uiStepDialog( p, uiDialog::Setup( VolumeReader::sUserName(),
		    VolumeReader::sUserName(), mTODOHelpID ),
		    vr )
    , volumereader_( vr )
    , seisctxt_( new CtxtIOObj( SeisTrcTranslatorGroup::ioContext() ) )
{
    const char* hortxt = "Horizon";

    if ( vr )
	seisctxt_->setObj( vr->getVolumeID() );

    seisctxt_->ctxt.forread = true;

    seissel_ = new uiSeisSel( this, *seisctxt_, uiSeisSel::Setup(false,false) );
    seissel_->attach( alignedBelow, namefld_ );
}


uiVolumeReader::~uiVolumeReader()
{
    delete seisctxt_->ioobj;
    delete seisctxt_;
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

