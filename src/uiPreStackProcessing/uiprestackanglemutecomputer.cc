/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackanglemutecomputer.h"
#include "uiprestackanglemute.h"

#include "trckeysampling.h"
#include "prestackanglemutecomputer.h"
#include "prestackmute.h"
#include "prestackmutedeftransl.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiraytrace1d.h"
#include "uiseissubsel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uiveldesc.h"
#include "od_helpids.h"
#include "survinfo.h"


namespace PreStack
{

uiAngleMuteComputer::uiAngleMuteComputer( uiParent* p )
    : uiDialog( p, uiDialog::Setup(tr("Angle Mute Computer"),
				mNoDlgTitle,
				mODHelpKey(mAngleMuteComputerHelpID) ) )
    , processor_(new AngleMuteComputer)
{
    anglecompgrp_ = new uiAngleCompGrp( this, processor_->params(), true );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, anglecompgrp_ );

    subsel_ = uiSeisSubSel::get( this, Seis::SelSetup( false ) );
    TrcKeySampling hs; subsel_->getSampling( hs );
    hs.step_ = BinID( SI().inlStep()*20, SI().crlStep()*20 );
    subsel_->setInput( hs );
    subsel_->attach( alignedBelow, anglecompgrp_ );
    subsel_->attach( ensureBelow, sep );

    IOObjContext ctxt = mIOObjContext( MuteDef );
    ctxt.forread_ = false;
    mutedeffld_ = new uiIOObjSel( this, ctxt );
    mutedeffld_->attach( alignedBelow, subsel_ );
}


uiAngleMuteComputer::~uiAngleMuteComputer()
{
    delete processor_;
}


bool uiAngleMuteComputer::acceptOK(CallBacker*)
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    const IOObj* ioobj = mutedeffld_->ioobj();
    if ( !ioobj )
	return false;

    TrcKeySampling hrg;
    if ( !subsel_->isAll() )
	subsel_->getSampling( hrg );

    processor_->params().tks_ = hrg;
    processor_->params().outputmutemid_ = ioobj->key();

    uiTaskRunner taskrunner(this);
    if ( !TaskRunner::execute(&taskrunner,*processor_) )
    {
	uiMSG().error( processor_->errMsg() );
	return false;
    }
    return true;
}

} // namespace PreStack
