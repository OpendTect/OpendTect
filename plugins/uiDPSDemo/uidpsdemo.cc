/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidpsdemo.cc,v 1.2 2009-11-04 11:16:06 cvsbert Exp $";

#include "uidpsdemo.h"

#include "datapointset.h"
#include "emsurfacetr.h"
#include "seistrctr.h"

#include "uiseissel.h"
#include "uigeninput.h"
#include "uimsg.h"


uiDPSDemo::uiDPSDemo( uiParent* p )
	: uiDialog(p,Setup("DataPointSet demo","Data extraction parameters",
		    	   mNoHelpID))
	, dps_(*new DataPointSet(false))
{
    horfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D) );

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    seisfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(false,false) );
    seisfld_->attach( alignedBelow, horfld_ );

    nrptsfld_ = new uiGenInput( this, "Number of points to extract",
	    			IntInpSpec(1000) );
    nrptsfld_->attach( alignedBelow, seisfld_ );
}


uiDPSDemo::~uiDPSDemo()
{
    delete &dps_;
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiDPSDemo::acceptOK( CallBacker* )
{
    const IOObj* horioobj = horfld_->ioobj(); // emits its own error message
    if ( !horioobj ) return false;
    const IOObj* seisioobj = seisfld_->ioobj(); // this one, too
    if ( !seisioobj ) return false;

    const int nrpts = nrptsfld_->getIntValue();
    if ( nrpts < 2 )
	mErrRet( "Please enter a valid number of points" )

    return doWork( *horioobj, *seisioobj, nrpts );
}


bool uiDPSDemo::doWork( const IOObj& horioobj, const IOObj& seisioobj,
			int nrpts )
{
    mErrRet("TODO: implement")
}
