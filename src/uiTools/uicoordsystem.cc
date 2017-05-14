/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uicoordsystem.h"

#include "survinfo.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uitoolbutton.h"
#include "uilineedit.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "od_helpids.h"

using namespace Coords;

mImplFactory1Param( uiPositionSystem, uiParent*,
		    uiPositionSystem::factory );


uiPositionSystem::uiPositionSystem( uiParent* p, const uiString& caption )
    : uiDlgGroup( p, caption )
    , si_( 0 )
{}


uiPositionSystemSelGrp::uiPositionSystemSelGrp( uiParent* p,
					      bool onlyorthogonal,
					      bool projectiononly,
					      const SurveyInfo* si,
					      const PositionSystem* fillfrom )
    : uiDlgGroup( p, tr("Coordinate system properties") )
    , si_(si ? si : &SI())
{
    uiStringSet names;
    PositionSystem::getSystemNames( onlyorthogonal, projectiononly,
	    			    names, coordsystempars_ );

    coordsystemsuis_.allowNull();

    for ( int idx=0; idx<coordsystempars_.size(); idx++ )
    {
	BufferString key;
	if ( !coordsystempars_[idx]->get( PositionSystem::sKeyFactoryName(),
					  key ) )
	{
	    coordsystempars_.removeSingle( idx );
	    names.removeSingle( idx );
	    idx--;
	    continue;
	}

	uiPositionSystem* systemui =
		uiPositionSystem::factory().create( key, this );

	coordsystemsuis_ += systemui;

	if ( !systemui )
	    continue;

	systemui->setSurveyInfo( si );

	if ( fillfrom && key==systemui->factoryKeyword() )
	    systemui->initFields( fillfrom );

	systemui->display( false );
    }

    if ( names.size() > 1 )
    {
	coordsystemsel_ = new uiGenInput( this, tr("Coordinate system"),
				      StringListInpSpec(names) );
	coordsystemsel_->attach( leftBorder );
	mAttachCB( coordsystemsel_->valuechanged,
	       uiPositionSystemSelGrp::systemChangedCB);
    }
    else
    {
	coordsystemsel_ = 0;
    }

    if ( coordsystemsel_ )
    {
	const BufferString selname = fillfrom ? fillfrom->factoryKeyword() : "";
	for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
	{
	    if ( coordsystemsuis_[idx] )
		coordsystemsuis_[idx]->attach( alignedBelow, coordsystemsel_ );

	    if ( selname==coordsystemsuis_[idx]->factoryKeyword() )
	    {
		coordsystemsel_->setValue(idx);
	    }
	}
    }

    systemChangedCB( 0 );
}


uiPositionSystemSelGrp::~uiPositionSystemSelGrp()
{

}

void uiPositionSystemSelGrp::systemChangedCB(CallBacker *)
{
    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
    {
	if ( coordsystemsuis_[idx] )
	    coordsystemsuis_[idx]->display(idx==selidx);
    }
}


bool uiPositionSystemSelGrp::acceptOK()
{
    outputsystem_ = 0;

    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    if ( coordsystemsuis_[selidx] )
    {
	if ( !coordsystemsuis_[selidx]->acceptOK() )
	    return false;

	outputsystem_ = coordsystemsuis_[selidx]->outputSystem();
    }
    else
    {
	BufferString key;
	coordsystempars_[selidx]->get( sKey::Name(), key );
	outputsystem_ = PositionSystem::factory().create( key );
	if ( !outputsystem_->usePar(*coordsystempars_[selidx]) )
	{
	    outputsystem_ = 0;
	}
    }

    return outputsystem_;
}


uiPositionSystemDlg::uiPositionSystemDlg( uiParent* p, bool orthogonalonly,
			bool projectiononly, const PositionSystem* coordsys )
    : uiDialog(p,uiDialog::Setup(tr("Coordinate Reference System"),mNoDlgTitle,
				 mODHelpKey(mLatLong2CoordDlgHelpID) ))
{
    coordsysselfld_ = new Coords::uiPositionSystemSelGrp( this, orthogonalonly,
	    					projectiononly, 0, coordsys );
}


uiPositionSystemDlg::~uiPositionSystemDlg()
{
}


RefMan<PositionSystem> uiPositionSystemDlg::getCoordSystem()
{
    if ( !coordsysselfld_->outputSystem() )
	coordsysselfld_->acceptOK();

    return coordsysselfld_->outputSystem();
}


bool uiPositionSystemDlg::acceptOK( CallBacker* )
{
    if ( !getCoordSystem() )
	return false;

    return true;
}


bool uiPositionSystemDlg::ensureGeographicTransformOK( uiParent* p,
						       SurveyInfo* si )
{
    if ( !si ) si = const_cast<SurveyInfo*>( &SI() );
    if ( si->getCoordSystem() && si->getCoordSystem()->geographicTransformOK() )
	return true;

    uiString msg( tr("The survey '%1' does not have a Coordinate System defined"
	       " that can convert the X/Y coordinates to Geographical Lat/Long."
	       " Do you want to define it now?").arg(si->name()) );
    if ( !uiMSG().askGoOn(msg,tr("Define Coordinate System"),
				uiStrings::sCancel()) )
	return false;

    uiPositionSystemDlg dlg( p, true, false, si->getCoordSystem() );
    if ( !dlg.go() || !dlg.getCoordSystem()
	    || !dlg.getCoordSystem()->geographicTransformOK() )
	return false;

    si->setCoordSystem( dlg.getCoordSystem() );
    return true;
}


uiPositionSystemSel::uiPositionSystemSel( uiParent* p, const uiString& seltxt,
				bool orthogonalonly, bool projectiononly,
				const PositionSystem* coordsys )
    : uiCompoundParSel(p,seltxt)
    , orthogonalonly_(orthogonalonly), projectiononly_(projectiononly)
    , dlg_(0)
{
    if ( coordsys )
	coordsystem_ = coordsys->clone();

    txtfld_->setElemSzPol( uiObject::Wide );
    butPush.notify( mCB(this,uiPositionSystemSel,selCB) );
}


void uiPositionSystemSel::selCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiPositionSystemDlg( this, orthogonalonly_, projectiononly_,
					coordsystem_ );

    if ( dlg_->go() )
    {
	coordsystem_ = dlg_->getCoordSystem();
	updateSummary();
    }
}


BufferString uiPositionSystemSel::getSummary() const
{
    if ( !coordsystem_ )
	return BufferString::empty();

    return coordsystem_->summary();
}

