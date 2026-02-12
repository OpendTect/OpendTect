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
#include "genc.h"
#include "iopar.h"
#include "isopachmaker.h"
#include "multiid.h"
#include "posvecdataset.h"
#include "survinfo.h"

#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiIsochronMakerGrp::uiIsochronMakerGrp( uiParent* p, EM::ObjectID horid )
    : uiGroup(p,"Create Isochron")
    , horid_(horid)
{
    baseemobj_ = EM::EMM().getObject( horid_ );
    if ( !baseemobj_ )
	basehorsel_ = new uiHorizon3DSel( this, true, uiStrings::sHorizon() );

    horsel_ = new uiHorizon3DSel( this, true, tr("Calculate to") );
    horsel_->setInput( baseemobj_ ? baseemobj_->multiID() : MultiID::udf() );
    mAttachCB( horsel_->selectionDone, uiIsochronMakerGrp::toHorSelCB );
    if ( !baseemobj_ )
    {
	horsel_->setInput( MultiID::udf() );
	horsel_->attach( alignedBelow, basehorsel_ );
    }

    attrnmfld_ = new uiGenInput( this, uiStrings::sAttribName(),
				 StringInpSpec() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, horsel_ );
    attrnmfld_->setDefaultTextValidator();

    if ( SI().zIsTime() )
    {
	msecsfld_ = new uiGenInput( this, tr("Output unit"),
				    BoolInpSpec(true,uiStrings::sMsec(mPlural),
						uiStrings::sSec(mPlural)) );
	msecsfld_->attach( alignedBelow, attrnmfld_ );
    }

    setHAlignObj( basehorsel_ ? basehorsel_ : horsel_ );

    mAttachCB( postFinalize(), uiIsochronMakerGrp::toHorSelCB );
}


BufferString uiIsochronMakerGrp::getHorizonName( EM::ObjectID horid )
{
    MultiID mid( EM::EMM().getMultiID( horid ) );
    return EM::EMM().objectName( mid );
}


uiIsochronMakerGrp::~uiIsochronMakerGrp()
{}


void uiIsochronMakerGrp::toHorSelCB( CallBacker* )
{
    if ( horsel_->ioobj(true) )
	attrnmfld_->setText( BufferString("Isochron: ",
					  horsel_->ioobj()->name()) );
}


bool uiIsochronMakerGrp::checkInputFlds()
{
    if ( basehorsel_ && !basehorsel_->ioobj() )
	return false;

    if ( !horsel_->ioobj() )
	return false;

    BufferString attrnm =  attrnmfld_->text();
    if ( attrnm.isEmpty() )
    {
	uiMSG().message( tr("Please enter attribute name") );
	attrnm.add( "Isochron: " ).add( horsel_->ioobj()->name() );
	attrnmfld_->setText( attrnm );
	return false;
    }

    return true;
}


bool uiIsochronMakerGrp::fillPar( IOPar& par )
{
    const MultiID baseid = baseemobj_ ? baseemobj_->multiID() :
			   basehorsel_ ? basehorsel_->key() : MultiID::udf();

    if ( baseid.isUdf() )
    {
	uiMSG().error( tr("No valid base horizon found") );
	return false;
    }

    const EM::IOObjInfo eminfo( baseid );

    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );

    const BufferString attrnm = attrnmfld_->text();
    bool isoverwrite = false;
    if ( attrnms.isPresent(attrnm.buf()) )
    {
	const uiString errmsg = tr("Entered Isochron name already exists as "
				   "an attribute in %1. Overwrite?")
				    .arg( eminfo.name() );
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;

	isoverwrite = true;
    }

    const MultiID horselid = horsel_->key();
    if ( baseid == horselid )
    {
	uiMSG().error( tr("Selected horizon is the same as base horizon.") );
	return false;
    }

    par.setYN( IsochronMaker::sKeyIsOverWriteYN(), isoverwrite );
    par.set( IsochronMaker::sKeyHorizonID(), baseid );
    par.set( IsochronMaker::sKeyCalculateToHorID(), horselid );
    par.set( IsochronMaker::sKeyAttribName(), attrnmfld_->text() );

    if ( msecsfld_ )
	par.setYN( IsochronMaker::sKeyOutputInMilliSecYN(),
		   msecsfld_->getBoolValue() );

    return true;
}


const char* uiIsochronMakerGrp::attributeName() const
{
    return attrnmfld_->text();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

//uiIsochronMakerBatch
uiIsochronMakerBatch::uiIsochronMakerBatch( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrCreate(uiStrings::sIsochron()),
	       mODHelpKey(mIsochronMakerBatchHelpID)))
{
    grp_ = new uiIsochronMakerGrp( this, EM::ObjectID::udf() );
    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, grp_ );
    batchfld_->jobSpec().prognm_ = GetODApplicationName( "od_isopach" );
}


uiIsochronMakerBatch::~uiIsochronMakerBatch()
{}


bool uiIsochronMakerBatch::prepareProcessing()
{
    return grp_->checkInputFlds();
}


bool uiIsochronMakerBatch::fillPar()
{
    IOPar& par = batchfld_->jobSpec().pars_;

    return grp_->fillPar( par );
}


bool uiIsochronMakerBatch::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( grp_->attributeName() );
    return batchfld_->start();
}


//uiIsochronMakerDlg
uiIsochronMakerDlg::uiIsochronMakerDlg( uiParent* p, EM::ObjectID emid )
    : uiDialog(p,Setup(uiStrings::phrCreate(uiStrings::sIsochron()),
		       mODHelpKey(mIsopachMakerHelpID)))
    , dps_( new DataPointSet(false,true) )
{
    grp_ = new uiIsochronMakerGrp( this, emid );
    const uiString title = tr("Calculate Isochron and add as an Attribute to"
			      " '%1' (bottom)" )
			       .arg( grp_->getHorizonName( emid ) );
    setTitleText( title );
}


uiIsochronMakerDlg::~uiIsochronMakerDlg()
{}


bool uiIsochronMakerDlg::acceptOK( CallBacker* )
{
    if ( !grp_->checkInputFlds() )
	return false;

    return doWork();
}


bool uiIsochronMakerDlg::doWork()
{
    IOPar par;
    if ( !grp_->fillPar(par) )
	return false;

    MultiID mid1, mid2;
    par.get( IsochronMaker::sKeyHorizonID(), mid1 );
    par.get( IsochronMaker::sKeyCalculateToHorID(), mid2 );
    uiTaskRunner taskrunner( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid2, &taskrunner );
    mDynamicCastGet(EM::Horizon3D*,h2,emobj)
    if ( !h2 )
	mErrRet(uiStrings::phrCannotLoad(tr("selected horizon")))
    h2->ref();

    const EM::ObjectID emidbase = EM::EMM().getObjectID( mid1 );
    EM::EMObject* emobjbase = EM::EMM().getObject( emidbase );
    mDynamicCastGet(EM::Horizon3D*,h1,emobjbase)
    if ( !h1 )
    {
	h2->unRef();
	mErrRet(uiStrings::phrCannotFind(tr("reference horizon")))
    }

    h1->ref();

    int dataidx = -1;
    BufferString attrnm;
    if ( !par.get( IsochronMaker::sKeyAttribName(), attrnm ) )
	return false;

    dataidx = h1->auxdata.addAuxData( attrnm );
    IsochronMaker ipmaker( *h1, *h2, attrnm, dataidx, dps_.ptr() );
    if ( SI().zIsTime() )
    {
	bool isinmsec = false;
	par.getYN( IsochronMaker::sKeyOutputInMilliSecYN(), isinmsec );
	ipmaker.setUnits( isinmsec );
    }

    const bool rv = TaskRunner::execute( &taskrunner, ipmaker );

    h1->unRef();
    h2->unRef();
    return rv;
}
