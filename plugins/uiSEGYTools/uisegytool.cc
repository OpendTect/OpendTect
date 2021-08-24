/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2015
_______________________________________________________________________

-*/


#include "uisegytool.h"
#include "uisegyread.h"
#include "uiwellimpsegyvsp.h"
#include "uidialog.h"
#include "uichecklist.h"
#include "uimsg.h"


uiSEGYTool::uiSEGYTool( uiParent* p, IOPar* previop, int choice )
    : parent_(p)
    , choice_(choice)
    , isnext_(false)
{
    if ( previop )
    {
	pars_ = *previop;
	isnext_ = true;
    }
}


void uiSEGYTool::go()
{
    if ( choice_ < 0 )
    {
	uiDialog dlg( parent_, uiDialog::Setup(tr("SEG-Y Tool"),
	    isnext_ ? tr("Import more data?") : tr("What do you want to do?"),
	    mTODOHelpKey) );
	uiCheckList* choicefld = new uiCheckList( &dlg, uiCheckList::OneOnly );
	choicefld->addItem( tr("Import SEG-Y file(s) to OpendTect data") )
	    .addItem( tr("Scan SEG-Y file(s) to use in-place") )
	    .addItem( tr("Import VSP data from SEG-Y file") )
	    .addItem( isnext_ ? tr("Quit SEG-Y import")
			      : tr("Cancel the operation") )
	    .setChecked( 3, true );
	if ( !dlg.go() )
	    return;
	choice_ = choicefld->firstChecked();
    }

    switch ( choice_ )
    {
    case 0: case 1:
	if ( segyread_ )
	    { segyread_->raiseCurrent(); return; }
	segyread_ = 0;
	if ( !launchSEGYWiz() )
	    return;
    break;
    case 2:
	doVSPTool();
	return;
    break;
    default:
    return;
    }
}


void uiSEGYTool::doVSPTool()
{
    if ( choice_ < 0 )
    {
	uiDialog dlg( parent_, uiDialog::Setup(tr("VSP Import"),
					mNoDlgTitle,mTODOHelpKey) );
	uiCheckList* choicefld = new uiCheckList( &dlg, uiCheckList::OneOnly );
	choicefld->addItem(tr("2-D VSP (will be imported as 2-D seismic line)"))
	    .addItem( tr("3-D VSP (can only be imported as 3D cube)") )
	    .addItem( tr("Zero-offset (single trace) VSP") )
	    .addItem( isnext_ ? tr("Quit import") : tr("Cancel the operation") )
	    .setChecked( 3, true );
	if ( !dlg.go() )
	    return;
	choice_ = choicefld->firstChecked();
    }

    switch ( choice_ )
    {
	case 0: case 1: {
	    const Seis::GeomType gt( choice_ == 0 ? Seis::Line : Seis::Vol );
	    putInPar( gt, pars_ );
	    if ( !launchSEGYWiz() )
		return;
	break; }
	case 2: {
	    uiWellImportSEGYVSP dlg( parent_ );
	    if ( !dlg.go() )
		return;
	break; }
	default:
	    return;
    }
}


bool uiSEGYTool::launchSEGYWiz()
{
    if ( segyread_ )
    {
	uiMSG().error( tr("Please finish your current SEG-Y import first") );
	segyread_->raiseCurrent();
	return false;
    }

    switch ( choice_ )
    {
	case 0:
	    segyread_ = new uiSEGYRead( parent_,
				uiSEGYRead::Setup(uiSEGYRead::Import) );
	break;
	case 1: {
	    uiSEGYRead::Setup su( uiSEGYRead::DirectDef );
	    su.geoms_ -= Seis::Line;
	    segyread_ = new uiSEGYRead( parent_, su );
	break; }
	default:
	    return false;
    }

    segyread_->pars().merge( pars_ );
    segyread_->processEnded.notify( mCB(this,uiSEGYTool,toolEnded) );
    return true;
}


void uiSEGYTool::toolEnded( CallBacker* )
{
    if ( !segyread_ || segyread_->state() == uiVarWizard::cCancelled() )
	{ segyread_ = 0; return; }

    pars_ = segyread_->pars();
    segyread_ = 0;

    isnext_ = true;
    go();
}
