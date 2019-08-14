/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
________________________________________________________________________

-*/
#include "changepolygonz.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "survinfo.h"
#include "uichangepolygonz.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiChangePolygonZ::uiChangePolygonZ( uiParent* p, Pick::Set& ps )
	: uiDialog(p,uiDialog::Setup(tr("Change Polygon Z Values"),
		   tr("Changing Z values of polygon '%1'").arg(ps.getName()),
		   mNoHelpKey))
	, set_(ps)
    {
	changezfld_ = new uiGenInput( this, tr("Get Z values from"),
		      BoolInpSpec(true,tr("Constant Z"), tr("Horizon")) );
	changezfld_->valuechanged.notify(
		     mCB(this,uiChangePolygonZ,changeZvalCB) );

	uiString constzlbl = tr("Constant Z value").withSurvZUnit();
	constzfld_ = new uiGenInput( this,constzlbl, FloatInpSpec(0) );
	constzfld_->attach( alignedBelow, changezfld_ );

	IOObjContext surfctxt( EMHorizon3DTranslatorGroup::ioContext() );
	surfctxt.forread_ = true;

	horinpfld_ = new uiIOObjSel( this, surfctxt, tr("Select Horizon") );
	horinpfld_->attach( alignedBelow, changezfld_ );
	horinpfld_->display( false );
	horinpfld_->inpBox()->setCurrentItem( 0 );
    }


uiChangePolygonZ::~uiChangePolygonZ()
{
}


bool uiChangePolygonZ::acceptOK()
{
    ChangePolygonZ* newz = nullptr;
    const bool iszconstant = changezfld_->getBoolValue();
    if ( !iszconstant )
    {
	DBKey horid = horinpfld_->key();
	newz = new ChangePolygonZ( set_, horid );
    }
    else
    {
	double zconst = constzfld_->getFValue();
	const bool zistime = SI().zIsTime();
	if ( zistime )
	    zconst /= 1000;
	newz = new ChangePolygonZ( set_, zconst );
    }

    uiTaskRunnerProvider trp( this );
    const bool res = newz->doShift( trp );

    if ( !res )
	uiMSG().message( tr("Failed to shift Polygon") );

    return res;
}


void uiChangePolygonZ::changeZvalCB( CallBacker* )
{
    const bool iszconstant = changezfld_->getBoolValue();
    horinpfld_->display( !iszconstant );
    constzfld_->display( iszconstant );
}
