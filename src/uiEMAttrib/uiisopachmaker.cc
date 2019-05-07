/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
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
#include "dbkey.h"
#include "posvecdataset.h"
#include "survinfo.h"

#include "uibatchjobdispatchersel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunnerprovider.h"
#include "od_helpids.h"

#define mIsoMapType uiStrings::sIsoMapType(SI().zIsTime())

uiIsochronMakerGrp::uiIsochronMakerGrp( uiParent* p, const EM::ObjID& horid )
	: uiGroup(p,"Create Isochron")
	, ctio_(*mMkCtxtIOObj(EMHorizon3D))
	, basectio_(*mMkCtxtIOObj(EMHorizon3D))
	, baseemobj_(0)
	, horid_(horid)
	, basesel_(0)
	, msecsfld_(0)
{
    baseemobj_ = EM::MGR().getObject( horid_ );
    if ( !baseemobj_ )
    {
	basectio_.ctxt_.forread_ = true;
	basesel_ = new uiIOObjSel( this, basectio_, uiStrings::sHorizon() );
    }

    ctio_.ctxt_.forread_ = true;
    horsel_ = new uiIOObjSel( this, ctio_, tr("Calculate for") );
    horsel_->selectionDone.notify( mCB(this,uiIsochronMakerGrp,toHorSel) );
    if ( !baseemobj_ )
    {
	horsel_->setInput( EM::ObjID() );
	horsel_->attach( alignedBelow, basesel_ );
    }

    attrnmfld_ = new uiGenInput( this, uiStrings::sAttribName(),
				 StringInpSpec() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, horsel_ );
    toHorSel(0);

    if ( SI().zIsTime() )
    {
	msecsfld_ = new uiGenInput( this, tr("Output in"),
			    BoolInpSpec(true,uiStrings::sMSec(false,mPlural),
					     uiStrings::sSec(false,mPlural)) );
	msecsfld_->attach( alignedBelow, attrnmfld_ );
    }

    setHAlignObj( basesel_ ? basesel_ : horsel_ );
}


BufferString uiIsochronMakerGrp::getHorNm( const EM::ObjID& horid )
{
    return EM::MGR().objectName( horid );
}


uiIsochronMakerGrp::~uiIsochronMakerGrp()
{
    delete ctio_.ioobj_; delete &ctio_;
    delete basectio_.ioobj_; delete &basectio_;
}


void uiIsochronMakerGrp::toHorSel( CallBacker* )
{
    if ( horsel_->ioobj(true) )
	attrnmfld_->setText( BufferString("I: ",horsel_->ioobj()->name()) );
}


bool uiIsochronMakerGrp::chkInputFlds()
{
    if ( basesel_ && !basesel_->commitInput() )
	return false;

    horsel_->commitInput();
    if ( !horsel_->ioobj() ) return false;

    BufferString attrnm =  attrnmfld_->text();
    if ( attrnm.isEmpty() )
    {
	uiMSG().error( uiStrings::phrEnter(tr("attribute name")) );
	attrnm.add( "I: " ).add( horsel_->ioobj()->name() );
	attrnmfld_->setText( attrnm );
	return false;
    }

    return true;
}


bool uiIsochronMakerGrp::fillPar( IOPar& par )
{
    par.set( IsochronMaker::sKeyHorizonID(), basesel_ ? basesel_->ioobj()->key()
						      : baseemobj_->dbKey() );
    par.set( IsochronMaker::sKeyCalculateToHorID(), horsel_->ioobj()->key() );
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
    : uiDialog(p, Setup(tr("Create %1").arg(mIsoMapType), mNoDlgTitle,
    mODHelpKey(mIsochronMakerBatchHelpID)))
{
    grp_ = new uiIsochronMakerGrp( this, EM::ObjID() );
    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, grp_ );
    batchfld_->jobSpec().prognm_ = "od_isopach";
}


bool uiIsochronMakerBatch::prepareProcessing()
{
    return grp_->chkInputFlds();
}


bool uiIsochronMakerBatch::fillPar()
{
    IOPar& par = batchfld_->jobSpec().pars_;
    if ( !grp_->fillPar( par ) )
	return false;

    EM::ObjID emid;
    par.get( IsochronMaker::sKeyHorizonID(), emid );
    EM::IOObjInfo eminfo( emid );
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


bool uiIsochronMakerBatch::acceptOK()
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    batchfld_->setJobName( grp_->attrName() );
    return batchfld_->start();
}


//uiIsochronMakerDlg
uiIsochronMakerDlg::uiIsochronMakerDlg( uiParent* p, const DBKey& emid )
    : uiDialog(p,Setup(tr("Create %1").arg(mIsoMapType),mNoDlgTitle,
			mODHelpKey(mIsopachMakerHelpID)))
    , dps_( new DataPointSet(false,true) )
{
    dps_->ref();
    grp_ = new uiIsochronMakerGrp( this, emid );
    uiString title( uiStrings::phrCreate(tr("%1 for '%2'")
						.arg(mIsoMapType)
						.arg(grp_->getHorNm(emid))) );
    setTitleText( title );
}


uiIsochronMakerDlg::~uiIsochronMakerDlg()
{
    dps_->unRef();
}


bool uiIsochronMakerDlg::acceptOK()
{
    if ( !grp_->chkInputFlds() )
	return false;

    return doWork();
}


bool uiIsochronMakerDlg::doWork()
{
    IOPar par;
    grp_->fillPar(par);
    DBKey emid1, emid2;
    par.get( IsochronMaker::sKeyHorizonID(), emid1 );
    par.get( IsochronMaker::sKeyCalculateToHorID(), emid2 );
    uiTaskRunnerProvider trprov( this );
    EM::Object* emobj = EM::MGR().loadIfNotFullyLoaded( emid2, trprov );
    mDynamicCastGet(EM::Horizon3D*,h2,emobj)
    if ( !h2 )
	mErrRet(tr("Cannot load selected horizon"))
    h2->ref();

    EM::Object* emobjbase = EM::MGR().getObject( emid1 );
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

    bool rv = trprov.execute( ipmaker );
    h1->unRef(); h2->unRef();
    return rv;
}
