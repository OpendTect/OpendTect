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
#include "welldata.h"
#include "welllogset.h"
#include "wellman.h"

#include "uiattribfactory.h"
#include "uilistbox.h"
#include "uiwellsel.h"

using namespace Attrib;


mInitAttribUI(uiWellLogAttrib,WellLog,"WellLog",sKeyBasicGrp())


uiWellLogAttrib::uiWellLogAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.3")

{
    wellfld_ = new uiWellSel( this, true );
    wellfld_->selectionDone.notify( mCB(this,uiWellLogAttrib,selDone) );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Logs" );
    llb->setStretch( 1, 1 );
    logsfld_ = llb->box();
    llb->attach( alignedBelow, wellfld_ );

    setHAlignObj( wellfld_ );
}


void uiWellLogAttrib::selDone( CallBacker* )
{
    logsfld_->setEmpty();
    const IOObj* ioobj = wellfld_->ioobj( true );
    if ( !ioobj ) return;

    const MultiID wellid = ioobj->key();
    const bool isloaded = Well::MGR().isLoaded( wellid );
    Well::Data* wd = Well::MGR().get( wellid );
    const Well::LogSet& logs = wd->logs();
    BufferStringSet lognms; logs.getNames( lognms );
    logsfld_->addItems( lognms );
    if ( !isloaded )
	delete Well::MGR().release( wellid );
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
