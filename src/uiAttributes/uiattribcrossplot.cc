/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra / Bert
 Date:          March 2003 / Feb 2008
 RCS:           $Id: uiattribcrossplot.cc,v 1.14 2008-02-25 15:11:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattribcrossplot.h"
#include "uidatapointset.h"
#include "seisioobjinfo.h"
#include "attribsel.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posprovider.h"
#include "posfilterset.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "keystrs.h"
#include "executor.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"

#include "uimsg.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uiioobjsel.h"
#include "uiposdataedit.h"
#include "uiposprovider.h"
#include "uiposfilterset.h"

using namespace Attrib;

uiAttribCrossPlot::uiAttribCrossPlot( uiParent* p, const Attrib::DescSet& d )
	: uiDialog(p,uiDialog::Setup("Attribute cross-plotting",
		     "Select attributes and locations for cross-plot"
		     ,"101.3.0"))
	, ads_(d)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this,
	    					  "Attributes to calculate" );
    attrsfld_ = llb->box();
    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
    {
	attrsfld_->addItem( attrinf.attrnms.get(idx), false );
	attrdefs_.add( attrinf.attrnms.get(idx) );
    }
    for ( int idx=0; idx<attrinf.ioobjids.size(); idx++ )
    {
	BufferStringSet bss;
	SeisIOObjInfo sii( MultiID( attrinf.ioobjids.get(idx) ) );
	sii.getDefKeys( bss, true );
	for ( int inm=0; inm<bss.size(); inm++ )
	{
	    const char* defkey = bss.get(inm).buf();
	    const char* ioobjnm = attrinf.ioobjnms.get(idx).buf();
	    attrsfld_->addItem(
		    SeisIOObjInfo::defKey2DispName(defkey,ioobjnm) );
	    attrdefs_.add( defkey );
	}
    }
    if ( !attrsfld_->isEmpty() )
	attrsfld_->setCurrentItem( int(0) );
    attrsfld_->setMultiSelect( true );

    uiPosProvider::Setup psu( ads_.is2D(), true );
    psu.seltxt( "Select locations by" ).choicetype( uiPosProvider::Setup::All );
    posprovfld_ = new uiPosProvider( this, psu );
    posprovfld_->setExtractionDefaults();
    posprovfld_->attach( alignedBelow, llb );

    uiPosFilterSet::Setup fsu( ads_.is2D() );
    fsu.seltxt( "Location filters" ).incprovs( false );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, posprovfld_ );
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
}


#define mDPM DPM(DataPackMgr::PointID)

#define mErrRet(s) \
{ \
    if ( dps ) mDPM.release(dps->id()); \
    if ( s ) uiMSG().error(s); return false; \
}

bool uiAttribCrossPlot::acceptOK( CallBacker* )
{
    DataPointSet* dps = 0;
    PtrMan<Pos::Provider> prov = posprovfld_->createProvider();
    if ( !prov )
	mErrRet("Internal: no Pos::Provider")

    uiTaskRunner tr( this );
    PtrMan<Executor> provinit = prov->initializer();
    if ( provinit && !tr.execute(*provinit) )
	mErrRet(0)
    else if ( !prov->initialize() )
	mErrRet("Cannot initialize the position generator")

    const bool is2d = prov->is2D();

    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<attrdefs_.size(); idx++ )
    {
	if ( attrsfld_->isSelected(idx) )
	    dcds += new DataColDef( attrsfld_->textOfItem(idx),
		    		    attrdefs_.get(idx) );
    }
    if ( dcds.isEmpty() )
	mErrRet("Please select at least one attribute to evaluate")

    IOPar iop; posfiltfld_->fillPar( iop );
    if ( is2d )
    {
	Pos::Filter2D* filt = Pos::Filter2D::make( iop );
	Pos::Provider2D* p2d = (Pos::Provider2D*)prov.ptr();
	dps = new DataPointSet( *p2d, dcds, filt );
    }
    else
    {
	Pos::Filter3D* filt = Pos::Filter3D::make( iop );
	Pos::Provider3D* p3d = (Pos::Provider3D*)prov.ptr();
	dps = new DataPointSet( *p3d, dcds, filt );
    }
    if ( dps->size() < 1 )
	mErrRet("No positions selected")

    dps->setName( "Attribute cross-plot" );
    mDPM.add( dps );

    Attrib::EngineMan aem;
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps, ads_ );
    if ( !tr.execute(*tabextr) )
	mErrRet(0)

    uiDataPointSet* dlg = new uiDataPointSet( this, *dps,
			uiDataPointSet::Setup("Attribute data").modal(false) );
    return dlg->go() ? true : false;
}
