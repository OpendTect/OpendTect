/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseiswvltman.cc,v 1.2 2006-10-16 16:45:07 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseiswvltman.h"
#include "wavelet.h"
#include "ioobj.h"
#include "iostrm.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "color.h"
#include "survinfo.h"
#include "statruncalc.h"

#include "uibutton.h"
#include "uicanvas.h"
#include "uiioobjsel.h"
#include "uitextedit.h"
#include "iodrawtool.h"
#include "draw.h"


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Wavelet management",
                                     "Manage wavelets",
                                     "").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext() )
{
    createDefaultUI();

    uiPushButton* impbut = new uiPushButton( this, "&Import", false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );
    impbut->attach( alignedBelow, selgrp );
    uiPushButton* crbut = new uiPushButton( this, "&Create standard", false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightAlignedBelow, selgrp );

    wvltfld = new uiCanvas( this, "Wavelet disp canvas" );
    wvltfld->setBackgroundColor( Color::White );
    wvltfld->attach( ensureRightOf, selgrp );
    wvltfld->setStretch( 1, 1 );
    wvltfld->attach( heightSameAs, selgrp );

    infofld->attach( ensureBelow, impbut );
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    wvltfld->setPrefWidthInChar( 10 );
    selChg(0);
}


uiSeisWvltMan::~uiSeisWvltMan()
{
}


void uiSeisWvltMan::impPush( CallBacker* )
{
}


void uiSeisWvltMan::crPush( CallBacker* )
{
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt;

    Wavelet* wvlt = Wavelet::get( curioobj_ );
    if ( wvlt )
    {
	BufferString tmp;
	tmp += "Number of samples: "; tmp += wvlt->size(); tmp += "\n";
	tmp += "Sample interval "; tmp += SI().getZUnit(true); tmp += ": ";
	tmp += wvlt->sampleRate() * SI().zFactor(); tmp += "\n";
	Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
	rc.addValues( wvlt->size(), wvlt->samples() );
	tmp += "Min/Max amplitude: ";
	tmp += rc.min(); tmp += "/"; tmp += rc.max(); tmp += "\n";
	txt += tmp;
    }

    txt += getFileInfo();
    infofld->setText( txt );

    if ( wvlt )
    {
	wvltfld->drawTool()->drawText( 1, 1, wvlt->name(), Alignment::Start,
		true, true );
    }

    delete wvlt;
}
