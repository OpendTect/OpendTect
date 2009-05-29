/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisemblanceattrib.cc,v 1.3 2009-05-29 10:45:18 cvsnanne Exp $";

#include "uisemblanceattrib.h"
#include "semblanceattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "survinfo.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"



using namespace Attrib;

mInitAttribUI(uiSemblanceAttrib,Semblance,"Semblance","Experimental")

uiSemblanceAttrib::uiSemblanceAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"")
    , sz2dfld_(0)
    , sz3dfld_(0)
{
    inpfld_ = getInpFld( is2d );

    uiGenInput* attachobj = 0;
    if ( is2d )
    {
	sz2dfld_ = new uiGenInput( this, "Calculation size (trc/sample)",
			IntInpSpec().setName("trcsz"),
			IntInpSpec().setName("zsz") );
	attachobj = sz2dfld_;
    }
    else
    {
	sz3dfld_ = new uiGenInput( this, "Calculation size (inl/crl/sample)",
			IntInpSpec().setName("inlsz"),
			IntInpSpec().setName("crlsz"),
			IntInpSpec().setName("zsz") );
	attachobj = sz3dfld_;
    }

    attachobj->attach( alignedBelow, inpfld_ );
    setHAlignObj( inpfld_ );
}


bool uiSemblanceAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Semblance::attribName()) )
	return false;

    if ( sz3dfld_ )
    {
	mIfGetInt( Semblance::inlszStr(), isz_, sz3dfld_->setValue(isz_,0) );
	mIfGetInt( Semblance::crlszStr(), csz_, sz3dfld_->setValue(csz_,1) );
	mIfGetInt( Semblance::zszStr(), zsz_, sz3dfld_->setValue(zsz_,2) );
    }
    else
    {
	mIfGetInt( Semblance::crlszStr(), csz_, sz2dfld_->setValue(csz_,0) );
	mIfGetInt( Semblance::zszStr(), zsz_, sz2dfld_->setValue(zsz_,1) );
    }

    return true;
}


bool uiSemblanceAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiSemblanceAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Semblance::attribName()) )
	return false;

    if ( sz3dfld_ )
    {
	mSetInt( Semblance::inlszStr(), sz3dfld_->getIntValue(0) );
	mSetInt( Semblance::crlszStr(), sz3dfld_->getIntValue(1) );
	mSetInt( Semblance::zszStr(), sz3dfld_->getIntValue(2) );
    }
    else
    {
	mSetInt( Semblance::inlszStr(), 1 );
	mSetInt( Semblance::crlszStr(), sz2dfld_->getIntValue(0) );
	mSetInt( Semblance::zszStr(), sz2dfld_->getIntValue(1) );
    }

    return true;
}


bool uiSemblanceAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}
