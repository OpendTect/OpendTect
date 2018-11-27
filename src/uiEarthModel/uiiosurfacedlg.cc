/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "ctxtioobj.h"
#include "dirlist.h"
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
#include "filepath.h"
#include "ioobj.h"
#include "rangeposprovider.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"
#include "od_helpids.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Surface& surf,
				      float shift )
    : uiDialog(p,uiDialog::Setup( uiStrings::sOutputSelection(),mNoDlgTitle,
                                 mODHelpKey(mWriteSurfaceDlgHelpID) ))
    , surface_(surf)
{
    mDynamicCastGet(const EM::Horizon3D*,hor,&surface_);
    const bool hasshift = hor && !mIsZero(shift,SI().zStep(OD::UsrWork)*1e-3f);
    mDynamicCastGet(const EM::PolygonBody*,plgbody,&surface_);
    const bool usesubsel = !plgbody;

    iogrp_ = new uiSurfaceWrite( this, surface_,
			     uiSurfaceWrite::Setup(surface_.getTypeStr(),
						   surface_.getUserTypeStr())
			     .withdisplayfld(!hasshift).withsubsel(usesubsel) );
}


bool uiWriteSurfaceDlg::acceptOK()
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


bool uiReadSurfaceDlg::acceptOK()
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


uiStoreAuxData::uiStoreAuxData( uiParent* p, const EM::Horizon3D& surf,
				AuxID auxid )
    : uiDialog(p,uiDialog::Setup( uiStrings::sOutputSelection(),
				 tr("Specify attribute name"),
				 mODHelpKey(mStoreAuxDataHelpID) ))
    , surface_(surf)
    , auxid_(auxid)
{
    const Interval<float> valrg = surface_.auxdata.valRange( auxid_ );
    const BufferString attrnm( surface_.auxdata.auxDataName(auxid) );
    setTitleText( tr("Save '%1' (values %2 - %3)")
		    .arg( attrnm ).arg( valrg.start ).arg( valrg.stop ) );

    attrnmfld_ = new uiGenInput( this, uiStrings::sAttribute() );
    attrnmfld_->setText( attrnm );

    uiUnitSel::Setup ussu( PropertyRef::Other, uiStrings::sUnit() );
    ussu.selproptype( true ).mode( uiUnitSel::Setup::Full )
	.nodefsave( true ).withnone( true );
    unitfld_ = new uiUnitSel( this, ussu );
    const UnitOfMeasure* uom = surface_.auxdata.unit( auxid_ );
    unitfld_->setUnit( uom );
    unitfld_->attach( alignedBelow, attrnmfld_ );
}


const char* uiStoreAuxData::auxdataName() const
{
    return attrnmfld_->text();
}


const UnitOfMeasure* uiStoreAuxData::unit() const
{
    return unitfld_->getUnit();
}


bool uiStoreAuxData::acceptOK()
{
    dooverwrite_ = false;
    BufferString attrnm = auxdataName();
    const bool ispres = checkIfAlreadyPresent( attrnm.buf() );
    if ( ispres )
    {
	uiString msg = tr("This surface already has an attribute called:\n%1"
	                  "\nDo you wish to overwrite this data?").arg(attrnm);
	if ( !uiMSG().askOverwrite(msg) )
	    return false;
	dooverwrite_ = true;
    }

    EM::Horizon3D& hor = const_cast<EM::Horizon3D&>( surface_ );
    hor.auxdata.setAuxDataName( auxid_, attrnm );
    hor.auxdata.setUnit( auxid_, unit() );
    return true;
}


bool uiStoreAuxData::checkIfAlreadyPresent( const char* attrnm )
{
    EM::IOObjInfo eminfo( surface_.dbKey() );
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

#define mGetWinNm(ioobj) \
    mGet( ioobj,\
	uiStrings::phrCopy(toUiString("%1 %2").arg(uiStrings::s2D())  \
					    .arg(uiStrings::sHorizon(1))), \
	uiStrings::phrCopy(toUiString("%1 %2").arg(uiStrings::s3D())  \
					    .arg(uiStrings::sHorizon(1))), \
	uiStrings::phrCopy(uiStrings::sFaultStickSet()), \
	uiStrings::phrCopy(uiStrings::sFault())) \


uiCopySurface::uiCopySurface( uiParent* p, const IOObj& ioobj,
			      const uiSurfaceRead::Setup& su )
    : uiDialog(p,Setup(mGetWinNm(ioobj),mNoDlgTitle,mGetHelpID(ioobj)))
    , ctio_(*mkCtxtIOObj(ioobj))
{
    inpfld = new uiSurfaceRead( this, su );
    inpfld->setIOObj( ioobj.key() );

    ctio_.ctxt_.forread_ = false;
    ctio_.setObj( 0 );

    if ( ioobj.group() == EMFault3DTranslatorGroup::sGroupName() )
	outfld = new uiIOObjSel( this, ctio_,
				 uiStrings::phrOutput(uiStrings::sFault()) );
    else if ( ioobj.group() != EM::FaultStickSet::typeStr() )
	outfld = new uiIOObjSel( this, ctio_,
				 uiStrings::phrOutput(uiStrings::sSurface()) );
    else
	outfld = new uiIOObjSel( this, ctio_,
			    uiStrings::phrOutput(uiStrings::sFaultStickSet()) );

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

bool uiCopySurface::acceptOK()
{
    if ( !inpfld->processInput() ) return false;
    if ( !outfld->commitInput() )
	mErrRet( (outfld->isEmpty() ? uiStrings::phrSelect(uiStrings::phrOutput
		 (uiStrings::sSurface())) : uiString::empty()) )

    const IOObj* ioobj = inpfld->selIOObj();
    if ( !ioobj ) mErrRet(uiStrings::phrCannotFind(uiStrings::sSurface()))

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    inpfld->getSelection( sdsel );

    RefMan<EM::Object> emobj = EM::MGR().createTempObject( ioobj->group() );
    if ( !emobj )
	mErrRet(uiStrings::phrCannotCreate(uiStrings::sObject()))
    emobj->setDBKey( ioobj->key() );

    mDynamicCastGet(EM::Surface*,surface,emobj.ptr())
    PtrMan<Executor> loader = surface->geometry().loader( &sdsel );
    if ( !loader ) mErrRet(uiStrings::phrCannotRead(uiStrings::sSurface()))

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *loader ) ) return false;

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
    const DBKey mid = newioobj->key();
    emobj->setDBKey( mid );
    PtrMan<Executor> saver = surface->geometry().saver( &outsdsel, &mid );
    if ( !saver ) mErrRet(uiStrings::phrCannotSave(uiStrings::sSurface()))

    if ( !TaskRunner::execute( &taskrunner, *saver ) ) return false;

    const BufferString oldsetupname = EM::Surface::getSetupFileName( *ioobj );
    const BufferString newsetupname = EM::Surface::getSetupFileName( *newioobj);
    if ( File::exists(oldsetupname) )
	File::copy( oldsetupname, newsetupname );

    return true;
}



uiCopyFaultSet::uiCopyFaultSet( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,Setup(uiStrings::phrCopy(uiStrings::sFaultSet()),mNoDlgTitle,
		       mTODOHelpKey))
{
    IOObjContext ctxt = mIOObjContext(EMFaultSet3D);
    inpfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrInput(uiStrings::sFaultSet()) );
    inpfld_->setInput( ioobj );
    inpfld_->selectionDone.notify( mCB(this,uiCopyFaultSet,inpSelCB) );

    surflist_ = new uiListBox( this, uiListBox::Setup(OD::ChooseAtLeastOne,
			    uiStrings::phrSelect(uiStrings::sFault(mPlural))) );
    surflist_->attach( alignedBelow, inpfld_ );

    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrOutput(uiStrings::sFaultSet()) );
    outfld_->attach( alignedBelow, surflist_ );

    postFinalise().notify( mCB(this,uiCopyFaultSet,inpSelCB) );
}


void uiCopyFaultSet::inpSelCB( CallBacker* )
{
    const IOObj* selobj = inpfld_->ioobj( true );
    if ( !selobj )
	return;

    surflist_->setEmpty();
    DirList dl( selobj->fullUserExpr(), File::FilesInDir, "*.flt" );
    if ( dl.isEmpty() )
	return;

    BufferStringSet idstrs;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const File::Path fp = dl.fullPath( idx );
	idstrs.add( fp.baseName() );
    }

    surflist_->addItems( idstrs );
}


bool uiCopyFaultSet::acceptOK()
{
    const IOObj* inpobj = inpfld_->ioobj();
    if ( !inpobj )
	return false;

    BufferStringSet selflts;
    surflist_->getChosen( selflts );
    if ( selflts.isEmpty() )
	return false;

    const IOObj* outobj = outfld_->ioobj();
    if ( !outobj )
	return false;

    const BufferString inpdirnm = inpobj->fullUserExpr();
    const BufferString outdirnm = outobj->fullUserExpr();
    if ( !File::isDirectory(outdirnm) )
	File::createDir( outdirnm );

    uiString errmsg;
    for ( int idx=0; idx<selflts.size(); idx++ )
    {
	File::Path inpfp( inpdirnm, selflts.get(idx) );
	inpfp.setExtension( ".flt" );
	File::Path outfp( outdirnm, toString(idx+1) );
	outfp.setExtension( ".flt" );
	if ( !File::copy(inpfp.fullPath(),outfp.fullPath(),&errmsg) )
	{
	    uiMSG().error( toUiString("%1: %2")
		    .arg(uiStrings::phrCannotCopy(uiStrings::sFault()))
		    .arg(errmsg) );
	    return false;
	}
    }

    return true;
}
