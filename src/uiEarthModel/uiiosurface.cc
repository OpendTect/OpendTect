/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.cc,v 1.4 2003-08-04 13:24:08 nanne Exp $
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
#include "uimsg.h"


const int cListHeight = 8;


uiIOSurface::uiIOSurface( uiParent* p )
    : uiGroup(p,"Surface selection")
    , ctio(*new CtxtIOObj(EMHorizonTranslator::ioContext()))
    , patchfld(0)
    , attrlistfld(0)
    , rgfld(0)
{
}


uiIOSurface::~uiIOSurface()
{
}


void uiIOSurface::mkAttribFld()
{
    attrlistfld = new uiLabeledListBox( this, "Calculated attributes", false,
					uiLabeledListBox::AboveMid );
    attrlistfld->box()->rightButtonClicked.notify( 
	    				mCB(this,uiIOSurface,deSelect) );
    attrlistfld->setPrefHeightInChar( cListHeight );
    attrlistfld->setStretch( 1, 1 );
}


void uiIOSurface::mkPatchFld( bool labelabove )
{
    patchfld = new uiLabeledListBox( this, "Available patches", true,
				     labelabove ? uiLabeledListBox::AboveMid 
				     		: uiLabeledListBox::LeftTop );
    patchfld->setPrefHeightInChar( cListHeight );
    patchfld->setStretch( 1, 1 );
}


void uiIOSurface::mkRangeFld()
{
    rgfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withstep() );
}


void uiIOSurface::mkObjFld( const char* lbl, bool imp )
{
    ctio.ctxt.forread = imp;
    objfld = new uiIOObjSel( this, ctio, lbl );
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
    patchfld->display( patches.size() > 1 );
}


void uiIOSurface::fillRangeFld( const BinIDSampler& bids )
{
    if ( !rgfld ) return;

    rgfld->setInput( bids );
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels )
{
    if ( rgfld )
    {
	const BinIDRange* rg = rgfld->getRange();
	mDynamicCastGet(const BinIDSampler*,smpl,rg);
	sels.rg = *smpl;
    }

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


void uiIOSurface::deSelect( CallBacker* )
{
    attrlistfld->box()->clear();
}


void uiIOSurface::objSel( CallBacker* )
{
    IOObj* ioobj = objfld->ctxtIOObj().ioobj;
    if ( !ioobj ) return;

    fillFields( ioobj->key() );
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
	mkPatchFld( false );
	patchfld->attach( alignedBelow, savefld );
    }

    mkRangeFld();
    if ( patchfld )
	rgfld->attach( alignedBelow, patchfld );
    else if ( savefld )
	rgfld->attach( alignedBelow, savefld );

    mkObjFld( "Output Horizon", false );
    objfld->attach( alignedBelow, rgfld );

    fillFields( hor_.id() );
    setHAlignObj( rgfld );

    savePush(0);
}


bool uiSurfaceOutSel::processInput()
{
    if ( patchfld && !patchfld->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    if ( !saveAuxDataOnly() && !objfld->commitInput( true ) )
    {
	uiMSG().error( "Please select output" );
	return false;
    }

    return true;
}


const char* uiSurfaceOutSel::auxDataName() const
{
    return attrnmfld->text();
}


void uiSurfaceOutSel::savePush( CallBacker* )
{
    if ( savefld )
	objfld->display( !savefld->getBoolValue() );
}


bool uiSurfaceOutSel::saveAuxDataOnly() const
{
    return savefld ? savefld->getBoolValue() : false;
}




uiSurfaceAuxSel::uiSurfaceAuxSel( uiParent* p, const MultiID& emid )
    : uiIOSurface( p )
{
    mkAttribFld();

    fillFields( emid );
}


uiSurfaceSel::uiSurfaceSel( uiParent* p )
    : uiIOSurface( p )
{
    mkObjFld( "Input Horizon", true );
    objfld->selectiondone.notify( mCB(this,uiIOSurface,objSel) );

    mkAttribFld();
    attrlistfld->attach( alignedBelow, objfld );

    mkPatchFld( true );
    patchfld->attach( rightTo, attrlistfld );

    mkRangeFld();
    rgfld->attach( alignedBelow, attrlistfld );

    setHAlignObj( rgfld );
}


uiSurfaceSel::~uiSurfaceSel()
{
}


bool uiSurfaceSel::processInput()
{
    if ( patchfld && !patchfld->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    return true;
}
