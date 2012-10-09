/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uihorsavefieldgrp.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"



uiHorSaveFieldGrp::uiHorSaveFieldGrp( uiParent* p, EM::Horizon* hor, bool is2d )
    : uiGroup( p )
    , horizon_( hor )
    , newhorizon_( 0 ) 
    , savefld_( 0 )
    , addnewfld_( 0 )		     
    , outputfld_( 0 )
    , is2d_( is2d )
    , usefullsurvey_( false )		   
{
    if ( horizon_ ) horizon_->ref();

    savefld_ = new uiGenInput( this, "Save horizon",
	    		       BoolInpSpec(true,"As new","Overwrite") );
    savefld_->valuechanged.notify( mCB(this,uiHorSaveFieldGrp,saveCB) );

    IOObjContext ctxt = is2d ? EMHorizon2DTranslatorGroup::ioContext()
			     : EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ctxt, "Output Horizon" );
    outputfld_->attach( alignedBelow, savefld_ );
    
    addnewfld_ = new uiCheckBox( this, "Display after create"  );
    addnewfld_->attach( alignedBelow, outputfld_ );

    setHAlignObj( savefld_ );    
    saveCB(0);
}


uiHorSaveFieldGrp::~uiHorSaveFieldGrp()
{
    if ( horizon_ ) horizon_->unRef();
    if ( newhorizon_ ) newhorizon_->unRef();
}


void uiHorSaveFieldGrp::setSaveFieldName( const char* nm )
{ savefld_->setName( nm ); }


void uiHorSaveFieldGrp::saveCB( CallBacker* )
{
    outputfld_->display( savefld_->getBoolValue() );
    addnewfld_->display( savefld_->getBoolValue() );
}


bool uiHorSaveFieldGrp::displayNewHorizon() const
{ return savefld_->getBoolValue() && addnewfld_->isChecked(); }


bool uiHorSaveFieldGrp::overwriteHorizon() const
{ return !savefld_->getBoolValue(); }


void uiHorSaveFieldGrp::setFullSurveyArray( bool yn )
{
    if ( usefullsurvey_ != yn )
    {
	usefullsurvey_ = yn;
    	if ( yn )
	    expandToFullSurveyArray();
    }
}


bool uiHorSaveFieldGrp::needsFullSurveyArray() const
{ return  usefullsurvey_; }


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

EM::Horizon* uiHorSaveFieldGrp::readHorizon( const MultiID& mid )
{
    EM::ObjectID oid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( oid );

    Executor* reader = 0;
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	reader = EM::EMM().objectLoader( mid );
	if ( !reader ) 
	    mErrRet( "Could not read horizon." );

	uiTaskRunner dlg( this );
	if ( !dlg.execute(*reader) )
	{
	    delete reader;
	    mErrRet( "Could not read horizon." );
	}

	oid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( oid );
    }

    mDynamicCastGet(EM::Horizon*,hor,emobj)
    horizon_ = hor;
    horizon_->ref();
    delete reader;
    return horizon_;
}


bool uiHorSaveFieldGrp::saveHorizon()
{
    const bool savenew = savefld_->getBoolValue();
    if ( !newhorizon_ && savenew && !createNewHorizon() )
	return false;
    PtrMan<Executor> exec = savenew ? newhorizon_->saver() : horizon_->saver();

    if ( !exec ) mErrRet( "Cannot save horizon" );

    uiTaskRunner dlg( this );
    return dlg.execute( *exec );
}


bool uiHorSaveFieldGrp::acceptOK( CallBacker* )
{
    if ( savefld_->getBoolValue() )
    {
	if ( !outputfld_->commitInput() )
    	    mErrRet(outputfld_->isEmpty() ? "Please select output horizon" : 
		    "Cannot continue: write permission problem" )

	if ( !createNewHorizon() )		
	    return false;
    }

    return true;
}


bool uiHorSaveFieldGrp::createNewHorizon()
{
    if ( !horizon_ )
	mErrRet( "No selected horizon, cannot create a new one." );

    if ( needsFullSurveyArray() )
	expandToFullSurveyArray();

    if ( newhorizon_ )
    {
	newhorizon_->unRef();
	newhorizon_ = 0;
    }

    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.createObject( is2d_ ? EM::Horizon2D::typeStr()
	    					: EM::Horizon3D::typeStr(),
					  outputfld_->getInput() );
    
    mDynamicCastGet(EM::Horizon*,horizon,em.getObject(objid));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );
    
    newhorizon_ = horizon;      
    newhorizon_->ref();
    newhorizon_->setMultiID( horizon_->multiID() );

    EM::SurfaceIOData sd;
    em.getSurfaceData( horizon_->multiID(), sd );
    EM::SurfaceIODataSelection sdsel( sd );

    uiTaskRunner tr( this );
    PtrMan<Executor> loader = newhorizon_->geometry().loader( &sdsel );
    if ( !loader || !tr.execute(*loader) ) 
	mErrRet( "New horizon data loading failed" );

    newhorizon_->setMultiID( outputfld_->ioobj()->key() );
    File::copy( horizon_->name(), newhorizon_->name() );

    return true;
}


void uiHorSaveFieldGrp::expandToFullSurveyArray()
{
    if ( !horizon_ || !horizon_->geometry().nrSections() )
	return;
    
    const EM::SectionID sid = horizon_->geometry().sectionID( 0 );
    mDynamicCastGet( Geometry::ParametricSurface*, surf,
	    horizon_->sectionGeometry( sid ) );
    if ( !surf || !needsFullSurveyArray() )
	return;
    
    StepInterval<int> rowrg = horizon_->geometry().rowRange( sid );
    StepInterval<int> colrg = horizon_->geometry().colRange( sid, -1 );
    
    const StepInterval<int> survcrlrg = SI().crlRange(true);
    while ( colrg.start-colrg.step>=survcrlrg.start )
    {
	const int newcol = colrg.start-colrg.step;
	surf->insertCol( newcol );
	colrg.start = newcol;
    }
    
    while ( colrg.stop+colrg.step<=survcrlrg.stop )
    {
	const int newcol = colrg.stop+colrg.step;
	surf->insertCol( newcol );
	colrg.stop = newcol;
    }
    
    const StepInterval<int> survinlrg = SI().inlRange(true);
    while ( rowrg.start-rowrg.step>=survinlrg.start )
    {
	const int newrow = rowrg.start-rowrg.step;
	surf->insertRow( newrow );
	rowrg.start = newrow;
    }
    
    while ( rowrg.stop+rowrg.step<=survinlrg.stop )
    {
	const int newrow = rowrg.stop+rowrg.step;
	surf->insertRow( newrow );
	rowrg.stop = newrow;
    }
}



