/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uicreateattriblogdlg.cc,v 1.15 2009-06-23 06:58:20 cvsranojay Exp $";

#include "uicreateattriblogdlg.h"

#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribsel.h"
#include "bufstringset.h"
#include "binidvalset.h"
#include "datainpspec.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "welltrack.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiCreateAttribLogDlg::uiCreateAttribLogDlg( uiParent* p,
					    const BufferStringSet& wellnames,
					    const Attrib::DescSet* attrib , 
					    const NLAModel* mdl,
					    bool singlewell )
    : uiDialog(p,uiDialog::Setup("Create Attribute Log",
				 "Specify parameters for the new attribute log",
				 mTODOHelpID) )
    , nlamodel_(mdl)
    , wellnames_(wellnames)
    , singlewell_(singlewell)
    , attrib_(attrib)
    , sellogidx_(-1)
{
    attribfld_ = attrib ? new uiAttrSel( this, *attrib_ )
			: new uiAttrSel( this, 0, uiAttrSelData(false) );
    attribfld_->setNLAModel( nlamodel_ );
    attribfld_->selectiondone.notify( mCB(this,uiCreateAttribLogDlg,selDone) );

    if ( !singlewell )
    {
	welllistfld_ = new uiListBox( this );
	welllistfld_->attach( alignedBelow, attribfld_ );
	welllistfld_->setMultiSelect();
	welllistfld_->addItems( wellnames );
    }

    // TODO: Get markers from all wells
    ObjectSet<Well::Data>& wells = Well::MGR().wells();
    markernames_.add( Well::TrackSampler::sKeyDataStart() );
    for ( int idx=0; !wells.isEmpty() && idx<wells.size(); idx++ )
	markernames_.add( wells[0]->markers()[idx]->name() );
    markernames_.add( Well::TrackSampler::sKeyDataEnd() );

    StringListInpSpec slis( markernames_ );
    topmrkfld_ = new uiGenInput( this, "Extract between",
	    					slis.setName("Top Marker") );

    if ( singlewell )
	topmrkfld_->attach( alignedBelow, attribfld_ );
    else
	topmrkfld_->attach( alignedBelow, welllistfld_ );
    topmrkfld_->setValue( (int)0 );
    topmrkfld_->setElemSzPol( uiObject::Medium );
    botmrkfld_ = new uiGenInput( this, "", slis.setName("Bottom Marker") );
    botmrkfld_->attach( rightOf, topmrkfld_ );
    botmrkfld_->setValue( markernames_.size()-1 );
    botmrkfld_->setElemSzPol( uiObject::Medium );

    const bool zinft = SI().depthsInFeetByDefault();
    const float defstep = zinft ? 0.5 : 0.15;
    BufferString lbl = "Step "; lbl += zinft ? "(ft)" : "(m)";
    stepfld_ = new uiGenInput( this, lbl, FloatInpSpec(defstep) );
    stepfld_->attach( rightOf, botmrkfld_ );

    lognmfld_ = new uiGenInput( this, "Log name" );
    lognmfld_->attach( alignedBelow, topmrkfld_ );
}


uiCreateAttribLogDlg::~uiCreateAttribLogDlg()
{
}


void uiCreateAttribLogDlg::selDone( CallBacker* )
{
    const char* inputstr = attribfld_->getInput();
    lognmfld_->setText( inputstr );
}


static int getWellIndex( const char* wellnm )
{
    for ( int idx=0; idx<Well::MGR().wells().size(); idx++ )
    {
	if ( !strcmp(Well::MGR().wells()[idx]->name(),wellnm) )
	    return idx;
    }

    return -1;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiCreateAttribLogDlg::acceptOK( CallBacker* )
{
    BufferStringSet selwells;
    if ( !singlewell_ )
    {
	if ( welllistfld_->nrSelected() < 1 )
	    mErrRet( "Select at least one well" );
	welllistfld_->getSelectedItems( selwells );
    }
    else
	selwells.add( *wellnames_[0] );
    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	const int wellidx = getWellIndex( selwells.get(idx) );
	if ( wellidx<0 ) continue;

	if ( !inputsOK(wellidx) )
	    return false;

	Well::Data* wd = Well::MGR().wells()[ wellidx ];
	BinIDValueSet bidset( 2, true );
	TypeSet<BinIDValueSet::Pos> positions;
	TypeSet<float> mdepths;
	if ( !getPositions(bidset,*wd,positions,mdepths) )
	    continue;

	if ( positions.isEmpty() )
	    mErrRet( "No positions extracted from well" );

	if ( !extractData(bidset) )
	    return false;

	if ( !createLog(bidset,*wd,positions,mdepths) )
	    return false;
    }

    return true;
}


bool uiCreateAttribLogDlg::inputsOK( int wellno )
{
    Well::Data* wd = Well::MGR().wells()[ wellno ];
    if ( SI().zIsTime() && !wd->d2TModel() )
	mErrRet( "No depth to time model defined" );

    const Attrib::DescID seldescid = attribfld_->attribID();
    const int outputnr = attribfld_->outputNr();
    if ( seldescid.asInt() < 0 && (nlamodel_ && outputnr<0) )
	mErrRet( "No valid attribute selected" );

    if( stepfld_->getfValue()<0 || stepfld_->getfValue(0)>100 )
	mErrRet( "Please Enter a valid step value" );
    
    BufferString lognm = lognmfld_->text();
    if ( lognm.isEmpty() )
	mErrRet( "Please provide logname" );

    sellogidx_ = wd->logs().indexOf( lognm );
    if ( sellogidx_ >= 0 )
    {
	BufferString msg( "Log: '" ); msg += lognm;
	msg += "' is already present.\nDo you wish to overwrite this log?";
	if ( !uiMSG().askOverwrite(msg) ) return false;
    }

    return true;
}


bool uiCreateAttribLogDlg::getPositions( BinIDValueSet& bidset, Well::Data& wd,
					 TypeSet<BinIDValueSet::Pos>& positions,
					 TypeSet<float>& mdepths )
{
    const bool zinft = SI().depthsInFeetByDefault();
    const float step = stepfld_->getfValue();
    const int topmarker = markernames_.indexOf( topmrkfld_->text() );
    const int bottommarker = markernames_.indexOf( botmrkfld_->text() );
    float start = 0;
    float stop = 0;
    if ( topmarker == 0)
	start = wd.track().dah(0);
    else 
	start = wd.markers()[ topmarker-1 ]->dah();

    if ( markernames_.size()-1 != bottommarker )
    {
	if ( wd.markers().size() <= bottommarker-2 )
	{
	    BufferString msg( "Cannot create log for Well: " );
	    msg += wd.name() ;
	    uiMSG().error(msg);
	    return false;
	}
	stop = wd.markers()[ bottommarker-1 ]->dah();
    }
    else
	stop = wd.track().dah( wd.track().size()-1 );

    if ( start > stop )
    {
	BufferString msg( "Please choose the Markers correctly" );
	uiMSG().error(msg);
	return false;
    }

    const StepInterval<float> intv = StepInterval<float>( start, stop, step );
    const int nrsteps = intv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFactor;
	Coord3 pos = wd.track().getPos( md );
	const BinID bid = SI().transform( pos );
	if ( !bid.inl && !bid.crl ) continue;

	if ( SI().zIsTime() )
	    pos.z = wd.d2TModel()->getTime( md );
	bidset.add( bid, pos.z, (float)idx );
	mdepths += md;
	positions += BinIDValueSet::Pos(0,0);
    }
    
    BinIDValueSet::Pos pos;
    while ( bidset.next(pos) )
    {
	float& vidx = bidset.getVals(pos)[1];
	int posidx = mNINT(vidx);
	positions[posidx] = pos;
	mSetUdf(vidx);
    }
    return true;
}


bool uiCreateAttribLogDlg::extractData( BinIDValueSet& bidset )
{
    Attrib::SelSpec selspec;
    attribfld_->fillSelSpec( selspec );
    Attrib::EngineMan aem;
    aem.setAttribSet( attrib_ );
    aem.setNLAModel( nlamodel_ );
    aem.setAttribSpec( selspec );

    BufferString errmsg;
    ObjectSet<BinIDValueSet> bivsset;
    bivsset += &bidset;
    PtrMan<Attrib::Processor> process =
	aem.createLocationOutput( errmsg, bivsset );
    if ( !process ) mErrRet( errmsg );
    uiTaskRunner uiexec( this);
    return uiexec.execute(*process);
}


bool uiCreateAttribLogDlg::createLog( const BinIDValueSet& bidset,
	 Well::Data& wd, const TypeSet<BinIDValueSet::Pos>& positions,
	 const TypeSet<float>& mdepths )
{
    BufferString lognm = lognmfld_->text();
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
	wd.logs().add( newlog );
	sellogidx_ = wd.logs().size() - 1;
    }
    else
    {
	Well::Log& log = wd.logs().getLog( sellogidx_ );
	log.erase();
	for ( int idx=0; idx<newlog->size(); idx++ )
	    log.addValue( newlog->dah(idx), newlog->value(idx) );
	delete newlog;
    }

    return true;
}
