/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.cc,v 1.8 2003-08-15 13:19:42 nanne Exp $
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


const int cListHeight = 5;


uiIOSurface::uiIOSurface( uiParent* p )
    : uiGroup(p,"Surface selection")
    , ctio(*new CtxtIOObj(EMHorizonTranslator::ioContext()))
    , patchfld(0)
    , attribfld(0)
    , rgfld(0)
{
}


uiIOSurface::~uiIOSurface()
{
}


void uiIOSurface::mkAttribFld()
{
    attribfld = new uiLabeledListBox( this, "Calculated attributes", false,
					uiLabeledListBox::AboveMid );
    attribfld->box()->rightButtonClicked.notify( 
	    				mCB(this,uiIOSurface,deSelect) );
    attribfld->setPrefHeightInChar( cListHeight );
    attribfld->setStretch( 1, 1 );
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


void uiIOSurface::fillAttribFld( const ObjectSet<BufferString>& valnames )
{
    if ( !attribfld ) return;

    attribfld->box()->empty();
    for ( int idx=0; idx<valnames.size(); idx++)
	attribfld->box()->addItem( valnames[idx]->buf() );
    attribfld->box()->setSelected( 0 );
}


void uiIOSurface::fillPatchFld( const ObjectSet<BufferString>& patches )
{
    if ( !patchfld ) return;

    patchfld->box()->empty();
    for ( int idx=0; idx<patches.size(); idx++ )
	patchfld->box()->addItem( patches[idx]->buf() );
    patchfld->box()->selAll( true );
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

    int curitm = attribfld ? attribfld->box()->currentItem() : -1;
    if ( curitm >= 0 )
	sels.selvalues += curitm;
}


IOObj* uiIOSurface::selIOObj() const
{
    return ctio.ioobj;
}


void uiIOSurface::deSelect( CallBacker* )
{
    attribfld->box()->clear();
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


bool uiSurfaceOutSel::surfaceOnly() const
{
    return !savefld;
}


bool uiSurfaceOutSel::surfaceAndData() const
{
    return savefld ? !savefld->getBoolValue() : false;
}



uiSurfaceSel::uiSurfaceSel( uiParent* p, bool showattribfld )
    : uiIOSurface( p )
{
    mkObjFld( "Input Horizon", true );
    objfld->selectiondone.notify( mCB(this,uiIOSurface,objSel) );

    mkPatchFld( showattribfld );

    if ( showattribfld )
    {
	mkAttribFld();
	attribfld->attach( alignedBelow, objfld );
	patchfld->attach( rightTo, attribfld );
    }
    else
	patchfld->attach( alignedBelow, objfld );

    mkRangeFld();
    rgfld->attach( alignedBelow, showattribfld ? (uiObject*)attribfld
	    				       : (uiObject*)patchfld );

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
