/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          February 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiwellattribsel.h"

#include "attribengman.h"
#include "attribprocessor.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "ptrman.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"

#include "uiattrsel.h"
#include "mousecursor.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"


uiWellAttribSel::uiWellAttribSel( uiParent* p, Well::Data& wd,
       				  const Attrib::DescSet& as,
       				  const NLAModel* mdl )
    : uiDialog(p,uiDialog::Setup("Attributes",
				 "Attribute selection",
				 "mNoHelpID"))
    , attrset_(as)
    , nlamodel_(mdl)
    , wd_(wd)
    , sellogidx_(-1)
{
    attribfld = new uiAttrSel( this, &attrset_ );
    attribfld->setNLAModel( nlamodel_ );
    attribfld->selectionDone.notify( mCB(this,uiWellAttribSel,selDone) );

    const bool zinft = SI().depthsInFeetByDefault();
    BufferString lbl = "Depth range"; lbl += zinft ? "(ft)" : "(m)";
    rangefld = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    rangefld->attach( alignedBelow, attribfld );
    setDefaultRange();

    lognmfld = new uiGenInput( this, "Log name" );
    lognmfld->attach( alignedBelow, rangefld );
}


void uiWellAttribSel::selDone( CallBacker* )
{
    const char* inputstr = attribfld->getInput();
    lognmfld->setText( inputstr );
}


void uiWellAttribSel::setDefaultRange()
{
    StepInterval<float> dahintv;
    for ( int idx=0; idx<wd_.logs().size(); idx++ )
    {
	const Well::Log& log = wd_.logs().getLog(idx);
	const int logsz = log.size();
	if ( !logsz ) continue;

	dahintv.setFrom( wd_.logs().dahInterval() );
	const float width = log.dah(logsz-1) - log.dah(0);
	dahintv.step = width / (logsz-1);
	break;
    }

    if ( !dahintv.width() )
    {
	const Well::Track& track = wd_.track();
	const int sz = track.size();
	dahintv.start = track.dah(0); dahintv.stop = track.dah(sz-1);
	dahintv.step = dahintv.width() / (sz-1);
    }

    const bool zinft = SI().depthsInFeetByDefault();
    if ( zinft )
    {
	dahintv.start *= mToFeetFactor;
	dahintv.stop *= mToFeetFactor;
	dahintv.step *= mToFeetFactor;
    }

    dahintv.sort();
    rangefld->setValue( dahintv );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiWellAttribSel::acceptOK( CallBacker* )
{
    if ( !inputsOK() )
	return false;

    MouseCursorChanger cursor( MouseCursor::Wait );

    BinIDValueSet bidset( 2, true );
    TypeSet<BinIDValueSet::Pos> positions;
    TypeSet<float> mdepths;
    getPositions( bidset, positions, mdepths );
    if ( positions.isEmpty() )
	mErrRet( "No positions extracted from well" )

    if ( !extractData(bidset) )
	return false;

    if ( !createLog(bidset,positions,mdepths) )
	return false;
    return true;
}


bool uiWellAttribSel::inputsOK()
{
    if ( SI().zIsTime() && !wd_.d2TModel() )
	mErrRet( "No depth to time model defined" );

    attribfld->processInput();
    const Attrib::DescID seldescid = attribfld->attribID();
    const int outputnr = attribfld->outputNr();
    if ( seldescid.asInt() < 0 && (nlamodel_ && outputnr<0) )
	mErrRet( "No valid attribute selected" )

    BufferString lognm = lognmfld->text();
    if ( lognm.isEmpty() )
	mErrRet( "Please provide logname" );

    sellogidx_ = wd_.logs().indexOf( lognm );
    if ( sellogidx_ >= 0 )
    {
	BufferString msg( "Log: '" ); msg += lognm;
	msg += "' is already present.\nDo you wish to overwrite this log?";
	if ( !uiMSG().askOverwrite(msg) ) return false;
    }

    return true;
}


void uiWellAttribSel::getPositions( BinIDValueSet& bidset, 
				    TypeSet<BinIDValueSet::Pos>& positions,
				    TypeSet<float>& mdepths )
{
    const bool zinft = SI().depthsInFeetByDefault();
    const StepInterval<float> intv = rangefld->getFStepInterval();
    const int nrsteps = intv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFactor;
	Coord3 pos = wd_.track().getPos( md );
	const BinID bid = SI().transform( pos );
	if ( !bid.inl && !bid.crl ) continue;

	if ( SI().zIsTime() )
	    pos.z = wd_.d2TModel()->getTime( md );
	bidset.add( bid, pos.z, (float)idx );
	mdepths += md;
	positions += BinIDValueSet::Pos(0,0);
    }

    BinIDValueSet::Pos pos;
    while ( bidset.next(pos) )
    {
	float& vidx = bidset.getVals(pos)[1];
	int posidx = mNINT32(vidx);
	positions[posidx] = pos;
	mSetUdf(vidx);
    }
}


bool uiWellAttribSel::extractData( BinIDValueSet& bidset )
{
    Attrib::SelSpec selspec;
    attribfld->fillSelSpec( selspec );
    Attrib::EngineMan aem;
    aem.setAttribSet( &attrset_ );
    aem.setNLAModel( nlamodel_ );
    aem.setAttribSpec( selspec );

    BufferString errmsg;
    ObjectSet<BinIDValueSet> bivsset;
    bivsset += &bidset;
    PtrMan<Attrib::Processor> process =
		aem.createLocationOutput( errmsg, bivsset );
    if ( !process ) mErrRet( errmsg );
    uiTaskRunner uitr( this );
    return uitr.execute( *process );
}


bool uiWellAttribSel::createLog( const BinIDValueSet& bidset,
				 const TypeSet<BinIDValueSet::Pos>& positions,
				 const TypeSet<float>& mdepths )
{
    BufferString lognm = lognmfld->text();
    Well::Log* newlog = new Well::Log( lognm );
    float v[2]; BinID bid;
    for ( int idx=0; idx<mdepths.size(); idx++ )
    {
	bidset.get( positions[idx], bid, v );
	if ( !mIsUdf(v[1]) )
	    newlog->addValue( mdepths[idx], v[1] );
    }

    if ( !newlog->size() )
    {
	uiMSG().error( "No values collected" );
	delete newlog;
	return false;
    }

    if ( sellogidx_ < 0 )
    {
	wd_.logs().add( newlog );
	sellogidx_ = wd_.logs().size() - 1;
    }
    else
    {
	Well::Log& log = wd_.logs().getLog( sellogidx_ );
	log.erase();
	for ( int idx=0; idx<newlog->size(); idx++ )
	    log.addValue( newlog->dah(idx), newlog->value(idx) );
	delete newlog;
    }

    return true;
}
