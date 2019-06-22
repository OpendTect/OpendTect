/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2010
-*/


#include "volprocsurfacelimitedfiller.h"
#include "uivolprocsurfacelimitedfiller.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emobject.h"
#include "keystrs.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uihorauxdatasel.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uitable.h"
#include "uivolprocchain.h"
#include "volprocchain.h"
#include "zdomain.h"
#include "uivolprocregionfiller.h"
#include "od_helpids.h"

namespace VolProc
{

uiStepDialog* uiSurfaceLimitedFiller::createInstance( uiParent* p, Step* step,
						      bool is2d )
{
    mDynamicCastGet(SurfaceLimitedFiller*,slf,step)
    return slf ? new uiSurfaceLimitedFiller( p, slf, is2d ) : 0;
}


static const int cNameCol = 0;
static const int cSideCol = 1;
static const int cColorCol = 2;


uiSurfaceLimitedFiller::uiSurfaceLimitedFiller( uiParent* p,
						SurfaceLimitedFiller* slf,
						bool is2d )
    : uiStepDialog( p, SurfaceLimitedFiller::sFactoryDisplayName(), slf, is2d )
    , surfacefiller_(slf)
    , table_(0)
    , usestartvalfld_(0),usegradientfld_(0),gradienttypefld_(0)
    , startgridfld_(0),gradgridfld_(0)
{
    setHelpKey( mODHelpKey(mSurfaceLimitedFillerHelpID) );

    if ( !surfacefiller_ ) return;

    table_ = new uiTable( this, uiTable::Setup(4).rowgrow(true).fillrow(true)
	    .rightclickdisabled(true).selmode(uiTable::Single),
	    "Surface Limits Table" );
    uiStringSet collbls;
    collbls.add( uiStrings::sName() )
	   .add( tr("Side") )
	   .add( uiStrings::sColor() );
    table_->setColumnLabels( collbls );
    table_->setLeftMargin( 0 );
    table_->setSelectionBehavior( uiTable::SelectRows );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setRowResizeMode( uiTable::Interactive );
    table_->setColumnStretchable( cNameCol, true );
    addbutton_ = new uiPushButton( this, uiStrings::sAdd(),
	    mCB(this,uiSurfaceLimitedFiller,addSurfaceCB), false );
    addbutton_->attach( rightOf, table_ );

    removebutton_ = new uiPushButton( this, uiStrings::sRemove(),
	    mCB(this,uiSurfaceLimitedFiller,removeSurfaceCB), false );
    removebutton_->attach( alignedBelow, addbutton_ );
    removebutton_->setSensitive( false );

    const int geosz = surfacefiller_->nrOfSurfaces();
    for ( int idx=0; idx<geosz; idx++ )
    {
	const DBKey* mid = surfacefiller_->getSurfaceID(idx);
	PtrMan<IOObj> ioobj = mid->getIOObj();
	const char dir = surfacefiller_->getSurfaceFillSide(idx);
	addSurfaceTableEntry( *ioobj, false, dir );
    }

    startvalfld_ = new uiGenInput( this, tr("Start value constant"),
	    FloatInpSpec(surfacefiller_->getStartValue()) );

    float gradient = surfacefiller_->getGradient();
    if ( !mIsUdf(gradient) )
	gradient /= SI().zDomain().userFactor();
    gradientfld_ = new uiGenInput( this, uiRegionFiller::sGradientLabel(),
	    FloatInpSpec( gradient ) );

    if ( is2d_ )
    {
	startvalfld_->attach( ensureBelow, table_ );
	gradientfld_->attach( alignedBelow, startvalfld_ );
    }
    else
    {
	uiHorizonAuxDataSel::HorizonAuxDataInfo auxdatainfo( true, this );
	const bool hasauxdata = auxdatainfo.dbkys_.size();
	const uiString fromhorattribstr = tr("From Horizon Data");

	usestartvalfld_ = new uiGenInput( this, tr("Start value"),
		BoolInpSpec( !hasauxdata || surfacefiller_->usesStartValue(),
			 uiStrings::sConstant(false), fromhorattribstr ) );
	usestartvalfld_->setSensitive( hasauxdata );
	usestartvalfld_->valuechanged.notify(
		mCB(this, uiSurfaceLimitedFiller,useStartValCB) );
	usestartvalfld_->attach( ensureBelow, table_ );
	startvalfld_->attach( alignedBelow, usestartvalfld_ );

	const DBKey* starthorid = surfacefiller_->getStartValueHorizonID();
	const DBKey startmid = starthorid ? *starthorid : DBKey::getInvalid();
	startgridfld_ = new uiHorizonAuxDataSel( this, startmid,
		slf->getStartAuxdataIdx(), &auxdatainfo );
	startgridfld_->attach( alignedBelow, usestartvalfld_ );

	const uiString gradientsurfdatalabel = SI().zDomain().isDepth()
	    ? fromhorattribstr
	    : toUiString("%1 (/%2)").arg( fromhorattribstr )
				  .arg(uiStrings::sTimeUnitString());

	usegradientfld_ = new uiGenInput( this, uiStrings::sGradient(),
		BoolInpSpec( !hasauxdata || surfacefiller_->usesGradientValue(),
			 uiStrings::sConstant(false), gradientsurfdatalabel ) );
	usegradientfld_->setSensitive( hasauxdata );
	usegradientfld_->valuechanged.notify(
		mCB(this,uiSurfaceLimitedFiller,useGradientCB) );
	usegradientfld_->attach( alignedBelow, startvalfld_ );
	gradientfld_->attach( alignedBelow, usegradientfld_ );

	const DBKey* gradhorid = surfacefiller_->getGradientHorizonID();
	const DBKey gradmid = gradhorid ? *gradhorid : DBKey::getInvalid();
	gradgridfld_ = new uiHorizonAuxDataSel( this, gradmid,
		slf->getGradAuxdataIdx(), &auxdatainfo );
	gradgridfld_->attach( alignedBelow, usegradientfld_ );

	StringListInpSpec str;
	str.addString( uiStrings::sVertical() );
	//str.addString( uiStrings::sNormal() ); TODO
	gradienttypefld_ = new uiGenInput( this, uiStrings::sType(), str );
	gradienttypefld_->attach( rightOf, usegradientfld_ );
	gradienttypefld_->display( false ); //!SI().zIsTime() );
    }

    uiString labl = tr("Reference %1")
	.arg( SI().zIsTime() ? uiStrings::sTime() : uiStrings::sDepth() );
    userefdepthfld_ = new uiGenInput( this, labl,
	    BoolInpSpec(surfacefiller_->usesRefZValue(),
		uiStrings::sConstant(true),uiStrings::sHorizon()));
    userefdepthfld_->valuechanged.notify(
	    mCB(this,uiSurfaceLimitedFiller,useRefValCB) );
    userefdepthfld_->attach( alignedBelow, gradientfld_ );

    float refdepth = surfacefiller_->getRefZValue();
    if ( !mIsUdf(refdepth) ) refdepth *= SI().zDomain().userFactor();
    refdepthfld_ = new uiGenInput( this, ZDomain::SI().getLabel(),
	    FloatInpSpec( refdepth ) );
    refdepthfld_->attach( alignedBelow, userefdepthfld_ );

    IOObjContext ctxt = is2d ? EMHorizon2DTranslatorGroup::ioContext()
			     : EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread_ = true;
    refhorizonfld_ = new uiIOObjSel( this, ctxt, uiStrings::sHorizon() );
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
    PtrMan<CtxtIOObj> allhorio = is2d_ ? mMkCtxtIOObj(EMHorizon2D)
					: mMkCtxtIOObj(EMHorizon3D);
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiIOObjSelDlg dlg( this, sdsu, *allhorio );
    if ( !dlg.go() )
	return;

    const int nrsel = dlg.nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const DBKey mid( dlg.chosenID(idx) );
	if ( !surfacelist_.isPresent(mid) )
	{
	    IOObj* ioobj = mid.getIOObj();
	    addSurfaceTableEntry( *ioobj, false, 0 );
	    delete ioobj;
	}
    }

    delete allhorio->ioobj_;
}


void uiSurfaceLimitedFiller::addSurfaceTableEntry( const IOObj& ioobj,
						   bool isfault, char side )
{
    const int row = surfacelist_.size();
    if ( !row )
	removebutton_->setSensitive( true );

    if ( row==table_->nrRows() )
	table_->insertRows( row, 1 );

    uiStringSet sidenms;
    if ( isfault )
    {
	sidenms.add(uiStrings::sRight());
	sidenms.add(uiStrings::sLeft());
    }
    else
    {
	sidenms.add(uiStrings::sAbove());
	sidenms.add(uiStrings::sBelow());
    }

    uiComboBox* sidesel = new uiComboBox( 0, sidenms, 0 );
    sidesel->setCurrentItem( side==-1 ? 0 : 1 );

    table_->setCellObject( RowCol(row,cSideCol), sidesel );
    table_->setText( RowCol(row,cNameCol), ioobj.name() );
    table_->setCellReadOnly( RowCol(row,cNameCol), true );
    surfacelist_ += ioobj.key();

    Color col = Color::White();
    const EM::Object* emobj = EM::MGR().getObject( ioobj.key() );
    if ( emobj )
	col = emobj->preferredColor();
    else
    {
	IOPar pars;
	EM::MGR().readDisplayPars( ioobj.key(), pars );
	pars.get( sKey::Color(), col );
    }

    table_->setCellColor( RowCol(row,cColorCol), col );
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
    if ( !usestartvalfld_ )
	return;

    const bool useval = usestartvalfld_->getBoolValue();
    if ( !useval && !startgridfld_->nrHorizonsWithData() )
    {
	uiMSG().warning( tr("No Horizon data available for any horizon, \
			  could only use constant start value.") );
	usestartvalfld_->setValue( true );
	return;
    }

    startvalfld_->display( useval );
    startgridfld_->display( !useval );
}


void uiSurfaceLimitedFiller::useGradientCB( CallBacker* )
{
    if ( !usegradientfld_ )
	return;

    const bool useval = usegradientfld_->getBoolValue();
    if ( !useval && !gradgridfld_->nrHorizonsWithData() )
    {
	uiMSG().warning( tr("No Horizon data available for any horizon, "
                               "could only use constant gradient.") );
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


#define mErrRet(s) { usw.readyNow(); uiMSG().error(s); return false; }

bool uiSurfaceLimitedFiller::acceptOK()
{
    if ( !uiStepDialog::acceptOK() )
	return false;

    const bool usestartval = usestartvalfld_ ? usestartvalfld_->getBoolValue()
					     : true;
    const bool usegradient = usegradientfld_ ? usegradientfld_->getBoolValue()
					     : true;
    const bool userefval = userefdepthfld_->getBoolValue();

    surfacefiller_->useStartValue( usestartval );
    surfacefiller_->useGradientValue( usegradient );
    surfacefiller_->useRefZValue( userefval );
    surfacefiller_->setGradientVertical(
	    gradienttypefld_ ? !gradienttypefld_->getIntValue() : false );

    uiUserShowWait usw( this, uiStrings::sCollectingData() );
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
	if ( mIsUdf(startvalfld_->getFValue()) )
	    mErrRet(tr("Please provide the start value"))

	surfacefiller_->setStartValue( startvalfld_->getFValue() );
    }
    else
    {
	if (!surfacefiller_->setStartValueHorizon(&startgridfld_->selectedID()))
	    mErrRet(tr("Cannot set start value horizon"))

	if ( startgridfld_->auxdataidx()<0 )
	    mErrRet(tr("No Horizon data available"))
	else
	    surfacefiller_->setStartAuxdataIdx( startgridfld_->auxdataidx() );
    }

    if ( usegradient )
    {
	if ( mIsUdf(gradientfld_->getFValue()) )
	    mErrRet(tr("Please provide the gradient"))

	surfacefiller_->setGradient(
		gradientfld_->getFValue()*SI().zDomain().userFactor() );
    }
    else
    {
	if ( !surfacefiller_->setGradientHorizon(&gradgridfld_->selectedID()) )
	    mErrRet(tr("Cannot set gradient horizon"))

	if ( gradgridfld_->auxdataidx()<0 )
	    mErrRet(tr("No Horizon data available"))
	else
	    surfacefiller_->setGradAuxdataIdx( gradgridfld_->auxdataidx() );
    }

    if ( userefval )
    {
	if ( mIsUdf(refdepthfld_->getFValue()) )
	    mErrRet(tr("Please provide the reference z value"))

	surfacefiller_->setRefZValue(
		refdepthfld_->getFValue()/SI().zDomain().userFactor());
    }
    else
    {
	const IOObj* obj = refhorizonfld_->ioobj();
	if ( !obj )
	    mErrRet(tr("Reference horizon does not exit"))

	const DBKey dbky = obj->key();
	if ( !surfacefiller_->setRefHorizon( &dbky ) )
	    mErrRet(tr("Cannot set reference horizon"))
    }

    return true;
}

} // namespace VolProc
