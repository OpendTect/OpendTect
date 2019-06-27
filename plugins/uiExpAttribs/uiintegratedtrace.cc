/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood
 Date:		June 2019
________________________________________________________________________

-*/



#include "uiintegratedtrace.h"
#include "integratedtrace.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribstorprovider.h"

#include "uiattrsel.h"
#include "uiattribfactory.h"
#include "uigeninput.h"

#include "od_helpids.h"


using namespace Attrib;

static uiWord sDispName()
{
    return od_static_tr("sDispName","Integrated Trace");
}


mInitAttribUI(uiIntegratedTrace,IntegratedTrace,sDispName(),sExperimentalGrp())

uiIntegratedTrace::uiIntegratedTrace( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mNoHelpKey )
{
    inpfld_ = createInpFld( is2d );
    setHAlignObj( inpfld_ );

    gatefld_ = new uiGenInput( this, gateLabel(),
			      DoubleInpIntervalSpec().setName("Z start",0)
						     .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );
    gatefld_->display( false );

}

bool uiIntegratedTrace::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


uiRetVal uiIntegratedTrace::getInput( Desc& desc )
{
    return fillInp( inpfld_, desc, 0 );
}


bool uiIntegratedTrace::setParameters( const Desc& desc )
{
    mIfGetFloatInterval( IntegratedTrace::gateStr(),gate,
			 gatefld_->setValue( gate ) );
    return true;
}


bool uiIntegratedTrace::getParameters( Desc& desc )
{
    mSetFloatInterval( IntegratedTrace::gateStr(), gatefld_->getFInterval() );
    return true;
}

#define mSetParam( type, nm, str, fn )\
{ \
    mDynamicCastGet(type##Param*, nm, newdesc->getValParam(str))\
    nm->setValue( fn );\
}


void uiIntegratedTrace::fillInSDDescParams( Desc* newdesc ) const
{
    mSetParam( ZGate, gate, IntegratedTrace::gateStr(),
	       gatefld_->getFInterval() )
}
