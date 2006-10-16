/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseiswvltman.cc,v 1.1 2006-10-16 14:58:29 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseiswvltman.h"
#include "wavelet.h"
#include "ioobj.h"
#include "iostrm.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "statruncalc.h"

#include "uiioobjsel.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"


static const int cPrefWidth = 50;


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Wavelet management",
                                     "Manage wavelets",
                                     "").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext() )
{
    createDefaultUI();
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();

    selgrp->setPrefWidthInChar( cPrefWidth );
    infofld->setPrefWidthInChar( cPrefWidth );
    selChg(0);
}


uiSeisWvltMan::~uiSeisWvltMan()
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
}
