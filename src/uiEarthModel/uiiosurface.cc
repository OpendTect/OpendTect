/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.cc,v 1.2 2003-07-29 13:03:09 nanne Exp $
________________________________________________________________________

-*/

#include "uiiosurface.h"

#include "uigeninput.h"
#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "uiioobjsel.h"
#include "ioman.h"
#include "iodirentry.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "emhorizontransl.h"
#include "emsurfaceiodata.h"


uiIOSurface::uiIOSurface( uiParent* p )
    : uiGroup(p,"Surface selection")
    , ctio(*new CtxtIOObj(EMHorizonTranslator::ioContext()))
    , patchfld(0)
    , attrlistfld(0)
{
}


uiIOSurface::~uiIOSurface()
{
}


void uiIOSurface::mkAttribFld()
{
    attrlistfld = new uiLabeledListBox( this, "Calculated attributes", true,
					uiLabeledListBox::AboveMid );
    attrlistfld->setPrefHeightInChar( 8 );
    attrlistfld->setStretch( 1, 1 );
}


void uiIOSurface::mkPatchFld()
{
    patchfld = new uiLabeledListBox( this, "Available patches", true,
				     uiLabeledListBox::AboveMid );
    patchfld->setPrefHeightInChar( 8 );
    patchfld->setStretch( 1, 1 );
}


void uiIOSurface::mkRangeFld()
{
    rgfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withstep() );
}


void uiIOSurface::fillFields( const MultiID& id )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( id, sd );

    fillAttribFld( sd.valnames );
    fillPatchFld( sd.patches );
    fillRangeFld( sd.rg );
}


void uiIOSurface::fillAttribFld( ObjectSet<BufferString> valnames )
{
    if ( !attrlistfld ) return;

    attrlistfld->box()->empty();
    for ( int idx=0; idx<valnames.size(); idx++)
	attrlistfld->box()->addItem( valnames[idx]->buf() );
    attrlistfld->box()->selAll( false );
}


void uiIOSurface::fillPatchFld( ObjectSet<BufferString> patches )
{
    if ( !patchfld ) return;

    patchfld->box()->empty();
    for ( int idx=0; idx<patches.size(); idx++ )
	patchfld->box()->addItem( patches[idx]->buf() );
    patchfld->box()->selAll( true );
}


void uiIOSurface::fillRangeFld( const BinIDSampler& bids )
{
    rgfld->setInput( bids );
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels )
{
    const BinIDRange* rg = rgfld->getRange();
    mDynamicCastGet(const BinIDSampler*,smpl,rg);
    sels.rg = *smpl;

    sels.selpatches.erase();
    int nrpatches = patchfld ? patchfld->box()->size() : 1;
    for ( int idx=0; idx<nrpatches; idx++ )
    {
	if ( nrpatches == 1 || patchfld->box()->isSelected(idx) )
	    sels.selpatches += idx;
    }

    sels.selvalues.erase();

    int curitm = attrlistfld ? attrlistfld->box()->currentItem() : 0;
    if ( curitm >= 0 )
	sels.selvalues += curitm;
}


IOObj* uiIOSurface::selIOObj() const
{
    return ctio.ioobj->clone();
}



uiSurfaceOutSel::uiSurfaceOutSel( uiParent* p, const EM::Horizon& hor_ )
    : uiIOSurface(p)
    , savefld(0)
{
    if ( hor_.nrAuxData() )
    {
	attrnmfld = new uiGenInput( this, "Attribute" );
	attrnmfld->setText( hor_.auxDataName(0) );

	savefld = new uiGenInput( this, "Save", 
	    		BoolInpSpec("Attribute only","Horizon and attribute") );
	savefld->attach( alignedBelow, attrnmfld );
	savefld->valuechanged.notify( mCB(this,uiSurfaceOutSel,savePush) );
    }

    if ( hor_.nrPatches() > 1 )
    {
	mkPatchFld();
	patchfld->attach( rightTo, attrlistfld );
    }

    mkRangeFld();
    if ( patchfld )
	rgfld->attach( alignedBelow, patchfld );
    else if ( savefld )
	rgfld->attach( alignedBelow, savefld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Horizon" );
    outfld->attach( alignedBelow, rgfld );

    fillFields( hor_.id() );
    savePush(0);
}


void uiSurfaceOutSel::processInput()
{
    outfld->commitInput( true );
}


const char* uiSurfaceOutSel::auxDataName() const
{
    return attrnmfld->text();
}


void uiSurfaceOutSel::savePush( CallBacker* )
{
    if ( savefld )
	outfld->display( !savefld->getBoolValue() );
}


bool uiSurfaceOutSel::saveAuxDataOnly() const
{
    return savefld ? savefld->getBoolValue() : false;
}




uiSurfaceAuxSel::uiSurfaceAuxSel( uiParent* p, const MultiID& emid )
    : uiIOSurface( p )
{
    mkAttribFld();

    mkRangeFld();
    rgfld->attach( alignedBelow, attrlistfld );

    fillFields( emid );
}


uiSurfaceSel::uiSurfaceSel( uiParent* p )
    : uiIOSurface( p )
{
    IOM().to( ctio.ctxt.stdSelKey() );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    if ( ctio.ioobj )
	entrylist->setSelected( ctio.ioobj->key() );
    entrylist->setName( "Select" );
    objlistfld = new uiLabeledListBox( this, entrylist->Ptr(), false,
				       uiLabeledListBox::AboveMid );
    objlistfld->setPrefHeightInChar( 8 );
    objlistfld->setStretch( 1, 1 );
    objlistfld->box()->selectionChanged.notify( mCB(this,uiSurfaceSel,selChg) );

    mkAttribFld();
    attrlistfld->attach( rightTo, objlistfld );

    if ( anyHorWithPatches() )
    {
	mkPatchFld();
	patchfld->attach( rightTo, attrlistfld );
    }

    mkRangeFld();
    rgfld->attach( alignedBelow, objlistfld );

    selChg(0);
}


uiSurfaceSel::~uiSurfaceSel()
{
    delete entrylist;
}


void uiSurfaceSel::selChg( CallBacker* )
{
    const int curitm = objlistfld->box()->currentItem();
    if ( !entrylist || curitm < 0 ) return;
    
    entrylist->setCurrent( objlistfld->box()->currentItem() );
    IOObj* ioobj = entrylist->selected();
    if ( !ioobj ) return;

    ctio.setObj( ioobj->clone() );
    fillFields( ioobj->key() );
}


bool uiSurfaceSel::anyHorWithPatches()
{
    int maxnrpatches = 1;
    for ( int idx=0; idx<entrylist->size(); idx++ )
    {
	IOObj* ioobj = (*entrylist)[idx]->ioobj;
	EM::SurfaceIOData sd;
	EM::EMM().getSurfaceData( ioobj->key(), sd );
	
	int nrpatches = sd.patches.size();
	if ( nrpatches > maxnrpatches )
	    maxnrpatches = nrpatches;
    }

    return maxnrpatches > 1;
}

