/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2008
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

namespace Coords
{

// uiCoordSystem
mImplFactory1Param( uiCoordSystem, uiParent*, uiCoordSystem::factory );


uiCoordSystem::uiCoordSystem( uiParent* p, const uiString& caption )
    : uiDlgGroup( p, caption )
    , si_(nullptr)
{}


// uiCoordSystemSelGrp
uiCoordSystemSelGrp::uiCoordSystemSelGrp( uiParent* p,
					      bool onlyorthogonal,
					      bool projectiononly,
					      const SurveyInfo* si,
					      const CoordSystem* fillfrom )
    : uiDlgGroup( p, tr("Coordinate System properties") )
    , si_(si ? si : &SI())
{
    uiStringSet names;
    CoordSystem::getSystemNames( onlyorthogonal, projectiononly,
				    names, coordsystempars_ );

    coordsystemsuis_.allowNull();

    for ( int idx=0; idx<coordsystempars_.size(); idx++ )
    {
	BufferString key;
	if ( !coordsystempars_[idx]->get( CoordSystem::sKeyFactoryName(),
					  key ) )
	{
	    coordsystempars_.removeSingle( idx );
	    names.removeSingle( idx );
	    idx--;
	    continue;
	}

	uiCoordSystem* systemui =
		uiCoordSystem::factory().create( key, this );

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
	coordsystemsel_ = new uiGenInput( this, tr("Coordinate System"),
				      StringListInpSpec(names) );
	coordsystemsel_->attach( leftBorder );
	mAttachCB( coordsystemsel_->valuechanged,
	       uiCoordSystemSelGrp::systemChangedCB);
    }

    if ( coordsystemsel_ )
    {
	const BufferString selname = fillfrom ? fillfrom->factoryKeyword() : "";
	for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
	{
	    if ( !coordsystemsuis_[idx] )
		continue;

	    coordsystemsuis_[idx]->attach( alignedBelow, coordsystemsel_ );
	    if ( selname==coordsystemsuis_[idx]->factoryKeyword() )
	    {
		coordsystemsel_->setValue(idx);
	    }
	}
    }

    systemChangedCB( 0 );
}


uiCoordSystemSelGrp::~uiCoordSystemSelGrp()
{

}

void uiCoordSystemSelGrp::systemChangedCB(CallBacker *)
{
    const int selidx = coordsystemsel_ ? coordsystemsel_->getIntValue() : 0;

    for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
    {
	if ( coordsystemsuis_[idx] )
	    coordsystemsuis_[idx]->display(idx==selidx);
    }
}


bool uiCoordSystemSelGrp::acceptOK()
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
	outputsystem_ = CoordSystem::factory().create( key );
	if ( !outputsystem_->usePar(*coordsystempars_[selidx]) )
	{
	    outputsystem_ = 0;
	}
    }

    return outputsystem_;
}


void uiCoordSystemSelGrp::fillFromSI()
{
    ConstRefMan<CoordSystem> crs = si_ ? si_->getCoordSystem() : nullptr;
    if ( crs )
	fillFrom( *crs );
}


void uiCoordSystemSelGrp::fillFrom( const Coords::CoordSystem& crs )
{
    crs.ref();
    const BufferString systemnm = crs.factoryKeyword();
    for ( int idx=0; idx<coordsystemsuis_.size(); idx++ )
    {
	uiCoordSystem* uics = coordsystemsuis_[idx];
	if ( !uics ) continue;

	const bool hit = systemnm == uics->factoryKeyword();
	uics->display( hit );
	if ( hit )
	{
	    coordsystemsel_->setValue( idx );
	    uics->initFields( &crs );
	}
    }

    crs.unRef();
}


// uiCoordSystemDlg
uiCoordSystemDlg::uiCoordSystemDlg( uiParent* p, bool orthogonalonly,
			bool projectiononly, const SurveyInfo* si,
			const CoordSystem* coordsys )
    : uiDialog(p,uiDialog::Setup(tr("Coordinate Reference System"),mNoDlgTitle,
				 mODHelpKey(mLatLong2CoordDlgHelpID) ))
{
    coordsysselfld_ = new Coords::uiCoordSystemSelGrp( this, orthogonalonly,
						projectiononly, si, coordsys );
}


uiCoordSystemDlg::~uiCoordSystemDlg()
{
}


RefMan<CoordSystem> uiCoordSystemDlg::getCoordSystem()
{
    if ( !coordsysselfld_->outputSystem() )
	coordsysselfld_->acceptOK();

    return coordsysselfld_->outputSystem();
}


bool uiCoordSystemDlg::acceptOK( CallBacker* )
{
    return coordsysselfld_->acceptOK();
}


bool uiCoordSystemDlg::ensureGeographicTransformOK( uiParent* p,
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

    uiCoordSystemDlg dlg( p, true, false, si, si->getCoordSystem() );
    if ( !dlg.go() || !dlg.getCoordSystem()
	    || !dlg.getCoordSystem()->geographicTransformOK() )
	return false;

    si->setCoordSystem( dlg.getCoordSystem() );
    return true;
}


// uiCoordSystemSel
uiCoordSystemSel::uiCoordSystemSel( uiParent* p,
			bool orthogonalonly, bool projectiononly,
			const CoordSystem* coordsys, const uiString& seltxt )
    : uiCompoundParSel(p,seltxt)
    , orthogonalonly_(orthogonalonly)
    , projectiononly_(projectiononly)
    , changed(this)
{
    if ( coordsys )
	coordsystem_ = coordsys->clone();

    txtfld_->setElemSzPol( uiObject::WideMax );
    mAttachCB( butPush, uiCoordSystemSel::selCB );
}


uiCoordSystemSel::~uiCoordSystemSel()
{
    detachAllNotifiers();
}


void uiCoordSystemSel::selCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiCoordSystemDlg( this, orthogonalonly_, projectiononly_,
					&SI(), coordsystem_ );

    if ( dlg_->go() )
    {
	coordsystem_ = dlg_->getCoordSystem();
	updateSummary();
	changed.trigger();
    }
}


void uiCoordSystemSel::setCoordSystem( const CoordSystem* coordsystem )
{
    coordsystem_ = coordsystem ? coordsystem->clone() : nullptr;
    updateSummary();
    changed.trigger();
}


void uiCoordSystemSel::doSel()
{
    selCB( nullptr );
}


BufferString uiCoordSystemSel::getSummary() const
{
    if ( !coordsystem_ )
	return BufferString::empty();

    return coordsystem_->summary();
}

} // namespace Coords
