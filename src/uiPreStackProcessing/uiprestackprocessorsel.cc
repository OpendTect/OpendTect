/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2009
-*/


#include "uiprestackprocessorsel.h"

#include "ioman.h"
#include "prestackprocessortransl.h"
#include "prestackprocessor.h"
#include "uiprestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "od_helpids.h"

namespace PreStack
{

uiProcSel::uiProcSel( uiParent* p, const uiString& lbl, const MultiID* mid,
		      bool withedit )
    : uiGroup( p )
    , editbut_(0)
    , selectionDone(this)
{
    const IOObjContext ctxt = PreStackProcTranslatorGroup::ioContext();
    selfld_ = new uiIOObjSel( this, ctxt, lbl );
    ConstPtrMan<IOObj> ioobj = mid ? IOM().get(*mid) : 0;
    if ( ioobj ) selfld_->setInput( *ioobj );
    selfld_->selectionDone.notify( mCB(this,uiProcSel,selDoneCB));

    if ( withedit )
    {
	editbut_ = new uiPushButton( this, uiStrings::sEmptyString(),
		mCB(this,uiProcSel,editPushCB), false );
	editbut_->attach( rightOf, selfld_ );
    }

    setHAlignObj( selfld_ );
    selDoneCB( 0 );
}


uiProcSel::~uiProcSel()
{
}


void uiProcSel::setSel( const MultiID& mid )
{
    selfld_->setInput( mid );
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
    if ( editbut_ )
    {
	const IOObj* ioobj = selfld_->ioobj( true );
	editbut_->setText(
		m3Dots(ioobj ? uiStrings::sEdit() : uiStrings::sCreate()) );
    }

    selectionDone.trigger();
}


void uiProcSel::editPushCB( CallBacker* )
{
    uiString title;
    ProcessManager man;
    const IOObj* ioobj =  selfld_->ioobj( true );
    if ( ioobj )
    {
	uiString errmsg;
	PreStackProcTranslator::retrieve( man, ioobj, errmsg );
	title = uiStrings::sEdit();
    }
    else
	title = uiStrings::sCreate();

    title = toUiString("%1 %2 %3").arg(title)
				  .arg(uiStrings::sPreStack().toLower())
				  .arg(uiStrings::sProcessing().toLower());

    uiDialog dlg( this, uiDialog::Setup( title, mNoDlgTitle,
                                        mODHelpKey(mPreStackProcSelHelpID) ) );
    dlg.enableSaveButton(tr("Save on OK"));
    dlg.setSaveButtonChecked( true );
    PreStack::uiProcessorManager* grp = new uiProcessorManager( &dlg, man );
    grp->setLastMid( ioobj ? ioobj->key() : MultiID() );

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
		if ( uiMSG().askSave(tr("Current settings are not saved.\n\n"
					"Do you want to discard them?")) )
		    break;

		continue;
	    }
	}

	selfld_->setInput( grp->lastMid() );
	selDoneCB( 0 );

	break;
    }
}


} // namespace PreStack
