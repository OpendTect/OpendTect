/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurface.cc,v 1.45 2007-12-18 14:58:16 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiiosurface.h"

#include "uibutton.h"
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
#include "survinfo.h"


const int cListHeight = 5;

#define mMakeCtxtIOObj(typ) \
    !strcmp(typ,EMHorizon2DTranslatorGroup::keyword) ? \
			*mMkCtxtIOObj(EMHorizon2D) : \
    !strcmp(typ,EMHorizon3DTranslatorGroup::keyword) ? \
			*mMkCtxtIOObj(EMHorizon3D) : *mMkCtxtIOObj(EMFault)

uiIOSurface::uiIOSurface( uiParent* p, bool forread_, const char* typ )
    : uiGroup(p,"Surface selection")
    , ctio( mMakeCtxtIOObj(typ) )
    , sectionfld(0)
    , attribfld(0)
    , rgfld(0)
    , attrSelChange(this)
    , forread(forread_)
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
    attribfld->setStretch( 1, 1 );
    attribfld->box()->selectionChanged.notify( mCB(this,uiIOSurface,attrSel) );
}


void uiIOSurface::mkSectionFld( bool labelabove )
{
    sectionfld = new uiLabeledListBox( this, "Available patches", true,
				     labelabove ? uiLabeledListBox::AboveMid 
				     		: uiLabeledListBox::LeftTop );
//  sectionfld->setPrefHeightInChar( cListHeight );
    sectionfld->setStretch( 1, 1 );
    sectionfld->box()->selectionChanged.notify( 
	    				mCB(this,uiIOSurface,ioDataSelChg) );
}


void uiIOSurface::mkRangeFld()
{
    rgfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withstep(true) );
    rgfld->butPush.notify( mCB(this,uiIOSurface,ioDataSelChg) );
}


void uiIOSurface::mkObjFld( const char* lbl )
{
    ctio.ctxt.forread = forread;
    objfld = new uiIOObjSel( this, ctio, lbl );
    if ( forread )
	objfld->selectiondone.notify( mCB(this,uiIOSurface,objSel) );
}


void uiIOSurface::fillFields( const MultiID& id )
{
    EM::SurfaceIOData sd;

    if ( forread )
    {
	const char* res = EM::EMM().getSurfaceData( id, sd );
	if ( res )
	    { uiMSG().error( res ); return; }
    }
    else
    {
	const EM::ObjectID emid = EM::EMM().getObjectID( id );
	mDynamicCastGet(EM::Surface*,emsurf,EM::EMM().getObject(emid));
	if ( emsurf )
	    sd.use(*emsurf);
	else
	{
	    uiMSG().error( "Surface not loaded" );
	    return;
	}
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
    sectionfld->box()->selectAll( true );
}


void uiIOSurface::fillRangeFld( const HorSampling& hrg )
{
    if ( !rgfld ) return;
    uiBinIDSubSel::Data subseldata = rgfld->data();
    subseldata.cs_.hrg = subseldata.allowedrange_.hrg = hrg;
    rgfld->setData( subseldata );
}


bool uiIOSurface::haveAttrSel() const
{
    for ( int idx=0; idx<attribfld->box()->size(); idx++ )
    {
	if ( attribfld->box()->isSelected(idx) )
	    return true;
    }
    return false;
}


void uiIOSurface::getSelection( EM::SurfaceIODataSelection& sels ) const
{
    if ( !rgfld || !rgfld->data().isRg() )
	sels.rg.init( false );
    else
	sels.rg = rgfld->data().cs_.hrg;

    if ( SI().sampling(true) != SI().sampling(false) )
    {
	if ( sels.rg.isEmpty() )
	    sels.rg.init( true );
	sels.rg.limitTo( SI().sampling(true).hrg );
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


void uiIOSurface::attrSel( CallBacker* )
{
    attrSelChange.trigger();
}


uiSurfaceWrite::uiSurfaceWrite( uiParent* p, const EM::Surface& surf_, 
				const char* typ )
    : uiIOSurface(p,false,typ)
{
    if ( typ != EMHorizon2DTranslatorGroup::keyword )
    {
	if ( surf_.nrSections() > 1 )
	    mkSectionFld( false );

	mkRangeFld();
	if ( sectionfld )
	    rgfld->attach( alignedBelow, sectionfld );
    }
    
    mkObjFld( "Output Surface" );
    if ( rgfld )
    {
	objfld->attach( alignedBelow, rgfld );
	setHAlignObj( rgfld );
    }

    replacefld = new uiCheckBox( this, "Replace in tree" );
    replacefld->attach( alignedBelow, objfld );

    fillFields( surf_.multiID() );

    replacefld->setChecked( true );
    ioDataSelChg( 0 );
}


bool uiSurfaceWrite::processInput()
{
    if ( sectionfld && !sectionfld->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    if ( !objfld->commitInput(true) )
    {
	uiMSG().error( "Please select output" );
	return false;
    }

    return true;
}


bool uiSurfaceWrite::replaceInTree() const       
{ return replacefld->isChecked(); }


void uiSurfaceWrite::ioDataSelChg( CallBacker* )
{
    bool issubsel = sectionfld &&
		    sectionfld->box()->size()!=sectionfld->box()->nrSelected();

    if ( rgfld && rgfld->data().isRg() )
    {
	const HorSampling& hrg = rgfld->data().cs_.hrg;
	const HorSampling& maxhrg = rgfld->data().allowedrange_.hrg;
	issubsel = issubsel || hrg.inlRange()!=maxhrg.inlRange();
	issubsel = issubsel || hrg.crlRange()!=maxhrg.crlRange();
    }

    if ( replacefld && issubsel )
    {
	replacefld->setChecked( false );
	replacefld->setSensitive( false );
    }
    else if ( replacefld && !replacefld->sensitive() )
    {
	replacefld->setSensitive( true );
	replacefld->setChecked( true );
    }
}


uiSurfaceRead::uiSurfaceRead( uiParent* p, const char* typ, 
			      bool showattribfld )
    : uiIOSurface(p,true,typ)
{
    mkObjFld( "Input Surface" );

    mkSectionFld( showattribfld );

    if ( objfld->ctxtIOObj().ioobj )
	objSel(0);

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


void uiSurfaceRead::setIOObj( const MultiID& mid )
{
    ctio.setObj( mid );
    objfld->updateInput();
    objSel(0);
}


bool uiSurfaceRead::processInput()
{
    if ( !objfld->commitInput(false) )
    {
	uiMSG().error( "Please select input" );
	return false;
    }

    if ( sectionfld && !sectionfld->box()->nrSelected() )
    {
	uiMSG().error( "Please select at least one patch" );
	return false;
    }

    return true;
}
