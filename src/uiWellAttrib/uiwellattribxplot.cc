/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribxplot.h"

#include "attribdescset.h"

#include "uidatapointset.h"
#include "uiwelllogextract.h"
#include "od_helpids.h"


using namespace Attrib;

uiWellAttribCrossPlot::uiWellAttribCrossPlot( uiParent* p,
					      const Attrib::DescSet* d )
    : uiDialog(p,uiDialog::Setup(tr("Attribute/Well Cross-plotting"),
		 mNoDlgTitle,
		 mODHelpKey(mWellAttribCrossPlotHelpID) ).modal(false))
{
    setOkText( uiStrings::sNext() );

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

    auto* uidps =
	new uiDataPointSet( this, *wellextractgrp_->getDPS(),
			    uiDataPointSet::Setup(tr("Well attribute data"),
						  false), dpsdispmgr_ );
    IOPar& iop = uidps->storePars();
    uidps->setDeleteOnClose( true );
    wellextractgrp_->getDescSet()->fillPar( iop );
    if ( iop.name().isEmpty() )
	iop.setName( "Attributes" );
    uidps->setGroupType( "well" );
    BufferStringSet wellnms;
    wellextractgrp_->getWellNames( wellnms );
    uidps->setGroupNames( wellnms );
    return uidps->go() ? true : false;
}
