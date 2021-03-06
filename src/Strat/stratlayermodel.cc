/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2010
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
}


Strat::LayerModel::~LayerModel()
{
    deepErase( seqs_ );
}


Strat::LayerModel& Strat::LayerModel::operator =( const LayerModel& oth )
{
    if ( this == &oth )
	return *this;

    setEmpty();
    props_ = oth.props_;
    elasticpropsel_ = oth.elasticpropsel_;
    for ( int iseq=0; iseq<oth.seqs_.size(); iseq++ )
    {
	auto* newseq = new LayerSequence( *oth.seqs_[iseq] );
	newseq->properties() = props_;
	seqs_ += newseq;
    }

    return *this;
}


bool Strat::LayerModel::isValid() const
{
    for ( int iseq=0; iseq<seqs_.size(); iseq++ )
    {
	if ( !seqs_[iseq]->isEmpty() )
	    return true;
    }

    return false;
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
    auto* newseq = new LayerSequence( &props_ );
    seqs_ += newseq;
    return *newseq;
}


Strat::LayerSequence& Strat::LayerModel::addSequence(
				const LayerSequence& inpls )
{
    auto* newls = new LayerSequence( &props_ );
    newls->setStartDepth( inpls.startDepth() );

    const PropertySelection& inpprops = inpls.properties();
    for ( int ilay=0; ilay<inpls.size(); ilay++ )
    {
	const Layer& inplay = *inpls.layers()[ilay];
	auto* newlay = new Layer( inplay.unitRef() );
	newlay->setThickness( inplay.thickness() );
	newlay->setContent( inplay.content() );
	for ( int iprop=1; iprop<props_.size(); iprop++ )
	{
	    const int idxof = inpprops.indexOf( props_[iprop] );
	    newlay->setValue( iprop,
			idxof < 0 ? mUdf(float) : inplay.value(idxof) );
	}
	newls->layers() += newlay;
    }

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
		PropertySelection& props, int& nrseqs, bool& mathpreserve )
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
	    const Property* p = PROPS().find( propnm.buf() );
	    if ( !p )
	    {
		ErrMsg( BufferString("Property not found: ",propnm) );
		return false;
	    }

	    props += p;
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
    PropertySelection newprops;
    if ( !readHeader(strm,newprops,nrseqs,mathpreserve) )
	return false;

    const int nrprops = newprops.size();
    props_ = newprops;

    BufferString word;
    const RefTree& rt = RT();

    for ( int iseq=0; iseq<nrseqs; iseq++ )
    {
	strm.skipUntil( 'S' ); // skip "#S.."
	BufferString linestr;
	strm.getLine( linestr );
	SeparString separlinestr( linestr.buf(), od_tab );
	LayerSequence* seq = new LayerSequence( &props_ );
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
			newlay->setValue( iprop, iop, props_ );
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
    const int nrprops = props_.size();
    strm << "#M" << modnr << od_tab << nrprops << od_tab << nrseqs << od_endl;

    if ( mathpreserve )
	strm << "#MATH PRESERVED" << od_endl;

    for ( int iprop=0; iprop<nrprops; iprop++ )
	strm << "#P" << iprop << od_tab << props_[iprop]->name() << od_endl;

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


//uiStratLayerModelLMProvider
Strat::LayerModelProviderImpl::LayerModelProviderImpl()
    : modl_(new LayerModel)
{
    setEmpty();
}


Strat::LayerModelProviderImpl::~LayerModelProviderImpl()
{
    delete modl_;
    delete modled_;
}


Strat::LayerModel& Strat::LayerModelProviderImpl::getCurrent()
{
    return *curmodl_;
}


Strat::LayerModel& Strat::LayerModelProviderImpl::getEdited( bool yn )
{
    return yn ? *modled_ : *modl_;
}


void Strat::LayerModelProviderImpl::setUseEdited( bool yn )
{
    curmodl_ = yn ? modled_ : modl_;
}


void Strat::LayerModelProviderImpl::setEmpty()
{
    modl_->setEmpty();
    if ( modled_ )
	modled_->setEmpty();

    curmodl_ = modl_;
}


void Strat::LayerModelProviderImpl::setBaseModel( LayerModel* newmdl )
{
    delete modl_;
    modl_ = newmdl;
}


void Strat::LayerModelProviderImpl::resetEditing()
{
    delete modled_;
    modled_ = new LayerModel;
    curmodl_ = modl_;
}


void Strat::LayerModelProviderImpl::initEditing()
{
    if ( !modled_ )
	modled_ = new LayerModel;

    *modled_ = *modl_;
    curmodl_ = modled_;
}
