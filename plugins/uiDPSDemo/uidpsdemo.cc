/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uidpsdemo.h"

#include "datapointset.h"
#include "binidvalset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "statrand.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "binidsurface.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrcprop.h"
#include "seisselectionimpl.h"

#include "uiseissel.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uidatapointset.h"
#include "uimsg.h"


uiDPSDemo::uiDPSDemo( uiParent* p, DataPointSetDisplayMgr* dpsdispmgr )
	: uiDialog(p,Setup("DataPointSet demo","Data extraction parameters",
		    	   mNoHelpID))
	, dps_(0)
	, dpsdispmgr_(dpsdispmgr)
{
    horfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D) );

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    seisfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(false,false) );
    seisfld_->attach( alignedBelow, horfld_ );

    nrptsfld_ = new uiGenInput( this, "Number of points to extract",
	    			IntInpSpec(10000) );
    nrptsfld_->attach( alignedBelow, seisfld_ );
}


uiDPSDemo::~uiDPSDemo()
{
    if ( dps_ )
	DPM(DataPackMgr::PointID()).release( dps_->id() );
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiDPSDemo::acceptOK( CallBacker* )
{
    const IOObj* horioobj = horfld_->ioobj(); // emits its own error message
    if ( !horioobj ) return false;
    const IOObj* seisioobj = seisfld_->ioobj(); // this one, too
    if ( !seisioobj ) return false;

    const int nrpts = nrptsfld_->getIntValue();
    if ( nrpts < 2 )
	mErrRet( "Please enter a valid number of points" )

    return doWork( *horioobj, *seisioobj, nrpts );
}



bool uiDPSDemo::doWork( const IOObj& horioobj, const IOObj& seisioobj,
			int nrpts )
{
    uiTaskRunner tr( this );
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( horioobj.key(), &tr );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) return false;

    dps_ = new DataPointSet( false );
    dps_->dataSet().add( new DataColDef("Amplitude") );
    dps_->dataSet().add( new DataColDef("Peakedness") );
    dps_->dataSet().add( new DataColDef("PeakSkew") );
    dps_->dataSet().add( new DataColDef("Frequency") );
    DPM(DataPackMgr::PointID()).addAndObtain( dps_ );

    hor->ref();
    const bool isok = getRandPositions(*hor,nrpts,*dps_);
    BufferStringSet sectionnms;
    for ( EM::SectionID isect=0; isect<hor->nrSections(); isect++ )
	sectionnms.add( hor->sectionName(isect) );
    hor->unRef();

    if ( !isok || !getSeisData(seisioobj,*dps_,tr) )
	return false;

    BufferString wintitl( horioobj.name(), " / ", seisioobj.name() );
    uiDataPointSet::Setup su( wintitl, false );
    su.canaddrow( true );
    uiDataPointSet* uidps =
	new uiDataPointSet( parent(), *dps_, su, dpsdispmgr_ );
    uidps->setDeleteOnClose( true );
    if ( sectionnms.size() > 1 )
	{ uidps->setGroupNames(sectionnms); uidps->setGroupType("Section"); }

    uidps->show();
    return true;
}


#define mSectGeom(sect) (*hor.geometry().sectionGeometry(sect))

bool uiDPSDemo::getRandPositions( const EM::Horizon3D& hor, int nrpts,
       				 DataPointSet& dps )
{
    // This is a bit complex - because we want to handle multiple horizon
    // sections.

    TypeSet<int> nrsectnodes;
    int totnrnodes = 0;
    for ( EM::SectionID isect=0; isect<hor.nrSections(); isect++ )
    {
	const int nrnodes = mSectGeom(isect).nrKnots();
	nrsectnodes += nrnodes;
	totnrnodes += nrnodes;
    }
    if ( totnrnodes < 1 )
	mErrRet( "Horizon is empty" )

    Stats::RandGen::init();
    bool needrandsel = nrpts < totnrnodes;
    const int actualnrpts = needrandsel ? nrpts : totnrnodes;
    const int maxnrunsuccessful = actualnrpts * 1000;
    int nrunsuccessful = 0;
#   define mNextTry() { if ( needrandsel ) ipt--; nrunsuccessful++; continue; }
    for ( int ipt=0; ipt<actualnrpts; ipt++ )
    {
	if ( nrunsuccessful > maxnrunsuccessful )
	    break;

	int selnodenr = needrandsel ? Stats::RandGen::getIndex( totnrnodes )
	    			    : ipt;
	BinID bid; EM::SectionID selsect = 0;
	for ( EM::SectionID isect=0; isect<nrsectnodes.size(); isect++ )
	{
	    if ( nrsectnodes[isect] < selnodenr )
		selnodenr -= nrsectnodes[isect];
	    else
		{ selsect = isect; break; }
	}
	bid = BinID( mSectGeom(selsect).getKnotRowCol(selnodenr) );

	// Checking whether position is already in set. Here, we have to use
	// the BinIDValueSet, because we don't want to call
	// DataPointSet::dataChanged() after every addRow().
	if ( needrandsel && dps.bivSet().valid(bid) )
	    mNextTry()

	const float z = (float) (mSectGeom(selsect).getKnot(bid,false).z);
	if ( mIsUdf(z) )
	    mNextTry()

	// Add the position to the set, set will allocate all the columns.
	// We store section+1 because DataPointSet's groups start at 1
	DataPointSet::Pos dpspos( bid,
						(float) (mSectGeom(selsect).getKnot(bid,false).z) );
	DataPointSet::DataRow dr( dpspos, selsect+1 );
	dps.addRow( dr );
    }

    // This builds the index table in the DataPointSet.
    // Call this after adding or removing rows, then you can use the DPS again.
    dps.dataChanged();

    return true;
}


bool uiDPSDemo::getSeisData( const IOObj& ioobj, DataPointSet& dps,
			     TaskRunner& tr )
{
    SeisTrcReader rdr( &ioobj );
    Seis::TableSelData* tsd = new Seis::TableSelData( dps.bivSet() );
    rdr.setSelData( tsd );
    if ( !rdr.prepareWork() )
	mErrRet(rdr.errMsg())

    SeisTrcBuf tbuf(true);
    SeisBufReader br( rdr, tbuf );
    if ( !tr.execute(br) )
	return false;

    const int icomp = 0; // ignore other components for now
    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	const SeisTrc& trc = *tbuf.get( idx );
	DataPointSet::RowID rid = dps.findFirst( trc.info().binid );
	if ( rid < 0 )
	    { pErrMsg("Huh?"); continue; }

	const float z = dps.z( rid );
	if ( mIsUdf(z) ) continue; // Node in horizon with undef Z ...

	// (finally) fill the DPS row with some stuff from the seismics
	float* vals = dps.getValues( rid );
	vals[0] = trc.getValue( z, icomp );
	if ( !vals[0] )
	    vals[1] = vals[2] = mUdf(float);
	else
	{
	    const float vm1 = trc.getValue( z-trc.info().sampling.step, icomp );
	    const float v1  = trc.getValue( z+trc.info().sampling.step, icomp );
	    vals[1] = (vm1 + v1) * .5f;
	    vals[2] = vm1 - v1;
	    vals[1] /= vals[0]; vals[2] /= vals[0];
	}
	vals[3] = SeisTrcPropCalc(trc,icomp).getFreq( trc.nearestSample(z) );
    }

    return true;
}
