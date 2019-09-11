/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rahul Gogia
 Date:		July 2019
________________________________________________________________________

-*/
#include "polygonzchanger.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "survinfo.h"
#include "uipolygonzchanger.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiPolygonZChanger::uiPolygonZChanger( uiParent* p, Pick::Set& ps )
	: uiDialog(p,uiDialog::Setup(tr("Change Z value of polygon '%1'").
	    arg(ps.getName()),uiString::empty(),mNoHelpKey))
	, set_(ps)
    {
	isconstzfld_ = new uiGenInput( this, uiStrings::sUse(),
		      BoolInpSpec(true,tr("Constant Z"), tr("Horizon")) );
	isconstzfld_->valuechanged.notify(
		     mCB(this,uiPolygonZChanger,changeZvalCB) );

	uiString constzlbl = tr("Z value").withSurvZUnit();
	zvalfld_ = new uiGenInput( this,constzlbl,
		FloatInpSpec(SI().zRange().start*SI().zDomain().userFactor()) );
	zvalfld_->attach( alignedBelow, isconstzfld_ );

	IOObjContext surfctxt( EMHorizon3DTranslatorGroup::ioContext() );
	surfctxt.forread_ = true;

	horinpfld_ = new uiIOObjSel( this, surfctxt, tr("Select Horizon") );
	horinpfld_->attach( alignedBelow, isconstzfld_ );
	horinpfld_->display( false );
	horinpfld_->inpBox()->setCurrentItem( 0 );
    }


uiPolygonZChanger::~uiPolygonZChanger()
{}


bool uiPolygonZChanger::acceptOK()
{
    EM::PolygonZChanger* zchanger = nullptr;
    const bool zisconstant = isconstzfld_->getBoolValue();

    if ( !zisconstant )
    {
	const DBKey horid = horinpfld_->key();
	if ( !horid.isValid() )
	    return false;
	zchanger = new EM::PolygonZChanger( set_, horinpfld_->key() );
    }
    else
    {
	float zconst = zvalfld_->getFValue();
	if ( SI().zIsTime() )
	    zconst /= SI().zDomain().userFactor();
	zchanger = new EM::PolygonZChanger( set_, zconst );
    }

    return returnApplyZChanger( *zchanger );
}


bool uiPolygonZChanger::returnApplyZChanger( const EM::PolygonZChanger& zchanger )
{
    uiTaskRunnerProvider trp( this );

    auto uirv = zchanger.doWork(trp);
    if (!uirv.isOK())
    { uiMSG().error( uirv ); return false; }

    return true;
}


void uiPolygonZChanger::changeZvalCB( CallBacker* )
{
    const bool zisconstant = isconstzfld_->getBoolValue();
    horinpfld_->display( !zisconstant );
    zvalfld_->display( zisconstant );
}
