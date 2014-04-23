/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwellattribxplot.h"

#include "attribdescset.h"

#include "uidatapointset.h"
#include "uiwelllogextract.h"
#include "od_helpids.h"


using namespace Attrib;

uiWellAttribCrossPlot::uiWellAttribCrossPlot( uiParent* p,
					      const Attrib::DescSet* d )
	: uiDialog(p,uiDialog::Setup("Attribute/Well cross-plotting",
		     "Select attributes and logs for cross-plot"
		     ,mODHelpKey(mWellAttribCrossPlotHelpID) ).modal(false))
    	, dpsdispmgr_(0)
{
    wellextractgrp_ =
	new uiWellLogExtractGrp( this, uiWellLogExtractGrp::Setup(), d );
}

#define mDPM DPM(DataPackMgr::PointID())

uiWellAttribCrossPlot::~uiWellAttribCrossPlot()
{
}


void uiWellAttribCrossPlot::setDescSet( const Attrib::DescSet* newads )
{
    wellextractgrp_->setDescSet( newads );
}


#define mErrRet(s) { deepErase(dcds); if ( s ) uiMSG().error(s); return false; }

bool uiWellAttribCrossPlot::acceptOK( CallBacker* )
{
    if ( !wellextractgrp_->extractDPS() )
	return false;

    uiDataPointSet* uidps =
	new uiDataPointSet( this, *wellextractgrp_->getDPS(),
			    uiDataPointSet::Setup("Well attribute data",false),
			    dpsdispmgr_ );
    IOPar& iop = uidps->storePars();
    wellextractgrp_->getDescSet()->fillPar( iop );
    if ( iop.name().isEmpty() )
	iop.setName( "Attributes" );
    uidps->setGroupType( "well" );
    BufferStringSet wellnms;
    wellextractgrp_->getWellNames( wellnms );
    uidps->setGroupNames( wellnms );
    return uidps->go() ? true : false;
}
