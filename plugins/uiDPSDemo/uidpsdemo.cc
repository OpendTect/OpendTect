/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2009
________________________________________________________________________

-*/

#include "uidpsdemo.h"

#include "datapointset.h"
#include "binnedvalueset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "statrand.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon3d.h"
#include "binidsurface.h"
#include "seistrc.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seistrctr.h"
#include "seistrcprop.h"
#include "seistableseldata.h"

#include "uiseissel.h"
#include "uigeninput.h"
#include "uidatapointset.h"
#include "uitaskrunnerprovider.h"
#include "uimsg.h"


uiDPSDemo::uiDPSDemo( uiParent* p, DataPointSetDisplayMgr* dpsdispmgr )
	: uiDialog(p,Setup(tr("DataPointSet demo"),
                           tr("Data extraction parameters"),
			   mNoHelpKey))
	, dps_(0)
	, dpsdispmgr_(dpsdispmgr)
{
    horfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D) );

    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    seisfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(false,false) );
    seisfld_->attach( alignedBelow, horfld_ );

    nrptsfld_ = new uiGenInput( this, tr("Number of points to extract"),
				IntInpSpec(10000) );
    nrptsfld_->attach( alignedBelow, seisfld_ );
}


uiDPSDemo::~uiDPSDemo()
{}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiDPSDemo::acceptOK()
{
    const IOObj* horioobj = horfld_->ioobj(); // emits its own error message
    if ( !horioobj ) return false;
    const IOObj* seisioobj = seisfld_->ioobj(); // this one, too
    if ( !seisioobj ) return false;

    const int nrpts = nrptsfld_->getIntValue();
    if ( nrpts < 2 )
	mErrRet( uiStrings::phrEnter(tr("a valid number of points")) )

    return doWork( *horioobj, *seisioobj, nrpts );
}



bool uiDPSDemo::doWork( const IOObj& horioobj, const IOObj& seisioobj,
			int nrpts )
{
    uiTaskRunnerProvider trprov( this );
    EM::Object* emobj = EM::MGR().loadIfNotFullyLoaded( horioobj.key(),
							  trprov );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) return false;

    dps_ = new DataPointSet( false );
    dps_->dataSet().add( new DataColDef("Amplitude") );
    dps_->dataSet().add( new DataColDef("Peakedness") );
    dps_->dataSet().add( new DataColDef("PeakSkew") );
    dps_->dataSet().add( new DataColDef("Frequency") );
    DPM(DataPackMgr::PointID()).add( dps_ );

    hor->ref();
    const bool isok = getRandPositions(*hor,nrpts,*dps_);
    hor->unRef();

    if ( !isok || !getSeisData(seisioobj,*dps_,trprov) )
	return false;

    uiString wintitl( toUiString("%1/%2").arg(horioobj.name()).
		      arg(seisioobj.name()) );
    uiDataPointSet::Setup su( wintitl, false );
    su.canaddrow( true );
    uiDataPointSet* uidps =
	new uiDataPointSet( parent(), *dps_, su, dpsdispmgr_ );

    uidps->show();
    return true;
}


#define mGeomElem (*hor.geometry().geometryElement())

bool uiDPSDemo::getRandPositions( const EM::Horizon3D& hor, int nrpts,
				 DataPointSet& dps )
{
    const int nrnodes = mGeomElem.nrKnots();
    if ( nrnodes < 1 )
	mErrRet( tr("Horizon is empty") )

    bool needrandsel = nrpts < nrnodes;
    const int actualnrpts = needrandsel ? nrpts : nrnodes;
    const int maxnrunsuccessful = actualnrpts * 1000;
    int nrunsuccessful = 0;
#   define mNextTry() { if ( needrandsel ) ipt--; nrunsuccessful++; continue; }
    for ( int ipt=0; ipt<actualnrpts; ipt++ )
    {
	if ( nrunsuccessful > maxnrunsuccessful )
	    break;

	int selnodenr = needrandsel ? Stats::randGen().getIndex( nrnodes )
				    : ipt;
	BinID bid = BinID( mGeomElem.getKnotRowCol(selnodenr) );

	// Checking whether position is already in set. Here, we have to use
	// the BinnedValueSet, because we don't want to call
	// DataPointSet::dataChanged() after every addRow().
	if ( needrandsel && dps.bivSet().isValid(bid) )
	    mNextTry()

	const float z = (float) (mGeomElem.getKnot(bid,false).z_);
	if ( mIsUdf(z) )
	    mNextTry()

	// Add the position to the set, set will allocate all the columns.
	// We store 1 because DataPointSet's groups start at 1
	DataPointSet::Pos dpspos( bid,
		    (float) (mGeomElem.getKnot(bid,false).z_) );
	DataPointSet::DataRow dr( dpspos, 1 );
	dps.addRow( dr );
    }

    // This builds the index table in the DataPointSet.
    // Call this after adding or removing rows, then you can use the DPS again.
    dps.dataChanged();

    return true;
}


bool uiDPSDemo::getSeisData( const IOObj& ioobj, DataPointSet& dps,
			     TaskRunnerProvider& trprov )
{
    uiRetVal uirv;
    Seis::Provider* prov = Seis::Provider::create( ioobj, &uirv );
    if ( !prov )
	mErrRet( uirv );

    prov->setSelData( new Seis::TableSelData(dps.bivSet()) );

    SeisTrcBuf tbuf(true);
    SeisBufReader br( *prov, tbuf );
    if ( !trprov.execute( br ) )
	return false;

    const int icomp = 0; // ignore other components for now
    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	const SeisTrc& trc = *tbuf.get( idx );
	DataPointSet::RowID rid = dps.findFirst( trc.info().binID() );
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
	    const float vm1 =
		trc.getValue( z-trc.info().sampling_.step, icomp );
	    const float v1  =
		trc.getValue( z+trc.info().sampling_.step, icomp );
	    vals[1] = (vm1 + v1) * .5f;
	    vals[2] = vm1 - v1;
	    vals[1] /= vals[0]; vals[2] /= vals[0];
	}
	vals[3] = SeisTrcPropCalc(trc,icomp).getFreq( trc.nearestSample(z) );
    }

    return true;
}
