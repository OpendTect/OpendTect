/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiwellattribxplot.cc,v 1.55 2012-07-23 09:32:25 cvssatyaki Exp $";

#include "uiwellattribxplot.h"

#include "attribdescset.h"

#include "uidatapointset.h"
#include "uiwelllogextract.h"


using namespace Attrib;

uiWellAttribCrossPlot::uiWellAttribCrossPlot( uiParent* p,
					      const Attrib::DescSet* d )
	: uiDialog(p,uiDialog::Setup("Attribute/Well cross-plotting",
		     "Select attributes and logs for cross-plot"
		     ,"111.1.1").modal(false))
    	, dpsdispmgr_(0)
{
    wellextractgrp_ = new uiWellLogExtractGrp( this, d );
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
    wellextractgrp_->extractDPS();
    uiDataPointSet* uidps =
	new uiDataPointSet( this, *wellextractgrp_->getDPS(),
			    uiDataPointSet::Setup("Well attribute data",false),
			    dpsdispmgr_ );
    uidps->setGroupType( "well" );
    BufferStringSet wellnms;
    wellextractgrp_->getWellNames( wellnms );
    uidps->setGroupNames( wellnms );
    return uidps->go() ? true : false;
}
