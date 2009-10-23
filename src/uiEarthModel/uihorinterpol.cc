/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorinterpol.cc,v 1.11 2009-10-23 21:24:21 cvsyuancheng Exp $";

#include "uihorinterpol.h"

#include "array2dinterpol.h"
#include "arraynd.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "executor.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "rowcol.h"
#include "survinfo.h"
#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

uiHorizon3DInterpolDlg::uiHorizon3DInterpolDlg( uiParent* p,
						EM::Horizon3D* hor )
    : uiDialog( p, uiDialog::Setup::Setup("Horizon Gridding",
	       				  "Gridding parameters",
					  "HelpID" ) )
    , horizon_( hor )
    , newhorizon_( 0 )		     
    , inputhorsel_( 0 )
    , savefld_( 0 )
    , addnewfld_( 0 )
{
    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread = true;
	inputhorsel_ = new uiIOObjSel( this, ctxt );
    }

    const char* geometries[] = { "Full survey", "Bounding box",
				 "Convex hull", "Only holes", 0 };
    geometrysel_ = new uiGenInput( this, "Geometry",
	    			   StringListInpSpec( geometries ) );
    geometrysel_->setText( geometries[2] );

    if ( inputhorsel_ ) geometrysel_->attach( alignedBelow, inputhorsel_ );
    interpolsel_ = new uiArray2DInterpolSel( this, false, true, false, 0 );
    interpolsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
    interpolsel_->attach( alignedBelow, geometrysel_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, interpolsel_ );

    uiGroup* attgrp = 0;
    if ( horizon_ )
    {
	savefld_ = new uiGenInput( this, "Save gridded horizon",
				   BoolInpSpec(false, "As new", "Overwrite") );
	savefld_->attach( alignedBelow, interpolsel_ );
	savefld_->valuechanged.notify(
		mCB(this,uiHorizon3DInterpolDlg,saveChangeCB));
	attgrp = savefld_;

	addnewfld_ = new uiGenInput( this, "Display in the scene",
				      BoolInpSpec(true) );
    }

    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ctxt, "Output Horizon" );
    if ( attgrp )
	outputfld_->attach( alignedBelow, attgrp );
    else
	attgrp = outputfld_;

    attgrp->attach( alignedBelow, interpolsel_ );
    attgrp->attach( ensureBelow, sep );
    
    if ( addnewfld_ )
	addnewfld_->attach( alignedBelow, outputfld_ );

    saveChangeCB( 0 );
}


uiHorizon3DInterpolDlg::~uiHorizon3DInterpolDlg()
{
    if ( horizon_ ) horizon_->unRef();
    if ( newhorizon_ ) newhorizon_->unRef();
}


const char* uiHorizon3DInterpolDlg::helpID() const
{
    return interpolsel_->helpID();
}


void uiHorizon3DInterpolDlg::saveChangeCB( CallBacker* )
{
    outputfld_->display( savefld_ ? savefld_->getBoolValue() : true );
    if ( addnewfld_ )
    	addnewfld_->display( savefld_ ? savefld_->getBoolValue() : false );
}


bool uiHorizon3DInterpolDlg::acceptOK( CallBacker* )
{
    if ( !interpolsel_->acceptOK() )
	return false;

    PtrMan<Array2DInterpol> interpolator = interpolsel_->getResult();
    if ( !interpolator )
	return false;

    const IOObj* outputioobj = 0;
    if ( !savefld_ || savefld_->getBoolValue() )
    {
	outputioobj = outputfld_->ioobj();
	if ( !outputioobj )
	    return false;
    }
    
    uiTaskRunner tr( this );

    if ( inputhorsel_ )
    {
	const IOObj* ioobj = inputhorsel_->ioobj();
	if ( !ioobj )
	    return false;

	RefMan<EM::EMObject> obj =
	    EM::EMM().loadIfNotFullyLoaded( ioobj->key(), &tr );
	mDynamicCastGet( EM::Horizon3D*, hor, obj.ptr() );
	if ( !hor )
	{
	    uiMSG().error("Could not load horizon");
	    return false;
	}

	if ( horizon_ )
	    horizon_->unRef();

	horizon_ = hor;
	horizon_->ref();
    }

    if ( !horizon_ )
    {
	pErrMsg("Missing horizon!");
	return false;
    }

    MouseCursorChanger mcc( MouseCursor::Wait );

    Array2DInterpol::FillType filltype;
    switch ( geometrysel_->getIntValue() )
    {
	case 0:
	    filltype = Array2DInterpol::Full;
	    if ( !expandArraysToSurvey() )
	    {
		uiMSG().error("Cannot allocate memory for extrapolation");
		return false;
	    }
	    break;
	case 1:
	    filltype = Array2DInterpol::Full;
	    break;
	case 2:
	    filltype = Array2DInterpol::ConvexHull;
	    break;
	default:
	    filltype = Array2DInterpol::HolesOnly;
    }

    interpolator->setFillType( filltype );
    if ( savefld_ && savefld_->getBoolValue() && !createNewHorizon() )
	return false;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();

    EM::Horizon3D& usedhor = newhorizon_ ? *newhorizon_ : *horizon_;
    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	PtrMan<Array2D<float> > arr = horizon_->createArray2D( sid );

	if ( !arr )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	const int inlstepoutstep = horizon_->geometry().rowRange( sid ).step;
	const int crlstepoutstep = horizon_->geometry().colRange( sid ).step;

	interpolator->setRowStep( inlstepoutstep*inldist );
	interpolator->setColStep( crlstepoutstep*crldist );

	if ( !interpolator->setArray( *arr, &tr ) )
	{
	    BufferString msg( "Cannot setup interpolation on section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	if ( !tr.execute( *interpolator ) )
	{
	    BufferString msg( "Cannot interpolate section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	EM::SectionID usedsid = usedhor.geometry().sectionID( idx );;
	if ( !usedhor.setArray2D( *arr, usedsid, true, "Interpolation" ) )
	{
	    BufferString msg( "Cannot set new data to section " );
	    msg += usedsid;
	    ErrMsg( msg ); continue;
	}
    }

    if ( outputioobj )
    {
	if ( !savefld_ ) horizon_->setMultiID( outputioobj->key() );
	PtrMan<Executor> exec = usedhor.saver();
	if ( !tr.execute( *exec ) )
	{
	    uiMSG().error("Could not save horizon");
	    return false;
	}
    }

    return true;
}


bool uiHorizon3DInterpolDlg::createNewHorizon()
{
    if ( !savefld_ || !savefld_->getBoolValue() || !outputfld_->ioobj() )
	return false;

    EM::EMManager& em = EM::EMM();
    const BufferString typ = EM::Horizon3D::typeStr();
    EM::ObjectID objid = em.createObject( typ, outputfld_->getInput() );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid));
    if ( !horizon )
    {
	ErrMsg( "Cannot create horizon" );
	return false;
    }
    
    horizon->initClass();	
    if ( newhorizon_ ) newhorizon_->unRef();
    newhorizon_ = horizon;	
    newhorizon_->ref();
    newhorizon_->setMultiID( outputfld_->ioobj()->key() );

    for ( int idx=0; idx<newhorizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = newhorizon_->geometry().sectionID( idx );
	newhorizon_->geometry().removeSection( sid, false );
    }
    
    const RowCol step = horizon_->geometry().step();
    newhorizon_->geometry().setStep( step, step );

    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	const Geometry::BinIDSurface* surf = 
	    horizon_->geometry().sectionGeometry(sid);

	EM::SectionID usedsid = sid;
	BufferString sectnm = horizon_->sectionName( sid );
	usedsid = newhorizon_->geometry().addSection( sectnm, false );

	const RowCol start = surf->getKnotRowCol(0);
	const RowCol stop = surf->getKnotRowCol( surf->nrKnots()-1 );
	newhorizon_->geometry().sectionGeometry(usedsid)->
	    expandWithUdf( start, stop );
    }
    
    return true;
}


bool uiHorizon3DInterpolDlg::displayNewHorizon() const
{
    return savefld_ && savefld_->getBoolValue() && addnewfld_->getBoolValue(); 
}


bool uiHorizon3DInterpolDlg::expandArraysToSurvey()
{
    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ ) 
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	StepInterval<int> rowrg = horizon_->geometry().rowRange( sid );
	StepInterval<int> colrg = horizon_->geometry().colRange( sid );

	mDynamicCastGet( Geometry::ParametricSurface*, surf,
			 horizon_->sectionGeometry( sid ) );

	const StepInterval<int> survcrlrg = SI().crlRange(true);
	int nrcolstoinsert = -colrg.nearestIndex(survcrlrg.start);
	if ( nrcolstoinsert>0 )
	{
	    surf->insertCol( survcrlrg.start, nrcolstoinsert );
	    colrg.start = survcrlrg.start;
	}

	nrcolstoinsert = (survcrlrg.stop-colrg.stop)/colrg.step;
	if ( nrcolstoinsert>0 )
	{
	    surf->insertCol( colrg.stop+colrg.step, nrcolstoinsert );
	    colrg.stop += nrcolstoinsert*colrg.step;
	}

	const StepInterval<int> survinlrg = SI().inlRange(true);

	int nrrowstoinsert = -rowrg.nearestIndex(survinlrg.start);
	if ( nrrowstoinsert>0 )
	{
	    surf->insertRow( survinlrg.start, nrrowstoinsert );
	    rowrg.start = survinlrg.start;
	}

	nrrowstoinsert = (survinlrg.stop-rowrg.stop)/rowrg.step;
	if ( nrrowstoinsert>0 )
	{
	    surf->insertRow( rowrg.stop+rowrg.step, nrrowstoinsert );
	    rowrg.stop += nrrowstoinsert*rowrg.step;
	}
    }

    return true;
}
