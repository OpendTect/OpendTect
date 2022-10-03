/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "wellman.h"

#include "uiattribfactory.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiwellsel.h"
#include "od_helpids.h"


using namespace Attrib;


mInitAttribUI(uiWellLogAttrib,WellLog,"Log",sKeyBasicGrp())


uiWellLogAttrib::uiWellLogAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,mODHelpKey(mWellLogAttribHelpID))
{
    wellfld_ = new uiWellSel( this, true );
    mAttachCB( wellfld_->selectionDone, uiWellLogAttrib::selDone );

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Select Log") );
    logsfld_ = new uiListBox( this, su );
    logsfld_->setStretch( 1, 1 );
    logsfld_->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedBelow, wellfld_ );

    sampfld_ = new uiGenInput( this, tr("Log resampling method"),
			       StringListInpSpec(Stats::UpscaleTypeNames()) );
    sampfld_->setValue( Stats::UseAvg );
    sampfld_->attach( alignedBelow, logsfld_ );

    setHAlignObj( wellfld_ );
}


uiWellLogAttrib::~uiWellLogAttrib()
{
    detachAllNotifiers();
}


void uiWellLogAttrib::selDone( CallBacker* )
{
    logsfld_->setEmpty();
    const MultiID wellid = wellfld_->key( true );
    if ( wellid.isUdf() )
	return;

    BufferStringSet lognms;
    Well::MGR().getLogNamesByID( wellid, lognms );
    logsfld_->addItems( lognms );
}


bool uiWellLogAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != WellLog::attribName() )
	return false;

    const ValParam* par = desc.getValParam( WellLog::keyStr() );
    if ( par )
	wellfld_->setInput( MultiID(par->getStringValue(0)) );

    selDone( 0 );
    par = desc.getValParam( WellLog::logName() );
    if ( par )
	logsfld_->setCurrentItem( par->getStringValue(0) );

    mIfGetEnum( WellLog::upscaleType(), tp, sampfld_->setValue(tp) );

    return true;
}


bool uiWellLogAttrib::setInput( const Desc& desc )
{
    return true;
}


bool uiWellLogAttrib::setOutput( const Attrib::Desc& desc )
{
    return true;
}


bool uiWellLogAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != WellLog::attribName() )
	return false;

    mSetMultiID( WellLog::keyStr(), wellfld_->key() );
    mSetString( WellLog::logName(), logsfld_->getText() );
    mSetEnum( WellLog::upscaleType(), sampfld_->getIntValue() );

    return true;
}


bool uiWellLogAttrib::getInput( Desc& desc )
{
    return true;
}


bool uiWellLogAttrib::getOutput( Attrib::Desc& desc )
{
    return true;
}


void uiWellLogAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
}
