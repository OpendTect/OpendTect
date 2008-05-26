/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2006
 RCS:           $Id: uicompparsel.cc,v 1.3 2008-05-26 05:43:19 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uicompoundparsel.h"
#include "uigeninput.h"
#include "uibutton.h"


uiCompoundParSel::uiCompoundParSel( uiParent* p, const char* seltxt,
				    const char* buttxt )
    : uiGroup(p,seltxt)
    , butPush(this)
{
    txtfld = new uiGenInput( this, seltxt, "" );
    txtfld->setReadOnly( true );

    uiPushButton* but = new uiPushButton( this, buttxt ? buttxt : "Select",
	    				  false );
    but->setName( BufferString(buttxt," ",seltxt).buf() );
    but->activated.notify( mCB(this,uiCompoundParSel,doSel) );
    but->attach( rightOf, txtfld );

    setHAlignObj( txtfld );
    setHCentreObj( txtfld );

    mainObject()->finaliseDone.notify( mCB(this,uiCompoundParSel,updSummary) );
}


void uiCompoundParSel::doSel( CallBacker* )
{
    butPush.trigger();
    updateSummary();
}


void uiCompoundParSel::updSummary( CallBacker* )
{
    txtfld->setText( getSummary() );
}


void uiCompoundParSel::setSelText( const char* txt )
{
    txtfld->setTitleText( txt );
}
