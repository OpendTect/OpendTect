/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidpsselectednessdlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicolortable.h"
#include "uidatapointsetcrossplot.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"

#include "datacoldef.h"
#include "dpsdispmgr.h"
#include "posvecdataset.h"
#include "mathexpression.h"
#include "od_helpids.h"

uiDPSSelectednessDlg::uiDPSSelectednessDlg( uiParent* p,
					  uiDataPointSetCrossPlotter& plotter )
    : uiDPSAddColumnDlg(p,false)
    , plotter_(plotter)
    , selaxisfld_(0)
    , coltabfld_(0)
{
    const SelectionGrp* curgrp = plotter_.selectionGrps()[plotter_.curSelGrp()];
    uiString capt = tr("Calculating selectedness of '%1'")
					      .arg(toUiString(curgrp->name()));
    setCaption( capt );
    setHelpKey( mODHelpKey(mDPSSelectednessDlgHelpID) );

    uiString msg = tr("A new column will be added with selectedness values.\n"
		      "Specify the name of the new column.");
    uiLabel* msglbl = new uiLabel( this, msg );
    msglbl->attach( alignedAbove, nmfld_ );
 
    if ( plotter_.isY2Shown() )
    {
	selaxisfld_ =
	    new uiGenInput( this, tr("Calculate selectedness on"),
		    	    BoolInpSpec(true,tr("Y Axis"),tr("Y2 Axis")) );
	selaxisfld_->attach( alignedBelow, nmfld_ );
    }

    showoverlayfld_ =
	new uiCheckBox( this, tr("Show selectedness as overlay attribute"),
			mCB(this,uiDPSSelectednessDlg,showOverlayClicked) );
    showoverlayfld_->attach( alignedBelow, plotter_.isY2Shown() ? selaxisfld_
	    							: nmfld_ );


    showin3dscenefld_ = 
	new uiCheckBox( this, tr("Show selectedness in 3D scene"),
		        mCB(this,uiDPSSelectednessDlg,show3DSceneClicked) );

    showin3dscenefld_->attach( alignedBelow, showoverlayfld_ );

    coltabfld_ = new uiColorTableGroup( this, ColTab::Sequence("Rainbow") );
    coltabfld_->attach( alignedBelow, showin3dscenefld_ );
    coltabfld_->display( false );
}


void uiDPSSelectednessDlg::showOverlayClicked( CallBacker* )
{
    coltabfld_->display( showoverlayfld_->isChecked() ||
	   		 showin3dscenefld_->isChecked() );
}


void uiDPSSelectednessDlg::show3DSceneClicked( CallBacker* )
{
    coltabfld_->display( showoverlayfld_->isChecked() ||
	    		 showin3dscenefld_->isChecked() );
}


void uiDPSSelectednessDlg::addColumn()
{
    DataPointSet& dps = plotter_.dps();
    dps.dataSet().add( new DataColDef(nmfld_->text()) );
    for ( uiDataPointSet::DRowID rid=0; rid<dps.size(); rid++ )
    {
	const bool isy1 = selaxisfld_ && selaxisfld_->attachObj()->isDisplayed()
	    			? selaxisfld_->getBoolValue() : true;
	const float val = plotter_.getSelectedness( rid, !isy1 );
	BinIDValueSet& bvs = dps.bivSet();
	BinIDValueSet::SPos pos = dps.bvsPos( rid );
	BinID curbid;
	TypeSet<float> vals;
	bvs.get( pos, curbid, vals );
	vals[ vals.size()-1 ] = mIsUdf(val) ? mUdf(float) : val;
	bvs.set( pos, vals );
    }

    dps.dataChanged();
    plotter_.uidps().reDoTable();
}


void uiDPSSelectednessDlg::showOverlayAttrib()
{
    if ( !showoverlayfld_->isChecked() ) return;

    const DataPointSet& dps = plotter_.dps();
    const int dpscolid = dps.indexOf( nmfld_->text() );
    const int bvscolid = dps.bivSetIdx( dpscolid );
    ColTab::MapperSetup mapsu;
    mapsu.range_ = dps.bivSet().valRange( bvscolid );
    
    if ( selaxisfld_ && !selaxisfld_->getBoolValue() )
    {
	plotter_.setOverlayY2Cols( dpscolid );
	plotter_.setOverlayY2AttSeq( coltabfld_->colTabSeq() );
	plotter_.updateOverlayMapper( false );
	plotter_.setShowY4( true );
    }
    else
    {
	plotter_.setOverlayY1Cols( dpscolid );
	plotter_.setOverlayY1AttSeq( coltabfld_->colTabSeq() );
	plotter_.updateOverlayMapper( true );
	plotter_.setShowY3( true );
    }

    plotter_.drawContent();
    plotter_.reDrawSelections();
}


void uiDPSSelectednessDlg::showIn3DScene()
{
    if ( !showin3dscenefld_->isChecked() ) return;

    const DataPointSet& dps = plotter_.dps();
    const int dpscolid = dps.indexOf( nmfld_->text() );
    const int bvscolid = dps.bivSetIdx( dpscolid );
    ColTab::MapperSetup mapsu;
    mapsu.range_ = dps.bivSet().valRange( bvscolid );
    
    DataPointSetDisplayProp* dispprop =
	new DataPointSetDisplayProp( coltabfld_->colTabSeq(), mapsu, dpscolid );
    plotter_.uidps().setDisp( dispprop );
}


bool uiDPSSelectednessDlg::acceptOK( CallBacker* )
{
    DataPointSet& dps = plotter_.dps();
    BufferStringSet colnms;
    for ( int colidx=0; colidx<dps.nrCols(); colidx++ )
	colnms.add( dps.colName(colidx) );

    if ( colnms.isPresent(nmfld_->text()) )
    {
	uiMSG().error( tr( "Column '%1' already present, "
			   "choose a different name").arg( nmfld_->text()) );
	return false;
    }

    addColumn();
    showOverlayAttrib();
    showIn3DScene();
    return true;
}
