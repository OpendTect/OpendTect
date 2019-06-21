/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "attribprobelayer.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "coltabseqmgr.h"
#include "keystrs.h"
#include "seisioobjinfo.h"
#include "datadistributionextracter.h"


static const char* colSeqNm( const ConstRefMan<ColTab::Sequence>& cseq )
{
    return cseq.ptr() ? cseq->name().str() : 0;
}


AttribProbeLayer::AttribProbeLayer( DispType dt )
    : ProbeLayer()
    , colseq_(0)
    , mapper_(new ColTab::Mapper)
    , attrspec_(*new Attrib::SelSpec())
    , disptype_(dt)
    , selcomp_(0)
{
}


AttribProbeLayer::AttribProbeLayer( const AttribProbeLayer& oth )
    : ProbeLayer(oth)
    , attrspec_(*new Attrib::SelSpec())
{
    copyClassData( oth );
}

mImplMonitorableAssignment( AttribProbeLayer, ProbeLayer )

void AttribProbeLayer::copyClassData( const AttribProbeLayer& oth )
{
    disptype_ = oth.disptype_;
    attrspec_ = oth.attrspec_;
    attribdpid_ = oth.attribdpid_;
    colseq_ = oth.colseq_;
    selcomp_ = oth.selcomp_;
    mapper_ = oth.mapper_->clone();
    attrdp_ = oth.attrdp_;
}


Monitorable::ChangeType AttribProbeLayer::compareClassData(
	                                const AttribProbeLayer& oth ) const
{
    if ( disptype_ != oth.disptype_ )
	return cEntireObjectChange();

    mStartMonitorableCompare();
    mHandleMonitorableCompare( attribdpid_, cDataChange() );
    mHandleMonitorableCompare( attrspec_, cDataChange() );
    mHandleMonitorableCompare( colseq_, cColSeqChange() );
    mHandleMonitorableCompare( mapper_, cMapperChange() );
    mDeliverMonitorableCompare();
}


AttribProbeLayer::~AttribProbeLayer()
{
    sendDelNotif();
    delete &attrspec_;
}


ProbeLayer* AttribProbeLayer::createFrom( const IOPar& par )
{
    AttribProbeLayer* layer = new AttribProbeLayer;
    layer->usePar( par );
    return layer;
}


const AttribProbeLayer::Sequence& AttribProbeLayer::sequence() const
{
    mLock4Read();
    if ( colseq_ )
	return *colseq_;

    return *ColTab::SeqMGR().getDefault( attrspec_.isStored(0) );
}


void AttribProbeLayer::setSequence( const Sequence& seq )
{
    mLock4Read();
    if ( &seq == colseq_ )
	return;

    if ( !mLock2Write() && &seq == colseq_ )
	return;

    if ( replaceMonitoredRef(colseq_,seq,this) )
	mSendChgNotif( cColSeqChange(), cUnspecChgID() );
}


const char* AttribProbeLayer::sFactoryKey()
{
    return sKey::Attribute();
}


const char* AttribProbeLayer::layerType() const
{
    return sFactoryKey();
}


void AttribProbeLayer::initClass()
{
    PrLayFac().addCreateFunc( createFrom, sFactoryKey() );
}


void AttribProbeLayer::fillPar( IOPar& par ) const
{
    ProbeLayer::fillPar( par );

    mLock4Read();
    attrspec_.fillPar( par );

    IOPar ctabpar;
    ctabpar.update( sKey::Name(), colSeqNm(colseq_) );
    mapper_->setup().fillPar( ctabpar );
    par.removeWithKeyPattern( BufferString(sKey::ColTab(),".*") );
    par.mergeComp( ctabpar, sKey::ColTab() );
}


void AttribProbeLayer::usePar( const IOPar& par )
{
    ProbeLayer::usePar( par );
    mLock4Write();
    attrspec_.usePar( par );
    usePar4ColTab( par );
}


void AttribProbeLayer::usePar4ColTab( const IOPar& par )
{
    PtrMan<IOPar> ctabpar = par.subselect( sKey::ColTab() );
    if ( ctabpar && !ctabpar->isEmpty() )
    {
	if ( !colseq_ )
	{
	    BufferString seqnm;
	    ctabpar->get( sKey::Name(), seqnm );
	    if ( !seqnm.isEmpty() && ColTab::SeqMGR().isPresent(seqnm) )
		setSequence( *ColTab::SeqMGR().getAny(seqnm) );
	}
	mapper_->setup().usePar( *ctabpar );
    }
}


void AttribProbeLayer::handleDataPackChange()
{
    DataPackMgr& dpm = DPM( getDataPackManagerID() );
    attrdp_ = dpm.getDP( attribdpid_ );
    selcomp_ = 0;

    const ArrayND<float>* arr = 0;
    if ( attrdp_ && selcomp_ < attrdp_->nrArrays() )
	arr = attrdp_->arrayData( selcomp_<1 ? 0 : selcomp_ );

    if ( !arr )
	mapper_->distribution().setEmpty();
    else
    {
	RangeLimitedDataDistributionExtracter<float> extr( *arr,
						SilentTaskRunnerProvider() );
	mapper_->distribution() = *extr.getDistribution();
    }
}


void AttribProbeLayer::setDataPackID( DataPack::ID dpid )
{
    mLock4Read();

    if ( attribdpid_ == dpid )
	return;

    if ( !mLock2Write() && attribdpid_ == dpid )
	return;

    attribdpid_ = dpid;
    handleDataPackChange();
    mSendChgNotif( cDataChange(), cUnspecChgID() );
}


bool AttribProbeLayer::haveSavedDispPars() const
{
    PtrMan<SeisIOObjInfo> seisobjinfo = gtSeisInfo();
    PtrMan<IOPar> ctabpar;
    if ( seisobjinfo )
    {
	IOPar iop;
	seisobjinfo->getDisplayPars( iop );
	ctabpar = iop.subselect( sKey::ColTab() );
    }

    return ctabpar && !ctabpar->isEmpty();
}


void AttribProbeLayer::setSelSpec( const Attrib::SelSpec& as )
{
    mLock4Read();
    if ( attrspec_ == as )
	return;
    if ( !mLock2Write() && attrspec_ == as )
	return;

    attrspec_ = as;
    attribdpid_ = DataPack::cNoID();
    name_ = attrspec_.userRef();

    PtrMan<SeisIOObjInfo> seisobj = gtSeisInfo();
    IOPar iop;
    if ( !seisobj || !seisobj->getDisplayPars(iop) )
	mapper_->setup().setNotFixed();
    else
	usePar4ColTab( iop );

    mSendChgNotif( cDataChange(), cUnspecChgID() );
}


#define mNrComps() (attrdp_ ? attrdp_->nrArrays() : 0)

int AttribProbeLayer::nrAvialableComponents() const
{
    mLock4Read();
    return mNrComps();
}


void AttribProbeLayer::setSelectedComponent( int icomp )
{
    mLock4Read();

    if ( selcomp_ == icomp )
	return;
    if ( !mLock2Write() && selcomp_ == icomp )
	return;

    const int nrcomps = mNrComps();
    if ( icomp >= nrcomps )
	icomp = nrcomps - 1;
    if ( icomp < 0 )
	icomp = 0;
    selcomp_ = icomp;
    mSendChgNotif( cSelCompChange(), icomp );
}


void AttribProbeLayer::invalidateData()
{
    mLock4Write();
    attribdpid_ = DataPack::cNoID();
}


SeisIOObjInfo* AttribProbeLayer::gtSeisInfo() const
{
    const Attrib::SelSpec selspec = selSpec();

    const Attrib::Desc* desc = Attrib::DescSet::getGlobalDesc( selspec );
    if ( !desc || !desc->isStored() )
	return 0;

    const DBKey storedid = desc->getStoredID();
    if ( storedid.isInvalid() )
	{ pErrMsg("Huh"); return 0; }

    SeisIOObjInfo* seisobjinfo = new SeisIOObjInfo( storedid );
    if ( !seisobjinfo->isOK() )
	{ delete seisobjinfo; seisobjinfo = 0; }

    return seisobjinfo;
}


void AttribProbeLayer::saveDisplayPars()
{
    PtrMan<SeisIOObjInfo> seisobjinfo = gtSeisInfo();
    if ( !seisobjinfo )
	return;

    IOPar iop;
    seisobjinfo->getDisplayPars( iop );

    mLock4Read();
    mapper_->setup().setFixedRange( mapper_->setup().range() );
    IOPar coltabiop;
    coltabiop.set( sKey::Name(), colSeqNm(colseq_) );
    mapper_->setup().fillPar( coltabiop );

    iop.mergeComp( coltabiop, sKey::ColTab() );
    seisobjinfo->saveDisplayPars( iop );
}
