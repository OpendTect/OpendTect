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
static HelpKey getHelpKey( ObjectType objtype )
{
    HelpKey key = mNoHelpKey;
    switch( objtype )
    {
	case ObjectType::Hor3D:
	case ObjectType::Hor2D:
	    key = mODHelpKey( mProcessHorizonTime2DepthID );
	    break;
	case ObjectType::Flt3D:
	    key = mODHelpKey( mProcessFaultTime2DepthID );
	    break;
	case ObjectType::FltSet:
	    key = mODHelpKey( mProcessFaultSetTime2DepthID );
	    break;
	case ObjectType::FltSS2D:
	case ObjectType::FltSS3D:
	case ObjectType::FltSS2D3D:
	    key = mODHelpKey( mProcessFaultStickSetTime2DepthID );
	    break;
	default:
	    break;
    }

    return key;
}

uiTime2DepthDlg::uiTime2DepthDlg( uiParent* p, ObjectType objtype )
    : uiDialog(p,uiDialog::Setup(getDlgTitle(objtype),mNoDlgTitle,
					getHelpKey(objtype)).modal(false))
    , objtype_(objtype)
{
    setCtrlStyle( RunAndClose );

    IOObjContext ioobjctxt( nullptr );
    bool is2d = is2DObject();
    if ( objtype == ObjectType::Hor3D )
	ioobjctxt = mIOObjContext(EMHorizon3D);
    else if ( objtype == ObjectType::Hor2D )
	ioobjctxt = mIOObjContext(EMHorizon2D);
    else if ( objtype == ObjectType::Flt3D )
	ioobjctxt = mIOObjContext(EMFault3D);
    else if ( objtype == ObjectType::FltSet )
	ioobjctxt = mIOObjContext(EMFaultSet3D);
    else if ( objtype == ObjectType::FltSS2D )
	ioobjctxt = mIOObjContext(EMFaultStickSet);
    else if ( objtype == ObjectType::FltSS3D )
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
    const bool canhaveattribs = objtype_ == ObjectType::Hor3D;
    inptimesel_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(grpnm)
			.withsectionfld(false).withattribfld(canhaveattribs),
		&timeinf );
    inptimesel_->getObjSel()->setLabelText( uiStrings::phrInput(timeobjm) );
    mAttachCB( inptimesel_->inpChange, uiTime2DepthDlg::inpSelCB);
    inptimesel_->attach( alignedBelow, t2dtransfld_ );

    inpdepthsel_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(grpnm)
			.withsectionfld(false).withattribfld(canhaveattribs),
		&depthinf );
    inpdepthsel_->getObjSel()->setLabelText( uiStrings::phrInput(depthobjm));
    mAttachCB( inpdepthsel_->inpChange, uiTime2DepthDlg::inpSelCB);
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
    return objtype_ == ObjectType::Hor2D ||
	   objtype_ == ObjectType::FltSS2D;
}


uiRetVal uiTime2DepthDlg::canTransform( ObjectType objtype )
{
    uiRetVal ret;
    if ( objtype == ObjectType::Hor3D || objtype == ObjectType::Hor2D ||
	objtype == ObjectType::Flt3D || objtype == ObjectType::FltSet ||
	objtype == ObjectType::FltSS2D || objtype == ObjectType::FltSS3D )
	return ret;
    else
	ret.add( tr("Object type is not yet supported") );

    return ret;
}


uiString uiTime2DepthDlg::getDlgTitle( ObjectType objtyp ) const
{
    if ( objtyp == ObjectType::Hor3D )
	return tr("Transform 3D Horizon");
    else if ( objtyp == ObjectType::Hor2D )
	return tr("Transform 2D Horizon");
    else if ( objtyp == ObjectType::Flt3D )
	return tr("Transform Fault");
    else if ( objtyp == ObjectType::FltSet )
	return tr("Tranform FaultSet");
    else if ( objtyp == ObjectType::FltSS2D )
	return tr("Tranform FaultStickSet 2D");
    else if ( objtyp == ObjectType::FltSS3D )
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
    t2dtransfld_->display( todepth );
    inptimesel_->display( todepth );
    outdepthsel_->display( todepth );

    d2ttransfld_->display( !todepth );
    inpdepthsel_->display( !todepth );
    outtimesel_->display( !todepth );
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
    return todepth ? inptimesel_ : inpdepthsel_;
}


uiSurfaceWrite* uiTime2DepthDlg::getWorkingOutSurfWrite()
{
    const bool todepth = directionsel_->getBoolValue();
    return todepth ? outdepthsel_ : outtimesel_;
}


bool uiTime2DepthDlg::usePar( const IOPar& par )
{
    const bool is2d = is2DObject();
    const IOPar* dimpar = par.subselect( is2d ? sKey::TwoD() :
							    sKey::ThreeD() );
    if ( !dimpar )
	return false;

    const IOPar* objpar = dimpar->subselect(
				    ObjectTypeDef().getKey(objtype_) );
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
	    inptimesel_->getObjSel()->setInput( mid );

	if ( transfldpar )
	    t2dtransfld_->usePar( *transfldpar );
    }
    else
    {
	if ( !mid.isUdf() )
	    inpdepthsel_->getObjSel()->setInput( mid );

	if ( transfldpar )
	    d2ttransfld_->usePar( *transfldpar );
    }

    return true;
}


bool uiTime2DepthDlg::fillPar( IOPar& par ) const
{
    const bool is2d = is2DObject();
    const BufferString dimkey( is2d ? sKey::TwoD() : sKey::ThreeD() );
    const BufferString objtypekey( ObjectTypeDef().getKey(objtype_) );
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
    return objtype_ == ObjectType::Hor3D || objtype_ == ObjectType::Hor2D
	    || objtype_ == ObjectType::Flt3D
	    || objtype_ == ObjectType::FltSS2D
	    || objtype_ == ObjectType::FltSS3D;
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
