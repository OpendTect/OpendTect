/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlayermodel.h"

#include "executor.h"
#include "od_iostream.h"
#include "separstr.h"
#include "statparallelcalc.h"
#include "stratlayer.h"
#include "stratlayersequence.h"
#include "stratreftree.h"
#include "strattransl.h"

mDefSimpleTranslators(StratLayerModels,"Pseudo Wells",od,Mdl)


//------ LayerModel ------

Strat::LayerModel::LayerModel()
{
    if ( !proprefs_.isPresent(&Layer::thicknessRef()) )
    {
	pErrMsg("No thickness in property ref selection");
	proprefs_.add( &Layer::thicknessRef() );
    }

    elasticpropsel_.setEmpty();
}


Strat::LayerModel::LayerModel( const LayerModel& lm )
{ *this = lm; }


Strat::LayerModel::~LayerModel()
{
    deepErase( seqs_ );
}


Strat::LayerModel& Strat::LayerModel::operator =( const LayerModel& oth )
{
    if ( &oth == this )
	return *this;

    setEmpty();
    proprefs_ = oth.proprefs_;
    elasticpropsel_ = oth.elasticpropsel_;
    for ( const auto* seq : oth.seqs_ )
    {
	auto* newseq = new LayerSequenceOv( *seq );
	newseq->propertyRefs() = proprefs_;
	seqs_ += newseq;
    }

    return *this;
}


bool Strat::LayerModel::isValid() const
{
    for ( const auto* seq : seqs_ )
    {
	if ( !seq->isEmpty() )
	    return true;
    }

    return false;
}


bool Strat::LayerModel::isEmpty() const
{
    for ( const auto* seq : seqs_ )
    {
	if ( !seq->isEmpty() )
	    return false;
    }

    return true;
}


int Strat::LayerModel::nrLayers() const
{
    int ret = 0;
    for ( const auto* seq : seqs_ )
	ret += seq->size();
    return ret;
}


Interval<float> Strat::LayerModel::zRange() const
{
    if ( isEmpty() )
	return Interval<float>( 0, 0 );

    Interval<float> ret( seqs_[0]->zRange() );
    for ( int iseq=1; iseq<seqs_.size(); iseq++ )
	ret.include( seqs_[iseq]->zRange(), false );
    return ret;
}


float Strat::LayerModel::startDepth( Stats::Type st ) const
{
    const int sz = size();
    if ( sz <= 0 )
	return mUdf(float);

    TypeSet<float> zvals( sz, mUdf(float) );
    for ( int idx=0; idx<sz; idx++ )
	zvals[idx] = seqs_.get( idx )->startDepth();

    Stats::CalcSetup stsu;
    stsu.require( st );
    Stats::ParallelCalc calc( stsu, zvals.arr(), sz );
    if ( !calc.execute() )
	return mUdf(float);

    return float( calc.getValue(st) );
}


float Strat::LayerModel::overburdenVelocity( Stats::Type st ) const
{
    const int sz = size();
    if ( sz <= 0 )
	return mUdf(float);

    TypeSet<float> velvals( sz, mUdf(float) );
    for ( int idx=0; idx<sz; idx++ )
	velvals[idx] = seqs_.get( idx )->overburdenVelocity();

    Stats::CalcSetup stsu;
    stsu.require( st );
    Stats::ParallelCalc calc( stsu, velvals.arr(), sz );
    if ( !calc.execute() )
	return mUdf(float);

    return float( calc.getValue(st) );
}


void Strat::LayerModel::setEmpty()
{
    deepErase( seqs_ );
}


Strat::LayerSequence& Strat::LayerModel::addSequence()
{
    auto* newseq = new LayerSequenceOv( &proprefs_ );
    seqs_ += newseq;
    return *newseq;
}


Strat::LayerSequence& Strat::LayerModel::addSequence(
				const LayerSequence& inpls )
{
    auto* newls = new LayerSequenceOv( &proprefs_ );
    const PropertyRefSelection& inpprops = inpls.propertyRefs();
    for ( int ilay=0; ilay<inpls.size(); ilay++ )
    {
	const Layer& inplay = *inpls.layers()[ilay];
	auto* newlay = new Layer( inplay.unitRef() );
	newlay->setThickness( inplay.thickness() );
	newlay->setContent( inplay.content() );
	for ( int iprop=1; iprop<proprefs_.size(); iprop++ )
	{
	    const int idxof = inpprops.indexOf( proprefs_[iprop] );
	    newlay->setValue( iprop,
			idxof < 0 ? mUdf(float) : inplay.value(idxof) );
	}
	newls->layers() += newlay;
    }

    newls->setStartDepth( inpls.startDepth() );
    newls->setOverburdenVelocity_( inpls.overburdenVelocity() );
    seqs_ += newls;
    return *newls;
}


void Strat::LayerModel::removeSequence( int seqidx )
{
    if ( seqs_.validIdx(seqidx) )
	delete seqs_.removeSingle( seqidx );
}


void Strat::LayerModel::append( const LayerModel& oth )
{
    for ( int iseq=0; iseq<oth.size(); iseq++ )
	addSequence( oth.sequence(iseq) );
}


void Strat::LayerModel::prepareUse() const
{
    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
	seqs_[iseq]->prepareUse();
}


const Strat::RefTree& Strat::LayerModel::refTree() const
{
    return isEmpty() ? RT() : seqs_[0]->refTree();
}


bool Strat::LayerModel::readHeader( od_istream& strm,
		PropertyRefSelection& props, int& nrseqs, bool& mathpreserve )
{
    BufferString word;
    strm.getWord( word, false );
    if ( word[0] != '#' || word[1] != 'M' )
	{ ErrMsg( "File needs to start with '#M'" ); return false; }

    int nrprops = 0;
    strm >> nrprops >> nrseqs;
    if ( nrprops < 1 )
	{ ErrMsg( "No properties found in file" ); return false; }
    strm.skipLine();

    BufferString keyw;
    strm.getWord( keyw );
    mathpreserve = keyw == "#MATH";
    if ( mathpreserve )
	{ strm.skipLine(); strm.skipWord(); }

    for ( int iprop=0; iprop<nrprops; iprop++ )
    {
	if ( iprop )
	    strm.skipWord(); // skip "#P.."
	BufferString propnm;
	strm.getLine( propnm );
	if ( iprop != 0 )
	{
	    const PropertyRef* pr = PROPS().getByName( propnm.buf(), false );
	    if ( !pr )
	    {
		ErrMsg( BufferString("Property not found: ",propnm) );
		return false;
	    }

	    props.addIfNew( pr );
	}
    }

    if ( !strm.isOK() )
	{ ErrMsg("No sequences found"); return false; }

    return true;
}


namespace Strat
{
class LayerModelReader : public Executor
{ mODTextTranslationClass(LayerModelReader)
public:
LayerModelReader( Strat::LayerModel& lm, od_istream& strm,
		  const StepInterval<int>& readrg, bool mathpreserve,
		  float startdepth, float abovevel )
    : Executor("Reading pseudo-wells")
    , lm_(lm)
    , strm_(strm)
    , readrg_(readrg)
    , mathpreserve_(mathpreserve)
    , startdepth_(startdepth)
    , abovevel_(abovevel)
{
    nextreadidx_ = readrg_.start;
}

od_int64 nrDone() const
{
    return curidx_;
}

od_int64 totalNr() const
{
    return readrg_.stop;
}

uiString uiNrDoneText() const
{
    return tr("Pseudo-wells done");
}

bool skipWell()
{
    strm_.skipUntil( 'S' );
    while ( strm_.isOK() )
    {
	if ( strm_.peek() != '#' )
	{
	    strm_.skipLine();
	    continue;
	}

	strm_.ignore( 1 ); // skip '#'
	if ( strm_.peek() == 'S' )
	    break;

	strm_.skipLine();
    }

    return strm_.isOK();
}

int nextStep()
{
    if ( nextreadidx_ > readrg_.stop )
	return Finished();

    if ( !strm_.isOK() )
	return ErrorOccurred();

    while ( curidx_ < nextreadidx_ && skipWell() )
	curidx_++;

    strm_.skipUntil( 'S' );
    BufferString linestr;
    strm_.getLine( linestr );
    const SeparString separlinestr( linestr.buf(), od_tab );
    auto* seq = new LayerSequenceOv( &lm_.propertyRefs() );
    const int nrlays = separlinestr.getIValue( 1 );
    const float startdepth = separlinestr.size()>2 ? separlinestr.getFValue( 2 )
						   : startdepth_;
    const float ovvel = separlinestr.size()>3 ? separlinestr.getFValue( 3 )
					      : abovevel_;
    if ( !strm_.isOK() )
	return ErrorOccurred();

    const int nrprops = lm_.propertyRefs().size();
    BufferString word;
    const Strat::RefTree& rt = RT();
    for ( int ilay=0; ilay<nrlays; ilay++ )
    {
	strm_.skipWord(); // skip "#L.."
	if ( !strm_.getWord(word,false) )
	{
	    delete seq;
	    return ErrorOccurred();
	}

	FileMultiString fms( word );
	const BufferString unitname = fms[0];
	const Strat::UnitRef* ur = rt.find( unitname );
	mDynamicCastGet(const Strat::LeafUnitRef*,lur,ur)
	if ( !lur )
	    missinglayerunits_.addIfNew( unitname );

	auto* newlay = new Strat::Layer( lur ? *lur : rt.undefLeaf() );
	if ( fms.size() > 1 )
	{
	    const Content* c = rt.contents().getByName(fms[1]);
	    newlay->setContent( c ? *c : Content::unspecified() );
	}

	float val; strm_ >> val;
	newlay->setThickness( val );
	if ( !mathpreserve_ )
	{
	    for ( int iprop=1; iprop<nrprops; iprop++ )
	    {
		strm_ >> val;
		newlay->setValue( iprop, val );
	    }

	    strm_.skipLine();
	}
	else
	{
	    BufferString txt;
	    for ( int iprop=1; iprop<nrprops; iprop++ )
	    {
		strm_ >> txt;
		if ( txt.isNumber() )
		    newlay->setValue( iprop, toFloat(txt) );
		else
		{
		    IOPar iop; iop.getFrom( txt );
		    newlay->setValue( iprop, iop, lm_.proprefs_ );
		}
	    }
	}

	seq->layers() += newlay;
    }

    if ( !mIsUdf(startdepth) )
	seq->setStartDepth( startdepth );

    if ( !mIsUdf(ovvel) )
	seq->setOverburdenVelocity_( ovvel );

    lm_.seqs_ += seq;
    curidx_++;
    nextreadidx_ += readrg_.step;
    return MoreToDo();
}

    Strat::LayerModel&		lm_;
    od_istream&			strm_;
    const StepInterval<int>&	readrg_;
    bool			mathpreserve_;
    float			startdepth_;
    float			abovevel_;

    int				curidx_ = 0;
    int				nextreadidx_;
    BufferStringSet		missinglayerunits_;
};

} // namespace Strat

bool Strat::LayerModel::read( od_istream& strm, int start, int step,
			      uiString& msg, TaskRunner* trunner,
			      float z0, float v0 )
{
    int nrseqs = 0;
    bool mathpreserve = false;
    PropertyRefSelection newprops;
    if ( !readHeader(strm,newprops,nrseqs,mathpreserve) )
	return false;

    proprefs_ = newprops;
    const int nrseqtoread = (nrseqs-start-1)/step + 1;
    StepInterval<int> readrg( start, start + step*(nrseqtoread-1), step );

    LayerModelReader rdr( *this, strm, readrg, mathpreserve, z0, v0 );
    if ( !TaskRunner::execute(trunner,rdr) )
	return false;

    if ( !rdr.missinglayerunits_.isEmpty() )
	msg = tr("The following layer units present in the Pseudo-wells file "
		"are missing from the current Stratigraphic tree : %1")
	    .arg(rdr.missinglayerunits_.getDispString());

    return true;
}


bool Strat::LayerModel::read( od_istream& strm, int start, int step,
			      uiString& msg, TaskRunner* trunner )
{
    deepErase( seqs_ );
    return read( strm, start, step, msg, trunner, mUdf(float), mUdf(float) );
}


bool Strat::LayerModel::read( od_istream& strm )
{
    deepErase( seqs_ );
    uiString msg;
    return read( strm, 0, 1, msg, nullptr, mUdf(float), mUdf(float) );
}


bool Strat::LayerModel::write( od_ostream& strm, int modnr,
					bool mathpreserve ) const
{
    const int nrseqs = seqs_.size();
    const int nrprops = proprefs_.size();
    strm << "#M" << modnr << od_tab << nrprops << od_tab << nrseqs << od_endl;

    if ( mathpreserve )
	strm << "#MATH PRESERVED" << od_endl;

    for ( int iprop=0; iprop<nrprops; iprop++ )
	strm << "#P" << iprop << od_tab << proprefs_[iprop]->name() << od_endl;

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	const LayerSequence& seq = *seqs_[iseq];
	const int nrlays = seq.size();
	strm << "#S" << iseq << od_tab << nrlays
		     << od_tab << seq.startDepth()
		     << od_tab << seq.overburdenVelocity() << od_endl;

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    strm << "#L" << ilay << od_tab;
	    const Layer& lay = *seq.layers()[ilay];
	    if ( lay.content().isUnspecified() )
		strm << '\'' << lay.name() << '\'';
	    else
	    {
		FileMultiString fms( lay.name() );
		fms += lay.content().name();
		strm << '\'' << fms << '\'';
	    }
	    strm << od_tab << toString(lay.thickness());
	    for ( int iprop=1; iprop<nrprops; iprop++ )
	    {
		if ( !mathpreserve || !lay.isMath(iprop) )
		    strm << od_tab << toString(lay.value(iprop));
		else
		{
		    const LayerValue& lv = *lay.getLayerValue( iprop );
		    strm << "\t'" << lv.dumpStr() << '\'';
		}
	    }
	    strm << od_endl;
	}
    }
    return strm.isOK();
}


void Strat::LayerModel::setElasticPropSel( const ElasticPropSelection& elp )
{
    elasticpropsel_ = elp;
}


//Strat::LayerModelSuite
Strat::LayerModelSuite::LayerModelSuite()
    : curChanged(this)
    , baseChanged(this)
    , editingChanged(this)
    , modelChanged(this)
{
    addModel( "", uiString::empty() );
}


Strat::LayerModelSuite::~LayerModelSuite()
{}


void Strat::LayerModelSuite::setEmpty()
{
    baseModel().setEmpty();
    clearEditedData();
}


void Strat::LayerModelSuite::setBaseModel( LayerModel* newmod,
					   bool setascurrent )
{
    const bool hasbasedata = !baseModel().isEmpty();
    if ( newmod )
    {
	if ( size() > 0 && !baseModel().elasticPropSel().isEmpty() )
	    newmod->setElasticPropSel( baseModel().elasticPropSel() );

	mdls_.replace( 0, newmod );
    }
    else
	baseModel().setEmpty();

    clearEditedData();
    if ( setascurrent )
	setCurIdx( 0 );

    baseChanged.trigger( hasbasedata );
}


void Strat::LayerModelSuite::setCurIdx( int idx )
{
    if ( mdls_.validIdx(idx) && idx != curidx_ )
    {
	curidx_ = idx;
	curChanged.trigger();
    }
}


void Strat::LayerModelSuite::setDesc( int imdl, const char* dsc,
				      const uiString& uidsc )
{
    if ( mdls_.validIdx(imdl) )
    {
	descs_.get( imdl ) = dsc;
	uidescs_.get( imdl ) = uidsc;
    }
}


void Strat::LayerModelSuite::addModel( const char* dsc, const uiString& uidsc )
{
    auto* newmod = new LayerModel;
    mdls_.add( newmod );
    descs_.add( dsc );
    uidescs_.add( uidsc );
    if ( size() > 1 )
    {
	newmod->setElasticPropSel( baseModel().elasticPropSel() );
	newmod->propertyRefs() = baseModel().propertyRefs();
    }
}


void Strat::LayerModelSuite::removeModel( int idx )
{
    if ( idx < 1 )
	{ pErrMsg("attempt to remove base model"); return; }

    const bool haded = hasEditedData();

    mdls_.removeSingle( idx );
    descs_.removeSingle( idx );
    uidescs_.removeSingle( idx );

    const bool removediscur = idx == curidx_;
    if ( removediscur )
	curidx_--;

    if ( hasEditedData() != haded )
	editingChanged.trigger( haded );

    if ( removediscur )
	curChanged.trigger();
}


void Strat::LayerModelSuite::setElasticPropSel(
				const ElasticPropSelection& elpropsel )
{
    for ( auto* mdl : mdls_ )
	mdl->setElasticPropSel( elpropsel );
}


bool Strat::LayerModelSuite::hasEditedData() const
{
    return size() > 1 && !mdls_.get(1)->isEmpty();
}


void Strat::LayerModelSuite::clearEditedData()
{
    if ( !hasEditedData() )
	return;

    for ( int imdl=1; imdl<size(); imdl++ )
	mdls_.get( imdl )->setEmpty();

    editingChanged.trigger( true );
}


void Strat::LayerModelSuite::prepareEditing()
{
    if ( size() < 2 )
	return;

    const bool haded = hasEditedData();

    for ( int imdl=1; imdl<size(); imdl++ )
	*mdls_.get( imdl ) = baseModel();

    editingChanged.trigger( haded );
}
