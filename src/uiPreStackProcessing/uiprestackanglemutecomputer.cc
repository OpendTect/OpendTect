/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : July 2011
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
    , outctio_( *mMkCtxtIOObj(MuteDef) )
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

    outctio_.ctxt_.forread_ = false;
    mutedeffld_ = new uiIOObjSel( this, outctio_ );
    mutedeffld_->attach( alignedBelow, subsel_ );
}


uiAngleMuteComputer::~uiAngleMuteComputer()
{
    delete processor_;
    delete &outctio_;
}


bool uiAngleMuteComputer::acceptOK(CallBacker*)
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    if ( !mutedeffld_->commitInput() || !outctio_.ioobj_ )
    {
	uiMSG().error(tr("Please select a valid output mute function"));
	return false;
    }
    TrcKeySampling hrg;
    if ( !subsel_->isAll() )
	subsel_->getSampling( hrg );

    processor_->params().tks_ = hrg;
    processor_->params().outputmutemid_ = mutedeffld_->key(true);

    uiTaskRunner taskrunner(this);
    if ( !TaskRunner::execute( &taskrunner, *processor_ ) )
    {
	uiMSG().error( processor_->errMsg() );
	return false;
    }
    return true;
}

}; //namespace
