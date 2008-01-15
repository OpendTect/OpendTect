/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:		$Id: uiprestackattrib.cc,v 1.3 2008-01-15 16:19:43 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiprestackattrib.h"
#include "prestackattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "multiid.h"
#include "seispsioprov.h"
#include "uiattribfactory.h"
#include "uiseissel.h"
#include "uigeninput.h"

using namespace Attrib;


static const char* typestrs[] =
{
    "Math",
    0
};


mInitAttribUI(uiPreStackAttrib,PreStack,"PreStack",sKeyBasicGrp)


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.x")
	, ctio_(*mMkCtxtIOObj(SeisPS))
{
    inpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup( is2d, true ) );

    typefld_ = new uiGenInput( this, "Type", StringListInpSpec(typestrs) );
    typefld_->attach( alignedBelow, inpfld_ );
    setHAlignObj( inpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiPreStackAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),PreStack::attribName()) )
	return false;

    if ( desc.getValParam( PreStack::subtypeStr() ) )
    {
	BufferString subdefstr = desc.getValParam( PreStack::subtypeStr() )
	    						->getStringValue(0);
	//TODO: get Attr Name
    }
    return true;
}


bool uiPreStackAttrib::setInput( const Desc& desc )
{
    const Attrib::Desc* inpdesc = desc.getInput( 0 );
    if ( inpdesc )
    {
	const LineKey lk( inpdesc->getValParam(
		    	Attrib::StorageProvider::keyStr())->getStringValue(0) );
	const MultiID mid( lk.lineName() );
	inpfld_->setInput( mid );
    }
    return true;
}


bool uiPreStackAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),PreStack::attribName()) )
	return false;

    //TODO -> get All sub-attrib params and create a def string out of that
    return true;
}


bool uiPreStackAttrib::getInput( Desc& desc )
{
    inpfld_->processInput();
    
    Desc* inpdesc = new Desc( StorageProvider::attribName() );
    mDynamicCastGet( Attrib::SeisStorageRefParam*,keyparam,
			     inpdesc->getValParam(StorageProvider::keyStr()) )
    keyparam->setValue( inpfld_->getKey() );
    if ( !desc.setInput(0,inpdesc) )
    {
	errmsg_ += "The suggested attribute for input 0";
	errmsg_ += " is incompatible with the input (wrong datatype)";
    }
    return true;
}


bool uiPreStackAttrib::setOutput( const Attrib::Desc& desc )
{
    return true;
}


bool uiPreStackAttrib::getOutput( Attrib::Desc& desc )
{
    return true;
}
