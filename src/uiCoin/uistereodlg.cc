/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uistereodlg.cc,v 1.1 2002-07-15 08:00:49 nanne Exp $
________________________________________________________________________

-*/

#include "uistereodlg.h"
#include "uisoviewer.h"
#include "uislider.h"


uiStereoDlg::uiStereoDlg( uiParent* p, uiSoViewer* vwr_ )
	: uiDialog(p, uiDialog::Setup("Stereo viewing","Set stereo offset")
		      .canceltext(""))
	, vwr(vwr_)
{
    sliderfld = new uiSlider( this, "Stereo offset", true );
    sliderfld->valueChanged.notify( mCB(this,uiStereoDlg,sliderMove) );

    finaliseStart.notify( mCB(this,uiStereoDlg,doFinalise) );
}


void uiStereoDlg::doFinalise( CallBacker* )
{
    float offset = vwr->getStereoOffset();

    sliderfld->setMinValue( 10 );
    sliderfld->setMaxValue( 1000 );
    sliderfld->setTickMarks( false );
    sliderfld->setValue( offset );
}


bool uiStereoDlg::acceptOK( CallBacker* )
{
    sliderfld->processInput();
    float slval = sliderfld->getValue();
    vwr->setStereoOffset( slval );
    return true;
}


void uiStereoDlg::sliderMove( CallBacker* )
{
    float slval = sliderfld->getValue();
    vwr->setStereoOffset( slval );
}
