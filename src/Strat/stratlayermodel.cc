/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "stratlayermodel.h"

#include "od_iostream.h"
#include "separstr.h"
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
	auto* newseq = new LayerSequence( *seq );
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


void Strat::LayerModel::setEmpty()
{
    deepErase( seqs_ );
}


Strat::LayerSequence& Strat::LayerModel::addSequence()
{
    auto* newseq = new LayerSequence( &proprefs_ );
    seqs_ += newseq;
    return *newseq;
}


Strat::LayerSequence& Strat::LayerModel::addSequence(
				const LayerSequence& inpls )
{
    auto* newls = new LayerSequence( &proprefs_ );
    newls->setStartDepth( inpls.startDepth() );

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

    newls->prepareUse();
    seqs_ += newls;
    return *newls;
}


void Strat::LayerModel::removeSequence( int seqidx )
{
    if ( seqs_.validIdx(seqidx) )
	delete seqs_.removeSingle( seqidx );
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


bool Strat::LayerModel::read( od_istream& strm )
{
    deepErase( seqs_ );
    int nrseqs = 0;
    bool mathpreserve = false;
    PropertyRefSelection newprops;
    if ( !readHeader(strm,newprops,nrseqs,mathpreserve) )
	return false;

    const int nrprops = newprops.size();
    proprefs_ = newprops;

    BufferString word;
    const RefTree& rt = RT();

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	strm.skipUntil( 'S' ); // skip "#S.."
	BufferString linestr;
	strm.getLine( linestr );
	SeparString separlinestr( linestr.buf(), od_tab );
	auto* seq = new LayerSequence( &proprefs_ );
	int nrlays = separlinestr.getIValue( 1 );
	if ( separlinestr.size()>2 )
	{
	    float startdepth = separlinestr.getFValue( 2 );
	    seq->setStartDepth( startdepth );
	}
	if ( !strm.isOK() )
	    { ErrMsg("Error during read"); return false; }

	for ( int ilay=0; ilay<nrlays; ilay++ )
	{
	    strm.skipWord(); // skip "#L.."
	    if ( !strm.getWord(word,false) )
	    {
		ErrMsg( BufferString("Incomplete sequence found: ",iseq) );
		delete seq; seq = 0; break;
	    }
	    FileMultiString fms( word );
	    const UnitRef* ur = rt.find( fms[0] );
	    mDynamicCastGet(const LeafUnitRef*,lur,ur)
	    Layer* newlay = new Layer( lur ? *lur : rt.undefLeaf() );
	    if ( fms.size() > 1 )
	    {
		const Content* c = rt.contents().getByName(fms[1]);
		newlay->setContent( c ? *c : Content::unspecified() );
	    }
	    float val; strm >> val;
	    newlay->setThickness( val );
	    if ( !mathpreserve )
	    {
		for ( int iprop=1; iprop<nrprops; iprop++ )
		    { strm >> val; newlay->setValue( iprop, val ); }
		strm.skipLine();
	    }
	    else
	    {
		BufferString txt;
		for ( int iprop=1; iprop<nrprops; iprop++ )
		{
		    strm >> txt;
		    if ( txt.isNumber() )
			newlay->setValue( iprop, toFloat(txt) );
		    else
		    {
			IOPar iop; iop.getFrom( txt );
			newlay->setValue( iprop, iop, proprefs_ );
		    }
		}
	    }
	    seq->layers() += newlay;
	}
	if ( !seq )
	    break;

	seq->prepareUse();
	seqs_ += seq;
    }

    return true;
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
		     << od_tab << seq.startDepth() <<od_endl;

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
    {
	setCurIdx( 0 );
	baseChanged.trigger( hasbasedata );
    }
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
