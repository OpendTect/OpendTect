/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitime2depthdlg.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizonztransformer.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emsurft2dtransformer.h"
#include "executor.h"
#include "ioman.h"
#include "task.h"
#include "transl.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uigeninput.h"
#include "uiioobjselgrp.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"

namespace EM
{
uiTime2DepthDlg::uiTime2DepthDlg( uiParent* p, EMObjectType objtype )
    : uiDialog(p,uiDialog::Setup(getDlgTitle(objtype),mNoDlgTitle,
	       mODHelpKey(mProcessHorizonTime2DepthID)).modal(false))
    , objtype_(objtype)
{
    setCtrlStyle( RunAndClose );

    IOObjContext ioobjctxt( nullptr );
    bool is2d = is2DObject();
    if ( objtype == EMObjectType::Hor3D )
	ioobjctxt = mIOObjContext(EMHorizon3D);
    else if ( objtype == EMObjectType::Hor2D )
	ioobjctxt = mIOObjContext(EMHorizon2D);
    else if ( objtype == EMObjectType::Flt3D )
	ioobjctxt = mIOObjContext(EMFault3D);
    else if ( objtype == EMObjectType::FltSet )
	ioobjctxt = mIOObjContext(EMFaultSet3D);
    else if ( objtype == EMObjectType::FltSS2D )
	ioobjctxt = mIOObjContext(EMFaultStickSet);
    else if ( objtype == EMObjectType::FltSS3D )
	ioobjctxt = mIOObjContext(EMFaultStickSet);
    else
	return;

    const bool istime = SI().zIsTime();
    directionsel_ = new uiGenInput( this, tr("Convert from"),
	BoolInpSpec(true, tr("Time to Depth"), tr("Depth to Time"), true));
    directionsel_->setChecked( istime );
    mAttachCB( directionsel_->valueChanged, uiTime2DepthDlg::dirChangeCB );

    t2dtransfld_ = new uiZAxisTransformSel( this, false,
		ZDomain::sKeyTime(), ZDomain::sKeyDepth(), false, false, is2d );
    t2dtransfld_->attach( alignedBelow, directionsel_ );

    d2ttransfld_ = new uiZAxisTransformSel( this, false,
		ZDomain::sKeyDepth(), ZDomain::sKeyTime(), false, false, is2d );
    d2ttransfld_->attach( alignedBelow, directionsel_ );

    const ZDomain::Info& timeinf = ZDomain::TWT();
    const ZDomain::Info& depthinf = SI().depthsInFeet() ? ZDomain::DepthFeet()
							: ZDomain::DepthMeter();

    const BufferString grpnm( ioobjctxt.trgroup_->groupName() );
    const uiString uigrpnm( ioobjctxt.trgroup_->typeName() );
    const uiString timeobjm = uiStrings::phrJoinStrings( uiStrings::sTime(),
							 uigrpnm );
    const uiString depthobjm = uiStrings::phrJoinStrings( uiStrings::sDepth(),
							  uigrpnm );
    const bool canhaveattribs = objtype_ == EMObjectType::Hor3D;
    inptimehorsel_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(grpnm).withsubsel(true)
			.withsectionfld(false).withattribfld(canhaveattribs),
		&timeinf );
    inptimehorsel_->getObjSel()->setLabelText( uiStrings::phrInput(timeobjm) );
    mAttachCB( inptimehorsel_->inpChange, uiTime2DepthDlg::horSelCB );
    inptimehorsel_->attach( alignedBelow, t2dtransfld_ );

    inpdepthhorsel_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(grpnm).withsubsel(true)
			.withsectionfld(false).withattribfld(canhaveattribs),
		&depthinf );
    inpdepthhorsel_->getObjSel()->setLabelText( uiStrings::phrInput(depthobjm));
    mAttachCB( inpdepthhorsel_->inpChange, uiTime2DepthDlg::horSelCB );
    inpdepthhorsel_->attach( alignedBelow, d2ttransfld_ );

    outdepthhorsel_ = new uiSurfaceWrite( this,
	    uiSurfaceWrite::Setup(grpnm,uigrpnm), &depthinf );
    outdepthhorsel_->getObjSel()->setLabelText(uiStrings::phrOutput(depthobjm));
    outdepthhorsel_->attach( alignedBelow, inptimehorsel_ );

    outtimehorsel_ = new uiSurfaceWrite( this,
	    uiSurfaceWrite::Setup(grpnm,uigrpnm), &timeinf );
    outtimehorsel_->getObjSel()->setLabelText( uiStrings::phrOutput(timeobjm) );
    outtimehorsel_->attach( alignedBelow, inpdepthhorsel_ );

    mAttachCB( postFinalize(), uiTime2DepthDlg::dirChangeCB );
}


uiTime2DepthDlg::~uiTime2DepthDlg ()
{
    detachAllNotifiers();
}


bool uiTime2DepthDlg::is2DObject() const
{
    return objtype_ == EMObjectType::Hor2D ||
	   objtype_ == EMObjectType::FltSS2D;
}


uiRetVal uiTime2DepthDlg::canTransform( EMObjectType objtype )
{
    uiRetVal ret;
    if ( objtype == EMObjectType::Hor3D || objtype == EMObjectType::Hor2D ||
	objtype == EMObjectType::Flt3D || objtype == EMObjectType::FltSet ||
	objtype == EMObjectType::FltSS2D || objtype == EMObjectType::FltSS3D )
	return ret;
    else
	ret.add( tr("Object type is not yet supported") );

    return ret;
}


uiString uiTime2DepthDlg::getDlgTitle( EMObjectType objtyp ) const
{
    if ( objtyp == EMObjectType::Hor3D )
	return tr("Transform 3D Horizon");
    else if ( objtyp == EMObjectType::Hor2D )
	return tr("Transform 2D Horizon");
    else if ( objtyp == EMObjectType::Flt3D )
	return tr("Transform Fault");
    else if ( objtyp == EMObjectType::FltSet )
	return tr("Tranform FaultSet");
    else if ( objtyp == EMObjectType::FltSS2D )
	return tr("Tranform FaultStickSet 2D");
    else if ( objtyp == EMObjectType::FltSS3D )
	return tr("Tranform FaultStickSet 3D");

    return tr("Object Type Not Supported");
}


const ZDomain::Info& uiTime2DepthDlg::outZDomain() const
{
    const bool isdepth = t2dtransfld_->isDisplayed();
    return isdepth ? (SI().depthsInFeet() ? ZDomain::DepthFeet()
					  : ZDomain::DepthMeter())
		   : ZDomain::TWT();
}


void uiTime2DepthDlg::dirChangeCB( CallBacker* )
{
    const bool todepth = directionsel_->getBoolValue();
    const uiString lbl = todepth ? tr("Output Depth Horizon")
				 : tr("Output Time Horizon");
    t2dtransfld_->display( todepth );
    inptimehorsel_->display( todepth );
    outdepthhorsel_->display( todepth );

    d2ttransfld_->display( !todepth );
    inpdepthhorsel_->display( !todepth );
    outtimehorsel_->display( !todepth );
}


void uiTime2DepthDlg::horSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiSurfaceRead*,inpfld,cb)
    if ( !inpfld )
	return;

    BufferString hornm = inpfld->getObjSel()->getInput();
    if ( hornm.isEmpty() )
	return;

    if ( cb == inptimehorsel_ )
    {
	hornm.add( " Depth" );
	outdepthhorsel_->getObjSel()->setInputText( hornm.buf() );
    }
    else
    {
	hornm.add( " Time" );
	outtimehorsel_->getObjSel()->setInputText( hornm.buf() );
    }
}


const char* uiTime2DepthDlg::sKeyTime2Depth() const
{
    return "Time2Depth";
}


const char* uiTime2DepthDlg::sKeyTransformation() const
{
    return "Transformation";
}


RefMan<ZAxisTransform> uiTime2DepthDlg::getWorkingZAxisTransform() const
{
    const bool todepth = directionsel_->getBoolValue();
    auto* zatffld = todepth ? t2dtransfld_ : d2ttransfld_;
    return zatffld->getSelection();
}


const uiSurfaceRead* uiTime2DepthDlg::getWorkingInpSurfRead() const
{
    const bool todepth = directionsel_->getBoolValue();
    return todepth ? inptimehorsel_ : inpdepthhorsel_;
}


uiSurfaceWrite* uiTime2DepthDlg::getWorkingOutSurfWrite()
{
    const bool todepth = directionsel_->getBoolValue();
    return todepth ? outdepthhorsel_ : outtimehorsel_;
}


bool uiTime2DepthDlg::usePar( const IOPar& par )
{
    const bool is2d = is2DObject();
    const IOPar* dimpar = par.subselect( is2d ? sKey::TwoD() :
							    sKey::ThreeD() );
    if ( !dimpar )
	return false;

    const IOPar* objpar = dimpar->subselect(
				    EMObjectTypeDef().getKey(objtype_) );
    if ( !objpar )
	return false;

    bool ist2d = SI().zIsTime();
    objpar->getYN( sKeyTime2Depth(),ist2d );
    directionsel_->setValue( ist2d );
    MultiID mid;
    const IOPar* transfldpar = objpar->subselect( sKeyTransformation() );
    objpar->get( sKey::ID(), mid );
    if ( ist2d )
    {
	if ( !mid.isUdf() )
	    inptimehorsel_->getObjSel()->setInput( mid );

	if ( transfldpar )
	    t2dtransfld_->usePar( *transfldpar );
    }
    else
    {
	if ( !mid.isUdf() )
	    inpdepthhorsel_->getObjSel()->setInput( mid );

	if ( transfldpar )
	    d2ttransfld_->usePar( *transfldpar );
    }

    return true;
}


bool uiTime2DepthDlg::fillPar( IOPar& par ) const
{
    const bool is2d = is2DObject();
    const BufferString dimkey( is2d ? sKey::TwoD() : sKey::ThreeD() );
    const BufferString objtypekey( EMObjectTypeDef().getKey(objtype_) );
    const BufferString basekey( IOPar::compKey(dimkey,objtypekey) );
    const bool ist2d = directionsel_->getBoolValue();
    par.setYN( IOPar::compKey(basekey,sKeyTime2Depth()), ist2d );
    auto* readerfld = const_cast<uiSurfaceRead*>( getWorkingInpSurfRead() );
    par.set( IOPar::compKey(basekey,sKey::ID()),
					    readerfld->getObjSel()->key() );
    IOPar transfldpar;
    if ( ist2d )
    {
	if ( t2dtransfld_->isOK() && t2dtransfld_->getSelection() )
	    t2dtransfld_->fillPar( transfldpar );
    }
    else
    {
	if ( d2ttransfld_->isOK() && d2ttransfld_->getSelection() )
	    d2ttransfld_->fillPar( transfldpar );
    }

    par.mergeComp( transfldpar, IOPar::compKey(basekey,sKeyTransformation()) );
    return true;
}


bool uiTime2DepthDlg::hasSurfaceIOData() const
{
    return objtype_ == EMObjectType::Hor3D || objtype_ == EMObjectType::Hor2D
	    || objtype_ == EMObjectType::Flt3D
	    || objtype_ == EMObjectType::FltSS2D
	    || objtype_ == EMObjectType::FltSS3D;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTime2DepthDlg::acceptOK( CallBacker* )
{
    uiSurfaceWrite* outfld = getWorkingOutSurfWrite();
    if ( !outfld )
	return true;

    const uiSurfaceRead* inpsel = getWorkingInpSurfRead();
    const IOObj* inpioobj = inpsel->selIOObj();
    const IOObj* outioobj = outfld->selIOObj();
    if ( !inpioobj || !outioobj )
	mErrRet( uiStrings::phrCannotFindObjInDB() );

    RefMan<ZAxisTransform> zatf = getWorkingZAxisTransform();
    if ( !zatf )
	mErrRet( tr("Provide valid transformation") );

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    uiString errmsg;
    const MultiID inpmid = inpioobj->key();
    if ( hasSurfaceIOData() && !em.getSurfaceData(inpmid,sd,errmsg) )
	mErrRet(errmsg)

    auto* data = new SurfaceT2DTransfData( sd );
    data->inpmid_ = inpmid;
    data->outmid_ = outioobj->key();
    inpsel->getSelection( data->surfsel_ );
    ObjectSet<SurfaceT2DTransfData> datas;
    datas.add( data );

    PtrMan<Executor> exec = SurfaceT2DTransformer::createExecutor( datas,
							    *zatf, objtype_ );
    mDynamicCastGet(SurfaceT2DTransformer*,surftrans,exec.ptr());
    if ( !surftrans )
    {
	deepErase( datas );
	return false;
    }

    uiTaskRunner tskr( this );
    const bool errocc = TaskRunner::execute( &tskr, *surftrans );
    if ( !errocc || surftrans->errMsg().isError() )
    {
	deepErase( datas );
	uiMSG().errorWithDetails( surftrans->errMsg().messages(),
		    tr("Fail to transform the %1").arg(inpioobj->name()) );
	return false;
    }

    bool ret = true;
    RefMan<Surface> surf = surftrans->getTransformedSurface( data->outmid_ );
    if ( surf )
    {
	PtrMan<Executor> saver = surf->saver();
	if ( !saver || !TaskRunner::execute(&tskr,*saver) )
	{
	    deepErase( datas );
	    mErrRet( tr("Can not save tranformed data.") );
	}

	const MultiID outmid = surf->multiID();
	PtrMan<IOObj> obj = IOM().get( outmid );
	if ( surf->zDomain().fillPar(obj->pars()) )
	    IOM().commitChanges( *obj );

	ret = uiMSG().askGoOn( tr("Successfully transformed %1.\n"
			    "Do you want to tranform another object?").
			    arg(inpioobj->name()) );
    }
    else
    {
	ret = false;
	uiMSG().error( tr("Cannot save tranformed data.") );
    }

    deepErase( datas );
    return !ret;
}

} // namespace EM
