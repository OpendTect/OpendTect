/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrsurfout.h"

#include "array2dinterpolimpl.h"
#include "attribdesc.h"
#include "attriboutput.h"
#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "emsurfauxdataio.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

#include "uiarray2dinterpol.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uidlggroup.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uibatchjobdispatchersel.h"
#include "uistrings.h"
#include "od_helpids.h"

using namespace Attrib;

uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const DescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiAttrEMOut( p, ad, n, mid, "Calculate Horizon Data" )
    , interpol_(0)
{
    setHelpKey( mODHelpKey(mAttrSurfaceOutHelpID) );
    setCtrlStyle( RunAndClose );

    attrnmfld_ = new uiGenInput( pargrp_, uiStrings::sAttribName(),
				 StringInpSpec() );
    attrnmfld_->setElemSzPol( uiObject::Wide );
    attrnmfld_->attach( alignedBelow, attrfld_ );
    attrnmfld_->setDefaultTextValidator();

    filludffld_ = new uiGenInput( pargrp_, tr("Fill undefined parts"),
				  BoolInpSpec(false) );
    filludffld_->valueChanged.notify( mCB(this,uiAttrSurfaceOut,fillUdfSelCB) );
    filludffld_->attach( alignedBelow, attrnmfld_ );

    settingsbut_ = new uiPushButton( pargrp_, uiStrings::sSettings(),
				 mCB(this,uiAttrSurfaceOut,settingsCB), false);
    settingsbut_->display( false );
    settingsbut_->attach( rightOf, filludffld_ );

    objfld_ = new uiIOObjSel( pargrp_, mIOObjContext(EMHorizon3D),
			      uiStrings::phrCalculate(tr("on Horizon")) );
    objfld_->attach( alignedBelow, filludffld_ );
    objfld_->selectionDone.notify( mCB(this,uiAttrSurfaceOut,objSelCB) );
    pargrp_->setHAlignObj( objfld_ );

    batchjobfld_->jobSpec().pars_.set( IOPar::compKey(sKey::Output(),
				sKey::Type()), Output::surfkey() );
    batchjobfld_->jobSpecUpdated();
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
}


void uiAttrSurfaceOut::fillUdfSelCB( CallBacker* )
{
    const bool isdisplay = filludffld_->getBoolValue();
    settingsbut_->display( isdisplay );
    if ( settingsbut_->isDisplayed() )
    {
	InverseDistanceArray2DInterpol* tempinterpol =
					new InverseDistanceArray2DInterpol;
	const float defradius = 10*(SI().inlDistance()+SI().crlDistance());
	tempinterpol->setSearchRadius( defradius );
	tempinterpol->setFillType( Array2DInterpol::ConvexHull );
	tempinterpol->setStepSize( 1 );
	tempinterpol->setMaxHoleSize( mUdf(float) );
	interpol_ = tempinterpol;
    }
}


void uiAttrSurfaceOut::settingsCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this, uiDialog::Setup(tr("Interpolation"),
						tr("Interpolation Settings"),
						mNoHelpKey) );
    uiArray2DInterpolSel* interpolsel =
		new uiArray2DInterpolSel( &dlg, true, true, false, interpol_ );
    dlg.setGroup( interpolsel );
    if ( !dlg.go() )
	return;

    IOPar iop;
    interpolsel->fillPar( iop );
    iop.get( sKey::Name(), methodname_ );

    if ( interpol_ ) delete interpol_;

    interpol_ = interpolsel->getResult();
}


void uiAttrSurfaceOut::attribSel( CallBacker* )
{
    const StringPair sp( attrfld_->getInput() );
    if ( sp.second() == Attrib::Desc::sKeyAll() )
	attrnmfld_->setText( sp.first() );
    else
	attrnmfld_->setText( sp.getCompString(true) );
    objSelCB(0);
}


void uiAttrSurfaceOut::objSelCB( CallBacker* )
{
}


void uiAttrSurfaceOut::getJobName( BufferString& jobnm ) const
{
    const IOObj* ioobj = objfld_->ioobj( true );
    if ( ioobj )
	jobnm.add( ioobj->name() );

    const StringView attrnm = attrnmfld_->text();
    if ( !attrnm.isEmpty() )
	jobnm.add( " ").add( attrnm.buf() );
}


bool uiAttrSurfaceOut::prepareProcessing()
{
    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj ) return false;

    const StringView attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() )
    {
	uiMSG().error( tr("Please provide output attribute name") );
	return false;
    }

    return uiAttrEMOut::prepareProcessing();
}


bool uiAttrSurfaceOut::fillPar( IOPar& iopar )
{
    BufferString attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() )
	attrnm = attrfld_->getInput();

    iopar.set( sKey::Target(), attrnm ); // Must be called before fillPar

    if ( !uiAttrEMOut::fillPar(iopar) )
	return false;

    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj ) return false;

    if ( settingsbut_->isDisplayed() )
	fillGridPar( iopar );

    fillOutPar( iopar, Output::surfkey(),
		LocationOutput::surfidkey(), ioobj->key() );

    BufferStringSet outputnms;
    getDescNames( outputnms );

    uiStringSet errors;
    for ( int idx=0; idx<outputnms.size(); idx++ )
    {
	const BufferString& outputnm = outputnms.get( idx );
	BufferString attrfnm =
		EM::SurfaceAuxData::getFileName( *ioobj, outputnm.buf() );
	if ( !attrfnm.isEmpty() )
	    errors.add( tr("Horizon Data with name %1 already exists. "
			   "Overwrite?").arg(outputnm) );
	else
	{
	    attrfnm = EM::SurfaceAuxData::getFreeFileName( *ioobj );
	    const bool res =
		EM::dgbSurfDataWriter::writeDummyHeader( attrfnm, outputnm );
	    if ( !res )
		errors.add( tr("Cannot save Horizon data to: %1").arg(attrfnm));
	}
    }

    bool res = true;
    if ( !errors.isEmpty() )
    {
	uiString msg = errors.cat();
	res = uiMSG().askContinue( msg );
    }

    return res;
}


void uiAttrSurfaceOut::fillGridPar( IOPar& par ) const
{
    IOPar gridpar, iopar;
    if ( interpol_ )
    {
	gridpar.set( sKey::Name(), methodname_ );
	interpol_->fillPar( gridpar );
    }

    iopar.mergeComp( gridpar, "Grid" );
    par.merge( iopar );
}
