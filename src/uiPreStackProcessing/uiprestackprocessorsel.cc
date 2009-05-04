/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/

static const char* rcsID = "$Id: uiprestackprocessorsel.cc,v 1.3 2009-05-04 11:15:25 cvsranojay Exp $";

#include "uiprestackprocessorsel.h"

#include "ioman.h"
#include "prestackprocessortransl.h"
#include "prestackprocessor.h"
#include "uiprestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiioobjsel.h"

namespace PreStack
{

uiProcSel::uiProcSel( uiParent* p, const char* lbl,
				      const MultiID* mid ) 
    : uiGroup( p )
    , ctio_( *new CtxtIOObj(PreStackProcTranslatorGroup::ioContext() ) )
{
    if ( mid ) ctio_.ioobj = IOM().get( *mid );
    selfld_ = new uiIOObjSel( this, ctio_, lbl );
    selfld_->selectiondone.notify( mCB(this,uiProcSel,selDoneCB));
    editbut_ = new uiPushButton( this, ctio_.ioobj ? "Edit" : "Add",
	    			mCB(this,uiProcSel,editPushCB), false );
    editbut_->attach( rightOf, selfld_ );

    setHAlignObj( selfld_ );
}


uiProcSel::~uiProcSel()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiProcSel::setSel( const MultiID& mid )
{
    selfld_->ctxtIOObj().setObj( IOM().get( mid ) );
    selfld_->updateInput();
}


bool uiProcSel::getSel( MultiID& mid ) const
{
    if ( !selfld_->commitInput() )
	return false;

    mid = ctio_.ioobj->key();
    return true;
}


void uiProcSel::selDoneCB( CallBacker* cb )
{
    if ( selfld_->commitInput() )
	editbut_->setText( ctio_.ioobj ? "Edit ..." : "Add ..." );
}


void uiProcSel::editPushCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("Edit prestack processing",
					0, "od:Todo") );
    dlg.enableSaveButton("Save on OK");
    dlg.setSaveButtonChecked( true );
    ProcessManager man;
    MultiID mid;
    if ( getSel(mid) )
    {
	BufferString errmsg;
	PtrMan<IOObj> ioobj = IOM().get( mid );
	PreStackProcTranslator::retrieve( man, ioobj, errmsg );
    }

    PreStack::uiProcessorManager* grp = new uiProcessorManager( &dlg, man );
    grp->setLastMid( mid );

    while ( dlg.go() )
    {
	if ( grp->isChanged() )
	{
	    if ( dlg.saveButtonChecked() )
	    {
		if ( !grp->save() )
		    continue;
	    }
	    else
	    {
		if ( uiMSG().askSave("Current settings are not saved.\n"
				"Do you want to discard them?") )
		    break;

		continue;
	    }
	}

	selfld_->ctxtIOObj().setObj( IOM().get( grp->lastMid() ) );
	selfld_->updateInput();
	break;
    }
}


}; //namespace
