/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uimatlabstep.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitable.h"

#include "file.h"
#include "matlabstep.h"

namespace VolProc
{

uiStepDialog* uiMatlabStep::createInstance( uiParent* p, Step* step )
{
    mDynamicCastGet(MatlabStep*,ms,step);
    return ms ? new uiMatlabStep(p,ms) : 0; 
}

uiMatlabStep::uiMatlabStep( uiParent* p, MatlabStep* step )
    : uiStepDialog(p,MatlabStep::sFactoryDisplayName(), step )
{
    filefld_ = new uiFileInput( this, "Select shared object file" );
    filefld_->valuechanged.notify( mCB(this,uiMatlabStep,fileSelCB) );

    loadbut_ = new uiPushButton( this, "Load",
			mCB(this,uiMatlabStep,loadCB), true );
    loadbut_->setSensitive( false );
    loadbut_->attach( rightTo, filefld_ );

    partable_ = new uiTable( this, uiTable::Setup(5,2), "Parameter table" );
    BufferStringSet lbls; lbls.add( "Parameter" ).add( "Value" );
    partable_->setColumnLabels( lbls );
    partable_->setColumnReadOnly( 0, true );
    partable_->attach( alignedBelow, filefld_ );

    filefld_->setFileName( step->sharedLibFileName() );
    addNameFld( partable_ );
}


void uiMatlabStep::fileSelCB( CallBacker* )
{
    const char* fnm = filefld_->fileName();
    const bool isok = File::exists( fnm );
    loadbut_->setSensitive( isok );
}


void uiMatlabStep::loadCB( CallBacker* )
{
    const char* fnm = filefld_->fileName();
    const bool isok = File::exists( fnm );
    if ( !isok ) return;
}


bool uiMatlabStep::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    return true;
}

} // namespace VolProc
