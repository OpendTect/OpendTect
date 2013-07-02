/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki / Bert
 Date:          July 2013
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistratsynthexport.h"

#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uigeninput.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "stratsynth.h"
#include "zdomain.h"


class uiSSOutSel : public uiCheckedCompoundParSel
{
public:

uiSSOutSel( uiParent* p, const char* seltxt )
    : uiCheckedCompoundParSel( p, seltxt, false, "&Define" )
{
}



};

uiStratSynthExport::uiStratSynthExport( uiParent* p, const StratSynth& ss )
    : uiDialog(p,uiDialog::Setup("Export synthetic seismics and horizons",
				 getWinTitle(ss), mTODOHelpID) )
    , ss_(ss)
{
    crnewfld_ = new uiGenInput( this, "Mode",
			     BoolInpSpec(true,"Create New","Use existing") );
    crnewfld_->valuechanged.notify( mCB(this,uiStratSynthExport,crNewChg) );

    uiSeisSel::Setup sssu( Seis::Line );
    sssu.enabotherdomain( false ).zdomkey( ZDomain::sKeyTime() )
	.selattr( false ).allowsetdefault( false );
    linesetsel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
	    			sssu );
    linesetsel_->attach( alignedBelow, crnewfld_ );
    linenmsel_ = new uiSeis2DLineNameSel( this, false );
    linenmsel_->attach( alignedBelow, linesetsel_ );

    // poststcksel_ = new uiSSOutSel( );

    postFinalise().notify( mCB(this,uiStratSynthExport,crNewChg) );
}


uiStratSynthExport::~uiStratSynthExport()
{
}


BufferString uiStratSynthExport::getWinTitle( const StratSynth& ss ) const
{
    return BufferString( "TODO: create nice window title" );
}


void uiStratSynthExport::crNewChg( CallBacker* )
{
    const bool iscreate = crnewfld_->getBoolValue();
    if ( iscreate )
	uiMSG().error( "TODO: implement crNewChg" );
}


bool uiStratSynthExport::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: complete and implement" );
    return false;
}
