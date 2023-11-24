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
#include "ctxtioobj.h"
#include "ioman.h"
#include "mousecursor.h"
#include "task.h"
#include "transl.h"

#include "uigeninput.h"
#include "uiioobjselgrp.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"
#include "uiunitsel.h"

namespace EM
{
uiTime2DepthDlg::uiTime2DepthDlg ( uiParent* p, IOObjInfo::ObjectType objtype )
    : uiDialog(p,uiDialog::Setup(getDlgTitle(objtype), mNoDlgTitle,
								mNoHelpKey))
    , objtype_(objtype)
{
    IOObjContext ioobjctxt( nullptr );
    bool is2d = false;
    if ( objtype == IOObjInfo::Horizon3D )
	ioobjctxt = mIOObjContext(EMHorizon3D);
    else
	return;

    const bool istime = SI().zIsTime();
    auto* topgrp = new uiGroup( this, "topgrp" );
    directionsel_ = new uiGenInput( topgrp, tr("Convert from"),
	BoolInpSpec(true, tr("Time to Depth"), tr("Depth to Time"), true));
    directionsel_->setChecked( istime );
    mAttachCB(directionsel_->valueChanged,uiTime2DepthDlg::dirChangeCB);

    auto* zatfgrp = new uiGroup( this, "ZAxisTranformGroup" );
    zatfgrp->attach( alignedBelow, topgrp );
    t2dtransfld_ = new uiZAxisTransformSel( zatfgrp, false,
		ZDomain::sKeyTime(), ZDomain::sKeyDepth(), true, false, is2d );
    d2ttransfld_ = new uiZAxisTransformSel( zatfgrp, false,
		ZDomain::sKeyDepth(), ZDomain::sKeyTime(), true, false, is2d );

    auto* inphorgrp = new uiGroup( this, "HorizonGroup");
    inphorgrp->attach( alignedBelow, zatfgrp );

    const BufferString grpnm( ioobjctxt.trgroup_->groupName() );
    inptimehorsel_ = new uiSurfaceRead( inphorgrp, uiSurfaceRead::Setup(
	grpnm).withsubsel(true).withsectionfld(false), &ZDomain::TWT() );

    const ZDomain::Info& depthinf =
	SI().depthsInFeet() ? ZDomain::DepthFeet() : ZDomain::DepthMeter();
    inpdepthhorsel_ = new uiSurfaceRead( inphorgrp, uiSurfaceRead::Setup(
	grpnm).withsubsel(true).withsectionfld(false), &depthinf );

    auto* outgrp = new uiGroup( this, "outgrp" );
    outgrp->attach( alignedBelow, inphorgrp );
    ioobjctxt.forread_ = false;
    outfld_ = new uiIOObjSel( outgrp, ioobjctxt, uiString::empty() );

    mAttachCB( postFinalize(), uiTime2DepthDlg::dirChangeCB );
}


uiTime2DepthDlg::~uiTime2DepthDlg ()
{
    detachAllNotifiers();
}


uiRetVal uiTime2DepthDlg::canTransform( IOObjInfo::ObjectType objtype )
{
    uiRetVal ret;
    if ( objtype == IOObjInfo::Horizon3D )
	return ret;
    else
	ret.add( tr("Object type is not yet supported") );

    return ret;
}


uiString uiTime2DepthDlg::getDlgTitle( IOObjInfo::ObjectType objyyp ) const
{
    if ( objyyp == IOObjInfo::Horizon3D )
	return tr("Transform 3D Horizon");

    return toUiString("Object Type Not Supported");
}


const ZDomain::Info& uiTime2DepthDlg::outZDomain() const
{
    const bool isdepth = t2dtransfld_->isDisplayed();
    return isdepth && SI().depthsInFeet() ? ZDomain::DepthFeet() :
	isdepth ? ZDomain::DepthMeter() : ZDomain::TWT();
}


void uiTime2DepthDlg::dirChangeCB( CallBacker* )
{
    const bool todepth = directionsel_->getBoolValue();
    const uiString lbl = todepth ? tr("Output Depth Horizon")
				 : tr("Output Time Horizon");
    outfld_->setLabelText( lbl );
    t2dtransfld_->display( todepth );
    inptimehorsel_->display( todepth );
    d2ttransfld_->display( !todepth );
    inpdepthhorsel_->display( !todepth );
}


const uiSurfaceRead* uiTime2DepthDlg::getWorkingInpSurfRead() const
{
    const bool todepth = directionsel_->getBoolValue();
    return todepth ? inptimehorsel_ : inpdepthhorsel_;
}


RefMan<ZAxisTransform> uiTime2DepthDlg::getWorkingZAxisTransform() const
{
    const bool todepth = directionsel_->getBoolValue();
    auto* zatffld = todepth ? t2dtransfld_ : d2ttransfld_;
    return zatffld->getSelection();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTime2DepthDlg::acceptOK( CallBacker* )
{
    if ( !outfld_ )
	return true;

    const uiSurfaceRead* inpsel = getWorkingInpSurfRead();
    const IOObj* ioobj = inpsel->selIOObj();
    if ( !ioobj )
	mErrRet( tr("Cannot find input horizon in repository") );

    RefMan<ZAxisTransform> zatf = getWorkingZAxisTransform();
    if ( !zatf )
	mErrRet( tr("Provide valid transformation") );

    auto* data = new SurfaceT2DTransfData();
    data->inpmid_ = ioobj->key();
    data->outmid_ = outfld_->key();
    inpsel->getSelection( data->surfsel_ );
    ObjectSet<SurfaceT2DTransfData> datas;
    datas.add( data );

    SurfaceT2DTransformer transf( datas, *zatf, objtype_ );
    transf.setZDomain( outZDomain() );
    uiTaskRunner tskr( this );
    if ( !TaskRunner::execute(&tskr,transf) )
    {
	uiMSG().errorWithDetails( tr("Fail to transform the %1").
	    arg(ioobj->name()), transf.uiMessage() );

	deepErase( datas );
	return false;
    }

    bool ret = true;
    RefMan<Surface> surf = transf.getTransformedSurface( outfld_->key() );
    if ( surf )
    {
	PtrMan<Executor> saver = surf->saver();
	if ( !saver || !TaskRunner::execute(&tskr,*saver) )
	    mErrRet( tr("Can not save output horizon data.") );

	const MultiID outmid = surf->multiID();
	PtrMan<IOObj> obj = IOM().get( outmid );
	if ( surf->zDomain().fillPar(obj->pars()) )
	    IOM().commitChanges( *obj );

	ret = uiMSG().askGoOn( tr("Successfully transformed %1."
			    "Do you want to tranform another 3D Horizon?").
			    arg(ioobj->name()) );
    }

    deepErase( datas );
    return !ret;
}

}
