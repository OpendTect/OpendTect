/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackprocessorsel.h"

#include "ioman.h"
#include "od_helpids.h"
#include "prestackprocessortransl.h"
#include "prestackprocessor.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"


namespace PreStack
{

// uiPSProcObjSel

uiPSProcObjSel::uiPSProcObjSel( uiParent* p, const uiString& lbl,
				OD::GeomSystem gs )
    : uiIOObjSel(p,ioContext(gs),lbl)
    , gs_(gs)
{
}


uiPSProcObjSel::uiPSProcObjSel( uiParent* p, const uiIOObjSel::Setup& su,
				OD::GeomSystem gs )
    : uiIOObjSel(p,ioContext(gs),su)
    , gs_(gs)
{
}


uiPSProcObjSel::~uiPSProcObjSel()
{
}


StringView uiPSProcObjSel::reqType( OD::GeomSystem gs )
{
    return ::is3D( gs ) ? sKey::ThreeD()
			: (::is2D( gs ) ? sKey::TwoD() : sKey::Synthetic());
}


const IOObjContext& uiPSProcObjSel::ioContext( OD::GeomSystem gs )
{
    static PtrMan<IOObjContext> ctxt3d;
    static PtrMan<IOObjContext> ctxt2d;
    static PtrMan<IOObjContext> ctxtsynth;
    const bool is3d = ::is3D( gs );
    const bool is2d = ::is2D( gs );
    PtrMan<IOObjContext>& ctxt = is3d ? ctxt3d
				      : (is2d ? ctxt2d : ctxtsynth);
    if ( !ctxt )
    {
	ctxt = new IOObjContext( PreStackProcTranslatorGroup::ioContext() );
	ctxt->requireType( reqType(gs), is3d );
    }

    return *ctxt.ptr();
}



// uiProcSel

uiProcSel::uiProcSel( uiParent* p, const uiString& lbl, OD::GeomSystem gs,
		      int openidx, const uiStringSet* usemethods )
    : uiGroup(p)
    , gs_(gs)
    , openidx_(openidx)
    , selectionDone(this)
{
    if ( usemethods )
	usemethods_ = *usemethods;

    selfld_ = new uiPSProcObjSel( this, lbl, gs );
    mAttachCB( selfld_->selectionDone, uiProcSel::selDoneCB );

    editbut_ = new uiPushButton( this, uiString::empty(),
				 mCB(this,uiProcSel,editPushCB), false );
    editbut_->attach( rightOf, selfld_ );

    setHAlignObj( selfld_ );
    mAttachCB( postFinalize(), uiProcSel::initGrpCB );
}


uiProcSel::~uiProcSel()
{
    detachAllNotifiers();
}


void uiProcSel::initGrpCB( CallBacker* )
{
    selDoneCB( nullptr );
    if ( usemethods_.validIdx(openidx_) )
	editPushCB( nullptr );
}


void uiProcSel::setSel( const MultiID& mid )
{
    selfld_->setInput( mid );
}


bool uiProcSel::getSel( MultiID& mid, bool noerr ) const
{
    const IOObj* ioobj = selfld_->ioobj( noerr );
    if ( !ioobj )
	return false;

    mid = ioobj->key();
    return true;
}


bool uiProcSel::checkInput( const IOObj& ioobj ) const
{
    ProcessManager man( gs_ );
    uiString errmsg;
    if ( !PreStackProcTranslator::retrieve(man,&ioobj,errmsg) &&
	 man.nrProcessors() < 1 )
	return false;

    uiStringSet methods;
    if ( usemethods_.isEmpty() )
	uiProcessorManager::getMethods( gs_, methods );
    else
	methods = usemethods_;

    for ( int iproc=0; iproc<man.nrProcessors(); iproc++ )
    {
	const Processor* proc = man.getProcessor( iproc );
	if ( !proc )
	    continue;

	const uiString procuinm = proc->factoryDisplayName();
	if ( !methods.isPresent(procuinm) )
	    return false;
    }

    return true;
}


void uiProcSel::selDoneCB( CallBacker* cb )
{
    const IOObj* ioobj = selfld_->ioobj( true );
    if ( ioobj && !checkInput(*ioobj) )
    {
	ioobj = nullptr;
	if ( cb )
	    uiMSG().error( tr("The selected setup cannot be used "
			      "in this context") );
	selfld_->setEmpty();
    }

    editbut_->setText(
	    m3Dots(ioobj ? uiStrings::sEdit() : uiStrings::sCreate()) );

    if ( ioobj )
	selectionDone.trigger();
}


void uiProcSel::editPushCB( CallBacker* )
{
    uiString title;
    ProcessManager man( gs_ );
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

    auto* grp = new uiProcessorManager( &dlg, man, openidx_, &usemethods_ );
    grp->setLastMid( ioobj ? ioobj->key() : MultiID::udf() );

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
	selDoneCB( nullptr );

	break;
    }
}

} // namespace PreStack
