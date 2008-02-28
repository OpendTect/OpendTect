/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra / Bert
 Date:          March 2003 / Feb 2008
 RCS:           $Id: uiattribcrossplot.cc,v 1.19 2008-02-28 15:54:29 cvshelene Exp $
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
#include "posvecdataset.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribstorprovider.h"
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
		     ,"101.3.0").modal(false))
	, ads_(*d.clone())
{
    uiLabeledListBox* llb = new uiLabeledListBox( this,
	    					  "Attributes to calculate" );
    attrsfld_ = llb->box();
    Attrib::SelInfo attrinf( &ads_, 0, ads_.is2D() );
    for ( int idx=0; idx<attrinf.attrnms.size(); idx++ )
    {
	attrsfld_->addItem( attrinf.attrnms.get(idx), false );
	BufferString defstr;
	const Attrib::Desc* desc = ads_.getDesc( attrinf.attrids[idx] );
	if ( desc )
	    desc->getDefStr( defstr );
	BufferString fulldef = attrinf.attrids[idx].asInt(); fulldef += "`";
	fulldef += defstr;
	attrdefs_.add( fulldef );
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
    fsu.seltxt( "Location filters" ).incprovs( true );
    posfiltfld_ = new uiPosFilterSetSel( this, fsu );
    posfiltfld_->attach( alignedBelow, posprovfld_ );
}


uiAttribCrossPlot::~uiAttribCrossPlot()
{
    delete const_cast<Attrib::DescSet*>(&ads_);
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
    const char* msg = prov->initialize( tr );
    if ( msg ) mErrRet(*msg?msg:0)

    ObjectSet<DataColDef> dcds;
    for ( int idx=0; idx<attrdefs_.size(); idx++ )
    {
	if ( attrsfld_->isSelected(idx) )
	{
	    if ( !strncmp( attrsfld_->textOfItem(idx), "[", 1 ) 
		 && !strstr( attrdefs_.get(idx),
		     	     Attrib::StorageProvider::attribName() ) )
	    {
		int descid = prepareStoredDesc(idx);
		if ( descid<0 )
		{
		    BufferString err = "Cannot use stored data"; 
		    err += attrsfld_->textOfItem(idx);
		    mErrRet(err);
		}
		
		BufferString tmpstr;
		const Attrib::Desc* desc = ads_.getDesc( DescID(descid,true) );
		if ( !desc ) mErrRet("Huh?");
		desc->getDefStr( tmpstr );
		BufferString defstr = descid; defstr += "`"; defstr += tmpstr;
		attrdefs_.get(idx) = defstr;
	    }
	    
	    dcds += new DataColDef( attrsfld_->textOfItem(idx),
				    attrdefs_.get(idx) );
	}
    }
    if ( dcds.isEmpty() )
	mErrRet("Please select at least one attribute to evaluate")

    IOPar iop; posfiltfld_->fillPar( iop );
    Pos::Filter* filt = Pos::Filter::make( iop, prov->is2D() );
    dps = new DataPointSet( *prov, dcds, filt );
    if ( dps->size() < 1 )
	mErrRet("No positions selected")

    BufferString dpsnm; prov->getSummary( dpsnm );
    if ( filt )
    {
	BufferString filtsumm; filt->getSummary( filtsumm );
	dpsnm += " / "; dpsnm += filtsumm;
    }
    dps->setName( dpsnm );
    IOPar descsetpars;
    ads_.fillPar( descsetpars );
    const_cast<PosVecDataSet*>( &(dps->dataSet()) )->pars() = descsetpars;
    mDPM.add( dps );

    Attrib::EngineMan aem;
    PtrMan<Executor> tabextr = aem.getTableExtractor( *dps, ads_ );
    if ( !tr.execute(*tabextr) )
	mErrRet(0)

    uiDataPointSet* dlg = new uiDataPointSet( this, *dps,
			uiDataPointSet::Setup("Attribute data") );
    return dlg->go() ? true : false;
}


int uiAttribCrossPlot::prepareStoredDesc( int idx )
{
    IOPar iopar;
    BufferStringSet* errmsgs = new BufferStringSet();
    BufferString defstring = Attrib::StorageProvider::attribName();
    defstring += " "; defstring += Attrib::StorageProvider::keyStr();
    defstring += "="; defstring += attrdefs_.get(idx); defstring += " output=0";
    Desc* newdesc = ads_.createDesc( Attrib::StorageProvider::attribName(),
	    			     iopar, defstring, errmsgs );
    if ( !newdesc )
    {
	uiMSG().error( errmsgs->get(0) );
	return -1;
    }
    
    //TODO: we could maybe optimize by checking if stored desc already present
    Attrib::DescID did = const_cast<Attrib::DescSet*>(&ads_)->addDesc(newdesc);

    delete errmsgs;
    return did.asInt();
}
