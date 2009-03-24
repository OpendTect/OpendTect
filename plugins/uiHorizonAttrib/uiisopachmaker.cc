/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiisopachmaker.cc,v 1.6 2009-03-24 12:33:51 cvsbert Exp $";

#include "uiisopachmaker.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "emsurfaceauxdata.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "survinfo.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include <math.h>


uiIsopachMaker::uiIsopachMaker( uiParent* p, EM::ObjectID horid )
	: uiDialog(p,Setup("Create isopach",mNoDlgTitle, "104.4.4"))
	, ctio_(*mMkCtxtIOObj(EMHorizon3D))
	, basectio_(*mMkCtxtIOObj(EMHorizon3D))
	, baseemobj_(0)    				       
	, horid_(horid)
	, basesel_(0)	       
	, dps_(*new DataPointSet(false,true))
	, saveattr_(false)
{
    BufferString title( "Create isopach" );
    baseemobj_ = EM::EMM().getObject( horid_ );

    if ( !baseemobj_ )
    {
	basectio_.ctxt.forread = true;
	basesel_ = new uiIOObjSel( this, basectio_, "Horizon" );
	saveattr_ = true;
    }
    else
    {
	title += " '";
	title += getHorNm( horid );
	title += "'";
    }

    setTitleText( title.buf() );
    ctio_.ctxt.forread = true;
    horsel_ = new uiIOObjSel( this, ctio_, "Calculate to" );
    horsel_->selectiondone.notify( mCB(this,uiIsopachMaker,toHorSel) );

    if ( !baseemobj_ )
	horsel_->attach( alignedBelow, basesel_ );

    attrnmfld_ = new uiGenInput( this, "Attribute name", StringInpSpec() );
    attrnmfld_->attach( alignedBelow, horsel_ );
}


BufferString uiIsopachMaker::getHorNm( EM::ObjectID horid )
{
    MultiID mid( EM::EMM().getMultiID( horid ) );
    return EM::EMM().objectName( mid );
}


uiIsopachMaker::~uiIsopachMaker()
{
    delete ctio_.ioobj; delete &ctio_;
    delete basectio_.ioobj; delete &basectio_;
    delete &dps_;
}


void uiIsopachMaker::toHorSel( CallBacker* )
{
    attrnmfld_->setText( BufferString("I: ",ctio_.ioobj->name()) );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiIsopachMaker::acceptOK( CallBacker* )
{
    horsel_->commitInput();
    if ( !ctio_.ioobj )
	mErrRet("Please provide the isopach horizon")

    attrnm_ =  attrnmfld_->text();
    if ( attrnm_.isEmpty() || attrnm_ == "<auto>" )
	{ attrnm_ = "I: "; attrnm_ += ctio_.ioobj->name(); }

    return doWork();
}


class uiIsopachMakerCreater : public Executor
{
public:

uiIsopachMakerCreater( const EM::Horizon3D& hor1, const EM::Horizon3D& hor2,
       		     const char* attrnm, DataPointSet& dps, int dataidx )
    : Executor("Create isopach")
    , hor1_(hor1)
    , hor2_(hor2)
    , msg_("Creating isopach")
    , dataidx_(dataidx)
    , dps_(dps)
    , sectid1_(hor1.sectionID(0))
    , sectid2_(hor2.sectionID(0))
{
    iter_ = hor1.createIterator( sectid1_ );
    totnr_ = iter_->approximateSize();
    dps_.dataSet().add( new DataColDef(attrnm) );
}

~uiIsopachMakerCreater()
{
    delete iter_;
}

const char* message() const	{ return msg_.buf(); }
const char* nrDoneText() const	{ return "Positions handled"; }
od_int64 nrDone() const		{ return dps_.size(); }
od_int64 totalNr() const	{ return totnr_; }

int nextStep()
{
    for ( int idx=0; idx<1000; idx++ )
    {
	const EM::PosID posid = iter_->next();
	if ( posid.objectID() < 0 )
	    return finishWork();
	if ( posid.sectionID() != sectid1_ )
	    continue;
	const EM::SubID subid = posid.subID();
	const Coord3 pos1( hor1_.getPos( sectid1_, subid ) );
	const float z1 = pos1.z;
	const float z2 = hor2_.getPos( sectid2_, subid ).z;
	if ( mIsUdf(z1) || mIsUdf(z2) )
	{
	    if ( dataidx_ != -1 )
		hor1_.auxdata.setAuxDataVal( dataidx_, posid, mUdf(float) );
	    continue;
	}

	const float th = z1 > z2 ? z1 - z2 : z2 - z1;

	if ( dataidx_ != -1 )
	    hor1_.auxdata.setAuxDataVal( dataidx_, posid, th*SI().zFactor() );

	const DataPointSet::Pos dpspos( pos1 );
	DataPointSet::DataRow dr( dpspos );
	dr.data_ += th;
	dps_.addRow( dr );
    }

    dps_.dataChanged();
    return MoreToDo();
}

int finishWork()
{
    dps_.dataChanged();
    if ( dps_.isEmpty() )
    {
	msg_ = "No thickness values collected";
	return ErrorOccurred();
    }
    return Finished();
}

    const EM::Horizon3D& hor1_;
    const EM::Horizon3D& hor2_;
    DataPointSet&	dps_;
    EM::EMObjectIterator* iter_;
    int                 dataidx_;
    int			totnr_;
    BufferString	msg_;
    const EM::SectionID	sectid1_;
    const EM::SectionID	sectid2_;
};


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiIsopachMaker::doWork()
{
    if ( saveattr_ && !basesel_->commitInput() )
	mErrRet("Please select base horizon" )

    if ( !horsel_->commitInput() )
	mErrRet("Please select the horizon")
    uiTaskRunner tr( this );
    
    if ( !baseemobj_ )
    {
	baseemobj_ = EM::EMM().loadIfNotFullyLoaded( basectio_.ioobj->key(),
						     &tr );
	baseemobj_->ref();
    }
    else baseemobj_->ref();

    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ctio_.ioobj->key(),
	    						  &tr );
    mDynamicCastGet(EM::Horizon3D*,h2,emobj)
    if ( !h2 )
	mErrRet("Cannot load selected horizon")
    h2->ref();
    
    //emobj = EM::EMM().getObject( horid_ );
    mDynamicCastGet(EM::Horizon3D*,h1,baseemobj_)
    if ( !h1 )
	{ h2->unRef(); mErrRet("Cannot find base horizon") }

    int dataidx_ = -1;

    if ( saveattr_ )
    {
	if ( h1->auxdata.auxDataIndex( attrnm_ ) != -1 ) 
	    mErrRet( "Attribute with this name for selected horizon "
		     "already exits" )
	else
	    dataidx_ = h1->auxdata.addAuxData( attrnm_ );
    }

    uiIsopachMakerCreater mc( *h1, *h2, attrnm_, dps_, dataidx_ );
    bool rv = tr.execute( mc );
   
    if ( saveattr_ )
    {
	PtrMan<Executor> saver = h1->auxdata.auxDataSaver();
	if ( saver )
	    rv = tr.execute( *saver );
    }

    h1->unRef(); h2->unRef();
    return rv;
}
