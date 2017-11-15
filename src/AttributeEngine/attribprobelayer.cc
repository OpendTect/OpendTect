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
#include "attribdescsetsholder.h"
#include "coltabseqmgr.h"
#include "dbman.h"
#include "keystrs.h"
#include "seisioobjinfo.h"
#include "datadistributionextracter.h"

AttribProbeLayer::AttribProbeLayer( DispType dt )
    : ProbeLayer()
    , colseq_(ColTab::SeqMGR().getDefault(false))
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
    return *colseq_;
}


void AttribProbeLayer::setSequence( const Sequence& seq )
{
    mLock4Write();
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


const char* AttribProbeLayer::sAttribDataPackID()
{
    return IOPar::compKey( sKey::Attribute(), "DataPackID" );
}


const char* AttribProbeLayer::sAttribColTabName()
{
    return IOPar::compKey( sKey::Attribute(), "ColTabName" );
}


void AttribProbeLayer::initClass()
{
    PrLayFac().addCreateFunc( createFrom, sFactoryKey() );
}


void AttribProbeLayer::fillPar( IOPar& par ) const
{
    mLock4Read();

    ProbeLayer::fillPar( par );
    attrspec_.fillPar( par );
    par.set( sAttribDataPackID(), attribdpid_ );
    par.set( sAttribColTabName(), colseq_->name() );
    mapper_->setup().fillPar( par );
}


bool AttribProbeLayer::usePar( const IOPar& par )
{
    mLock4Write();

    attrspec_.usePar( par );
    par.get( sAttribDataPackID(), attribdpid_ );
    BufferString seqnm( colseq_->name() );
    par.get( sAttribColTabName(), seqnm );
    colseq_ = ColTab::SeqMGR().getAny( seqnm );
    mapper_->setup().usePar( par );
    return true;
}


void AttribProbeLayer::handleDataPackChange()
{
    DataPackMgr& dpm = DPM( getDataPackManagerID() );
    attrdp_ = dpm.get( attribdpid_ );
    selcomp_ = 0;

    const ArrayND<float>* arr = 0;
    if ( attrdp_ && selcomp_ < attrdp_->nrArrays() )
	arr = attrdp_->arrayData( selcomp_<1 ? 0 : selcomp_ );

    bool havedistrib = false;
    if ( arr )
    {
	RangeLimitedDataDistributionExtracter<float> extr( *arr );
	mapper_->distribution() = *extr.getDistribution();
	havedistrib = true;
    }

    if ( !havedistrib )
	mapper_->distribution().setEmpty();
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
    const Attrib::DescSet* attrset =
	Attrib::DSHolder().getDescSet( attrspec_.is2D(), true );
    const Attrib::Desc* desc =
	attrset ? attrset->getDesc( attrspec_.id() ) : 0;
    if ( !desc )
    {
	attrset = Attrib::DSHolder().getDescSet( attrspec_.is2D(), false );
	desc = attrset ? attrset->getDesc( attrspec_.id() ) : 0;
	if ( !desc )
	    return 0;
    }

    DBKey storedid = desc->getStoredID();
    if ( !desc->isStored() || storedid.isInvalid() )
	return 0;

    if ( !DBM().get(storedid) )
	return 0;

    SeisIOObjInfo* seisobj = new SeisIOObjInfo( storedid );
    return seisobj;
}


bool AttribProbeLayer::useDisplayPars()
{
    mLock4Read();

    if ( attrspec_.isNLA() )
	return false;

    PtrMan<SeisIOObjInfo> seisobj = gtSeisInfo();
    if ( !seisobj )
	return false;

    IOPar iop;
    if ( !seisobj->getDisplayPars(iop) )
	return false;

    mLock2Write();

    mapper_->setup().usePar( iop );

    BufferString seqnm = iop.find( sKey::Name() );
    if ( !seqnm.isEmpty() )
	colseq_ = ColTab::SeqMGR().getAny( seqnm );

    mSendChgNotif( cColSeqChange(), cUnspecChgID() );
    return true;
}


void AttribProbeLayer::saveDisplayPars()
{
    mLock4Read();

    if ( attrspec_.isNLA() )
	return;
        
    PtrMan<SeisIOObjInfo> seisobj = gtSeisInfo();
    if ( !seisobj )
	return;

    IOPar iop;
    if ( !seisobj->getDisplayPars(iop) )
	return;

    iop.set( sKey::Name(), colseq_->name() );
    mapper_->setup().fillPar( iop );
    iop.set( sKey::Type(), "Fixed" );

    seisobj->saveDisplayPars( iop );
}
