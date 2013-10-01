/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "volprocsurfacelimitedfiller.h"
#include "uivolprocsurfacelimitedfiller.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emobject.h"
#include "ioman.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uihorauxdatasel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitable.h"
#include "uivolprocchain.h"
#include "volprocchain.h"
#include "zdomain.h"

namespace VolProc
{


uiStepDialog* uiSurfaceLimitedFiller::createInstance( uiParent* upt, Step* ps )
{
    mDynamicCastGet( SurfaceLimitedFiller*, hp, ps );
    if ( !hp ) return 0;

    return new uiSurfaceLimitedFiller( upt, hp );
}


static const char* collbls[] = { "Name", "Side", "Color", 0 };
static const int cNameCol = 0;
static const int cSideCol = 1;
static const int cColorCol = 2;


uiSurfaceLimitedFiller::uiSurfaceLimitedFiller( uiParent* p, 
					    SurfaceLimitedFiller* hp )
    : uiStepDialog( p, SurfaceLimitedFiller::sFactoryDisplayName(), hp )
    , surfacefiller_( hp )
{
    setHelpID( "dgb:104.0.4" );

    if ( !surfacefiller_ ) return;
	
    table_ = new uiTable( this, uiTable::Setup(4).rowgrow(true).fillrow(true)
	    .rightclickdisabled(true).selmode(uiTable::Single), 
	    "Surface Limits Table" );
    table_->setColumnLabels( collbls ); 
    table_->setLeftMargin( 0 );
    table_->setSelectionBehavior( uiTable::SelectRows );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setRowResizeMode( uiTable::Interactive );
    table_->setColumnStretchable( cNameCol, true );
    addbutton_ = new uiPushButton( this, "&Add",
	    mCB(this,uiSurfaceLimitedFiller,addSurfaceCB), false );
    addbutton_->attach( rightOf, table_ );
    
    removebutton_ = new uiPushButton( this, "&Remove", 
	    mCB(this,uiSurfaceLimitedFiller,removeSurfaceCB), false );
    removebutton_->attach( alignedBelow, addbutton_ );
    removebutton_->setSensitive( false );
    
    const int geosz = surfacefiller_->nrOfSurfaces();
    for ( int idx=0; idx<geosz; idx++ )
    {
	const MultiID* mid = surfacefiller_->getSurfaceID(idx);
	PtrMan<IOObj> ioobj = IOM().get(*mid);
	const char dir = surfacefiller_->getSurfaceFillSide(idx);
	addSurfaceTableEntry( *ioobj, false, dir );
    }

    uiHorizonAuxDataSel::HorizonAuxDataInfo auxdatainfo( true );
    const bool hasauxdata = auxdatainfo.mids_.size();
    const char* constantstr = "Constant";
    const char* fromhorattribstr = "From Horizon Data";
	
    usestartvalfld_ = new uiGenInput( this, "Start value", 
	    BoolInpSpec( !hasauxdata || surfacefiller_->usesStartValue(), 
			 constantstr, fromhorattribstr ) );
    usestartvalfld_->setSensitive( hasauxdata );
    usestartvalfld_->valuechanged.notify( 
	    mCB(this, uiSurfaceLimitedFiller,useStartValCB) );
    usestartvalfld_->attach( ensureBelow, table_ );
    startvalfld_ = new uiGenInput( this, "Start value constant", 
	    FloatInpSpec(surfacefiller_->getStartValue()) );
    startvalfld_->attach( alignedBelow, usestartvalfld_ );
    
    const MultiID* starthorid = surfacefiller_->getStartValueHorizonID();
    const MultiID& startmid = starthorid ? *starthorid : "-1";
    startgridfld_ = new uiHorizonAuxDataSel( this, startmid,
	    hp->getStartAuxdataIdx(), &auxdatainfo );
    startgridfld_->attach( alignedBelow, usestartvalfld_ );
    
    usegradientfld_ = new uiGenInput( this, "Gradient",
	    BoolInpSpec( !hasauxdata || surfacefiller_->usesGradientValue(),
			 constantstr, fromhorattribstr ) );
    usegradientfld_->setSensitive( hasauxdata );
    usegradientfld_->valuechanged.notify(
	    mCB(this,uiSurfaceLimitedFiller,useGradientCB) );
    usegradientfld_->attach( alignedBelow, startvalfld_ );
    BufferString gradientlabel = "Gradient constant ";
    gradientlabel += "[/"; 
    gradientlabel += SI().getZUnitString( false ); 
    gradientlabel += "]";
    float gradient = surfacefiller_->getGradient();
    if ( !mIsUdf(gradient) )
	gradient /= SI().zDomain().userFactor();
    gradientfld_ = new uiGenInput( this, gradientlabel.buf(), 
	    FloatInpSpec( gradient ) );
    gradientfld_->attach( alignedBelow, usegradientfld_ );
    
    const MultiID* gradhorid = surfacefiller_->getGradientHorizonID();
    const MultiID& gradmid = gradhorid ? *gradhorid : "-1";
    gradgridfld_ = new uiHorizonAuxDataSel( this, gradmid, 
	    hp->getGradAuxdataIdx(), &auxdatainfo );
    gradgridfld_->attach( alignedBelow, usegradientfld_ );
	
    StringListInpSpec str;
    str.addString( "Vertical" );
    //str.addString( "Normal" ); TODO 
    gradienttypefld_ = new uiGenInput( this, "Type", str );
    gradienttypefld_->attach( rightOf, usegradientfld_ );
    gradienttypefld_->display( false ); //!SI().zIsTime() ); 
   
    BufferString labl = "Reference ";
    labl += SI().zIsTime() ? "time" : "depth";
    userefdepthfld_ = new uiGenInput( this, labl,
	    BoolInpSpec(surfacefiller_->usesRefZValue(),constantstr,"Horizon"));
    userefdepthfld_->valuechanged.notify( 
	    mCB(this,uiSurfaceLimitedFiller,useRefValCB) );
    userefdepthfld_->attach( alignedBelow, gradientfld_ );

    BufferString refdepthlabel = ZDomain::SI().userName();
    refdepthlabel += " ";
    refdepthlabel += SI().getZUnitString( true );
    float refdepth = surfacefiller_->getRefZValue();
    if ( !mIsUdf(refdepth) ) refdepth *= SI().zDomain().userFactor();
    refdepthfld_ = new uiGenInput( this, refdepthlabel.buf(), 
	    FloatInpSpec( refdepth ) );
    refdepthfld_->attach( alignedBelow, userefdepthfld_ );
   
    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread = true;
    refhorizonfld_ = new uiIOObjSel( this, ctxt, "Horizon" );
    refhorizonfld_->attach( alignedBelow, userefdepthfld_ );
    if ( !surfacefiller_->usesRefZValue() && surfacefiller_->getRefHorizonID() )
	refhorizonfld_->setInput( *surfacefiller_->getRefHorizonID() );

    addNameFld( refhorizonfld_ );

    useStartValCB( 0 );
    useGradientCB( 0 );
    useRefValCB( 0 );
}


uiSurfaceLimitedFiller::~uiSurfaceLimitedFiller()
{}


void uiSurfaceLimitedFiller::addSurfaceCB( CallBacker* )
{
    PtrMan<CtxtIOObj> allhorio =  mMkCtxtIOObj(EMHorizon3D);
    PtrMan<uiIOObjSelDlg> dlg = new uiIOObjSelDlg( this, *allhorio, 0, true );
    if ( !dlg->go() )
	return;
 
    for ( int idx=0; idx<dlg->nrSel(); idx++ )
    {
	if ( surfacelist_.isPresent(dlg->selected(idx)) )
	    continue;
	
	PtrMan<IOObj> ioobj = IOM().get( dlg->selected(idx) );
	addSurfaceTableEntry( *ioobj, false, 0 );
    }
}


void uiSurfaceLimitedFiller::addSurfaceTableEntry( const IOObj& ioobj,
       						   bool isfault, char side )
{
    const int row = surfacelist_.size();
    if ( !row )
	removebutton_->setSensitive( true );
    
    if ( row==table_->nrRows() )
	table_->insertRows( row, 1 );

    BufferStringSet sidenms;
    if ( isfault )
    {
	sidenms.add("Right");
	sidenms.add("Left");
    }
    else
    {
	sidenms.add("Above");
	sidenms.add("Below");
    }
    
    uiComboBox* sidesel = new uiComboBox( 0, sidenms, 0 );
    sidesel->setCurrentItem( side==-1 ? 0 : 1 );
    
    table_->setCellObject( RowCol(row,cSideCol), sidesel );
    table_->setText( RowCol(row,cNameCol), ioobj.name() );
    table_->setCellReadOnly( RowCol(row,cNameCol), true );
    surfacelist_ += ioobj.key();

    Color col = Color::White();
    const EM::EMObject*  emobj = EM::EMM().getObject(
	    EM::EMM().getObjectID(ioobj.key()) );
    if ( emobj )
	col = emobj->preferredColor();
    else
    {
	//TODO: Get from horizon on disk
    }
	
    table_->setColor( RowCol(row,cColorCol), col );
    table_->setCellReadOnly( RowCol(row,cColorCol), true );
}


void uiSurfaceLimitedFiller::removeSurfaceCB( CallBacker* )
{
    const int currow = table_->currentRow();
    if ( currow==-1 ) return;

    if ( currow<surfacelist_.size() )
    	surfacelist_.removeSingle( currow );

    table_->removeRow( currow );
    if ( !surfacelist_.size() )
	removebutton_->setSensitive( false );
}


void uiSurfaceLimitedFiller::useStartValCB( CallBacker* )
{
    const bool useval = usestartvalfld_->getBoolValue();
    if ( !useval && !startgridfld_->nrHorizonsWithData() )
    {
	uiMSG().warning( "No Horizon data available for any horizon, \
			  could only use constant start value." );
	usestartvalfld_->setValue( true );
	return;
    }
    
    startvalfld_->display( useval );
    startgridfld_->display( !useval );
}


void uiSurfaceLimitedFiller::useGradientCB( CallBacker* )
{
    const bool useval = usegradientfld_->getBoolValue();
    if ( !useval && !gradgridfld_->nrHorizonsWithData() )
    {
	uiMSG().warning( "No Horizon data available for any horizon, \
			  could only use constant gradient." );
	usegradientfld_->setValue( true );
	return;
    }
    
    gradientfld_->display( useval );
    gradgridfld_->display( !useval );
}


void uiSurfaceLimitedFiller::useRefValCB( CallBacker* )
{
    const bool useval = userefdepthfld_->getBoolValue();
    refdepthfld_->display( useval );
    refhorizonfld_->display( !useval );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSurfaceLimitedFiller::acceptOK( CallBacker* cb )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    const bool usestartval = usestartvalfld_->getBoolValue();
    const bool usegradient = usegradientfld_->getBoolValue();
    const bool userefval = userefdepthfld_->getBoolValue();

    surfacefiller_->useStartValue( usestartval );
    surfacefiller_->useGradientValue( usegradient );
    surfacefiller_->useRefZValue( userefval );
    surfacefiller_->setGradientVertical( !gradienttypefld_->getIntValue() );

    TypeSet<char> sidesels;
    for ( int idx=0; idx<surfacelist_.size(); idx++ )
    {
	mDynamicCastGet(uiComboBox*, selbox, 
		table_->getCellObject(RowCol(idx,cSideCol)) );    
    	sidesels += selbox->currentItem() ? 1 : -1;
    }
    surfacefiller_->setSurfaces( surfacelist_, sidesels );

    if ( usestartval )
    {
	if ( mIsUdf(startvalfld_->getfValue()) )
	    mErrRet("Please provide the start value")
	
	surfacefiller_->setStartValue( startvalfld_->getfValue() );
    }
    else
    {
	if (!surfacefiller_->setStartValueHorizon(&startgridfld_->selectedID()))
	    mErrRet("Cannot set start value horizon")
	
	if ( startgridfld_->auxdataidx()<0 )
	    mErrRet("No Horizon data available")
	else
	    surfacefiller_->setStartAuxdataIdx( startgridfld_->auxdataidx() );
    }

    if ( usegradient )
    {
	if ( mIsUdf(gradientfld_->getfValue()) )
    	    mErrRet("Please provide the gradient")
	
	surfacefiller_->setGradient(
		gradientfld_->getfValue()*SI().zDomain().userFactor() );
    }
    else
    {
	if ( !surfacefiller_->setGradientHorizon(&gradgridfld_->selectedID()) )
	    mErrRet("Cannot set gradient horizon")

	if ( gradgridfld_->auxdataidx()<0 )
	    mErrRet("No Horizon data available")
	else
    	    surfacefiller_->setGradAuxdataIdx( gradgridfld_->auxdataidx() );
    }

    if ( userefval )
    {
	if ( mIsUdf(refdepthfld_->getfValue()) )
    	    mErrRet("Please provide the reference z value")
	
	surfacefiller_->setRefZValue(
		refdepthfld_->getfValue()/SI().zDomain().userFactor());
    }
    else
    {
	const IOObj* obj = refhorizonfld_->ioobj();
	if ( !obj )
	    mErrRet("Reference horizon does not exit")

	const MultiID mid = obj->key();	
	if ( !surfacefiller_->setRefHorizon( &mid ) )
	    mErrRet("Cannot set reference horizon")
    }
    
    return true;
}


};//namespace

