/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id: attribengman.cc,v 1.42 2005-11-08 14:05:47 cvsnanne Exp $
________________________________________________________________________

-*/

#include "attribengman.h"
#include "attribprocessor.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribslice.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "attribparam.h"
#include "survinfo.h"
#include "segposinfo.h"
#include "ptrman.h"
#include "cubesampling.h"
#include "separstr.h"
#include "nlamodel.h"
#include "nladesign.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "seis2dline.h"
#include "binidvalset.h"



namespace Attrib
{
    
EngineMan::EngineMan()
	: inpattrset(0)
	, procattrset(0)
	, nlamodel(0)
	, cs_(*new CubeSampling)
	, cache(0)
	, udfval( mUdf(float) )
	, curattridx(0)
{
}


EngineMan::~EngineMan()
{
    delete procattrset;
    delete inpattrset;
    delete nlamodel;
    delete &cs_;
    if ( cache ) cache->unRef();
}


void EngineMan::getPossibleVolume( DescSet& attribset, CubeSampling& cs,
				   const char* linename, const DescID& outid )
{
    TypeSet<DescID> desiredids(1,outid);
    BufferString errmsg;
    DescID evalid = createEvaluateADS( attribset, desiredids, errmsg );
    Processor* proc = createProcessor( attribset, linename, evalid );
    if ( !proc ) return;

    proc->getProvider()->setDesiredVolume( cs );
    proc->getProvider()->getPossibleVolume( -1, cs );
}


Processor* EngineMan::usePar( const IOPar& iopar, DescSet& attribset, 
	      		      const char* linename )
{
    int outputidx = 0;
    TypeSet<DescID> ids;
    while ( true )
    {    
	BufferString outpstr = IOPar::compKey( "Output", outputidx );
	PtrMan<IOPar> outputpar = iopar.subselect( outpstr );
	if ( !outputpar )
	{
	    if ( !outputidx )
	    { outputidx++; continue; }
	    else 
		break;
	}

	int attribidx = 0;
	while ( true )
	{
	    BufferString attribidstr = 
			IOPar::compKey( "Attributes", attribidx );
	    int attribid;
	    if ( !outputpar->get(attribidstr,attribid) )
		break;

	    ids += DescID(attribid,true);
	    attribidx++;
	}

	outputidx++;
    }

    BufferString errmsg;
    DescID evalid = createEvaluateADS( attribset, ids, errmsg );
    proc_ = createProcessor( attribset, linename, evalid );

    for ( int idx=1; idx<ids.size(); idx++ )
	proc_->addOutputInterest(idx);
    
    PtrMan<IOPar> outpar = iopar.subselect( IOPar::compKey("Output",1) );
    const char* bsres = outpar ? outpar->find( sKey::BinIDSel ) : 0;
    if ( !bsres || *bsres != 'N' )
    {
	outpar->get( "In-line range", cs_.hrg.start.inl, cs_.hrg.stop.inl );
	outpar->get( "Cross-line range",cs_.hrg.start.crl, cs_.hrg.stop.crl);
	outpar->get( "Depth range", cs_.zrg.start, cs_.zrg.stop );
	cs_.zrg.start /= SI().zFactor();
	cs_.zrg.stop /= SI().zFactor();
    }
    else
    {
	cs_.init();
	if ( attribset.is2D() )
	{
	    cs_.hrg.start.inl = 0; cs_.hrg.stop.inl = mUdf(int);
	    cs_.hrg.start.crl = 1; cs_.hrg.stop.crl = mUdf(int);
	}
    }

    const Attrib::Desc* curdesc = attribset.getDesc( ids[0] );
    BufferString attribname = curdesc->isStored() ? "" : curdesc->userRef();
    LineKey lkey( linename, attribname );
    SeisTrcStorOutput* storeoutp = createOutput( iopar, lkey );
    
    proc_->addOutput( storeoutp );
    return proc_;
}


Processor* EngineMan::createProcessor( const DescSet& attribset,
				       const char* linename,
				       const DescID& outid )
{
    Desc* targetdesc = const_cast<Desc*>(attribset.getDesc(outid));
    if ( !targetdesc ) return 0;
    
    Processor* processor = new Processor( *targetdesc, linename );
    if ( !processor->isOK() )
    {
	delete processor;
	return 0;
    }

    processor->addOutputInterest( targetdesc->selectedOutput() );

    return processor;
}


SeisTrcStorOutput* EngineMan::createOutput( const IOPar& pars, 
					    const LineKey& lkey )
{
    const char* typestr = pars.find("Output.1.Type");

    if ( !strcmp(typestr,"Cube") )
    {
	SeisTrcStorOutput* outp = new SeisTrcStorOutput( cs_, lkey );
	outp->setGeometry(cs_);
	outp->doUsePar( pars );
	return outp;
    }

    return 0;
}


void EngineMan::setNLAModel( const NLAModel* m )
{
    delete nlamodel;
    nlamodel = m ? m->clone() : 0;
}


void EngineMan::setAttribSet( const DescSet* ads )
{
    delete inpattrset;
    inpattrset = ads ? ads->clone() : 0;
}


const char* EngineMan::getCurUserRef() const
{
    const int idx = curattridx;
    if ( !attrspecs_.size() || attrspecs_[idx].id() < 0 ) return "";

    SelSpec& ss = const_cast<EngineMan*>(this)->attrspecs_[idx];
    if ( attrspecs_[idx].isNLA() )
    {
	if ( !nlamodel ) return "";
	ss.setRefFromID( *nlamodel );
    }
    else
    {
	if ( !inpattrset ) return "";
	ss.setRefFromID( *inpattrset );
    }
    return attrspecs_[idx].userRef();
}


SliceSet* EngineMan::getSliceSetOutput()
{
    if ( !proc_ )
	return 0;

    if ( proc_->outputs.size()==1 && !cache )
	return proc_->outputs[0]->getSliceSet();

    ObjectSet<SliceSet> slsets;
    for ( int idx=0; idx<proc_->outputs.size(); idx++ )
    {
	if ( !proc_->outputs[idx] || !proc_->outputs[idx]->getSliceSet() )
	    continue;

	SliceSet& slset = *new SliceSet;
	slset.sampling = proc_->outputs[idx]->getSliceSet()->sampling;
	slset.direction = slset.sampling.defaultDir();
	for ( int idy=0; idy<proc_->outputs[idx]->getSliceSet()->size(); idy++ )
	{
	    Slice* slc = ( *proc_->outputs[idx]->getSliceSet() )[idy];
	    slset += slc;
	}

	slset.ref();
	slsets += &slset;
    }

    bool prevismine = false;
    if ( !cache )
    {
	cache = slsets[0];
	slsets.remove(0);
	prevismine = true;
    }

    SliceSet* outslcs = new SliceSet;
    CubeSampling csamp(cs_);
    outslcs->sampling = csamp;
    outslcs->direction = csamp.defaultDir();
#define mGetDim(nr) \
    const int dim##nr = csamp.size( direction(outslcs->direction,nr) )

    mGetDim(0); mGetDim(1); mGetDim(2);
    float undfval;
    bool udefvalfound = false;
    for ( int idx=0; idx<cache->size(); idx++ )
    {
	if ( (*cache)[idx] )
	{
	    udefvalfound = true;
	    undfval= (*cache)[idx]->undefValue();
	    break;
	}
    }
    if ( !udefvalfound )
    {
	for ( int idy=0; idy<slsets.size(); idy++ )
	{
	    if ( !slsets[idy] ) continue;
	    for ( int idx=0; idx<slsets[idy]->size(); idx++ )
	    {
		if ( (*slsets[idy])[idx] )
		{
		    udefvalfound = true;
		    undfval= (*slsets[idy])[idx]->undefValue();
		    break;
		}
	    }

	    if ( udefvalfound ) break;
	}
    }

    if ( !udefvalfound )
	undfval = mUndefValue;

    for ( int idx=0; idx<dim0; idx++ )
    {
	Slice* slice = new Slice( dim1, dim2, udfval );
	slice->ref();
	*outslcs += slice;
    }

    int i0, i1, i2;
    for ( int iset=-1; iset<slsets.size(); iset++ )
    {
	const SliceSet& slset = *(iset < 0 ? cache : slsets[iset]);

	CubeSampling datacs( slset.sampling );
#define mCheckRg(memb,op) \
	if ( datacs.memb op outslcs->sampling.memb ) \
	    datacs.memb = outslcs->sampling.memb
	mCheckRg(hrg.start.inl,<); mCheckRg(hrg.stop.inl,>);
	mCheckRg(hrg.start.crl,<); mCheckRg(hrg.stop.crl,>);
	mCheckRg(zrg.start,<); mCheckRg(zrg.stop,>);

	BinID bid;
	const float hzstep = outslcs->sampling.zrg.step * .5;
	for ( int inl =  datacs.hrg.start.inl;
		  inl <= datacs.hrg.stop.inl;
		    inl += datacs.hrg.step.inl )
	{
	    for ( int   crl =  datacs.hrg.start.crl;
			crl <= datacs.hrg.stop.crl;
			crl += datacs.hrg.step.crl )
	    {
		for ( float z =  datacs.zrg.start;
			    z <  datacs.zrg.stop + hzstep;
			    z += datacs.zrg.step )
		{
		    slset.getIdxs( inl, crl, z, i0, i1, i2 );
		    float val = slset[i0] ? slset[i0]->get( i1, i2 ) : udfval;
		    outslcs->getIdxs( inl, crl, z, i0, i1, i2 );
		    ((*outslcs)[i0])->set( i1, i2, val );
		}
	    }
	}
    }

    for ( int idx=0; idx<slsets.size(); idx++ )
	slsets[idx]->unRef();

    return outslcs;
}


void EngineMan::setAttribSpecs( const TypeSet<SelSpec>& specs )
{ attrspecs_ = specs; }


void EngineMan::setAttribSpec( const SelSpec& spec )
{
    attrspecs_.erase();
    attrspecs_ += spec;
}


void EngineMan::setCubeSampling( const CubeSampling& newcs )
{
    cs_ = newcs;
    cs_.normalise();
}


#define mErrRet() \
	delete &descset; desc->unRef(); return;

DescSet* EngineMan::createNLAADS( DescID& nladescid, BufferString& errmsg,
       				  const DescSet* addtoset )
{
    if ( !attrspecs_.size() ) return 0;
    DescSet* descset = addtoset ? addtoset->clone() : new DescSet;
    if ( !addtoset && !descset->usePar(nlamodel->pars()) )
    {
	errmsg = descset->errMsg();
	delete descset;
	return 0;
    }

    BufferString s;
    nlamodel->dump(s);
    BufferString defstr( nlamodel->nlaType(true) );
    defstr += " specification=\""; defstr += s; defstr += "\"";

    addNLADesc( defstr, nladescid, *descset, attrspecs_[0].id().asInt(), 
		nlamodel, errmsg );

    return descset;
}


void EngineMan::addNLADesc( const char* specstr, DescID& nladescid,
			    DescSet& descset, int outputnr,
			    const NLAModel* nlamdl, BufferString& errmsg )
{
    Desc* desc = PF().createDescCopy( "NN" );
    desc->setDescSet( &descset );

    if ( !desc->parseDefStr(specstr) )
    { 
	errmsg = "cannot parse definition string"; errmsg += specstr;
	mErrRet(); 
    }

    desc->setHidden( true );

    const int nrinputs = desc->nrInputs();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	const char* inpname = desc->inputSpec(idx).getDesc();
	DescID descid = descset.getID( inpname, true );
	if ( descid < 0 && IOObj::isKey(inpname) )
	{
	    descid = descset.getID( inpname, false );
	    if ( descid < 0 )
	    {
		// It could be 'storage', but it's not yet in the set ...
		PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
		if ( ioobj )
		{
		    Desc* stordesc = 
			PF().createDescCopy( StorageProvider::attribName() );
		    stordesc->setDescSet( &descset );
		    ValParam* idpar = 
			stordesc->getValParam( StorageProvider::keyStr() );
		    idpar->setValue( inpname );
		    stordesc->setUserRef( ioobj->name() );
		    descid = descset.addDesc( stordesc );
		    if ( descid < 0 )
		    {
			errmsg = "NLA input '";
			errmsg += inpname;
			errmsg += "' cannot be found in the provided set.";
			mErrRet();
		    }
		}
	    }
	}

	desc->setInput( idx, descset.getDesc(descid) );
    }

    if ( outputnr > desc->nrOutputs() )
    {
	errmsg = "Output "; errmsg += outputnr; 
	errmsg += " not present.";
	mErrRet();
    }
    
    const NLADesign& nlades = nlamdl->design();
    desc->setUserRef( *nlades.outputs[outputnr] );
    desc->selectOutput( outputnr );

    nladescid = descset.addDesc( desc );
    if ( nladescid == DescID::undef() )
    {
	errmsg = descset.errMsg();
	mErrRet();
    }
}


DescID EngineMan::createEvaluateADS( DescSet& descset, 
				     const TypeSet<DescID>& outids,
				     BufferString& errmsg )
{
    if ( !outids.size() ) return DescID::undef();

    Desc* desc = PF().createDescCopy( "Evaluate" );
    desc->setDescSet( &descset );
    desc->setNrOutputs( Seis::UnknowData, outids.size() );

    desc->setHidden( true );

    const int nrinputs = outids.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
	desc->addInput( InputSpec("Data",true) );
	Desc* inpdesc = descset.getDesc( outids[idx] );
	if ( !inpdesc ) continue;
	
	desc->setInput( idx, inpdesc );
    }

    desc->setUserRef( "evaluate attributes" );
    desc->selectOutput(0);

    DescID evaldescid = descset.addDesc( desc );
    if ( evaldescid == DescID::undef() )
    {
	errmsg = descset.errMsg();
	desc->unRef();
    }

    return evaldescid;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }

#define mStepEps 1e-3


Processor* EngineMan::createScreenOutput2D( BufferString& errmsg,
	 				    ObjectSet<DataHolder>& dataset,
					    ObjectSet<SeisTrcInfo>& trcinfoset )
{
    proc_ = getProcessor( errmsg );
    if ( !proc_ ) 
	return 0; 
    
    LineKey lkey = linekey.buf();
    const Provider* prov = proc_->getProvider();
    if ( prov && !prov->getDesc().isStored() )
	lkey.setAttrName( proc_->getAttribName() );
    
    Interval<int> trcrg( cs_.hrg.start.crl, cs_.hrg.stop.crl );
    Interval<float> zrg( cs_.zrg.start, cs_.zrg.stop );
    TwoDOutput* attrout = new TwoDOutput( trcrg, zrg, lkey );
    attrout->setGeometry( trcrg, zrg );
    attrout->setOutput( dataset, trcinfoset );
    proc_->addOutput( attrout ); 

    return proc_;
}


Processor* EngineMan::createSliceSetOutput( BufferString& errmsg,
					    const SliceSet* prev )
{
    if ( cs_.isEmpty() )
	prev = 0;
#define mRg(dir) (prev->sampling.dir##rg)
    else if ( prev )
    {
	cache = const_cast<SliceSet*>(prev);
	cache->ref();
	if ( !mRg(z).isCompatible( cs_.zrg, mStepEps )
	  || mRg(h).step != cs_.hrg.step
	  || (mRg(h).start.inl - cs_.hrg.start.inl) % cs_.hrg.step.inl
	  || (mRg(h).start.crl - cs_.hrg.start.crl) % cs_.hrg.step.crl 
	  || mRg(h).start.inl > cs_.hrg.stop.inl
	  || mRg(h).stop.inl < cs_.hrg.start.inl
	  || mRg(h).start.crl > cs_.hrg.stop.crl
	  || mRg(h).stop.crl < cs_.hrg.start.crl
	  || mRg(z).start > cs_.zrg.stop + mStepEps*cs_.zrg.step
	  || mRg(z).stop < cs_.zrg.start - mStepEps*cs_.zrg.step )
	    // No overlap, gotta crunch all the numbers ...
	    prev = 0;
    }

#define mAddAttrOut(todocs) \
{ \
    SliceSetOutput* attrout = new SliceSetOutput(todocs); \
    attrout->setGeometry( todocs ); \
    attrout->setUndefValue( udfval ); \
    proc_->addOutput( attrout ); \
}

    proc_ = getProcessor(errmsg);
    if ( !proc_ ) 
	return 0; 

    if ( !prev )
	mAddAttrOut( cs_ )
    else
    {
	CubeSampling todocs( cs_ );
	if ( mRg(h).start.inl > cs_.hrg.start.inl )
	{
	    todocs.hrg.stop.inl = mRg(h).start.inl - cs_.hrg.step.inl;
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop.inl < cs_.hrg.stop.inl )
	{
	    todocs = cs_;
	    todocs.hrg.start.inl = mRg(h).stop.inl + cs_.hrg.step.inl;
	    mAddAttrOut( todocs )
	}

	const int startinl = mMAX(cs_.hrg.start.inl, mRg(h).start.inl );
	const int stopinl = mMIN( cs_.hrg.stop.inl, mRg(h).stop.inl );

	if ( mRg(h).start.crl > cs_.hrg.start.crl )
	{
	    todocs = cs_;
	    todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	    todocs.hrg.stop.crl = mRg(h).start.crl - cs_.hrg.step.crl;
	    mAddAttrOut( todocs )
	}
	
	if ( mRg(h).stop.crl < cs_.hrg.stop.crl )
	{
	    todocs = cs_;
	    todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	    todocs.hrg.start.crl = mRg(h).stop.crl + cs_.hrg.step.crl;
	    mAddAttrOut( todocs )
	}

	todocs = cs_;
	todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	todocs.hrg.start.crl = mMAX(cs_.hrg.start.crl, mRg(h).start.crl );
	todocs.hrg.stop.crl = mMIN(cs_.hrg.stop.crl, mRg(h).stop.crl );

	if ( mRg(z).start > cs_.zrg.start + mStepEps*cs_.zrg.step )
	{
	    todocs.zrg.stop = mRg(z).start - cs_.zrg.step;
	    mAddAttrOut( todocs )
	}
	    
	if ( mRg(z).stop < cs_.zrg.stop - mStepEps*cs_.zrg.step )
	{
	    todocs.zrg = cs_.zrg;
	    todocs.zrg.start = mRg(z).stop + cs_.zrg.step;
	    mAddAttrOut( todocs )
	}
    }

    return proc_;
}


class AEMFeatureExtracter : public Executor
{
public:
AEMFeatureExtracter( EngineMan& aem, const BufferStringSet& inputs,
		     const ObjectSet<BinIDValueSet>& bivsets )
    : Executor("Feature Extracter")
{
    const int nrinps = inputs.size();
    const DescSet* attrset = aem.procattrset ? aem.procattrset : aem.inpattrset;
    for ( int idx=0; idx<inputs.size(); idx++ )
    {
	const DescID id = attrset->getID( inputs.get(idx), true );
	if ( id == DescID::undef() )
	    continue;
	SelSpec ss( 0, id );
	ss.setRefFromID( *attrset );
	aem.attrspecs_ += ss;
    }

    ObjectSet<BinIDValueSet>& bvs = 
	const_cast<ObjectSet<BinIDValueSet>&>(bivsets);

    aem.computeIntersect2D(bvs);
    proc = aem.createLocationOutput( errmsg, bvs );
}

~AEMFeatureExtracter()		{ delete proc; }

int totalNr() const		{ return proc ? proc->totalNr() : -1; }
int nrDone() const		{ return proc ? proc->nrDone() : 0; }
const char* nrDoneText() const	{ return proc ? proc->nrDoneText() : ""; }

const char* message() const
{
    return *(const char*)errmsg ? errmsg.buf() 
	: (proc ? proc->message() : "Cannot create output");
}

int haveError( const char* msg )
{
    if ( msg ) errmsg = msg;
    return -1;
}

int nextStep()
{
    if ( !proc ) return haveError( 0 );

    int rv = proc->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( proc->message() );
}

    BufferString		errmsg;
    Processor*			proc;
    TypeSet<DescID>		outattribs;
};


Executor* EngineMan::createFeatureOutput( const BufferStringSet& inputs,
				    const ObjectSet<BinIDValueSet>& bivsets )
{
    return new AEMFeatureExtracter( *this, inputs, bivsets );
}


void EngineMan::computeIntersect2D( ObjectSet<BinIDValueSet>& bivsets ) const
{
    if ( !inpattrset || !attrspecs_.size() )
	return;

    if ( !inpattrset->is2D() )
	return;

    Desc* storeddesc;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
    {
	const Desc* desc = inpattrset->getDesc( attrspecs_[idx].id() );
	if ( !desc ) continue;
	if ( desc->isStored() )
	{
	    storeddesc = const_cast<Desc*>(desc);
	    break;
	}
	else
	{
	    Desc* candidate = desc->getStoredInput();
	    if ( candidate )
	    {
		storeddesc = candidate;
		break;
	    }
	}
    }

    const LineKey lk( storeddesc->getValParam(
			StorageProvider::keyStr())->getStringValue(0) );

    const MultiID key( lk.lineName() );
    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj ) return;
    const Seis2DLineSet lset(ioobj->fullUserExpr(true));

    PosInfo::LineSet2DData linesetgeom;
    if ( !lset.getGeometry( linesetgeom ) ) return;

    ObjectSet<BinIDValueSet> newbivsets;
    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	BinIDValueSet* newset = new BinIDValueSet(bivsets[idx]->nrVals(), true);
	ObjectSet<PosInfo::LineSet2DData::IR> resultset;
	linesetgeom.intersect( *bivsets[idx], resultset );

	for ( int idy=0; idy<resultset.size(); idy++)
	    newset->append(*resultset[idy]->posns_);
	
	newbivsets += newset;
    }
    bivsets = newbivsets;
    
/*    ObjectSet<BinIDValueSet> newbivsets;
    for ( int idx=0; idx<bivsets.size(); idx++ )
	newbivsets += new BinIDValueSet(bivsets[idx]->nrVals(), true);
    
    for ( int idx=0; idx<lset.nrLines(); idx++ )
    {
	if ( strcmp( lset.attribute(idx), "Seis" ) ) continue;
	Line2DGeometry linegeom;
	if ( !lset.getGeometry( idx,linegeom ) ) return;
	BinID prevbid(-1,-1);
	for ( int idy=0; idy<linegeom.posns.size(); idy++ )
	{
	    BinID bid = SI().transform( linegeom.posns[idy].coord );
	    if ( bid == prevbid ) continue;
	    prevbid = bid;
	    for ( int idz=0; idz<bivsets.size(); idz++ )
	    {
		if ( bivsets[idz]->includes(bid) )
		{
		    BinIDValueSet::Pos pos = bivsets[idz]->findFirst(bid);

		    while ( true )
		    {
			BinIDValues bidvalues;
			bivsets[idz]->get(pos,bidvalues);
			newbivsets[idz]->add(bidvalues);
			bivsets[idz]->next( pos );
			if ( bid != bivsets[idz]->getBinID(pos) )
			    break;
		    }
		}
	    }
	}
    }
    bivsets = newbivsets;*/
}


Processor* EngineMan::createLocationOutput( BufferString& errmsg,
					    ObjectSet<BinIDValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) mErrRet("No locations to extract data on")

    proc_ = getProcessor(errmsg);
    if ( !proc_ )
	return 0; 

    ObjectSet<LocationOutput> outputs;
    for ( int idx=0; idx<bidzvset.size(); idx++ )
    {
	BinIDValueSet& bidzvs = *bidzvset[idx];
	LocationOutput* attrout = new LocationOutput( bidzvs );
	outputs += attrout;
    }
    
    if ( !outputs.size() )
        return 0;

    for ( int idx=0; idx<outputs.size(); idx++ )
	proc_->addOutput( outputs[idx] );

    return proc_;
}

#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }

Processor* EngineMan::getProcessor( BufferString& errmsg )
{
    if ( procattrset )
	{ delete procattrset; procattrset = 0; }

    if ( !inpattrset || !attrspecs_.size() )
	mErrRet( "No attribute set or input specs" )

    TypeSet<DescID> outattribs;
    for ( int idx=0; idx<attrspecs_.size(); idx++ )
	outattribs += attrspecs_[idx].id();

    DescID outid = outattribs[0];

    errmsg = "";
    bool doeval = false;
    if ( !attrspecs_[0].isNLA() )
    {
	procattrset = inpattrset->optimizeClone( outattribs );
	if ( !procattrset ) return false;

	if ( outattribs.size() > 1 )
	{
	    doeval = true;
	    outid = createEvaluateADS( *procattrset, outattribs, errmsg);
	}
    }
    else
    {
// TODO: Is it necessary to fill model pars here?
//	inpattrset->fillPar( const_cast<NLAModel*>(nlamodel)->pars() );
	DescID nlaid( SelSpec::cNoAttrib() );
	procattrset = createNLAADS( nlaid, errmsg );
	if ( *(const char*)errmsg )
	    mErrRet(errmsg)
	outid = nlaid;
    }

    proc_ = createProcessor( *procattrset, lineKey().buf(), outid );
    if ( !proc_ )
	mErrRet( "Invalid input data,\nNo processor created" )
	    
    if ( doeval )
    {
	for ( int idx=1; idx<attrspecs_.size(); idx++ )
	    proc_->addOutputInterest(idx);
    }
    
    return proc_;
}


Processor* EngineMan::createTrcSelOutput( BufferString& errmsg,
					  const BinIDValueSet& bidvalset,
					  SeisTrcBuf& output, float outval,
					  Interval<float>* extraz )
{
    proc_ = getProcessor(errmsg);
    if ( !proc_ )
	return 0;

    TrcSelectionOutput* attrout	= 
			new TrcSelectionOutput( bidvalset, outval, extraz );
    attrout->setOutput( &output );

    proc_->addOutput( attrout );

    return proc_;
};


int EngineMan::getNrOutputsToBeProcessed() const
{
    return proc_ ? proc_->outputs.size() : 0;
}

} // namespace Attrib
