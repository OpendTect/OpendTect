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
#ifdef __win__
#include "..\EarthModel\emsurft2dtransformer.h"
#else
#include "../EarthModel/emsurft2dtransformer.h"
#endif
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

static HelpKey getHelpKey( IOObjInfo::ObjectType objtype )
{
    HelpKey key = mNoHelpKey;
    switch (objtype)
    {
    case IOObjInfo::Horizon3D:
    case IOObjInfo::Horizon2D:
	key = mODHelpKey(mProcessHorizonTime2DepthID);
	break;
    case IOObjInfo::Fault:
	key = mODHelpKey(mProcessFaultTime2DepthID);
	break;
    case IOObjInfo::FaultSet:
	key = mODHelpKey(mProcessFaultSetTime2DepthID);
	break;
    case IOObjInfo::FaultStickSet:
	key = mODHelpKey( mProcessFaultStickSetTime2DepthID );
	break;
    default:
	break;
    }

    return key;
}

uiTime2DepthDlg::uiTime2DepthDlg( uiParent* p, IOObjInfo::ObjectType objtype,
								    bool is2d )
    : uiDialog(p,uiDialog::Setup(getDlgTitle(objtype,is2d),mNoDlgTitle,
							getHelpKey(objtype)))
    , objtype_(objtype)
    , is2d_(is2d)
{
    setCtrlStyle( RunAndClose );

    IOObjContext ioobjctxt( nullptr );
    if ( objtype == IOObjInfo::Horizon3D )
	ioobjctxt = mIOObjContext(EMHorizon3D);
    else if ( objtype == IOObjInfo::Horizon2D )
	ioobjctxt = mIOObjContext(EMHorizon2D);
    else if ( objtype == IOObjInfo::Fault )
	ioobjctxt = mIOObjContext(EMFault3D);
    else if ( objtype == IOObjInfo::FaultSet )
	ioobjctxt = mIOObjContext(EMFaultSet3D);
    else if ( objtype == IOObjInfo::FaultStickSet )
	ioobjctxt = mIOObjContext(EMFaultStickSet);
    else
	return;

#ifdef __debug__
    const bool istime = SI().zIsTime();
    directionsel_ = new uiGenInput( this, tr("Convert from"),
	BoolInpSpec(true, tr("Time to Depth"), tr("Depth to Time"), true));
    directionsel_->setChecked( istime );
    mAttachCB( directionsel_->valueChanged, uiTime2DepthDlg::dirChangeCB );
#endif

    t2dtransfld_ = new uiZAxisTransformSel( this, false,
		ZDomain::sKeyTime(), ZDomain::sKeyDepth(), false, false, is2d );

    d2ttransfld_ = new uiZAxisTransformSel( this, false,
		ZDomain::sKeyDepth(), ZDomain::sKeyTime(), false, false, is2d );
#ifdef __debug__
    t2dtransfld_->attach( alignedBelow, directionsel_ );
    d2ttransfld_->attach( alignedBelow, directionsel_ );
#endif

    const ZDomain::Info& timeinf = ZDomain::TWT();
    const ZDomain::Info& depthinf = SI().depthsInFeet() ? ZDomain::DepthFeet()
							: ZDomain::DepthMeter();

    const BufferString grpnm( ioobjctxt.trgroup_->groupName() );
    const uiString uigrpnm( ioobjctxt.trgroup_->typeName() );
    const uiString timeobjm = uiStrings::phrJoinStrings( uiStrings::sTime(),
							 uigrpnm );
    const uiString depthobjm = uiStrings::phrJoinStrings( uiStrings::sDepth(),
							  uigrpnm );
    const bool canhaveattribs = objtype_ == IOObjInfo::Horizon3D;
    inptimesel_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(grpnm).withsectionfld(false)
		.withattribfld(canhaveattribs), &timeinf );
    inptimesel_->getObjSel()->setLabelText( uiStrings::phrInput(timeobjm) );
    mAttachCB( inptimesel_->inpChange, uiTime2DepthDlg::inpSelCB );
    inptimesel_->attach( alignedBelow, t2dtransfld_ );

    inpdepthsel_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(grpnm).withsectionfld(false)
		.withattribfld(canhaveattribs), &depthinf );
    inpdepthsel_->getObjSel()->setLabelText( uiStrings::phrInput(depthobjm));
    mAttachCB( inpdepthsel_->inpChange, uiTime2DepthDlg::inpSelCB );
    inpdepthsel_->attach( alignedBelow, d2ttransfld_ );

    outdepthsel_ = new uiSurfaceWrite( this,
	    uiSurfaceWrite::Setup(grpnm,uigrpnm), &depthinf );
    outdepthsel_->getObjSel()->setLabelText(uiStrings::phrOutput(depthobjm));
    outdepthsel_->attach( alignedBelow, inptimesel_ );

    outtimesel_ = new uiSurfaceWrite( this,
	    uiSurfaceWrite::Setup(grpnm,uigrpnm), &timeinf );
    outtimesel_->getObjSel()->setLabelText( uiStrings::phrOutput(timeobjm) );
    outtimesel_->attach( alignedBelow, inpdepthsel_ );

    mAttachCB( postFinalize(), uiTime2DepthDlg::dirChangeCB );
}


uiTime2DepthDlg::~uiTime2DepthDlg ()
{
    detachAllNotifiers();
}


bool uiTime2DepthDlg::is2DObject() const
{
    return objtype_ == IOObjInfo::Horizon2D;
}


uiRetVal uiTime2DepthDlg::canTransform( IOObjInfo::ObjectType objtype )
{
    uiRetVal ret;
    if ( objtype == IOObjInfo::Horizon3D || objtype == IOObjInfo::Horizon2D ||
	 objtype == IOObjInfo::Fault || objtype == IOObjInfo::FaultSet ||
	 objtype == IOObjInfo::FaultStickSet )
	return ret;
    else
	ret.add( tr("Object type is not yet supported") );

    return ret;
}


bool uiTime2DepthDlg::hasSurfaceIOData() const
{
    return objtype_ == IOObjInfo::Horizon2D || objtype_ == IOObjInfo::Horizon3D
	|| objtype_ == IOObjInfo::Fault
	|| objtype_ == IOObjInfo::FaultStickSet;
}


uiString uiTime2DepthDlg::getDlgTitle( IOObjInfo::ObjectType objtyp,
							    bool is2d ) const
{
    if ( objtyp == IOObjInfo::Horizon3D )
	return tr("Transform 3D Horizon");
    else if ( objtyp == IOObjInfo::Horizon2D )
	return tr("Transform 2D Horizon");
    else if ( objtyp == IOObjInfo::Fault )
	return tr("Transform Fault");
    else if ( objtyp == IOObjInfo::FaultSet )
	return tr("Tranform FaultSet");
    else if ( is2d && objtyp == IOObjInfo::FaultStickSet )
	return tr("Tranform FaultStickSet 2D");
    else if ( objtyp == IOObjInfo::FaultStickSet )
	return tr("Tranform FaultStickSet 3D");

    return toUiString("Object Type Not Supported");
}


const ZDomain::Info& uiTime2DepthDlg::outZDomain() const
{
    const bool isoutputdepth = directionsel_ ? directionsel_->getBoolValue() :
							    SI().zIsTime();
    return isoutputdepth ? (SI().depthsInFeet() ? ZDomain::DepthFeet()
				: ZDomain::DepthMeter()) : ZDomain::TWT() ;
}


void uiTime2DepthDlg::dirChangeCB( CallBacker* )
{
    const bool isoutputdepth = outZDomain().isDepth();

    t2dtransfld_->display( isoutputdepth );
    inptimesel_->display( isoutputdepth );
    outdepthsel_->display( isoutputdepth );

    d2ttransfld_->display( !isoutputdepth );
    inpdepthsel_->display( !isoutputdepth );
    outtimesel_->display( !isoutputdepth );
}


void uiTime2DepthDlg::inpSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiSurfaceRead*,inpfld,cb)
    if ( !inpfld )
	return;

    BufferString nm = inpfld->getObjSel()->getInput();
    if ( nm.isEmpty() )
	return;

    if ( cb == inptimesel_ )
    {
	nm.add( " Depth" );
	outdepthsel_->getObjSel()->setInputText( nm.buf() );
    }
    else
    {
	nm.add( " Time" );
	outtimesel_->getObjSel()->setInputText( nm.buf() );
    }
}


RefMan<ZAxisTransform> uiTime2DepthDlg::getWorkingZAxisTransform() const
{
    auto* zatffld = outZDomain().isDepth() ? t2dtransfld_ : d2ttransfld_;
    return zatffld->acceptOK() ? zatffld->getSelection() : nullptr;
}


const uiSurfaceRead* uiTime2DepthDlg::getWorkingInpSurfRead() const
{
    return outZDomain().isDepth() ? inptimesel_ : inpdepthsel_;
}


uiSurfaceWrite* uiTime2DepthDlg::getWorkingOutSurfWrite()
{
    return outZDomain().isDepth() ? outdepthsel_ : outtimesel_;
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
    if ( hasSurfaceIOData() && !em.getSurfaceData(inpmid, sd, errmsg))
	mErrRet(errmsg)

    auto* data = new SurfaceT2DTransfData( sd );
    data->inpmid_ = inpmid;
    data->outmid_ = outioobj->key();
    inpsel->getSelection( data->surfsel_ );
    ObjectSet<SurfaceT2DTransfData> datas;
    datas.add( data );

    PtrMan<Executor> exec = SurfaceT2DTransformer::createExecutor( datas,
						    *zatf, objtype_, is2d_ );
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
	uiMSG().errorWithDetails( tr("Fail to transform the %1").
			    arg(inpioobj->name()), surftrans->errMsg() );
	deepErase( datas );
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
	    mErrRet( tr("Cannot save transformed data.") );
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
	uiMSG().error( tr("Cannot save transformed data.") );
    }

    deepErase( datas );
    return !ret;
}

} // namespace EM
