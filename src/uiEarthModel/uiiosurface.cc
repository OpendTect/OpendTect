/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.cc,v 1.23 2004-08-09 14:09:31 kristofer Exp $
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
#include "emsurface.h"
#include "emsurfacetr.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "uimsg.h"


const int cListHeight = 5;


uiIOSurface::uiIOSurface( uiParent* p, bool ishor )
    : uiGroup(p,"Surface selection")
    , ctio( ishor ? *mMkCtxtIOObj(EMHorizon) : *mMkCtxtIOObj(EMFault))
    , sectionfld(0)
    , attribfld(0)
    , rgfld(0)
{
}


uiIOSurface::~uiIOSurface()
{
    delete ctio.ioobj; delete &ctio;
}


void uiIOSurface::mkAttribFld()
{
    attribfld = new uiLabeledListBox( this, "Calculated attributes", true,
					uiLabeledListBox::AboveMid );
    attribfld->setPrefHeightInChar( cListHeight );
    attribfld->setStretch( 1, 1 );
}


void uiIOSurface::mkSectionFld( bool labelabove )
{
    sectionfld = new uiLabeledListBox( this, "Available patches", true,
				     labelabove ? uiLabeledListBox::AboveMid 
				     		: uiLabeledListBox::LeftTop );
    sectionfld->setPrefHeightInChar( cListHeight );
    sectionfld->setStretch( 1, 1 );
}


void uiIOSurface::mkRangeFld()
{
    rgfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withstep() );
}


void uiIOSurface::mkObjFld( const char* lbl, bool imp )
{
    ctio.ctxt.forread = imp;
    objfld = new uiIOObjSel( this, ctio, lbl );
    if ( imp )
	objfld->selectiondone.notify( mCB(this,uiIOSurface,objSel) );
}


void uiIOSurface::fillFields( const MultiID& id )
{
    EM::SurfaceIOData sd;
    const EM::ObjectID emid = EM::EMM().multiID2ObjectID( id );
    mDynamicCastGet( EM::Surface*, emsurf, EM::EMM().getObject(emid) );
    if ( emsurf )
	sd.use(*emsurf);
    else
    {
	const char* res = EM::EMM().getSurfaceData( id, sd );
	if ( res )
	    { uiMSG().error( res ); return; }
    }

    fillAttribFld( sd.valnames );
    fillSectionFld( sd.sections );
    fillRangeFld( sd.rg );
}


void uiIOSurface::fillAttribFld( const BufferStringSet& valnames )
{
    if ( !attribfld ) return;

    attribfld->box()->empty();
    for ( int idx=0; idx<valnames.size(); idx++)
	attribfld->box()->addItem( valnames[idx]->buf() );
}


void uiIOSurface::fillSectionFld( const BufferStringSet& sections )
{
    if ( !sectionfld ) return;

    sectionfld->box()->empty();
    for ( int idx=0; idx<sections.size(); idx++ )
	sectionfld->box()->addItem( sections[idx]->buf() );
    sectionfld->box()->selAll( true );
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

    sels.selsections.erase();
    int nrsections = sectionfld ? sectionfld->box()->size() : 1;
    for ( int idx=0; idx<nrsections; idx++ )
    {
	if ( nrsections == 1 || sectionfld->box()->isSelected(idx) )
	    sels.selsections += idx;
    }

    sels.selvalues.erase();
    int nrattribs = attribfld ? attribfld->box()->size() : 0;
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	if ( attribfld->box()->isSelected(idx) )
	    sels.selvalues += idx;
    }
}


IOObj* uiIOSurface::selIOObj() const
{
    return ctio.ioobj;
}


void uiIOSurface::objSel( CallBacker* )
{
    IOObj* ioobj = objfld->ctxtIOObj().ioobj;
    if ( !ioobj ) return;

    fillFields( ioobj->key() );
}




uiSurfaceWrite::uiSurfaceWrite( uiParent* p, const EM::Surface& surf_, 
				bool ishor )
    : uiIOSurface(p,ishor)
    , savefld(0)
{
    if ( surf_.auxdata.nrAuxData() )
    {
	attrnmfld = new uiGenInput( this, "Attribute" );
	attrnmfld->setText( surf_.auxdata.auxDataName(0) );

	savefld = new uiGenInput( this, "Save", 
	    		BoolInpSpec("Attribute only","Surface and attribute") );
	savefld->attach( alignedBelow, attrnmfld );
	savefld->valuechanged.notify( mCB(this,uiSurfaceWrite,savePush) );
    }

    if ( surf_.geometry.nrSections() > 1 )
    {
	mkSectionFld( false );
	if ( savefld ) sectionfld->attach( alignedBelow, savefld );
    }

    mkRangeFld();
    if ( sectionfld )
	rgfld->attach( alignedBelow, sectionfld );
    else if ( savefld )
	rgfld->attach( alignedBelow, savefld );

    mkObjFld( "Output Surface", false );
    objfld->attach( alignedBelow, rgfld );

    fillFields( surf_.multiID() );
    setHAlignObj( rgfld );

    savePush(0);
}


bool uiSurfaceWrite::processInput()
{
    if ( sectionfld && !sectionfld->box()->nrSelected() )
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


const char* uiSurfaceWrite::auxDataName() const
{
    return attrnmfld->text();
}


void uiSurfaceWrite::savePush( CallBacker* )
{
    if ( savefld )
	objfld->display( !savefld->getBoolValue() );
}


bool uiSurfaceWrite::saveAuxDataOnly() const
{
    return savefld ? savefld->getBoolValue() : false;
}


bool uiSurfaceWrite::surfaceOnly() const
{
    return !savefld;
}


bool uiSurfaceWrite::surfaceAndData() const
{
    return savefld ? !savefld->getBoolValue() : false;
}



uiSurfaceRead::uiSurfaceRead( uiParent* p, bool ishor, bool showattribfld )
    : uiIOSurface(p,ishor)
{
    mkObjFld( "Input Surface", true );

    mkSectionFld( showattribfld );

    if ( showattribfld )
    {
	mkAttribFld();
	attribfld->attach( alignedBelow, objfld );
	sectionfld->attach( rightTo, attribfld );
    }
    else
	sectionfld->attach( alignedBelow, objfld );

    mkRangeFld();
    rgfld->attach( alignedBelow, showattribfld ? (uiObject*)attribfld
	    				       : (uiObject*)sectionfld );

    setHAlignObj( rgfld );
}


uiSurfaceRead::~uiSurfaceRead()
{
}


bool uiSurfaceRead::processInput()
{
    if ( sectionfld && !sectionfld->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    return true;
}
