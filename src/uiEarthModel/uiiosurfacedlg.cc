/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "ctxtioobj.h"
#include "emfaultstickset.h"
#include "emfault3d.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "empolygonbody.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "ioobj.h"
#include "rangeposprovider.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"
#include "uipossubsel.h"
#include "od_helpids.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Surface& surf,
				      float shift )
    : uiDialog(p,uiDialog::Setup( uiStrings::sOutputSelection(),mNoDlgTitle,
				 mODHelpKey(mWriteSurfaceDlgHelpID) ))
    , surface_(surf)
{
    mDynamicCastGet(const EM::Horizon*,hor,&surface_);
    const bool hasshift = hor && !mIsZero(shift,SI().zRange(true).step*1e-3f);
    mDynamicCastGet(const EM::PolygonBody*,plgbody,&surface_);
    const bool usesubsel = !plgbody;

    iogrp_ = new uiSurfaceWrite( this, surface_,
			     uiSurfaceWrite::Setup(surface_.getTypeStr(),
						   surface_.getUserTypeStr())
			     .withdisplayfld(!hasshift).withsubsel(usesubsel) );
    if ( hasshift )
    {
	BufferString newnm = hor->name();
	const float usrshift = shift * SI().zDomain().userFactor();
	newnm.add(" (").add(usrshift,SI().nrZDecimals()).add(")");
	iogrp_->getObjSel()->setInputText( newnm.buf() );
    }
}


bool uiWriteSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp_->processInput();
}


void uiWriteSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp_->getSelection( sels );
    sels.selvalues.erase();
}


const IOObj* uiWriteSurfaceDlg::ioObj() const
{
    return iogrp_->selIOObj();
}


bool uiWriteSurfaceDlg::replaceInTree() const
{
    return iogrp_->replaceInTree();
}


//Not used anymore, keep it?
uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup( uiStrings::sInputSelection(),
	       mNoDlgTitle,mNoHelpKey))
{
    iogrp_ = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(typ).withattribfld(false) );
}


bool uiReadSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp_->processInput();
}


const IOObj* uiReadSurfaceDlg::ioObj() const
{
    return iogrp_->selIOObj();
}


void uiReadSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp_->getSelection( sels );
}


uiStoreAuxData::uiStoreAuxData( uiParent* p, const EM::Horizon3D& surf )
    : uiDialog(p,uiDialog::Setup( uiStrings::sOutputSelection(),
				 tr("Specify attribute name"),
				 mODHelpKey(mStoreAuxDataHelpID) ))
    , surface_(surf)
{
    attrnmfld_ = new uiGenInput( this, uiStrings::sAttribute() );
    attrnmfld_->setText( surface_.auxdata.auxDataName(0) );
}


const char* uiStoreAuxData::auxdataName() const
{ return attrnmfld_->text(); }


bool uiStoreAuxData::acceptOK( CallBacker* )
{
    dooverwrite_ = false;
    BufferString attrnm = attrnmfld_->text();
    const bool ispres = checkIfAlreadyPresent( attrnm.buf() );
    if ( ispres )
    {
	uiString msg = tr("This surface already has an attribute called:\n%1"
			  "\nDo you wish to overwrite this data?").arg(attrnm);
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
	dooverwrite_ = true;
    }

    if ( attrnm != surface_.auxdata.auxDataName(0) )
	const_cast<EM::Horizon3D&>(surface_).
	    auxdata.setAuxDataName( 0, attrnm.buf() );

    return true;
}


bool uiStoreAuxData::checkIfAlreadyPresent( const char* attrnm )
{
    EM::IOObjInfo eminfo( surface_.multiID() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );

    bool present = false;
    for ( int idx=0; idx<attrnms.size(); idx++ )
    {
	if ( attrnms.get(idx) == attrnm )
	{
	    present = true;
	    break;
	}
    }

    return present;
}


#define mGet( ioobj, hor2d, hor3d, emfss, flt3d ) \
    (ioobj.group() == EMHorizon2DTranslatorGroup::sGroupName() ? hor2d : \
    (ioobj.group() == EMHorizon3DTranslatorGroup::sGroupName() ? hor3d : \
    (ioobj.group() == EMFaultStickSetTranslatorGroup::sGroupName()? emfss\
							    : flt3d )))

#define mGetHelpID(ioobj) \
    mGet( ioobj, mODHelpKey(mCopySurface2DHelpID), \
	  mODHelpKey(mCopySurface3DHelpID), \
	  mODHelpKey(mCopySurfaceStickSetsHelpID), \
	  mODHelpKey(mCopySurfaceFaultsHelpID) )

#define mGetStr(phrfn,ioobj) \
    mGet( ioobj,\
	uiStrings::phrfn(uiStrings::phrJoinStrings(uiStrings::s2D(),  \
		uiStrings::sHorizon(1))), \
	uiStrings::phrfn(uiStrings::phrJoinStrings(uiStrings::s3D(),  \
		uiStrings::sHorizon(1))), \
	uiStrings::phrfn(uiStrings::sFaultStickSet()), \
	uiStrings::phrfn(uiStrings::sFault()))

#define mGetWinStr(ioobj) \
    mGetStr( phrCopy, ioobj )

#define mGetOutputStr(ioobj) \
    mGetStr( phrOutput, ioobj )


uiCopySurface::uiCopySurface( uiParent* p, const IOObj& ioobj,
			      const uiSurfaceRead::Setup& su )
    : uiDialog(p,Setup(mGetWinStr(ioobj),mNoDlgTitle,mGetHelpID(ioobj)))
    , ctio_(*mkCtxtIOObj(ioobj))
{
    inpfld = new uiSurfaceRead( this, su );
    inpfld->setIOObj( ioobj.key() );

    ctio_.ctxt_.forread_ = false;
    ctio_.setObj( 0 );

    outfld = new uiIOObjSel( this, ctio_, mGetOutputStr(ioobj) );
    outfld->attach( alignedBelow, inpfld );
}


uiCopySurface::~uiCopySurface()
{
    delete ctio_.ioobj_;
    delete &ctio_;
}


CtxtIOObj* uiCopySurface::mkCtxtIOObj( const IOObj& ioobj )
{
    if ( ioobj.group() == EM::Horizon2D::typeStr() )
	return mMkCtxtIOObj( EMHorizon2D );
    if ( ioobj.group() == EM::Horizon3D::typeStr() )
	return mMkCtxtIOObj( EMHorizon3D );
    if ( ioobj.group() == EM::FaultStickSet::typeStr() )
	return mMkCtxtIOObj( EMFaultStickSet );
    if ( ioobj.group() == EM::Fault3D::typeStr() )
	return mMkCtxtIOObj( EMFault3D );
    return 0;
}


#define mErrRet(msg) { if ( !msg.isEmpty() ) uiMSG().error(msg); return false; }

bool uiCopySurface::acceptOK( CallBacker* )
{
    if ( !inpfld->processInput() ) return false;
    if ( !outfld->commitInput() )
	mErrRet( (outfld->isEmpty() ? uiStrings::phrSelect(uiStrings::phrOutput
		 (uiStrings::sSurface())) : uiStrings::sEmptyString()) )

    const IOObj* ioobj = inpfld->selIOObj();
    if ( !ioobj ) mErrRet(uiStrings::phrCannotFind(uiStrings::sSurface()))

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    inpfld->getSelection( sdsel );

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet(uiStrings::phrCannotCreate(tr("Object")))
    emobj->setMultiID( ioobj->key() );

    mDynamicCastGet(EM::Surface*,surface,emobj.ptr())
    PtrMan<Executor> loader = surface->geometry().loader( &sdsel );
    if ( !loader ) mErrRet(uiStrings::phrCannotRead(uiStrings::sSurface()))

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute(&taskrunner,*loader) )
	return false;

    uiPosSubSel* pss = inpfld->getPosSubSel();
    Pos::Filter* pf = pss ? pss->curProvider() : 0;
    mDynamicCastGet(Pos::Provider3D*,p3d,pf);
    if ( p3d )
	surface->apply( *pf );

    EM::SurfaceIOData outsd;
    outsd.use( *surface );
    EM::SurfaceIODataSelection outsdsel( outsd );
    outsdsel.setDefault();

    mDynamicCastGet(Pos::RangeProvider3D*,rp3d,pf);
    if ( rp3d )
	outsdsel.rg = rp3d->sampling().hsamp_;

    IOObj* newioobj = outfld->ctxtIOObj().ioobj_;
    const MultiID& mid = newioobj->key();
    emobj->setMultiID( mid );
    PtrMan<Executor> saver = surface->geometry().saver( &outsdsel, &mid );
    if ( !saver ) mErrRet(uiStrings::phrCannotSave(uiStrings::sSurface()))

    if ( !TaskRunner::execute(&taskrunner,*saver) )
	return false;

    const BufferString oldsetupname = EM::Surface::getSetupFileName( *ioobj );
    const BufferString newsetupname = EM::Surface::getSetupFileName( *newioobj);
    if ( File::exists(oldsetupname) )
	File::copy( oldsetupname, newsetupname );

    return true;
}
