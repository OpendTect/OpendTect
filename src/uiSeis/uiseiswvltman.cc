/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseiswvltman.cc,v 1.3 2006-10-18 10:53:25 cvsbert Exp $
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
#include "uiioobjsel.h"
#include "uitextedit.h"
#include "uisectiondisp.h"
#include "arrayndimpl.h"


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Wavelet management",
                                     "Manage wavelets",
                                     "").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext() )
    , fdctxt_(*new FlatDisp::Context(false))
    , fda2d_(0)
    , fddata_(new FlatDisp::Data)
{
    createDefaultUI();

    uiPushButton* impbut = new uiPushButton( this, "&Import", false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );
    impbut->attach( alignedBelow, selgrp );
    uiPushButton* crbut = new uiPushButton( this, "&Create standard", false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightAlignedBelow, selgrp );

    fdctxt_.annot_.x1name_ = "Amplitude";
    fdctxt_.annot_.x2name_ = SI().zIsTime() ? "Time" : "Depth";
    fdctxt_.ddpars_.dispvd_ = false;
    fdctxt_.ddpars_.dispwva_ = true;
    fdctxt_.ddpars_.wva_.overlap_ = 0;
    fdctxt_.ddpars_.wva_.clipperc_ = 0;
    wvltfld = new uiSectionDisp( this, fdctxt_ );
    wvltfld->attach( ensureRightOf, selgrp );
    wvltfld->setStretch( 1, 1 );
    wvltfld->attach( heightSameAs, selgrp );
    wvltfld->setData( fddata_ );

    infofld->attach( ensureBelow, impbut );
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    wvltfld->setPrefWidthInChar( 10 );
    selChg(0);
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    delete fda2d_;
    delete fddata_;
    delete &fdctxt_;
}


void uiSeisWvltMan::impPush( CallBacker* )
{
}


void uiSeisWvltMan::crPush( CallBacker* )
{
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt = getFileInfo();
    Wavelet* wvlt = Wavelet::get( curioobj_ );

    if ( !wvlt )
	fddata_->set( true, 0, "" );
    else
    {
	const int wvltsz = wvlt->size();
	assign( fdctxt_.posdata_.x2rg_, wvlt->samplePositions() );
	delete fda2d_;
	fda2d_ = new Array2DImpl<float>( 1, wvltsz );
	memcpy( fda2d_->getData(), wvlt->samples(), wvltsz * sizeof(float) );
	fddata_->set( true, fda2d_, wvlt->name() );

	Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
	rc.addValues( wvltsz, wvlt->samples() );

	BufferString tmp;
	tmp += "Number of samples: "; tmp += wvlt->size(); tmp += "\n";
	tmp += "Sample interval "; tmp += SI().getZUnit(true); tmp += ": ";
	tmp += wvlt->sampleRate() * SI().zFactor(); tmp += "\n";
	tmp += "Min/Max amplitude: ";
	tmp += rc.min(); tmp += "/"; tmp += rc.max(); tmp += "\n";
	txt += tmp;

	delete wvlt;
    }

    wvltfld->forceReDraw();
    infofld->setText( txt );
}
