/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id$";

#include "attriboutput.h"

#include "arraynd.h"
#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "bufstringset.h"
#include "binidvalset.h"
#include "convmemvalseries.h"
#include "datapointset.h"
#include "hiddenparam.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seiscbvs.h"
#include "seiscbvs2d.h"
#include "seistrc.h"
#include "seisselectionimpl.h"
#include "seistrctr.h"
#include "seistype.h"
#include "seiswrite.h"
#include "separstr.h"
#include "survinfo.h"

#include "attribsteering.h"		//tmp fix for od4.4

namespace Attrib
{

const char* Output::outputstr()	    { return sKey::Output; }
const char* Output::cubekey()	    { return sKey::Cube; }
const char* Output::surfkey()	    { return sKey::Surface; }
const char* Output::tskey()	    { return "Trace Selection"; }
const char* Output::scalekey()	    { return sKey::Scale; }
const char* Output::varzlinekey()   { return "Variable Z Line"; }

const char* SeisTrcStorOutput::seisidkey()	{ return "Seismic.ID"; }
const char* SeisTrcStorOutput::attribkey()	{ return sKey::Attributes; }
const char* SeisTrcStorOutput::inlrangekey()	{ return "In-line range"; }
const char* SeisTrcStorOutput::crlrangekey()	{ return "Cross-line range"; }
const char* SeisTrcStorOutput::depthrangekey()	{ return "Depth range"; }

const char* LocationOutput::filenamekey()	{ return "Output.File name"; }
const char* LocationOutput::locationkey()	{ return "Locations"; }
const char* LocationOutput::attribkey()		{ return sKey::Attribute; }
const char* LocationOutput::surfidkey()		{ return "Surface.ID"; }

HiddenParam<Output,BufferString> mainattrparmgr( "" );

Output::Output()
    : seldata_(new Seis::RangeSelData(true))
{
    seldata_->setIsAll( true );
}


Output::~Output()
{
    delete seldata_;
}


void Output::setMainAttrName( const BufferString& manm )
{
    mainattrparmgr.setParam( this, manm );
}


bool Output::wantsOutput( const BinID& bid ) const
{
    return wantsOutput( SI().transform( bid ) );
}


bool Output::wantsOutput( const Coord& c ) const
{
    return wantsOutput( SI().transform( c ) );
}


TypeSet<Interval<int> > Output::getLocalZRanges( const BinID& bid, float f,
						 TypeSet<float>& ts ) const
{
    return getLocalZRanges( SI().transform(bid), f, ts );
}


TypeSet<Interval<int> > Output::getLocalZRanges( const Coord& c, float f,
						 TypeSet<float>& ts ) const
{
    return getLocalZRanges( SI().transform(c), f, ts );
}


const LineKey& Output::curLineKey() const
{
    return seldata_->lineKey();
}


void Output::ensureSelType( Seis::SelType st )
{
    if ( seldata_->type() != st )
    {
	Seis::SelData* newseldata = Seis::SelData::get( st );
	newseldata->lineKey() = seldata_->lineKey();
	delete seldata_; seldata_ = newseldata;
    }
}


void Output::doSetGeometry( const CubeSampling& cs )
{
    if ( cs.isEmpty() ) return;

    ensureSelType( Seis::Range );
    ((Seis::RangeSelData*)seldata_)->cubeSampling() = cs;
}


DataCubesOutput::DataCubesOutput( const CubeSampling& cs )
    : desiredvolume_(cs)
    , dcsampling_(cs)
    , datacubes_(0)
    , udfval_(mUdf(float))
{
}


DataCubesOutput::~DataCubesOutput()
{ if ( datacubes_ ) datacubes_->unRef(); }


bool DataCubesOutput::getDesiredVolume( CubeSampling& cs ) const
{ cs=desiredvolume_; return true; }


bool DataCubesOutput::wantsOutput( const BinID& bid ) const
{ return desiredvolume_.hrg.includes(bid); }


TypeSet< Interval<int> > DataCubesOutput::getLocalZRanges( const BinID&,
							   float zstep,
       							   TypeSet<float>&)const
{
    if ( sampleinterval_.size() ==0 )
    {
	Interval<int> interval( mNINT32( desiredvolume_.zrg.start / zstep ),
				mNINT32( desiredvolume_.zrg.stop / zstep ) );
	const_cast<DataCubesOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_; 
}


void DataCubesOutput::adjustInlCrlStep( const CubeSampling& cs )
{
    if ( cs.hrg.step.inl > desiredvolume_.hrg.step.inl )
    {
	desiredvolume_.hrg.step.inl = cs.hrg.step.inl;
	dcsampling_.hrg.step.inl = cs.hrg.step.inl;
	desiredvolume_.hrg.start.inl = cs.hrg.start.inl;
    }
    if ( cs.hrg.step.crl > desiredvolume_.hrg.step.crl )
    {
	desiredvolume_.hrg.step.crl = cs.hrg.step.crl;
	dcsampling_.hrg.step.crl = cs.hrg.step.crl;
	desiredvolume_.hrg.start.crl = cs.hrg.start.crl;
    }
}


#define mGetSz(dir)\
	dir##sz = (dcsampling_.hrg.stop.dir - dcsampling_.hrg.start.dir)\
		  /dcsampling_.hrg.step.dir + 1;\

#define mGetZSz()\
	zsz = mNINT32( ( dcsampling_.zrg.stop - dcsampling_.zrg.start )\
	      /refstep + 1 );

void DataCubesOutput::collectData( const DataHolder& data, float refstep, 
				  const SeisTrcInfo& info )
{
    if ( !datacubes_ )
	init( refstep );

    if ( !datacubes_->includes(info.binid) )
	return;
		
    const int totalnrcubes = desoutputs_.size();
    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	if ( desout<datacubes_->nrCubes() )
	    continue;

	mDynamicCastGet( ConvMemValueSeries<float>*, cmvs,
			 data.series(desoutputs_[desout]) );

	if ( cmvs && cmvs->handlesUndef() )
	{
	    const BinDataDesc desc = cmvs->dataDesc();
	    datacubes_->addCube(mUdf(float), &desc );
	}
	else
	    datacubes_->addCube(mUdf(float));
    }

    int zsz; 
    mGetZSz();

    const Interval<int> inputrg( data.z0_, data.z0_+data.nrsamples_ - 1 );
    const Interval<int> outrg( datacubes_->z0_, datacubes_->z0_+zsz-1 );

    if ( !inputrg.overlaps(outrg,false) )
	return;

    const Interval<int> transrg( mMAX(inputrg.start, outrg.start),
	    			 mMIN(inputrg.stop, outrg.stop ) );

    const int inlidx =
	datacubes_->inlsampling_.nearestIndex(info.binid.inl);
    const int crlidx =
	datacubes_->crlsampling_.nearestIndex(info.binid.crl);

    for ( int desout=0; desout<desoutputs_.size(); desout++ )
    {
	if ( !data.series(desoutputs_[desout]) )
	    continue;

	mDynamicCastGet( ConvMemValueSeries<float>*, cmvs,
			 data.series(desoutputs_[desout]) );

	if ( !cmvs )
	{
	    for ( int idx=transrg.start; idx<=transrg.stop; idx++)
	    {
		const float val =
		    data.series(desoutputs_[desout])->value(idx-data.z0_);

		datacubes_->setValue( desout, inlidx, crlidx,
				      idx-datacubes_->z0_, val);
	    }
	}
	else
	{
	    mDynamicCastGet( ConvMemValueSeries<float>*, deststor,
		    	     datacubes_->getCube(desout).getStorage() );
	    const char elemsz = cmvs->dataDesc().nrBytes();

	    const od_int64 destoffset = transrg.start-datacubes_->z0_ +
		datacubes_->getCube(desout).info().getOffset(inlidx,crlidx,0);

	    char* dest = deststor->storArr() + destoffset * elemsz;
	    char* src = cmvs->storArr() +
			(transrg.start-data.z0_) * elemsz;
	    memcpy( dest, src, elemsz*(transrg.width()+1) );
	}
    }
}


const DataCubes* DataCubesOutput::getDataCubes() const
{
    return datacubes_;
}


DataCubes* DataCubesOutput::getDataCubes( float refstep )
{
    if ( !datacubes_ )
	init( refstep );

    return datacubes_;
}


void DataCubesOutput::init( float refstep )
{
    datacubes_ = new Attrib::DataCubes;
    datacubes_->ref();
    datacubes_->inlsampling_= StepInterval<int>(dcsampling_.hrg.start.inl,
						dcsampling_.hrg.stop.inl,
						dcsampling_.hrg.step.inl);
    datacubes_->crlsampling_= StepInterval<int>(dcsampling_.hrg.start.crl,
						dcsampling_.hrg.stop.crl,
						dcsampling_.hrg.step.crl);
    datacubes_->z0_ = mNINT32(dcsampling_.zrg.start/refstep);
    datacubes_->zstep_ = refstep;
    int inlsz, crlsz, zsz;
    mGetSz(inl); mGetSz(crl); mGetZSz();
    datacubes_->setSize( inlsz, crlsz, zsz );
}


SeisTrcStorOutput::SeisTrcStorOutput( const CubeSampling& cs,
				      const LineKey& lk )
    : desiredvolume_(cs)
    , auxpars_(0)
    , storid_(*new MultiID)
    , writer_(0)
    , trc_(0)
    , prevpos_(-1,-1)
    , storinited_(0)
    , errmsg_(0)
    , scaler_(0)
    , growtrctosi_(false)
{
    seldata_->lineKey() = lk;
    attribname_ = lk.attrName();
}


bool SeisTrcStorOutput::getDesiredVolume( CubeSampling& cs ) const
{
    cs = desiredvolume_;
    return true;
}


bool SeisTrcStorOutput::wantsOutput( const BinID& bid ) const
{
    return desiredvolume_.hrg.includes(bid);
}


bool SeisTrcStorOutput::setStorageID( const MultiID& storid )
{
    if ( !storid.isEmpty() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid;
	    return false;
	}
    }

    storid_ = storid;
    return true;
}


SeisTrcStorOutput::~SeisTrcStorOutput()
{
    delete writer_;
    delete &storid_;
    delete auxpars_;
    delete scaler_;
}


bool SeisTrcStorOutput::doUsePar( const IOPar& pars )
{
    errmsg_ = "";
    PtrMan<IOPar> outppar = pars.subselect( IOPar::compKey(sKey::Output,0) );
    if ( !outppar ) outppar = pars.subselect( IOPar::compKey(sKey::Output,1) );
    if ( !outppar )
    {
        errmsg_ = "Could not find Output keyword in parameter file";
	return false;
    }

    const char* storid = outppar->find( seisidkey() );
    if ( !setStorageID( storid ) )
    {
        errmsg_ = "Could not find output ID: "; errmsg_ += storid;
        return false;
    }

    SeparString sepstr( storid, '|' );

    if ( sepstr[1] && *sepstr[1] )
	attribname_ = sepstr[1];

    if ( sepstr[2] && *sepstr[2] && isDataType(sepstr[2]) )
	datatype_ += sepstr[2];

    const char* res = outppar->find( scalekey() );
    if ( res )
    {
	scaler_ = new LinScaler;
	scaler_->fromString( res );
	if ( scaler_->isEmpty() )
	    { delete scaler_; scaler_ = 0; }
    }
    
    auxpars_ = pars.subselect("Aux");
    return doInit();
    return true;
}//warning, only a small part of the old taken, see if some more is required


bool SeisTrcStorOutput::doInit()
{
    ensureSelType( Seis::Range );

    if ( *storid_.buf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid_;
	    return false;
	}

	writer_ = new SeisTrcWriter( ioseisout );
	is2d_ = writer_->is2D();

	if ( is2d_ && !datatype_.isEmpty() )
	    writer_->setDataType( datatype_.buf() );
	    

	if ( auxpars_ )
	{
	    writer_->lineAuxPars().merge( *auxpars_ );
	    delete auxpars_; auxpars_ = 0;
	}
    }

    desiredvolume_.normalise();
    seldata_->lineKey().setAttrName( "" );

    if ( !is2d_ )
    {
	CubeSampling& cs = ((Seis::RangeSelData*)seldata_)->cubeSampling();
	desiredvolume_.limitTo( cs );
    }

    return true;
}


bool SeisTrcStorOutput::isDataType( const char* reqdatatype) const
{
    BufferString datatypeinques;

    if ( !strcmp(reqdatatype, sKey::Steering) )
	datatypeinques += "Dip";
    else
	datatypeinques += reqdatatype;

    BufferStringSet datatypes( Seis::dataTypeNames() );

    if ( datatypes.indexOf(datatypeinques.buf()) < 0 )
	return false;

    return true;
}


class COLineKeyProvider : public LineKeyProvider
{
public:

COLineKeyProvider( const char* a, const char* lk )
	: attrnm_(a) , linename_(lk) {}

LineKey lineKey() const
{
    LineKey lk(linename_,attrnm_);
    return lk;
}
    BufferString        attrnm_;
    BufferString 	linename_;

};


void SeisTrcStorOutput::collectData( const DataHolder& data, float refstep, 
				     const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size())
	return;

    const int sz = data.nrsamples_;
    DataCharacteristics dc;

    if ( !trc_ )
    {
	trc_ = new SeisTrc( sz, dc );
	trc_->info() = info;
	trc_->info().sampling.step = refstep;
	trc_->info().sampling.start = data.z0_*refstep;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }
    else if ( trc_->info().binid != info.binid )
    {
	errmsg_ = "merge components of two different traces!";
	return;	    
    }
    else
    {
	const int curnrcomps = trc_->nrComponents();
	for ( int idx=curnrcomps; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( sz, dc, false );
    }

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    float val = data.series(desoutputs_[comp])->value(idx);
	    trc_->set(idx, val, comp);
	}
    }
    
    if ( scaler_ )
    {
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = scaler_->scale( val );
		trc_->set( idx, val, icomp );
	    }
	}
    }

    //tmp fix for od4.4: use arbitrary mStd2DTrcSpacing for steering 2D
    //instead of ( usually huge) bin size entered by user
    if ( writer_ && writer_->is2D() && isDataType(sKey::Steering) )
    {
	trc_->info().pick = 0;
	if ( mainattrparmgr.getParam(this) == BufferString("VolumeStatistics") )
	   return;

	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<sz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = val * SI().crlDistance() / mStd2DTrcSpacing;
		trc_->set( idx, val, icomp );
	    }
	}
    }
}


void SeisTrcStorOutput::writeTrc()
{
    if ( !writer_ || !trc_ ) return;

    SeisTrc* usetrc = trc_;
    PtrMan<SeisTrc> tmptrc = 0;
    if ( growtrctosi_ )
    {
	tmptrc = trc_->getExtendedTo( SI().zRange(true), true );
	usetrc = tmptrc;
    }
    
    if ( !storinited_ )
    {
	SeisTrcTranslator* transl = 0;
	if ( writer_->is2D() )
	{
	    if ( IOObj::isKey(attribname_) )
		attribname_ = IOM().nameOf(attribname_);

	    writer_->setLineKeyProvider( 
		new COLineKeyProvider( attribname_, curLineKey().lineName()) );
	}
	else
	{
	    transl = writer_->seisTranslator();
	    if ( transl )
		transl->setComponentNames( outpnames_ );
	}

	if ( !writer_->prepareWork(*usetrc) )
	    { errmsg_ = writer_->errMsg(); return; }

	if ( writer_->is2D() )
	{
	    if ( writer_->linePutter() )
	    {
		mDynamicCastGet( SeisCBVS2DLinePutter*, lp,
				 writer_->linePutter() )
		if ( lp && lp->tr )
		    lp->tr->setComponentNames( outpnames_ );
	    }
	}

	if ( transl )
	{
	    ObjectSet<SeisTrcTranslator::TargetComponentData>& cis
		             = transl->componentInfo();
	    for ( int idx=0; idx<cis.size(); idx++ )
		cis[idx]->datatype = outptypes_.size() ? outptypes_[idx] : 
		    					Seis::UnknowData;
	}

	storinited_ = true;
    }
    
    if ( !writer_->put(*usetrc) )
	{ errmsg_ = writer_->errMsg(); }

    delete trc_;
    trc_ = 0;
}


TypeSet< Interval<int> > SeisTrcStorOutput::getLocalZRanges( 
						    const BinID& bid,
						    float zstep,
						    TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<int> interval( mNINT32(desiredvolume_.zrg.start/zstep), 
				mNINT32(desiredvolume_.zrg.stop/zstep) );
	const_cast<SeisTrcStorOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


void SeisTrcStorOutput::deleteTrc()
{
    if ( trc_ ) delete trc_;
    trc_ = 0;
}


bool SeisTrcStorOutput::finishWrite()                                           
{                                                                               
    return writer_->close();                                                    
}


TwoDOutput::TwoDOutput( const Interval<int>& trg, const Interval<float>& zrg,
			const LineKey& lk )
    : errmsg_(0)
    , output_( 0 )
{
    seldata_->lineKey() = lk;
    setGeometry( trg, zrg );
    const bool undeftrg = trg.start<=0 && Values::isUdf(trg.stop);
    seldata_->setIsAll( undeftrg );
}


TwoDOutput::~TwoDOutput()
{
    if ( output_ ) output_->unRef();
}


bool TwoDOutput::wantsOutput( const BinID& bid ) const
{
    return seldata_->crlRange().includes(bid.crl,true);
} 
 

void TwoDOutput::setGeometry( const Interval<int>& trg,
			      const Interval<float>& zrg )
{
    ensureSelType( Seis::Range );
    seldata_->setZRange( zrg );
    seldata_->setCrlRange( trg );
}


bool TwoDOutput::getDesiredVolume( CubeSampling& cs ) const
{
    const Interval<int> rg( seldata_->crlRange() );
    cs.hrg.start.crl = rg.start; cs.hrg.stop.crl = rg.stop;
    const Interval<float> zrg( seldata_->zRange() );
    cs.zrg = StepInterval<float>( zrg.start, zrg.stop, SI().zStep() );
    cs.hrg.start.inl = cs.hrg.stop.inl = 0;
    return true;
}


bool TwoDOutput::doInit()
{
    seldata_->lineKey().setAttrName( "" );
    const Interval<int> rg( seldata_->crlRange() );
    if ( rg.start <= 0 && Values::isUdf(rg.stop) )
	seldata_->setIsAll( true );

    return true;
}


void TwoDOutput::collectData( const DataHolder& data, float refstep,
			      const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size() )
	return;

    if ( !output_ ) return;

    output_->dataset_ += data.clone();

    SeisTrcInfo* trcinfo = new SeisTrcInfo(info);
    trcinfo->sampling.step = refstep;
    //trcinfo->sampling.start = 0;
    output_->trcinfoset_ += trcinfo;
}


void TwoDOutput::setOutput( Data2DHolder& no )
{
    if ( output_ ) output_->unRef();
    output_ = &no;
    output_->ref();
}


TypeSet< Interval<int> > TwoDOutput::getLocalZRanges( const BinID& bid,
						      float zstep,
       						      TypeSet<float>& ) const
{
    if ( sampleinterval_.size() == 0 )
    {
	Interval<float> zrg( seldata_->zRange() );
	Interval<int> interval( mNINT32(zrg.start/zstep), mNINT32(zrg.stop/zstep) );
	const_cast<TwoDOutput*>(this)->sampleinterval_ += interval;
    }
    return sampleinterval_;
}


LocationOutput::LocationOutput( BinIDValueSet& bidvalset )
    : bidvalset_(bidvalset)
{
    ensureSelType( Seis::Table );
    seldata_->setIsAll( false );
    ((Seis::TableSelData*)seldata_)->binidValueSet().allowDuplicateBids( true );
    ((Seis::TableSelData*)seldata_)->binidValueSet() = bidvalset;

    arebiddupl_ = areBIDDuplicated();
}


void LocationOutput::collectData( const DataHolder& data, float refstep,
				  const SeisTrcInfo& info )
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( info.binid );
    if ( !pos.valid() ) return;

    const int desnrvals = desoutputs_.size()+1;
    if ( bidvalset_.nrVals() < desnrvals )
	bidvalset_.setNrVals( desnrvals );

    if ( !arebiddupl_ )
    {
	float* vals = bidvalset_.getVals( pos );
	computeAndSetVals( data, refstep, vals );
	return;
    }

    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    while ( true )
    {
	float* vals = bidvalset_.getVals( pos );
	int lowz;
	DataHolder::getExtraZAndSampIdxFromExactZ( vals[0], refstep, lowz );
	const int highz = lowz + 1;
	bool isfulldataok = datarg.includes(lowz-1,false) &&
	    		    datarg.includes(highz+1,false);
	bool canusepartdata = data.nrsamples_<4 && datarg.includes(lowz,false) 
			      && datarg.includes(highz,false);
	if ( isfulldataok || canusepartdata )
	    computeAndSetVals( data, refstep, vals );

	bidvalset_.next( pos );
	if ( info.binid != bidvalset_.getBinID(pos) )
	    break;
    }
}


void LocationOutput::computeAndSetVals( const DataHolder& data, float refstep,
					float* vals )
{
    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	int serieidx = desoutputs_[comp];
	vals[comp+1] = data.getValue( serieidx, vals[0], refstep );
    }
}


bool LocationOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
}


TypeSet< Interval<int> > LocationOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz) const
{
    //TODO not 100% optimized, case of picksets for instance->find better algo
    TypeSet< Interval<int> > sampleinterval;
    
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    while ( pos.valid() )
    {
	const float* vals = bidvalset_.getVals( pos );
	int zidx;
	DataHolder::getExtraZAndSampIdxFromExactZ( vals[0], zstep, zidx );
	Interval<int> interval( zidx, zidx );
	if ( arebiddupl_ )
	{
	    interval.start = zidx - 1;
	    interval.stop =  zidx + 2;
	}
	bool intvadded = sampleinterval.addIfNew( interval );
	if ( intvadded )
	    exactz += vals[0];
	
	bidvalset_.next( pos );
	if ( bid != bidvalset_.getBinID(pos) )
	    break;
    }

    return sampleinterval;
}


bool LocationOutput::areBIDDuplicated() const
{
    BinIDValueSet tmpset(bidvalset_);
    tmpset.allowDuplicateBids(false);
    
    return tmpset.totalSize()<bidvalset_.totalSize();
}

TrcSelectionOutput::TrcSelectionOutput( const BinIDValueSet& bidvalset,
					float outval )
    : bidvalset_(bidvalset)
    , outpbuf_(0)
    , outval_(outval)
    , stdstarttime_(mUdf(float))
    , stdtrcsz_(mUdf(float))
{
    delete seldata_;
    Seis::TableSelData& sd = *new Seis::TableSelData( bidvalset );
    seldata_ = &sd;
    sd.binidValueSet().allowDuplicateBids( true );

    const int nrinterv = bidvalset.nrVals() / 2;
    float zmin = mUdf(float);
    float zmax = -mUdf(float);
    for ( int idx=0; idx<nrinterv; idx+=2 )
    {
	float val = bidvalset.valRange(idx).start;
	if ( val < zmin ) zmin = val;
	val = bidvalset.valRange(idx+1).stop;
	if ( val > zmax ) zmax = val;
    }

    if ( !mIsUdf(zmin) && !mIsUdf(-zmax) )
    {
	BinIDValueSet::Pos pos;
	bidvalset.next( pos );
	sd.binidValueSet().add( bidvalset.getBinID(pos), zmin );
	sd.binidValueSet().add( bidvalset.getBinID(pos), zmax );
	
	stdtrcsz_ = zmax - zmin;
	stdstarttime_ = zmin;
    }
}


TrcSelectionOutput::~TrcSelectionOutput()
{
}


void TrcSelectionOutput::collectData( const DataHolder& data, float refstep,
				      const SeisTrcInfo& info )
{
    const int nrcomp = data.nrSeries();
    if ( !outpbuf_ || !nrcomp || nrcomp < desoutputs_.size() )
	return;

    const int trcsz = mNINT32(stdtrcsz_/refstep) + 1;
    const float globalsttime = stdstarttime_;
    const float trcstarttime = ( (int)(globalsttime/refstep) +1 ) * refstep;
    const int startidx = data.z0_ - mNINT32(trcstarttime/refstep);
    const int index = outpbuf_->find( info.binid );

    SeisTrc* trc;
    if ( index == -1 )
    {
	const DataCharacteristics dc;
	trc = new SeisTrc( trcsz, dc );
	for ( int idx=trc->data().nrComponents(); idx<desoutputs_.size(); idx++)
	    trc->data().addComponent( trcsz, dc, false );

	trc->info() = info;
	trc->info().sampling.start = trcstarttime;
	trc->info().sampling.step = refstep;
    }
    else
	trc = outpbuf_->get( index );

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<trcsz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc->set( idx, outval_, comp );
	    else  
	    {
		const float val = 
		    data.series(desoutputs_[comp])->value(idx-startidx);
		trc->set( idx, val, comp );
	    }
	}
    }

    if ( index == -1 )
	outpbuf_->add( trc );
}


bool TrcSelectionOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    return pos.valid();
}


void TrcSelectionOutput::setOutput( SeisTrcBuf* outp_ )
{
    outpbuf_ = outp_;
    if ( outpbuf_ )
	outpbuf_->erase();
}


void TrcSelectionOutput::setTrcsBounds( Interval<float> intv )
{
    stdstarttime_ = intv.start;
    stdtrcsz_ = intv.stop - intv.start;
    seldata_->setZRange( intv );
}


void TrcSelectionOutput::setLineKey( const LineKey& linekey )
{
    seldata_->lineKey() = linekey;
}


TypeSet< Interval<int> > TrcSelectionOutput::getLocalZRanges(
						const BinID& bid, float zstep,
       						TypeSet<float>&	) const
{
    BinIDValueSet::Pos pos = bidvalset_.findFirst( bid );
    BinID binid;
    TypeSet<float> values;
    bidvalset_.get( pos, binid, values );
    TypeSet< Interval<int> > sampleinterval;

    if ( values.isEmpty() && !mIsUdf(stdstarttime_) )
    {
	const float zmax = stdstarttime_ + stdtrcsz_;
	Interval<int> interval( mNINT32(stdstarttime_/zstep),
				mNINT32(zmax/zstep) );
	sampleinterval += interval;
	return sampleinterval;
    }

    for ( int idx=0; idx<values.size()/2; idx+=2 )
    {
	Interval<int> interval( mNINT32(values[idx]/zstep), 
				mNINT32(values[idx+1]/zstep) );
	sampleinterval += interval;
    }
 
    return sampleinterval;
}


const CubeSampling Trc2DVarZStorOutput::getCS()
{
    CubeSampling cs;
    cs.hrg.start.inl = 0; cs.hrg.stop.inl = mUdf(int);
    cs.hrg.start.crl = 1; cs.hrg.stop.crl = mUdf(int);
    return cs;
}


Trc2DVarZStorOutput::Trc2DVarZStorOutput( const LineKey& lk,
       					  DataPointSet* poszvalues,
					  float outval )
    : SeisTrcStorOutput( getCS(), lk )
    , poszvalues_(poszvalues)
    , outval_(outval)
{
    is2d_ = true;
    const int nrinterv = (poszvalues_->nrCols()+1)/2;
    float zmin = mUdf(float);
    float zmax = -mUdf(float);
    for ( int idx=0; idx<nrinterv; idx+=2 )
    {
	int z1colidx = idx==0 ? idx : idx+poszvalues_->nrFixedCols();
	int z2colidx = idx==0 ? poszvalues_->nrFixedCols() : z1colidx+1;
	float val = poszvalues_->bivSet().valRange( z1colidx ).start;
	if ( val < zmin ) zmin = val;
	val = poszvalues_->bivSet().valRange( z2colidx ).stop;
	if ( val > zmax ) zmax = val;
    }

    setGeometry( getCS() );
    seldata_->setZRange( Interval<float>(zmin,zmax) );
    stdtrcsz_ = zmax - zmin;
    stdstarttime_ = zmin;
}


void Trc2DVarZStorOutput::setTrcsBounds( Interval<float> intv )
{
    stdstarttime_ = intv.start;
    stdtrcsz_ = intv.stop - intv.start;
}


bool Trc2DVarZStorOutput::doInit()
{
    ensureSelType( Seis::Range );
    if ( *storid_.buf() )
    {
	PtrMan<IOObj> ioseisout = IOM().get( storid_ );
	if ( !ioseisout )
	{
	    errmsg_ = "Cannot find seismic data with ID: "; errmsg_ += storid_;
	    return false;
	}

	writer_ = new SeisTrcWriter( ioseisout );
	if ( !writer_->is2D() )
	{
	    errmsg_ = "Seismic data with ID: "; errmsg_ += storid_; 
	    errmsg_ +="is not 2D\nCannot create 2D output.";
	    return false;
	}
	
	if ( auxpars_ )
	{
	    writer_->lineAuxPars().merge( *auxpars_ );
	    delete auxpars_; auxpars_ = 0;
	}
    }

    desiredvolume_.normalise();
    seldata_->lineKey().setAttrName( "" );

    return true;
}


void Trc2DVarZStorOutput::collectData( const DataHolder& data, float refstep, 
				     const SeisTrcInfo& info )
{
    int nrcomp = data.nrSeries();
    if ( !nrcomp || nrcomp < desoutputs_.size())
	return;

    const int trcsz = mNINT32(stdtrcsz_/refstep) + 1;
    const float globalsttime = stdstarttime_;
    const float trcstarttime = ( (int)(globalsttime/refstep) +1 ) * refstep;
    const int startidx = data.z0_ - mNINT32(trcstarttime/refstep);
    DataCharacteristics dc;

    if ( !trc_ )
    {
	trc_ = new SeisTrc( trcsz, dc );
	trc_->info() = info;
	trc_->info().sampling.step = refstep;
	trc_->info().sampling.start = trcstarttime;
	for ( int idx=1; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( trcsz, dc, false );
    }
    else if ( trc_->info().binid != info.binid )
    {
	errmsg_ = "merge components of two different traces!";
	return;	    
    }
    else
    {
	const int curnrcomps = trc_->nrComponents();
	for ( int idx=curnrcomps; idx<desoutputs_.size(); idx++)
	    trc_->data().addComponent( trcsz, dc, false );
    }

    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	for ( int idx=0; idx<trcsz; idx++ )
	{
	    if ( idx < startidx || idx>=startidx+data.nrsamples_ )
		trc_->set( idx, outval_, comp );
	    else
	    {
		float val = data.series(desoutputs_[comp])->value(idx-startidx);
		trc_->set(idx, val, comp);
	    }
	}
    }
    
    if ( scaler_ )
    {
	for ( int icomp=0; icomp<trc_->data().nrComponents(); icomp++ )
	{
	    for ( int idx=0; idx<trcsz; idx++ )
	    {
		float val = trc_->get( idx, icomp );
		val = scaler_->scale( val );
		trc_->set( idx, val, icomp );
	    }
	}
    }
}


TypeSet< Interval<int> > Trc2DVarZStorOutput::getLocalZRanges( 
						    const Coord& coord,
						    float zstep,
						    TypeSet<float>& ) const
{
    TypeSet< Interval<int> > sampleinterval;
    DataPointSet::RowID rowid = poszvalues_->findFirst( coord );

    if ( rowid< 0 )
    {
	for ( int idx=0; idx<poszvalues_->size()-1; idx++ )
	{
	    if ( coord > poszvalues_->coord( idx )
		&& coord < poszvalues_->coord( idx+1 ) )
	    {
		const double distn = coord.distTo( poszvalues_->coord(idx) );
		const double distnp1 = coord.distTo(poszvalues_->coord(idx+1));
		if ( distn<distnp1 && distn<=maxdisttrcs_/2 )
		    { rowid = idx; break; }
		else if ( distnp1<distn && distnp1<=maxdisttrcs_/2 )
		    { rowid = idx+1; break; }
	    }
	}
    }

    if ( rowid< 0 ) return sampleinterval;

    const BinID bid = poszvalues_->binID(rowid);
    for ( int idx=rowid; idx<poszvalues_->size(); idx++ )
    {
	if ( poszvalues_->binID( idx ) != bid ) break;
	if ( mIsEqual( poszvalues_->coord(idx).x, coord.x, 1e-3 )
	   &&mIsEqual( poszvalues_->coord(idx).y, coord.y, 1e-3 ) )
	{
	    Interval<int> interval( mNINT32(poszvalues_->z(idx)/zstep),
		    		    mNINT32(poszvalues_->value(0,idx)/zstep) );
	    sampleinterval += interval;
	    const int nrextrazintv = (poszvalues_->nrCols()-1)/2;
	    for ( int idi=0; idi<nrextrazintv; idi+=2 ) //to keep it general
	    {
		Interval<int> intv( mNINT32(poszvalues_->value(idi+1,idx)/zstep),
				    mNINT32(poszvalues_->value(idi+2,idx)/zstep));
		sampleinterval += intv;
	    }
	}
    }

    return sampleinterval;
}


bool Trc2DVarZStorOutput::wantsOutput( const Coord& coord ) const
{
    //TODO : for some reason horizon coords in 2D are now rounded, check why
    Coord roundedcoord( (int)coord.x, (int)coord.y ); 
    return poszvalues_->findFirst( roundedcoord ) > -1;
}


bool Trc2DVarZStorOutput::finishWrite()
{                                                                               
    return writer_->close();                                                    
}


TableOutput::TableOutput( DataPointSet& datapointset, int firstcol )
    : datapointset_(datapointset)
    , firstattrcol_(firstcol)
{
    ensureSelType( Seis::Table );
    seldata_->setIsAll( false );
    ((Seis::TableSelData*)seldata_)->binidValueSet().allowDuplicateBids( true );
    ((Seis::TableSelData*)seldata_)->binidValueSet() = datapointset_.bivSet();

    arebiddupl_ = areBIDDuplicated();
    distpicktrc_ = TypeSet<float>( datapointset.size(), mUdf(float) );
}


void TableOutput::collectData( const DataHolder& data, float refstep,
			       const SeisTrcInfo& info )
{
    const Coord coord = info.coord;
    DataPointSet::RowID rid = useCoords() ? datapointset_.findFirst(coord)
					  : datapointset_.findFirst(info.binid);
    if ( rid< 0 && datapointset_.is2D() )
    {
	//TODO remove when datapointset is snaped
	for ( int idx=0; idx<datapointset_.size()-1; idx++ )
	{
	    if ( coord > datapointset_.coord( idx )
		 && coord < datapointset_.coord( idx+1 ) )
	    {
		const double distn = coord.distTo( datapointset_.coord(idx) );
		const double distnp1 = coord.distTo(datapointset_.coord(idx+1));
		if ( distn<distnp1 && distn<=maxdisttrcs_/2 )
		    { rid = idx; break; }
		else if ( distnp1<distn && distnp1<=maxdisttrcs_/2 )
		    { rid = idx+1; break; }
	    }
	}
    }

    if ( rid<0 ) return;

    const int desnrvals = desoutputs_.size() + firstattrcol_;
    if ( datapointset_.nrCols() < desnrvals )
	datapointset_.bivSet().setNrVals(desnrvals+datapointset_.nrFixedCols());

    if ( !arebiddupl_ )
    {
	float* vals = datapointset_.getValues( rid );
	computeAndSetVals( data, refstep, datapointset_.z(rid), vals );
	return;
    }

    const Interval<int> datarg( data.z0_, data.z0_+data.nrsamples_-1 );
    for ( int idx=rid; idx<datapointset_.size(); idx++ )
    {
	if ( info.binid != datapointset_.binID(idx) ) break;
	
	const float zval = datapointset_.z(idx);
	float* vals = datapointset_.getValues( idx );
	int lowz;
	DataHolder::getExtraZAndSampIdxFromExactZ( zval, refstep, lowz );
	const int highz = lowz + 1;
	bool isfulldataok = datarg.includes(lowz-1,false) &&
	    		    datarg.includes(highz+1,false);
	bool canusepartdata = data.nrsamples_<4 && datarg.includes(lowz,false) 
			      && datarg.includes(highz,false);
	if ( isfulldataok || canusepartdata )
	    computeAndSetVals( data, refstep, zval, vals );
    }
}


void TableOutput::computeAndSetVals( const DataHolder& data, float refstep,
				     float zval, float* vals )
{
    for ( int comp=0; comp<desoutputs_.size(); comp++ )
    {
	int serieidx = desoutputs_[comp];
	vals[comp+firstattrcol_] = data.getValue( serieidx, zval, refstep );
    }
}


bool TableOutput::wantsOutput( const BinID& bid ) const
{
    BinIDValueSet::Pos pos = datapointset_.bivSet().findFirst( bid );
    return pos.valid();
}


bool TableOutput::wantsOutput( const Coord& coord ) const
{
    return datapointset_.findFirst( coord ) > -1;
}


bool TableOutput::useCoords() const
{
    return datapointset_.is2D();
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const BinID& bid, float zstep,
						TypeSet<float>& exactz ) const
{
    TypeSet< Interval<int> > sampleinterval;
    
    const BinIDValueSet& bvs = datapointset_.bivSet();
    BinIDValueSet::Pos pos = bvs.findFirst( bid );

    DataPointSet::RowID rid = datapointset_.getRowID( pos );
    while ( pos.valid() && bid == bvs.getBinID(pos) )
    {
	addLocalInterval( sampleinterval, exactz, rid, zstep );
	datapointset_.bivSet().next( pos );
	rid++;
    }

    return sampleinterval;
}


TypeSet< Interval<int> > TableOutput::getLocalZRanges(
						const Coord& coord, float zstep,
						TypeSet<float>& exactz) const
{
    TypeSet< Interval<int> > sampleinterval;
    
    DataPointSet::RowID rid = datapointset_.findFirst( coord );

    if ( rid< 0 )
    {
	for ( int idx=0; idx<datapointset_.size()-1; idx++ )
	{
	    if ( coord > datapointset_.coord( idx )
		 && coord < datapointset_.coord( idx+1 ) )
	    {
		const double distn = coord.distTo( datapointset_.coord(idx) );
		const double distnp1 = coord.distTo(datapointset_.coord(idx+1));
		if ( distn<distnp1 && distn<=maxdisttrcs_/2 &&
		    ( mIsUdf(distpicktrc_[idx]) || distn<distpicktrc_[idx]) )
		{
		    rid = idx;
		    const_cast<TableOutput*>(this)->distpicktrc_[idx] = distn;
		    break;
		}
		else if ( distnp1<distn && distnp1<=maxdisttrcs_/2 &&
		    ( mIsUdf(distpicktrc_[idx+1]) || distn<distpicktrc_[idx+1]))
		{
		    rid = idx+1;
		    const_cast<TableOutput*>(this)->distpicktrc_[idx+1]=distnp1;
		    break;
		}
	    }
	}
    }
    else
    {
	const double dist = coord.distTo(datapointset_.coord(rid));
	const_cast<TableOutput*>(this)->distpicktrc_[rid]=dist;
    }

    if ( rid< 0 ) return sampleinterval;

    Coord truecoord = datapointset_.coord( rid );
    for ( int idx=rid; idx<datapointset_.size(); idx++ )
    {
	if ( truecoord != datapointset_.coord( idx ) ) break;
	addLocalInterval( sampleinterval, exactz, idx, zstep );
    }

    return sampleinterval;
}


void TableOutput::addLocalInterval( TypeSet< Interval<int> >& sampintv,
       				    TypeSet<float>& exactz,
				    int rid, float zstep ) const
{
    const float zval = datapointset_.z(rid);
    int zidx;
    DataHolder::getExtraZAndSampIdxFromExactZ( zval, zstep, zidx );
    Interval<int> interval( zidx, zidx );

    //Necessary if bid are duplicated and for a chain of attribs with stepout
    interval.start = zidx - 1;
    interval.stop =  zidx + 2;

    bool intvadded = sampintv.addIfNew( interval );
    if ( intvadded )
	exactz += zval;
}


bool TableOutput::areBIDDuplicated() const
{
    if ( datapointset_.isEmpty() ) return false;

    BinID prevbid( datapointset_.binID(0) );
    for ( int idx=1; idx<datapointset_.size(); idx++ )
    {
	const BinID bid( datapointset_.binID(idx) );
	if ( bid == prevbid ) return true;
	prevbid = bid;
    }

    return false;
}

} // namespace Attrib
