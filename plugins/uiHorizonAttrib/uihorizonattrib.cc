/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:		$Id: uihorizonattrib.cc,v 1.1 2006-09-22 15:14:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "horizonattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"

using namespace Attrib;


mInitUI( uiHorizonAttrib, "Horizon" )

uiHorizonAttrib::uiHorizonAttrib( uiParent* p )
    : uiAttrDescEd(p)
    , horctio_(*mMkCtxtIOObj(EMHorizon))
{
    inpfld = getInpFld();

    horfld = new uiIOObjSel( this, horctio_, "Horizon" );
    horfld->attach( alignedBelow, inpfld );

    outputfld = new uiGenInput( this, "Output", StringListInpSpec() );
    outputfld->attach( alignedBelow, horfld );

    setHAlignObj( inpfld );
}


uiHorizonAttrib::~uiHorizonAttrib()
{
    delete horctio_.ioobj; delete &horctio_;
}


const char* uiHorizonAttrib::getAttribName() const
{ return Horizon::attribName(); }


void uiHorizonAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
}


bool uiHorizonAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Horizon::attribName()) )
	return false;

    mIfGetString( Horizon::sKeyHorID(), horidstr,
		  IOObj* ioobj = IOM().get( MultiID(horidstr) );
		  horfld->ctxtIOObj().setObj( ioobj );
		  horfld->updateInput() )
    return true;
}


bool uiHorizonAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
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
    inpfld->processInput();
    fillInp( inpfld, desc, 0 );
    return true;
}
