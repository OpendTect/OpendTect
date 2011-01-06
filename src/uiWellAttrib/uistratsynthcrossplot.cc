/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratsynthcrossplot.cc,v 1.1 2011-01-06 15:24:39 cvsbert Exp $";

#include "uistratsynthcrossplot.h"
#include "uiattribsetbuild.h"
#include "uisplitter.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "seisbuf.h"
#include "seistrc.h"


#include "uilabel.h"

uiStratSynthCrossplot::uiStratSynthCrossplot( uiParent* p, const SeisTrcBuf& tb,
				    const Strat::LayerModel& lm )
    : uiDialog(p,Setup("Layer model/synthetics cross-plotting",
			mNoDlgTitle,mTODOHelpID))
    , tbuf_(tb)
    , lm_(lm)
{
    uiAttribDescSetBuild::Setup bsu( true );
    bsu.showdepthonlyattrs(false).showusingtrcpos(false).showps(false);
    seisattrfld_ = new uiAttribDescSetBuild( this, bsu );

    uiGroup* botgrp = new uiGroup( this, "Layer seq attr group" );
    new uiLabel( botgrp, "TODO: make Layer sequence attributes" );

    uiSplitter* spl = new uiSplitter( this, "Splitter", false );
    spl->addGroup( seisattrfld_ );
    spl->addGroup( botgrp );
}


uiStratSynthCrossplot::~uiStratSynthCrossplot()
{
}


bool uiStratSynthCrossplot::acceptOK( CallBacker* )
{
    return true;
}
