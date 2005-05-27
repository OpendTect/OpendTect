/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H.Payraudeau
 Date:          04/2005
 RCS:           $Id:
________________________________________________________________________

-*/

#include "attribengman.h"
#include "attribprocessor.h"
#include "attriboutput.h"
#include "attribprovider.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribslice.h"
#include "datachar.h"
//#include "featset.h"
#include "survinfo.h"
#include "datainterp.h"
#include "ptrman.h"
#include "cubesampling.h"
#include "separstr.h"
#include "nlamodel.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "stats.h"
#include <iostream>
#include "sets.h"
#include "linekey.h"



namespace Attrib
{
    
EngineMan::EngineMan()
	: inpattrset(0)
	, nlamodel(0)
	, cs(*new CubeSampling)
    	, outid(*new MultiID)
{
}


EngineMan::~EngineMan()
{
    clearProcessing();

    delete inpattrset;
    delete nlamodel;

    delete &cs;
    delete &outid;
}


void EngineMan::clearProcessing()
{
}


void EngineMan::usePar( const IOPar& iopar, 
			const DescSet& attribset, 
	      		const char* linename,
			ObjectSet<Processor>& procset )
{
    int indexoutp = 0;
    TypeSet<int> idstor;
    while ( true )
    {    
	BufferString multoutpstr = IOPar::compKey( "Output", indexoutp );
	PtrMan<IOPar> output = iopar.subselect( multoutpstr );
	if ( !output )
	{
	    if ( !indexoutp )
	    { indexoutp++; continue; }
	    else 
		break;
	}

	int indexattrib = 0;
	while ( true )
	{
	    BufferString attribidstr = 
			IOPar::compKey( "Attributes", indexattrib );
	    int attribid;
	    if ( !output->get(attribidstr,attribid) )
		break;

	    idstor += attribid;
	    indexattrib++;
	}

	indexoutp++;
    }

    ObjectSet<Desc> targetdescset;
    if ( !idstor.size() ) return;
    
    Desc* targetdesc = 	const_cast<Desc*>(attribset.getDesc(idstor[0]));
    targetdescset += targetdesc;
    
    Processor* processor = new Processor( *targetdesc, linename );
    processor->addOutputInterest( targetdesc->selectedOutput() );
    procset += processor;

    for ( int index=1; index<idstor.size(); index++ )
    {
	Desc* candidate = const_cast<Desc*>(
				attribset.getDesc( idstor[index] ) );
	if ( candidate )
	{
	    for ( int idx=0; idx< targetdescset.size(); idx++ )
	    {
		if ( candidate->isIdenticalTo( *targetdescset[idx], false ) )
		{
		    if ( targetdescset[idx]->selectedOutput() 
			    != candidate->selectedOutput() )
			    procset[idx]->addOutputInterest(
				    candidate->selectedOutput());
		}
		else
		{
		    Processor* proc = new Processor( *candidate, linename );
		    procset += proc;
		}
	    }
	}
	
	index++;
    }

    iopar.get( "Output.1.In-line range", cs.hrg.start.inl, cs.hrg.stop.inl );
    iopar.get( "Output.1.Cross-line range", cs.hrg.start.crl, cs.hrg.stop.crl );
    iopar.get( "Output.1.Depth range", cs.zrg.start, cs.zrg.stop );
    cs.zrg.start /= SI().zFactor();
    cs.zrg.stop /= SI().zFactor();

    LineKey lkey(linename,targetdesc->attribName());
    CubeOutput* cubeoutp = createOutput( iopar, lkey );
    for ( int idx=0; idx<procset.size(); idx++ )
	procset[idx]->addOutput(cubeoutp);
}


CubeOutput* EngineMan::createOutput( const IOPar& pars, LineKey lkey )
{
    const char* typestr = pars.find("Output.1.Type");

    if ( !strcmp( typestr, "Cube") )
    {
	CubeOutput* outp = new CubeOutput(cs, lkey);
	outp->doUsePar(pars);
	return outp;
    }
    return 0;
}

//pour l'instant copie integrale de l'ancien attribeng.
/*void AttribEngMan::setNLAModel( const NLAModel* m )
{
    delete nlamodel;
    nlamodel = m ? m->clone() : 0;
}


void AttribEngMan::setAttribSet( const AttribDescSet* ads )
{
    delete inpattrset;
    inpattrset = ads ? ads->clone() : 0;
}


const char* AttribEngMan::curUserDesc() const
{
    if ( attrspec.id() < 0 ) return "";

    AttribSelSpec& ss = const_cast<AttribEngMan*>(this)->attrspec;
    if ( attrspec.isNLA() )
    {
	if ( !nlamodel ) return "";
	ss.setRefFromID( *nlamodel );
    }
    else
    {
	if ( !inpattrset ) return "";
	ss.setRefFromID( *inpattrset );
    }
    return attrspec.userRef();
}


AttribSliceSet* AttribEngMan::getCubeOutput( Executor* ex )
{
    mDynamicCastGet(AttribCubeExecutor*,exec,ex)
    if ( !exec ) return 0;

    return exec->fetch3DData();
}


SeisTrcBuf* AttribEngMan::get2DLineOutput( Executor* ex )
{
    mDynamicCastGet(AttribCubeExecutor*,exec,ex)
    if ( !exec ) return 0;

    return exec->fetch2DData();
}


void AttribEngMan::setAttribSpec( const AttribSelSpec& a )
{
    attrspec = a;
}


void AttribEngMan::setOutputID( const MultiID& m )
{
    outid = m;
}


void AttribEngMan::setCubeSampling( const CubeSampling& newcs )
{
    cs = newcs;
    cs.normalise();
}


void AttribEngMan::addOutputAttrib( int id )
{
    outattribs += id;
}


#define mErrRet() \
	delete ads; delete ad; return 0

AttribDescSet* AttribEngMan::createNLAADS( int& outid, BufferString& errmsg,
       					   const AttribDescSet* addtoset )
{
    AttribDescSet* ads = addtoset ? addtoset->clone() : new AttribDescSet;
    CalcAttribDesc* ad = 0;

    if ( !addtoset && !ads->usePar(const_cast<NLAModel*>(nlamodel)->pars()) )
	{ errmsg = ads->errMsg(); mErrRet(); }

    BufferString s;
    nlamodel->dump(s);
    ad = new CalcAttribDesc( *ads );
    BufferString def( nlamodel->nlaType(true) );
    def += " specification=\""; def += s; def += "\"";

    if ( !ad->setDefStr(def.buf(),false) )
	{ errmsg = ad->errMsg(); mErrRet(); }

    ad->setHidden( true );
    ad->setUserRef( nlamodel->name() );

    const int nrinputs = ad->nrInputs();

    for ( int idx=0; idx<nrinputs; idx++ )
    {
	const char* inpname = ad->inputSpec(idx)->getDesc();
	int dnr = ads->descNr( inpname, true );
	if ( dnr < 0 && IOObj::isKey(inpname) )
	{
	    dnr = ads->descNr( inpname, false );
	    if ( dnr < 0 )
	    {
		// It could be 'storage', but it's not yet in the set ...
		PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
		if ( ioobj )
		{
		    AttribDesc* newdesc = new StorageAttribDesc( *ads );
		    newdesc->setDefStr(inpname,false);
		    newdesc->setUserRef( ioobj->name() );
		    dnr = ads->addAttrib( newdesc );
		}
	    }
	    if ( dnr < 0 )
	    {
		errmsg = "NLA input '";
		errmsg += inpname;
		errmsg += "' cannot be found in the provided set.";
		mErrRet();
	    }
	}
	ad->setInput( idx, ads->id(dnr) );
    }

    if ( !ad->selectAttrib(attrspec.id()) )
    {
	errmsg = "Output "; errmsg += attrspec.id(); errmsg += "not present.";
	mErrRet();
    }

    outid = ads->id( ads->addAttrib( ad ) );
    if ( outid == -1 )
    {
	errmsg = ads->errMsg();
	mErrRet();
    }

    return ads;
}


void AttribEngMan::setExecutorName( Executor* ex )
{
    if ( !ex ) return;
    BufferString usernm( curUserDesc() );
    if ( usernm == "" || !inpattrset ) return;
    if ( IOObj::isKey(usernm) )
    {
	IOObj* ioobj = IOM().get( MultiID(usernm.buf()) );
	if ( ioobj )
	{
	    usernm = ioobj->name();
	    delete ioobj;
	}
    }

    BufferString nm( ex->name() );
    if ( attrspec.isNLA() )
    {
	nm = "Applying ";
	nm += nlamodel->nlaType(true);
	nm += ": calculating";
    }
    else
    {
	const int descnr = inpattrset->descNr( attrspec.id() );
	const AttribDesc& ad = inpattrset->getAttribDesc( descnr );
	if ( ad.isStored() )
	    nm = "Reading from";
    }
    nm += " \"";
    nm += usernm;
    nm += "\"";

    ex->setName( nm );
}


bool AttribEngMan::handleAttribWithoutInputs()
{
    const int descnr = procattrset->descNr( attrspec.id() );
    if ( descnr < 0 ) return false;
    const AttribDesc& ad = procattrset->getAttribDesc( descnr );
    if ( ad.isStored() || ad.nrInputs() ) return true;

    int storedid = -1;
    MultiID key;
    bool hasstored = procattrset->getFirstStored( Both2DAnd3D, key );
    if ( !hasstored )
    {
	StorageAttribDesc* newdesc = new StorageAttribDesc( *procattrset );
	if ( !newdesc->setDefStr( "", true ) ) return false;
	storedid = procattrset->getStoredAttribID( newdesc->defStr(), 0, true );
	delete newdesc;
    }
    else
	storedid = procattrset->getStoredAttribID( key, 0, true );

    if ( storedid < 0 ) return false;

    BufferString expr = "Math expression=x0+0*x1 steering=no pos0=0,0 pos1=0,0";
    CalcAttribDesc* newad = new CalcAttribDesc( *procattrset );
    newad->setDefStr( expr, false );
    newad->setUserRef( "Math" );
    newad->setInput( 0, attrspec.id() );
    newad->setInput( 1, storedid );
    curattrid = procattrset->addAttrib( newad );
    attrspec.set( newad->userRef(), storedid, false, 0 );
    return true;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return 0; }


AttribOutputExecutor* AttribEngMan::mkOutputExecutor( BufferString& errmsg,
						      bool needid )
{
    errmsg = "";
    if ( procattrset )
	{ delete procattrset; procattrset = 0; }

    if ( !inpattrset ) mErrRet("No Attribute Set yet")
    if ( needid && attrspec.id() < 0 )
	mErrRet(attrspec.isNLA()?"No NLA available":"No Attribute selected")

    clearProcessing();
    curattrid = attrspec.id();

    if ( !attrspec.isNLA() )
    {
	if ( needid )
	{
	    procattrset = inpattrset->optimizeClone( attrspec.id(), attrid2 );
	    if ( !handleAttribWithoutInputs() ) 
		mErrRet( "Cannot display this attribute\n"
			 "Please import seismics first" )
	}
	else
	    procattrset = inpattrset->clone();

    }
    else
    {
	inpattrset->fillPar( const_cast<NLAModel*>(nlamodel)->pars() );
	procattrset = createNLAADS( curattrid, errmsg );
	if ( *(const char*)errmsg )
	    mErrRet(errmsg)
    }

    processor = new AttribDescSetProcessor(*procattrset);
    if ( processor->errMsg() && *processor->errMsg() )
	mErrRet(processor->errMsg())

    return new AttribOutputExecutor( *processor );
}


AttribOutputExecutor* AttribEngMan::getOutputExecutor( BufferString& errmsg,
						       bool needid )
{
    AttribOutputExecutor* ret = mkOutputExecutor( errmsg, needid );
    if ( !ret )
	return 0;

    ret->setAllowEdge( false );
    setExecutorName( ret );
    return ret;
}


#define mStepEps 1e-3


Executor* AttribEngMan::cubeOutputCreater( BufferString& errmsg,
				      const AttribSliceSet* prev )
{
    if ( cs.isEmpty() )
	prev = 0;
#define mRg(dir) (prev->sampling.dir##rg)
    else if ( prev )
    {
	if ( !mRg(z).isCompatible( cs.zrg, mStepEps )
	  || mRg(h).step != cs.hrg.step
	  || (mRg(h).start.inl - cs.hrg.start.inl) % cs.hrg.step.inl
	  || (mRg(h).start.crl - cs.hrg.start.crl) % cs.hrg.step.crl 
	  || mRg(h).start.inl > cs.hrg.stop.inl
	  || mRg(h).stop.inl < cs.hrg.start.inl
	  || mRg(h).start.crl > cs.hrg.stop.crl
	  || mRg(h).stop.crl < cs.hrg.start.crl
	  || mRg(z).start > cs.zrg.stop + mStepEps*cs.zrg.step
	  || mRg(z).stop < cs.zrg.start - mStepEps*cs.zrg.step )
	    // No overlap, gotta crunch all the numbers ...
	    prev = 0;
    }

#define mAddAttrOut(todocs) \
{ \
    CubeAttribOutput* attrout = new CubeAttribOutput(*processor,linekey.buf()); \
    attrout->setCubeID( outid ); \
    attrout->setGeometry( todocs ); \
    attrout->addAttribDescSetID( curattrid ); \
    attrout->setUndefValue( udfval ); \
    for ( int idx=0; idx<outattribs.size(); idx++ ) \
	attrout->addAttribDescSetID( outattribs[idx] ); \
    calcexec->addOutput( attrout ); \
}

    AttribOutputExecutor* calcexec = getOutputExecutor( errmsg, 
	    						!outattribs.size() );
    if ( !calcexec ) return 0;

    if ( !prev )

	mAddAttrOut( cs )

    else
    {
	CubeSampling todocs( cs );
	if ( mRg(h).start.inl > cs.hrg.start.inl )
	{
	    todocs.hrg.stop.inl = mRg(h).start.inl - cs.hrg.step.inl;
	    mAddAttrOut( todocs )
	}

	if ( mRg(h).stop.inl < cs.hrg.stop.inl )
	{
	    todocs = cs;
	    todocs.hrg.start.inl = mRg(h).stop.inl + cs.hrg.step.inl;
	    mAddAttrOut( todocs )
	}

	const int startinl = mMAX(cs.hrg.start.inl, mRg(h).start.inl );
	const int stopinl = mMIN( cs.hrg.stop.inl, mRg(h).stop.inl );

	if ( mRg(h).start.crl > cs.hrg.start.crl )
	{
	    todocs = cs;
	    todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	    todocs.hrg.stop.crl = mRg(h).start.crl - cs.hrg.step.crl;
	    mAddAttrOut( todocs )
	}
	
	if ( mRg(h).stop.crl < cs.hrg.stop.crl )
	{
	    todocs = cs;
	    todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	    todocs.hrg.start.crl = mRg(h).stop.crl + cs.hrg.step.crl;
	    mAddAttrOut( todocs )
	}

	todocs = cs;
	todocs.hrg.start.inl = startinl; todocs.hrg.stop.inl = stopinl;
	todocs.hrg.start.crl = mMAX(cs.hrg.start.crl, mRg(h).start.crl );
	todocs.hrg.stop.crl = mMIN(cs.hrg.stop.crl, mRg(h).stop.crl );

	if ( mRg(z).start > cs.zrg.start + mStepEps*cs.zrg.step )
	{
	    todocs.zrg.stop = mRg(z).start - cs.zrg.step;
	    mAddAttrOut( todocs )
	}
	    
	if ( mRg(z).stop < cs.zrg.stop - mStepEps*cs.zrg.step )
	{
	    todocs.zrg = cs.zrg;
	    todocs.zrg.start = mRg(z).stop + cs.zrg.step;
	    mAddAttrOut( todocs )
	}
    }

    errmsg = "";
    if ( !calcexec->init() )
    {
	errmsg = calcexec->message();
	delete calcexec;
	return 0;
    }

    Executor* res = new AttribCubeExecutor( calcexec, cs, prev );
    setExecutorName( res );
    return res;
}


class AEMFeatureExtracter : public Executor
{
public:
AEMFeatureExtracter( AttribEngMan& em, const BufferStringSet& inputs,
		     const ObjectSet<BinIDValueSet>& bivsets,
		     ObjectSet<FeatureSet>& f )
	: Executor("Attribute Extraction at locations")
	, fss(f)
{
    outexec = em.getOutputExecutor( errmsg, false );
    if ( !outexec ) return;

    BoolTypeSet issel;
    const int nrinps = inputs.size();
    const AttribDescSet* attrset = em.procattrset
				 ? em.procattrset : em.inpattrset;
    for ( int idx=0; idx<attrset->nrDescs(); idx++ )
    {
	const AttribDesc& ad = attrset->getAttribDesc(idx);
	bool dosel = false;
	for ( int iinp=0; iinp<nrinps; iinp++ )
	{
	    if ( ad.isIdentifiedBy(inputs.get(iinp)) )
		{ dosel = true; break; }
	}
	issel += dosel;
    }

    for ( int idx=0; idx<bivsets.size(); idx++ )
    {
	FeatureSetAttribOutput* output =
		new FeatureSetAttribOutput( *em.processor, em.linekey.buf() );
	output->setLocations( *bivsets[idx] );
	output->isselected = issel;

	FeatureSet* newfs = new FeatureSet;
	fss += newfs;
	output->setFeatureSet( newfs );

	outexec->addOutput( output, idx );
    }

    if ( outexec->init() )
	errmsg = "";
    else
    {
	errmsg = outexec->message();
	delete outexec; outexec = 0;
    }
}


const char* message() const
{
    return *(const char*)errmsg ? (const char*)errmsg
	 : outexec ? outexec->message() : "Cannot create output";
}

int totalNr() const		{ return outexec ? outexec->totalNr() : -1; }
int nrDone() const		{ return outexec ? outexec->nrDone() : 0; }
const char* nrDoneText() const	{ return outexec ? outexec->nrDoneText() : ""; }

int haveError( const char* msg )
{
    if ( msg ) errmsg = msg;
    deepErase( fss );
    return -1;
}

int nextStep()
{
    if ( !outexec ) return haveError( 0 );

    int rv = outexec->doStep();
    if ( rv >= 0 ) return rv;
    return haveError( outexec->message() );
}

    ObjectSet<FeatureSet>&	fss;
    BufferString		errmsg;
    AttribOutputExecutor*	outexec;

};


Executor* AttribEngMan::featureOutputCreator(
			const BufferStringSet& inputs,
			const ObjectSet<BinIDValueSet>& bivsets,
			ObjectSet<FeatureSet>& fss )
{
    return new AEMFeatureExtracter( *this, inputs, bivsets, fss );
}


Executor* AttribEngMan::tableOutputCreator( BufferString& errmsg,
				ObjectSet<BinIDValueSet>& bidzvset )
{
    if ( bidzvset.size() == 0 ) mErrRet("No locations to extract data on")

    AttribOutputExecutor* res = getOutputExecutor( errmsg, !outattribs.size() );
    if ( !res )
	return 0;

    ObjectSet<SurfaceAttribOutput> outputs;
    const int nrpatches = bidzvset.size();
    for ( int idx=0; idx<nrpatches; idx++ )
    {
	SurfaceAttribOutput* attrout = new SurfaceAttribOutput( *processor,
								linekey );
	attrout->addAttribDescSetID( curattrid );
	for ( int ida=0; ida<outattribs.size(); ida++ )
	    attrout->addAttribDescSetID( outattribs[ida] );

	BinIDValueSet& bidzvs = *bidzvset[idx];
	attrout->setLocations( bidzvs );
	attrout->setOutput( bidzvs );
	outputs += attrout;
    }
    
    if ( !outputs.size() )
    {
	delete res;
        return 0;
    }

    if ( !res )
    {
	deepErase( outputs );
	return 0;
    }

    for ( int idx=0; idx<outputs.size(); idx++ )
	res->addOutput( outputs[idx] );

    if ( res->init() )
	errmsg = "";
    else
    {
	errmsg = res->message();
	delete res; res = 0;
    }

    return res;
}


Executor* AttribEngMan::trcSelOutputCreator( BufferString& errmsg,
					     const TypeSet<BinID>& bids,
					     const Interval<float>& zrg,
					     SeisTrcBuf& output )
{
    AttribOutputExecutor* res = getOutputExecutor( errmsg );
    if ( !res )
	return 0;

    TrcSelectionAttribOutput* attrout
		= new TrcSelectionAttribOutput(*processor,linekey);
    attrout->setOutput( &output );
    attrout->setAttribDescSetID( curattrid );
    attrout->addTrcReq( bids, zrg );

    res->addOutput( attrout );

    errmsg = "";
    if ( !res->init() )
    {
	errmsg = res->message();
	delete res; res = 0;
    }
    
    return res;
};
*/
}//namespace

