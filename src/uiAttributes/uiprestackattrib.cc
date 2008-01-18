/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:		$Id: uiprestackattrib.cc,v 1.5 2008-01-18 11:37:02 cvsbert Exp $
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


mInitAttribUI(uiPreStackAttrib,PreStack,"PreStack",sKeyBasicGrp)


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.17")
	, ctio_(*mMkCtxtIOObj(SeisPS))
{
    inpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(is2d,true) );

    stattypefld_ = new uiGenInput( this, "Statistics type",
				    StringListInpSpec(Stats::TypeNames) );
    stattypefld_->attach( alignedBelow, inpfld_ );
    setHAlignObj( inpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiPreStackAttrib::setParameters( const Attrib::Desc& desc )
{
    RefMan<Attrib::Desc> tmpdesc = new Attrib::Desc( desc );
    RefMan<Attrib::PreStack> aps = new Attrib::PreStack( *tmpdesc );
    inpfld_->setInput( aps->psID() );
    stattypefld_->setValue( (int)aps->setup().stattype_ );
    return true;
}


bool uiPreStackAttrib::getParameters( Desc& desc )
{
    inpfld_->processInput();
    if ( !ctio_.ioobj )
	{ errmsg_ = "Please select the input data store"; return false; }

    mSetString("id",ctio_.ioobj->key());
    mSetEnum(Attrib::PreStack::stattypeStr(),stattypefld_->getIntValue());
    return true;
}
