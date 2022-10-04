/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiisopachmaker.h"

#include "datapointset.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "iopar.h"
#include "isopachmaker.h"
#include "multiid.h"
#include "posvecdataset.h"
#include "survinfo.h"

#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

uiIsochronMakerGrp::uiIsochronMakerGrp( uiParent* p, EM::ObjectID horid )
    : uiGroup(p,"Create Isochron")
    , horid_(horid)
{
    IOObjContext ctxt = mIOObjContext( EMHorizon3D );
    ctxt.forread_ = true;
    baseemobj_ = EM::EMM().getObject( horid_ );
    if ( !baseemobj_ )
	basesel_ = new uiIOObjSel( this, ctxt, uiStrings::sHorizon() );

    horsel_ = new uiIOObjSel( this, ctxt, tr("Calculate to") );
    horsel_->selectionDone.notify( mCB(this,uiIsochronMakerGrp,toHorSel) );
    if ( !baseemobj_ )
    {
	horsel_->setInput( MultiID("") );
	horsel_->attach( alignedBelow, basesel_ );
    }

    attrnmfld_ = new uiGenInput( this, uiStrings::sAttribName(),
				 StringInpSpec() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, horsel_ );
    attrnmfld_->setDefaultTextValidator();

    toHorSel(0);

    if ( SI().zIsTime() )
    {
	msecsfld_ = new uiGenInput( this, tr("Output in"),
				BoolInpSpec(true,tr("Milliseconds"),
				tr("Seconds")) );
	msecsfld_->attach( alignedBelow, attrnmfld_ );
    }

    setHAlignObj( basesel_ ? basesel_ : horsel_ );
}


BufferString uiIsochronMakerGrp::getHorNm( EM::ObjectID horid )
{
    MultiID mid( EM::EMM().getMultiID( horid ) );
    return EM::EMM().objectName( mid );
}


uiIsochronMakerGrp::~uiIsochronMakerGrp()
{
}


void uiIsochronMakerGrp::toHorSel( CallBacker* )
{
    if ( horsel_->ioobj(true) )
	attrnmfld_->setText( BufferString("I: ",horsel_->ioobj()->name()) );
}


bool uiIsochronMakerGrp::chkInputFlds()
{
    if ( basesel_ && !basesel_->ioobj() )
	return false;

    if ( !horsel_->ioobj() )
	return false;

    BufferString attrnm =  attrnmfld_->text();
    if ( attrnm.isEmpty() )
    {
	uiMSG().message( tr("Please enter attribute name") );
	attrnm.add( "I: " ).add( horsel_->ioobj()->name() );
	attrnmfld_->setText( attrnm );
	return false;
    }

    return true;
}


bool uiIsochronMakerGrp::fillPar( IOPar& par )
{
    par.set( IsochronMaker::sKeyHorizonID(), basesel_ ? basesel_->key()
						      : baseemobj_->multiID() );
    par.set( IsochronMaker::sKeyCalculateToHorID(), horsel_->key() );
    par.set( IsochronMaker::sKeyAttribName(), attrnmfld_->text() );
    if ( msecsfld_ )
	par.setYN( IsochronMaker::sKeyOutputInMilliSecYN(),
		   msecsfld_->getBoolValue() );

    return true;
}


const char* uiIsochronMakerGrp::attrName() const
{ return attrnmfld_->text(); }


#define mErrRet(s) { uiMSG().error(s); return false; }

//uiIsochronMakerBatch
uiIsochronMakerBatch::uiIsochronMakerBatch( uiParent* p )
    : uiDialog(p,Setup(tr("Create Isochron"),mNoDlgTitle,
		 mODHelpKey(mIsochronMakerBatchHelpID)) )
{
    grp_ = new uiIsochronMakerGrp( this, EM::ObjectID::udf() );
    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, grp_ );
    batchfld_->jobSpec().prognm_ = "od_isopach";
}


uiIsochronMakerBatch::~uiIsochronMakerBatch()
{}


bool uiIsochronMakerBatch::prepareProcessing()
{
    return grp_->chkInputFlds();
}


bool uiIsochronMakerBatch::fillPar()
{
    IOPar& par = batchfld_->jobSpec().pars_;
    if ( !grp_->fillPar( par ) )
	return false;

    MultiID mid;
    par.get( IsochronMaker::sKeyHorizonID(), mid );
    EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    BufferString attrnm;
    par.get( IsochronMaker::sKeyAttribName(), attrnm );
    isoverwrite_ = false;
    if ( attrnms.isPresent( attrnm ) )
    {
	uiString errmsg = tr("Attribute name %1 already exists, Overwrite?")
			.arg(attrnm);
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;

	isoverwrite_ = true;
    }

    par.setYN( IsochronMaker::sKeyIsOverWriteYN(), isoverwrite_ );
    return true;
}


bool uiIsochronMakerBatch::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( grp_->attrName() );
    return batchfld_->start();
}


//uiIsochronMakerDlg
uiIsochronMakerDlg::uiIsochronMakerDlg( uiParent* p, EM::ObjectID emid )
    : uiDialog(p,Setup(tr("Create Isochron"),mNoDlgTitle,
			mODHelpKey(mIsopachMakerHelpID)))
    , dps_( new DataPointSet(false,true) )
{
    grp_ = new uiIsochronMakerGrp( this, emid );
    uiString title = tr("Create Isochron for '%1'" )
			.arg( grp_->getHorNm( emid ) );
    setTitleText( title );
}


uiIsochronMakerDlg::~uiIsochronMakerDlg()
{
    delete dps_;
}


bool uiIsochronMakerDlg::acceptOK( CallBacker* )
{
    if ( !grp_->chkInputFlds() )
	return false;

    return doWork();
}


bool uiIsochronMakerDlg::doWork()
{
    IOPar par;
    grp_->fillPar(par);
    MultiID mid1, mid2;
    par.get( IsochronMaker::sKeyHorizonID(), mid1 );
    par.get( IsochronMaker::sKeyCalculateToHorID(), mid2 );
    uiTaskRunner taskrunner( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid2, &taskrunner );
    mDynamicCastGet(EM::Horizon3D*,h2,emobj)
    if ( !h2 )
	mErrRet(tr("Cannot load selected horizon"))
    h2->ref();

    EM::ObjectID emidbase = EM::EMM().getObjectID( mid1 );
    EM::EMObject* emobjbase = EM::EMM().getObject( emidbase );
    mDynamicCastGet(EM::Horizon3D*,h1,emobjbase)
    if ( !h1 )
    { h2->unRef(); mErrRet(tr("Cannot find base horizon")) }

    h1->ref();

    int dataidx = -1;
    BufferString attrnm;
    if ( !par.get( IsochronMaker::sKeyAttribName(), attrnm ) )
	return false;

    dataidx = h1->auxdata.addAuxData( attrnm );
    IsochronMaker ipmaker( *h1, *h2, attrnm, dataidx, dps_);
    if ( SI().zIsTime() )
    {
	bool isinmsec = false;
	par.getYN( IsochronMaker::sKeyOutputInMilliSecYN(), isinmsec );
	ipmaker.setUnits( isinmsec );
    }

    bool rv = TaskRunner::execute( &taskrunner, ipmaker );
    h1->unRef(); h2->unRef();
    return rv;
}
