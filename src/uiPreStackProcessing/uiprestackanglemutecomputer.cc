/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : July 2011
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackanglemutecomputer.h"
#include "uiprestackanglemute.h"

#include "horsampling.h"
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


namespace PreStack
{

uiAngleMuteComputer::uiAngleMuteComputer( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Angle Mute Computer",
				    mNoDlgTitle,"103.2.19") )
    , outctio_( *mMkCtxtIOObj(MuteDef) )
    , processor_(new AngleMuteComputer) 
{
    anglemutegrp_ = new uiAngleMuteGrp( this, processor_->params(), true );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, anglemutegrp_ );

    subsel_ = uiSeisSubSel::get( this, Seis::SelSetup( false ) );
    HorSampling hs; subsel_->getSampling( hs );
    hs.step = BinID( SI().inlStep()*20, SI().crlStep()*20 );
    subsel_->setInput( hs );
    subsel_->attach( alignedBelow, anglemutegrp_ );
    subsel_->attach( ensureBelow, sep );

    outctio_.ctxt.forread = false;
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
    if ( !anglemutegrp_->acceptOK() )
	return false;

    if ( !mutedeffld_->commitInput() || !outctio_.ioobj )
    {
	uiMSG().error("Please select a valid output mute function");
	return false;
    }
    HorSampling hrg;
    if ( !subsel_->isAll() )
	subsel_->getSampling( hrg );

    processor_->params().hrg_ = hrg;
    processor_->params().outputmutemid_ = mutedeffld_->key(true); 

    uiTaskRunner tr(this);
    if ( !TaskRunner::execute( &tr, *processor_ ) )
    {
	uiMSG().error( processor_->errMsg() );
	return false;
    }
    return true;
}

}; //namespace
