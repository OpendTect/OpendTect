/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    : uiDialog(p,uiDialog::Setup(tr("Change Z value of polygon"),
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
	  FloatInpSpec(SI().zRange(true).start*SI().zDomain().userFactor()) );
    zvalfld_->attach( alignedBelow, isconstzfld_ );

    IOObjContext surfctxt( EMHorizon3DTranslatorGroup::ioContext() );
    surfctxt.forread_ = true;
    horinpfld_ = new uiIOObjSel( this, surfctxt,
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
    { uiMSG().error( uirv ); return false; }

    return true;
}


void uiPolygonZChanger::changeZvalCB( CallBacker* )
{
    const bool zisconstant = isconstzfld_->getBoolValue();
    horinpfld_->display( !zisconstant );
    zvalfld_->display( zisconstant );
}
