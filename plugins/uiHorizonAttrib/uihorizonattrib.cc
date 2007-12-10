/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:		$Id: uihorizonattrib.cc,v 1.9 2007-12-10 12:59:52 cvsbert Exp $
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

static const char* sDefHorOut[] = { "Z", "Surface Data", 0 };

mInitAttribUI(uiHorizonAttrib,Horizon,"Horizon",sKeyPositionGrp)


uiHorizonAttrib::uiHorizonAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.100")
    , horctio_(*mGetCtxtIOObj(EMHorizon3D,Surf))
{
    inpfld_ = getInpFld();

    horfld_ = new uiIOObjSel( this, horctio_, "Horizon" );
    horfld_->selectiondone.notify( mCB(this,uiHorizonAttrib,horSel) );
    horfld_->attach( alignedBelow, inpfld_ );

    typefld_ = new uiGenInput( this, "Output", StringListInpSpec(sDefHorOut) );
    typefld_->valuechanged.notify( mCB(this,uiHorizonAttrib,typeSel) );
    typefld_->attach( alignedBelow, horfld_ );

    surfdatafld_ = new uiGenInput( this, "Select surface data",
	    			   StringListInpSpec() );
    surfdatafld_->attach( alignedBelow, typefld_ );
    
    setHAlignObj( inpfld_ );
    typeSel(0);
}


uiHorizonAttrib::~uiHorizonAttrib()
{
    delete horctio_.ioobj; delete &horctio_;
}


bool uiHorizonAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Horizon::attribName()) )
	return false;

    mIfGetString( Horizon::sKeyHorID(), horidstr,
		  IOObj* ioobj = IOM().get( MultiID(horidstr) );
		  horfld_->ctxtIOObj().setObj( ioobj );
		  horfld_->updateInput() );

    if ( horctio_.ioobj )
	horSel(0);
    
    mIfGetEnum(Horizon::sKeyType(), typ, typefld_->setValue(typ));

    mIfGetString( Horizon::sKeySurfDataName(), surfdtnm, 
		  surfdatafld_->setValue(surfdatanms_.indexOf( surfdtnm )<0 
		      ? 0 : surfdatanms_.indexOf(surfdtnm) ) );

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
	        horctio_.ioobj ? horctio_.ioobj->key().buf() : "" );
    mSetEnum( Horizon::sKeyType(), typefld_->getIntValue() );
    const int typ = typefld_->getIntValue();
    if ( typ==1 )
    {
	int surfdataidx = surfdatafld_->getIntValue();
	const char* surfdatanm = surfdatanms_.get(surfdataidx);
	mSetString( Horizon::sKeySurfDataName(), surfdatanm )
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

    surfdatanms_.erase();
    for ( int idx=0; idx<iodata.valnames.size(); idx++ )
	surfdatanms_.add( iodata.valnames.get(idx).buf() );
    surfdatafld_->newSpec( StringListInpSpec(surfdatanms_), 0 );
}


void uiHorizonAttrib::typeSel(CallBacker*)
{
    surfdatafld_->display( typefld_->getIntValue() == 1 );
}

