/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiisopachmaker.cc,v 1.3 2008-09-22 13:17:03 cvskris Exp $";

#include "uiisopachmaker.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "emsurfaceauxdata.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include <math.h>


uiIsopachMaker::uiIsopachMaker( uiParent* p, EM::ObjectID horid )
	: uiDialog(p,Setup("Create isopach",
			    BufferString("Create isopach from '",
					 getHorNm(horid),"'"), "104.4.4"))
	, ctio_(*mMkCtxtIOObj(EMHorizon3D))
	, horid_(horid)
	, dps_(*new DataPointSet(false,true))
{
    ctio_.ctxt.forread = true;
    horsel_ = new uiIOObjSel( this, ctio_, "Calculate to" );

    attrnmfld_ = new uiGenInput( this, "Attribute name",
	    			 StringInpSpec("<auto>") );
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
    delete &dps_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiIsopachMaker::acceptOK( CallBacker* )
{
    horsel_->commitInput( false );
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
       		     const char* attrnm, DataPointSet& dps )
    : Executor("Create isopach")
    , hor1_(hor1)
    , hor2_(hor2)
    , msg_("Creating isopach")
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
	    continue;

	const float th = z1 > z2 ? z1 - z2 : z2 - z1;
	const DataPointSet::Pos dpspos( pos1 );
	DataPointSet::DataRow dr( dpspos );
	dr.data_ += th;
	dps_.addRow( dr );
    }

    dps_.dataChanged();
    return MoreToDo;
}

int finishWork()
{
    dps_.dataChanged();
    if ( dps_.isEmpty() )
    {
	msg_ = "No thickness values collected";
	return ErrorOccurred;
    }
    return Finished;
}

    const EM::Horizon3D& hor1_;
    const EM::Horizon3D& hor2_;
    DataPointSet&	dps_;
    EM::EMObjectIterator* iter_;
    int			totnr_;
    BufferString	msg_;
    const EM::SectionID	sectid1_;
    const EM::SectionID	sectid2_;
};


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiIsopachMaker::doWork()
{
    if ( !horsel_->commitInput(false) )
	mErrRet("Please select the horizon")
    uiTaskRunner tr( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( ctio_.ioobj->key(),
	    						  &tr );
    mDynamicCastGet(EM::Horizon3D*,h2,emobj)
    if ( !h2 )
	mErrRet("Cannot load selected horizon")
    h2->ref();
    emobj = EM::EMM().getObject( horid_ );
    mDynamicCastGet(EM::Horizon3D*,h1,emobj)
    if ( !h1 )
	{ h2->unRef(); mErrRet("Cannot find base horizon") }

    h1->ref();
    uiIsopachMakerCreater mc( *h1, *h2, attrnm_, dps_ );
    bool rv = tr.execute( mc );
    h1->unRef(); h2->unRef();
    return rv;
}
