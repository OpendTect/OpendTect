/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/


#include "uiwelllogattrib.h"
#include "welllogattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "ioobj.h"
#include "ptrman.h"
#include "welldata.h"
#include "welllogset.h"
#include "wellmanager.h"

#include "uiattribfactory.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiwellsel.h"
#include "od_helpids.h"


using namespace Attrib;


mInitAttribUINoSynth( uiWellLogAttrib, WellLog,
		      uiStrings::sWellLog(), sBasicGrp() );


uiWellLogAttrib::uiWellLogAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,mODHelpKey(mWellLogAttribHelpID))

{
    wellfld_ = new uiWellSel( this, true );
    wellfld_->selectionDone.notify( mCB(this,uiWellLogAttrib,selDone) );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select Log") );
    logsfld_ = new uiListBox( this, su );
    logsfld_->setStretch( 1, 1 );
    logsfld_->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedBelow, wellfld_ );

    sampfld_ = new uiGenInput( this, tr("Log resampling method"),
			       StringListInpSpec(Stats::UpscaleTypeDef()) );
    sampfld_->setValue( Stats::UseAvg );
    sampfld_->attach( alignedBelow, logsfld_ );

    setHAlignObj( wellfld_ );
}


void uiWellLogAttrib::selDone( CallBacker* )
{
    logsfld_->setEmpty();
    const DBKey wellid = wellfld_->key( true );
    if ( wellid.isInvalid() )
	return;

    BufferStringSet lognms;
    Well::MGR().getLogNames( wellid, lognms );
    logsfld_->addItems( lognms );
}


bool uiWellLogAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != WellLog::attribName() )
	return false;

    const ValParam* par = desc.getValParam( WellLog::keyStr() );
    if ( par )
	wellfld_->setInput( DBKey(par->getStringValue(0)) );

    selDone( 0 );
    par = desc.getValParam( WellLog::logName() );
    if ( par )
	logsfld_->setCurrentItem( par->getStringValue(0) );

    mIfGetEnum( WellLog::upscaleType(), tp, sampfld_->setValue(tp) );

    return true;
}


bool uiWellLogAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != WellLog::attribName() )
	return false;

    mSetString( WellLog::keyStr(), wellfld_->key().toString() );
    mSetString( WellLog::logName(), logsfld_->getText() );
    mSetEnum( WellLog::upscaleType(), sampfld_->getIntValue() );

    return true;
}
