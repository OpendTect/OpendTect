/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.cc,v 1.1 2003-07-16 09:56:21 nanne Exp $
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


uiIOSurface::uiIOSurface( uiParent* p, const EM::Horizon* hor_ )
    : uiGroup(p,"Surface selection")
    , ctio(*new CtxtIOObj(EMHorizonTranslator::ioContext()))
    , hor(hor_)
    , readgrp(0)
    , writegrp(0)
    , patchfld(0)
    , entrylist(0)
{
    if ( hor->nrAuxData() )
    {
	writegrp = new uiGroup( this, "Write group" );
	attrnmfld = new uiGenInput( writegrp, "Attribute" );
	attrnmfld->setText( hor->auxDataName(0) );

	savefld = new uiGenInput( writegrp, "Save", 
	    		BoolInpSpec("Horizon and attribute","Horizon only") );
	savefld->attach( alignedBelow, attrnmfld );
    }
    
    createSharedFields( hor->nrPatches() > 1 );

    outfld = new uiIOObjSel( this, ctio, "Output Horizon" );
    outfld->attach( alignedBelow, rgfld );

    fillFields( hor->id() );
}


uiIOSurface::uiIOSurface( uiParent* p, CtxtIOObj& c )
    : uiGroup(p,"Surface selection")
    , ctio(c)
    , hor(0)
    , readgrp(0)
    , writegrp(0)
    , patchfld(0)
    , entrylist(0)    
{
    readgrp = new uiGroup( this, "Read group" );
    IOM().to( ctio.ctxt.stdSelKey() );
    entrylist = new IODirEntryList( IOM().dirPtr(), ctio.ctxt );
    if ( ctio.ioobj )
	entrylist->setSelected( ctio.ioobj->key() );
    entrylist->setName( "Select" );
    objlistfld = new uiLabeledListBox( readgrp, entrylist->Ptr(), false,
				       uiLabeledListBox::AboveMid );
    objlistfld->setPrefHeightInChar( 8 );
    objlistfld->setStretch( 1, 1 );
    objlistfld->box()->selectionChanged.notify( 
					    mCB(this,uiIOSurface,selChg) );

    attrlistfld = new uiLabeledListBox( readgrp, 
					"Calculated attributes", true,
					uiLabeledListBox::AboveMid );
    attrlistfld->setPrefHeightInChar( 8 );
    attrlistfld->setStretch( 1, 1 );
    attrlistfld->attach( rightTo, objlistfld );

    createSharedFields( anyHorWithPatches() );

    selChg(0);
}


void uiIOSurface::createSharedFields( bool withpatches )
{
    if ( withpatches )
    {
	patchfld = new uiLabeledListBox( this, "Available patches", true,
	       				 uiLabeledListBox::AboveMid );
	patchfld->setPrefHeightInChar( 8 );
	patchfld->setStretch( 1, 1 );
	if ( readgrp )
	    patchfld->attach( rightTo, readgrp );
	else if ( writegrp )
	    patchfld->attach( alignedBelow, writegrp );

    }

    rgfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withstep() );
    if ( readgrp )
	rgfld->attach( alignedBelow, readgrp );
    else if ( patchfld )
	rgfld->attach( alignedBelow, patchfld );
    else if ( writegrp )
	rgfld->attach( alignedBelow, writegrp );
}


uiIOSurface::~uiIOSurface()
{
    delete entrylist;
}


bool uiIOSurface::anyHorWithPatches()
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


void uiIOSurface::fillPatchField( ObjectSet<BufferString> patches )
{
    if ( !patchfld ) return;

    patchfld->box()->empty();
    for ( int idx=0; idx<patches.size(); idx++ )
	patchfld->box()->addItem( patches[idx]->buf() );
    patchfld->box()->setSelected( 0 );
}


void uiIOSurface::fillAttrField( ObjectSet<BufferString> valnames )
{
    attrlistfld->box()->empty();
    for ( int idx=0; idx<valnames.size(); idx++)
	attrlistfld->box()->addItem( valnames[idx]->buf() );
    attrlistfld->box()->setSelected( 0 );
}


void uiIOSurface::fillRangeField( const BinIDSampler& bids )
{
    rgfld->setInput( bids );
}


void uiIOSurface::selChg( CallBacker* )
{
    const int curitm = objlistfld->box()->currentItem();
    if ( !entrylist || curitm < 0 ) return;
    
    entrylist->setCurrent( objlistfld->box()->currentItem() );
    IOObj* ioobj = entrylist->selected();
    if ( !ioobj ) return;

    ctio.setObj( ioobj->clone() );
    fillFields( ioobj->key() );
}


void uiIOSurface::fillFields( const MultiID& id )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( id, sd );

    if ( readgrp ) 
	fillAttrField( sd.valnames );

    fillPatchField( sd.patches );
    fillRangeField( sd.rg );
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
    if ( writegrp && !savefld->getBoolValue() )
	return;

    int curitm = readgrp ? attrlistfld->box()->currentItem() : 0;
    if ( curitm >= 0 )
	sels.selvalues += curitm;
}


IOObj* uiIOSurface::selIOObj() const
{
    return ctio.ioobj->clone();
}

