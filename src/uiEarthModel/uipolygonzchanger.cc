/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipolygonzchanger.h"
#include "polygonzchanger.h"

#include "emhorizon3d.h"
#include "survinfo.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"


uiPolygonZChanger::uiPolygonZChanger( uiParent* p, Pick::Set& ps )
    : uiDialog(p,Setup(tr("Change Z value of polygon"),
		       toUiString("'%1'").arg(ps.name()),mNoHelpKey))
    , set_(ps)
{
    isconstzfld_ = new uiGenInput( this, uiStrings::sUse(),
		    BoolInpSpec(true,tr("Constant Z"),tr("Horizon")) );
    isconstzfld_->
	       valueChanged.notify( mCB(this,uiPolygonZChanger,changeZvalCB) );

    uiString constzlbl =
		    tr("Z value").addSpace().append( SI().getUiZUnitString() );
    zvalfld_ = new uiGenInput( this, constzlbl,
			       FloatInpSpec(SI().zRange(true).start_*SI()
						.zDomain().userFactor()) );
    zvalfld_->attach( alignedBelow, isconstzfld_ );

    horinpfld_ = new uiHorizon3DSel( this, true,
				uiStrings::phrSelect(uiStrings::sHorizon()) );
    horinpfld_->attach( alignedBelow, isconstzfld_ );
    horinpfld_->display( false );
    horinpfld_->inpBox()->setCurrentItem( 0 );
}


uiPolygonZChanger::~uiPolygonZChanger()
{}


bool uiPolygonZChanger::acceptOK( CallBacker* )
{
    EM::PolygonZChanger* zchanger = nullptr;
    const bool zisconstant = isconstzfld_->getBoolValue();

    if ( !zisconstant )
    {
	const MultiID horid = horinpfld_->key();
	if ( horid.isUdf() )
	    return false;

	zchanger = new EM::PolygonZChanger( set_, horid );
    }
    else
    {
	float zconst = zvalfld_->getFValue();
	if ( SI().zIsTime() )
	    zconst /= SI().zDomain().userFactor();

	zchanger = new EM::PolygonZChanger( set_, zconst );
    }

    return applyZChanges( *zchanger );
}


bool uiPolygonZChanger::applyZChanges( EM::PolygonZChanger& zchanger )
{
    uiTaskRunner trp( this, true );
    uiRetVal uirv = zchanger.doWork( trp );
    if ( !uirv.isOK() )
    {
	uiMSG().error( uirv );
	return false;
    }

    return true;
}


void uiPolygonZChanger::changeZvalCB( CallBacker* )
{
    const bool zisconstant = isconstzfld_->getBoolValue();
    horinpfld_->display( !zisconstant );
    zvalfld_->display( zisconstant );
}


uiEditPolygonDlg::uiEditPolygonDlg( uiParent* p, Pick::Set& ps )
    : uiEditPicksDlg( p, ps )
{
    if ( !ps.isPolygon() )
    {
	pErrMsg( "This dialog is for editing a polygon." );
	return;
    }

    uiButtonGroup* butgrp = polygongrp_->tblButGrp();
    if (!butgrp)
    {
	pErrMsg( "A valid button group should be created. Please debug!");
	return;
    }

    auto* chgzbut = new uiToolButton( butgrp, "alonghor",
				      tr("Bulk change z-values"),
				      mCB(this,uiEditPolygonDlg,chgZCB) );
    butgrp->addButton( chgzbut );

}


uiEditPolygonDlg::~uiEditPolygonDlg()
{}


void uiEditPolygonDlg::initClass()
{
    uiEditPolygonDlg::factory().addCreator( create, sKey::Polygon() );
}


void uiEditPolygonDlg::chgZCB( CallBacker* )
{
    uiPolygonZChanger dlg( this, ps_ );
    dlg.go();
}
