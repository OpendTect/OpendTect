/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistereodlg.cc,v 1.9 2009-07-22 16:01:39 cvsbert Exp $";

#include "uistereodlg.h"
#include "uisoviewer.h"
#include "uislider.h"


uiStereoDlg::uiStereoDlg( uiParent* p, ObjectSet<uiSoViewer>& vwrs_ )
	: uiDialog(p, uiDialog::Setup("Stereo viewing",
		    		      "Set stereo offset","50.0.2")
		      .canceltext(""))
	, vwrs(vwrs_)
{
    sliderfld = new uiSliderExtra( this, 
	    		uiSliderExtra::Setup("Stereo offset").withedit(true),
	   		"Offset slider" );
    sliderfld->sldr()->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );

    finaliseStart.notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    float offset = vwrs[0]->getStereoOffset();

    sliderfld->sldr()->setMinValue( 10 );
    sliderfld->sldr()->setMaxValue( 1000 );
    sliderfld->sldr()->setValue( offset );
}


bool uiStereoDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    float slval = sliderfld->sldr()->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
	vwrs[idx]->setStereoOffset( slval );

    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    float slval = sliderfld->sldr()->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
        vwrs[idx]->setStereoOffset( slval );
}
