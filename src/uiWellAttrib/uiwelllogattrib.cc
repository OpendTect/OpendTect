/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwelllogattrib.h"
#include "welllogattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "ioobj.h"
#include "ptrman.h"
#include "welldata.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"

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
    wellfld_->selectionDone.notify( mCB(this,uiWellLogAttrib,selDone) );

    uiLabeledListBox* llb = new uiLabeledListBox( this, tr("Select Log") );
    llb->setStretch( 1, 1 );
    logsfld_ = llb->box();
    logsfld_->setHSzPol( uiObject::Wide );
    llb->attach( alignedBelow, wellfld_ );

    sampfld_ = new uiGenInput( this, tr("Log resampling method"),
			       StringListInpSpec(Stats::UpscaleTypeNames()) );
    sampfld_->setValue( Stats::UseAvg );
    sampfld_->attach( alignedBelow, llb );

    setHAlignObj( wellfld_ );
}


void uiWellLogAttrib::selDone( CallBacker* )
{
    logsfld_->setEmpty();
    const MultiID wellid = wellfld_->key();
    RefMan<Well::Data> wd = new Well::Data;
    BufferStringSet lognms;
    if ( Well::MGR().isLoaded(wellid) )
    {
	wd = Well::MGR().get( wellid );
	if ( !wd )
	{
	    uiMSG().error( Well::MGR().errMsg() );
	    return;
	}

	wd->logs().getNames( lognms );
    }
    else
    {
	Well::Reader wrdr( wellid, *wd );
	wrdr.getLogInfo( lognms );
    }

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

    mSetString( WellLog::keyStr(), wellfld_->key().buf() );
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
