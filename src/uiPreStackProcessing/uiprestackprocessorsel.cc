/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/

static const char* rcsID = "$Id: uiprestackprocessorsel.cc,v 1.7 2009-08-27 09:58:39 cvsbert Exp $";

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
{
    const IOObjContext ctxt = PreStackProcTranslatorGroup::ioContext();
    selfld_ = new uiIOObjSel( this, ctxt, lbl );
    PtrMan<const IOObj> ioobj = mid ? IOM().get(*mid) : 0;
    if ( ioobj ) selfld_->setInput( *ioobj );
    selfld_->selectiondone.notify( mCB(this,uiProcSel,selDoneCB));
    editbut_ = new uiPushButton( this, ioobj ? "Edit" : "Create",
	    			mCB(this,uiProcSel,editPushCB), false );
    editbut_->attach( rightOf, selfld_ );

    setHAlignObj( selfld_ );
}


uiProcSel::~uiProcSel()
{
}


void uiProcSel::setSel( const MultiID& mid )
{
    selfld_->ctxtIOObj().setObj( IOM().get( mid ) );
    selfld_->updateInput();
}


bool uiProcSel::getSel( MultiID& mid ) const
{
    const IOObj* ioobj = selfld_->ioobj();
    if ( !ioobj )
	return false;

    mid = ioobj->key();
    return true;
}


void uiProcSel::selDoneCB( CallBacker* cb )
{
    const IOObj* ioobj = selfld_->ioobj( true );
    editbut_->setText( ioobj ? "Edit ..." : "Add ..." );
}


void uiProcSel::editPushCB( CallBacker* )
{
    BufferString title;
    ProcessManager man;
    MultiID mid;
    if ( getSel(mid) )
    {
	BufferString errmsg;
	PtrMan<IOObj> ioobj = IOM().get( mid );
	PreStackProcTranslator::retrieve( man, ioobj, errmsg );
	title = "Edit";
    }
    else
	title = "Create";

    title += " prestack processing";

    uiDialog dlg( this, uiDialog::Setup( title.buf(), 0, "103.2.13") );
    dlg.enableSaveButton("Save on OK");
    dlg.setSaveButtonChecked( true );
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
