/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihorsavefieldgrp.h"

#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "rangeposprovider.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uistrings.h"
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
    , rgfld_(0)
{
    init( false );
}


uiHorSaveFieldGrp::uiHorSaveFieldGrp( uiParent* p, EM::Horizon* hor,
				      bool is2d, bool withsubsel )
    : uiGroup( p )
    , horizon_( hor )
    , newhorizon_( 0 )
    , savefld_( 0 )
    , addnewfld_( 0 )
    , outputfld_( 0 )
    , is2d_( is2d )
    , usefullsurvey_( false )
{
    init( withsubsel );
}


void uiHorSaveFieldGrp::init( bool withsubsel )
{
   if ( horizon_ ) horizon_->ref();

   if ( withsubsel )
    {
	uiPosSubSel::Setup su( is2d_, false );
	su.choicetype( uiPosSubSel::Setup::RangewithPolygon );
	rgfld_ = new uiPosSubSel( this, su );
    }


    savefld_ = new uiGenInput( this, uiStrings::phrSave(uiStrings::sHorizon(1)),
				BoolInpSpec(true,tr("As new"),
				uiStrings::sOverwrite()) );

    savefld_->valueChanged.notify( mCB(this,uiHorSaveFieldGrp,saveCB) );

    if ( withsubsel )
	savefld_->attach( alignedBelow, rgfld_ );

    outputfld_ = new uiHorizonSel( this, is2d_, false,
				 uiStrings::phrOutput(uiStrings::sHorizon(1)) );
    outputfld_->attach( alignedBelow, savefld_ );

    addnewfld_ = new uiCheckBox( this, tr("Display after create") );
    addnewfld_->attach( alignedBelow, outputfld_ );

    setHAlignObj( savefld_ );

    const bool allowovrwrt = horizon_ ? EM::canOverwrite(horizon_->multiID())
				      : false ;
    allowOverWrite( allowovrwrt );
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


void uiHorSaveFieldGrp::allowOverWrite( bool yn )
{
    if ( !savefld_ || !outputfld_ || !addnewfld_ )
	return;

    if ( !yn ) savefld_->setValue( true );
    savefld_->setSensitive( yn );
    saveCB( 0 );
}


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


#define mErrRet(msg) { if ( !msg.isEmpty() ) uiMSG().error( msg ); return 0; }

EM::Horizon* uiHorSaveFieldGrp::readHorizon( const MultiID& mid )
{
    EM::ObjectID oid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( oid );

    Executor* reader = 0;
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	reader = EM::EMM().objectLoader( mid );
	if ( !reader )
	    mErrRet( uiStrings::phrCannotRead(uiStrings::sHorizon(1)));

	uiTaskRunner dlg( this );
	if ( !TaskRunner::execute( &dlg, *reader ) )
	{
	    delete reader;
	     mErrRet( uiStrings::phrCannotRead(uiStrings::sHorizon(1)));
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


EM::SurfaceIODataSelection uiHorSaveFieldGrp::getSelection( bool isnew ) const
{
    RefMan<EM::Horizon> horizon = isnew ? newhorizon_ : horizon_;
    Pos::Provider* prov = rgfld_ ? rgfld_->curProvider() : 0;
    mDynamicCastGet(Pos::Provider3D*,prov3d,prov);
    if ( prov3d )
    {
	horizon->setBurstAlert( true );
	horizon->apply( *prov3d );
	horizon->setBurstAlert( false );
    }

    EM::SurfaceIOData outsd;
    outsd.use( *horizon );
    EM::SurfaceIODataSelection outsdsel( outsd );
    outsdsel.setDefault();

    mDynamicCastGet(Pos::RangeProvider3D*,rgprov3d,prov3d);
    if ( rgprov3d )
	outsdsel.rg = rgprov3d->sampling().hsamp_;

    return outsdsel;
}

#undef mErrRet
#define mErrRet(msg) { if ( (msg).isSet() ) uiMSG().error( msg ); return false;}

bool uiHorSaveFieldGrp::saveHorizon()
{
    const bool savenew = savefld_->getBoolValue();
    if ( !newhorizon_ && savenew && !createNewHorizon() )
	return false;

    const EM::SurfaceIODataSelection sdsel = getSelection( savenew );
    PtrMan<Executor> exec = savenew ? newhorizon_->geometry().saver( &sdsel )
				    : horizon_->geometry().saver( &sdsel );

    if ( !exec ) mErrRet( tr("Cannot save horizon") );

    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, *exec );
}


bool uiHorSaveFieldGrp::acceptOK( CallBacker* )
{
    if ( savefld_->getBoolValue() )
    {
	if ( !outputfld_->ioobj() || !createNewHorizon() )
	    return false;
    }

    return true;
}


bool uiHorSaveFieldGrp::createNewHorizon()
{
    if ( !horizon_ )
	mErrRet( tr("No selected horizon, cannot create a new one.") );

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
	mErrRet( uiStrings::sCantCreateHor() );

    newhorizon_ = horizon;
    newhorizon_->ref();
    newhorizon_->setMultiID( horizon_->multiID() );

    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !em.getSurfaceData(horizon_->multiID(),sd,errmsg) )
	mErrRet( errmsg )

    EM::SurfaceIODataSelection sdsel( sd );

    uiTaskRunner taskrunner( this );
    PtrMan<Executor> loader = newhorizon_->geometry().loader( &sdsel );
    if ( !loader || !TaskRunner::execute( &taskrunner, *loader ) )
	mErrRet( tr("New horizon data loading failed") );

    newhorizon_->setMultiID( outputfld_->ioobj()->key() );
    File::copy( horizon_->name(), newhorizon_->name() );

    if ( needsFullSurveyArray() )
	expandToFullSurveyArray();

    return true;
}

#undef mErrRet

void uiHorSaveFieldGrp::expandToFullSurveyArray()
{
    if ( needsFullSurveyArray() )
	setHorRange( SI().inlRange(true), SI().crlRange(true) );
}


void uiHorSaveFieldGrp::setHorRange( const Interval<int>& newinlrg,
				     const Interval<int>& newcrlrg )
{
    EM::Horizon* hor = overwriteHorizon() ? horizon_ : newhorizon_;
    if ( !hor )
	return;

    mDynamicCastGet(Geometry::ParametricSurface*,surf,
		    hor->geometryElement())
    if ( !surf )
	return;

    StepInterval<int> rowrg = hor->geometry().rowRange();
    StepInterval<int> colrg = hor->geometry().colRange( -1 );

    while ( colrg.start-colrg.step >= newcrlrg.start )
    {
	const int newcol = colrg.start-colrg.step;
	surf->insertCol( newcol );
	colrg.start = newcol;
    }

    if ( colrg.start < newcrlrg.start )
	surf->removeCol( colrg.start, newcrlrg.start-1 );

    while ( colrg.stop+colrg.step <= newcrlrg.stop )
    {
	const int newcol = colrg.stop+colrg.step;
	surf->insertCol( newcol );
	colrg.stop = newcol;
    }

    if ( colrg.stop > newcrlrg.stop )
	surf->removeCol( newcrlrg.stop+1, colrg.stop );

    while ( rowrg.start-rowrg.step >= newinlrg.start )
    {
	const int newrow = rowrg.start-rowrg.step;
	surf->insertRow( newrow );
	rowrg.start = newrow;
    }

    if ( rowrg.start < newinlrg.start )
	surf->removeRow( rowrg.start, newinlrg.start-1 );

    while ( rowrg.stop+rowrg.step <= newinlrg.stop )
    {
	const int newrow = rowrg.stop+rowrg.step;
	surf->insertRow( newrow );
	rowrg.stop = newrow;
    }

    if ( rowrg.stop > newinlrg.stop )
	surf->removeRow( newinlrg.stop+1, rowrg.stop );
}
