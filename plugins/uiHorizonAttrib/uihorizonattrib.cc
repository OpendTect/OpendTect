/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uihorizonattrib.h"
#include "horizonattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "ctxtioobj.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uimsg.h"

using namespace Attrib;

static const char* sDefHorOut[] = { "Z", "Horizon Data", 0 };
static const char* sDefHorNoSurfdtOut[] = { "Z", 0 };

mInitAttribUI(uiHorizonAttrib,Horizon,"Horizon",sKeyPositionGrp())


uiHorizonAttrib::uiHorizonAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.100")
    , nrouttypes_( 2 )
{
    inpfld_ = createInpFld( is2d );

    horfld_ = new uiIOObjSel( this, is2d ? mIOObjContext(EMHorizon2D)
	    				 : mIOObjContext(EMHorizon3D),
			      "Horizon" );
    horfld_->selectionDone.notify( mCB(this,uiHorizonAttrib,horSel) );
    horfld_->attach( alignedBelow, inpfld_ );

    typefld_ = new uiGenInput( this, "Output", StringListInpSpec(sDefHorOut) );
    typefld_->valuechanged.notify( mCB(this,uiHorizonAttrib,typeSel) );
    typefld_->attach( alignedBelow, horfld_ );

    isrelbox_ = new uiCheckBox( this, "Relative" );
    isrelbox_->attach( rightOf, typefld_ );

    surfdatafld_ = new uiGenInput( this, "Select Horizon Data",
	    			   StringListInpSpec() );
    surfdatafld_->attach( alignedBelow, typefld_ );
    
    setHAlignObj( inpfld_ );
    typeSel(0);
}


uiHorizonAttrib::~uiHorizonAttrib()
{
}


bool uiHorizonAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Horizon::attribName()) )
	return false;

    mIfGetString( Horizon::sKeyHorID(), horidstr,
	    	  if ( horidstr && *horidstr )
		      horfld_->setInput( MultiID(horidstr) ); );

    if ( horfld_->ioobj(true) )
	horSel(0);
    
    mIfGetEnum(Horizon::sKeyType(), typ, typefld_->setValue(typ));

    mIfGetString( Horizon::sKeySurfDataName(), surfdtnm, 
		  surfdatafld_->setValue(surfdatanms_.indexOf( surfdtnm )<0 
		      ? 0 : surfdatanms_.indexOf(surfdtnm) ) );

    mIfGetBool(Horizon::sKeyRelZ(), isrel, isrelbox_->setChecked(isrel));

    typeSel(0);

    return true;
}


bool uiHorizonAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiHorizonAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Horizon::attribName()) )
	return false;

    mSetString( Horizon::sKeyHorID(),
	        horfld_->ioobj(true) ? horfld_->ioobj()->key().buf() : "" );
    const int typ = typefld_->getIntValue();
    mSetEnum( Horizon::sKeyType(), typ );
    if ( typ==0 )
    {
	mSetBool( Horizon::sKeyRelZ(), isrelbox_->isChecked() )
    }
    else if ( typ==1 )
    {
	int surfdataidx = surfdatafld_->getIntValue();
	if ( surfdatanms_.size() )
	{
	    const char* surfdatanm = surfdatanms_.get(surfdataidx);
	    mSetString( Horizon::sKeySurfDataName(), surfdatanm )
	}
    }
    
    return true;
}


bool uiHorizonAttrib::getInput( Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


void uiHorizonAttrib::horSel( CallBacker* )
{
    if ( !horfld_->ioobj() ) return;

    EM::IOObjInfo eminfo( horfld_->ioobj()->key() );
    if ( !eminfo.isOK() )
    {
	BufferString msg( "Cannot read '", horfld_->ioobj()->name().buf(), "'");
	uiMSG().error( msg );
	return;
    }

    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    surfdatanms_.erase();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	surfdatanms_.add( attrnms.get(idx).buf() );
    surfdatafld_->newSpec( StringListInpSpec(surfdatanms_), 0 );
    
    const bool actionreq = (surfdatanms_.size() && nrouttypes_<2) ||
			   (!surfdatanms_.size() && nrouttypes_>1);
    if ( actionreq )
    {
	nrouttypes_ = surfdatanms_.size() ? 2 : 1;
	typefld_->newSpec( StringListInpSpec(surfdatanms_.size() 
		    			? sDefHorOut : sDefHorNoSurfdtOut), 0);
	typeSel(0);
    }
}


void uiHorizonAttrib::typeSel(CallBacker*)
{
    const bool isz = typefld_->getIntValue() == 0;
    isrelbox_->display( isz );
    surfdatafld_->display( !isz );
}

