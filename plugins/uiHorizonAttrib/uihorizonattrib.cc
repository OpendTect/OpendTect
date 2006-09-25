/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:		$Id: uihorizonattrib.cc,v 1.2 2006-09-25 13:42:21 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "horizonattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

using namespace Attrib;

static const char* sDefHorOut[] = { "Z", "Input data", 0 };


mInitUI( uiHorizonAttrib, "Horizon" )

uiHorizonAttrib::uiHorizonAttrib( uiParent* p )
    : uiAttrDescEd(p)
    , horctio_(*mMkCtxtIOObj(EMHorizon))
{
    inpfld_ = getInpFld();

    horfld_ = new uiIOObjSel( this, horctio_, "Horizon" );
    horfld_->selectiondone.notify( mCB(this,uiHorizonAttrib,horSel) );
    horfld_->attach( alignedBelow, inpfld_ );

    outputfld_ = new uiGenInput( this, "Output", StringListInpSpec() );
    outputfld_->attach( alignedBelow, horfld_ );

    setHAlignObj( inpfld_ );
}


uiHorizonAttrib::~uiHorizonAttrib()
{
    delete horctio_.ioobj; delete &horctio_;
}


const char* uiHorizonAttrib::getAttribName() const
{ return Horizon::attribName(); }


void uiHorizonAttrib::set2D( bool yn )
{
    inpfld_->set2D( yn );
}


bool uiHorizonAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Horizon::attribName()) )
	return false;

    mIfGetString( Horizon::sKeyHorID(), horidstr,
		  IOObj* ioobj = IOM().get( MultiID(horidstr) );
		  horfld_->ctxtIOObj().setObj( ioobj );
		  horfld_->updateInput() )
    horSel(0);
    return true;
}


bool uiHorizonAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiHorizonAttrib::setOutput( const Attrib::Desc& desc )
{
    outputfld_->setValue( desc.selectedOutput() );
    return true;
}


bool uiHorizonAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Horizon::attribName()) )
	return false;

    mSetString( Horizon::sKeyHorID(),
	        horctio_.ioobj ? horctio_.ioobj->key().buf() : "" );

    return true;
}


bool uiHorizonAttrib::getInput( Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiHorizonAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outputfld_->getIntValue() );
    return true;
}


void uiHorizonAttrib::horSel( CallBacker* )
{
    if ( !horctio_.ioobj )
    {
	uiMSG().error( "No valid horizon selected" );
	return;
    }

    EM::SurfaceIOData iodata;
    const char* err = EM::EMM().getSurfaceData( horctio_.ioobj->key(), iodata );
    if ( err && *err )
    {
	uiMSG().error( err );
	return;
    }

    BufferStringSet outputs( sDefHorOut );
    for ( int idx=0; idx<iodata.valnames.size(); idx++ )
	outputs.add( iodata.valnames.get(idx).buf() );
    outputfld_->newSpec( StringListInpSpec(outputs), 0 );
}
