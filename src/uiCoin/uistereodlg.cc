/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uistereodlg.cc,v 1.2 2002-08-20 07:42:11 nanne Exp $
________________________________________________________________________

-*/

#include "uistereodlg.h"
#include "uisoviewer.h"
#include "uislider.h"


uiStereoDlg::uiStereoDlg( uiParent* p, ObjectSet<uiSoViewer>& vwrs_ )
	: uiDialog(p, uiDialog::Setup("Stereo viewing","Set stereo offset")
		      .canceltext(""))
	, vwrs(vwrs_)
{
    sliderfld = new uiSlider( this, "Stereo offset", true );
    sliderfld->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );

    finaliseStart.notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    float offset = vwrs[0]->getStereoOffset();

    sliderfld->setMinValue( 10 );
    sliderfld->setMaxValue( 1000 );
    sliderfld->setTickMarks( false );
    sliderfld->setValue( offset );
}


bool uiStereoDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    float slval = sliderfld->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
	vwrs[idx]->setStereoOffset( slval );

    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    float slval = sliderfld->getValue();
    for ( int idx=0; idx<vwrs.size(); idx++ )
        vwrs[idx]->setStereoOffset( slval );
}
