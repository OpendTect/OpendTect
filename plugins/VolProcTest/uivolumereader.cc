/*+
 * COPYRIGHT	(C) dGB Beheer B.V.
 * AUTHOR	Y.C. Liu
 * DATE		March 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uivolumereader.h"

#include "uivolumeprocessing.h"
#include "volumeprocessing.h"
#include "volumereader.h"
#include "uiioobjsel.h"
#include "seistrctr.h"


namespace VolProc
{

void uiReader::initClass()
{
    VolProc::uiPS().addCreator( create, VolumeReader::sKeyType() );
}    


uiReader::uiReader( uiParent* p, VolumeReader* vr )
	: uiDialog( p, uiDialog::Setup("Volume reader setup",0,mNoHelpID) )
	, volreader_( vr )
	, iocontext_( mMkCtxtIOObj(SeisTrc) )
{
    iocontext_->setObj( volreader_->getStorage() );
    iocontext_->ctxt.forread = true;
    uinputselfld_ = new uiIOObjSel( this, *iocontext_, "Input volume" );
}


uiReader::~uiReader()
{
    delete iocontext_->ioobj;
    delete iocontext_;
}


uiDialog* uiReader::create( uiParent* parent, ProcessingStep* ps )
{
    mDynamicCastGet( VolumeReader*, vr, ps );
    if ( !vr ) return 0;

    return new uiReader( parent, vr );
}


bool uiReader::acceptOK( CallBacker* )
{
    uinputselfld_->processInput();
    if ( !uinputselfld_->existingTyped() )
    return false;

    volreader_->setStorage( uinputselfld_->ctxtIOObj().ioobj->key() );
    return true;
}

} //namespace
