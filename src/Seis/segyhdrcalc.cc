/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "segyhdrcalc.h"
#include "segyhdr.h"
#include "mathexpression.h"
#include "executor.h"
#include "od_iostream.h"
#include "settings.h"


SEGY::HdrCalc::HdrCalc( const HdrEntry& he, const char* def )
    : he_(he)
    , def_(def)
{}


SEGY::HdrCalc::~HdrCalc()
{}


SEGY::HdrCalcSet::HdrCalcSet( const SEGY::HdrDef& hd )
    : hdef_(hd)
    , trcidxhe_("Trace index (sequence) number in file","INDEXNR")
{
    reSetSeqNr();
}


SEGY::HdrCalcSet::~HdrCalcSet()
{
    setEmpty();
}


void SEGY::HdrCalcSet::setEmpty()
{
    deepErase( exprs_ );
    deepErase( *this );
    heidxs_.erase();
}


int SEGY::HdrCalcSet::indexOf( const char* nm ) const
{
    const BufferString henm( nm );
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( henm == (*this)[idx]->he_.name() )
	    return idx;
    }
    return -1;
}


bool SEGY::HdrCalcSet::add( const char* dispstr )
{
    BufferString str( dispstr );
    char* ptrdef = str.find( '=' );
    if ( !ptrdef ) return false;
    *ptrdef++ = '\0'; mTrimBlanks(ptrdef);

    removeTrailingBlanks( str.getCStr() );
    const int heidx = hdef_.indexOf( str );
    if ( heidx < 0 ) return false;

    return add( *hdef_[heidx], ptrdef );
}


Math::Expression* SEGY::HdrCalcSet::gtME( const char* def, TypeSet<int>& heidxs,
					  uiString* emsg ) const
{
    Math::ExpressionParser mep( def );
    Math::Expression* me = mep.parse();
    if ( !me )
    {
	if ( emsg ) *emsg = mToUiStringTodo(mep.errMsg());
	return 0;
    }

    heidxs.erase();
    const int nrvars = me->nrVariables();
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	const Math::Expression::VarType vt = me->getType( ivar );
	const BufferString varnm( me->fullVariableExpression(ivar) );
	if ( vt != Math::Expression::Variable )
	{
	    if ( emsg )
	    {
		*emsg = tr("'%1' is a %2.\nThese are not supported.")
		      .arg(varnm)
		      .arg(vt == Math::Expression::Constant
		      ? tr("named constant") : tr("recursive expression"));
	    }
	    delete me; return 0;
	}

	int heidx = hdef_.indexOf( varnm );
	if ( heidx < 0 )
	{
	    if ( varnm == trcidxhe_.name() )
		heidx = -1;
	    else
	    {
		if ( emsg )
		{
		    *emsg = tr(" Found variable: '%1', which is not "
			       "a header field").arg(varnm);
		}
		delete me; return 0;
	    }
	}
	heidxs += heidx;
    }

    return me;
}


bool SEGY::HdrCalcSet::add( const SEGY::HdrEntry& he, const char* def,
			    uiString* emsg )
{
    TypeSet<int>* heidxs = new TypeSet<int>;
    Math::Expression* me = gtME( def, *heidxs, emsg );
    if ( !me )
	{ delete heidxs; return false; }

    *this += new SEGY::HdrCalc( he, def );
    exprs_ += me;
    heidxs_ += heidxs;
    return true;
}


bool SEGY::HdrCalcSet::set( int heidx, const char* def, uiString* emsg )
{
    if ( heidx >= size() )
	return false;

    TypeSet<int>* heidxs = new TypeSet<int>;
    Math::Expression* me = gtME( def, *heidxs, emsg );
    if ( !me )
	{ delete heidxs; return false; }

    HdrCalc& hc = *((*this)[heidx]);
    hc.def_ = def;
    delete exprs_.replace( heidx, me );
    delete heidxs_.replace( heidx, heidxs );
    return true;
}



void SEGY::HdrCalcSet::discard( int idx )
{
    delete removeSingle( idx );
    delete exprs_.removeSingle( idx );
    delete heidxs_.removeSingle( idx );
}


void SEGY::HdrCalcSet::apply( void* buf, bool needswap ) const
{
    if ( needswap )
	hdef_.swapValues( (unsigned char*)buf );

    for ( int iexpr=0; iexpr<exprs_.size(); iexpr++ )
    {
	Math::Expression& me = *(const_cast<Math::Expression*>(exprs_[iexpr]));
	const TypeSet<int>& heidxs = *heidxs_[iexpr];
	for ( int ivar=0; ivar<heidxs.size(); ivar++ )
	{
	    float val = mCast( float, seqnr_ );
	    const int heidx = heidxs[ivar];
	    if ( heidx >= 0 )
		val = mCast( float, hdef_[heidx]->getValue( buf ) );
	    me.setVariableValue( ivar, val );
	}

	const float meval = mCast(float,me.getValue());
	(*this)[iexpr]->he_.putValue( buf, mNINT32(meval) );
    }

    seqnr_++;
}

#define mSEGYFileHdrSize 3600

class SEGYHdrCalcSetapplier : public Executor
{ mODTextTranslationClass(SEGYHdrCalcSetapplier);
public:

SEGYHdrCalcSetapplier( const SEGY::HdrCalcSet& cs,
			od_istream& is, od_ostream& os, int dbpt,
			const SEGY::BinHeader* bh, const SEGY::TxtHeader* th )
    : Executor("Manipulate SEG-Y file")
    , tkzs_(cs)
    , inpstrm_(is)
    , outstrm_(os)
    , bptrc_(dbpt+240)
    , nrdone_(-1)
    , msg_(tr("Handling traces"))
    , needswap_(bh ? bh->isSwapped() : false)
{
    totalnr_ = inpstrm_.endPosition();
    inpstrm_.setReadPosition( 0 );

    buf_ = new unsigned char [bptrc_>mSEGYFileHdrSize?bptrc_:mSEGYFileHdrSize];
    if ( !inpstrm_.getBin(buf_,mSEGYFileHdrSize) )
	msg_ = tr("Cannot read file headers");
    else
    {
	if ( th )
	    OD::memCopy( buf_, th->txt_, 3200 );
	if ( bh )
	{
	    OD::memCopy( buf_+3200, bh->buf(), 400 );
	    if ( needswap_ )
		SEGY::BinHeader::hdrDef().swapValues( buf_+3200 );
	}

	if ( !outstrm_.addBin(buf_,mSEGYFileHdrSize) )
	    msg_ = tr("Cannot write to output file");
	else
	    nrdone_ = 0;
    }

    totalnr_ -= mSEGYFileHdrSize;
    totalnr_ /= bptrc_;
}

uiString uiMessage() const override		{ return msg_; }
uiString uiNrDoneText() const override		{ return tr("Traces handled"); }
od_int64 nrDone() const override		{ return nrdone_; }
od_int64 totalNr() const override		{ return totalnr_; }

int nextStep() override
{
    if ( nrdone_ < 0 )
	return ErrorOccurred();

    if ( nrdone_ >= totalnr_ )
	return Finished();

    if ( !inpstrm_.getBin(buf_,bptrc_) )
    {
	msg_ = tr("Unexpected early end of input file encountered");
	return ErrorOccurred();
    }
    tkzs_.apply( buf_, needswap_ );
    if ( !outstrm_.addBin(buf_,bptrc_) )
    {
	msg_ = tr("Cannot write to output file."
		  "\nWrote %1 traces.\nTotal: %2"
		  " available in input file")
	     .arg(nrdone_).arg(totalnr_);
	return ErrorOccurred();
    }

    nrdone_++;
    return nrdone_ == totalnr_ ? Finished() : MoreToDo();
}

    const SEGY::HdrCalcSet& tkzs_;
    od_istream&		inpstrm_;
    od_ostream&		outstrm_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    const unsigned int	bptrc_;
    unsigned char*	buf_;
    bool		needswap_;
    uiString		msg_;

};


Executor* SEGY::HdrCalcSet::getApplier( od_istream& is, od_ostream& os,
	int dbpt, const SEGY::BinHeader* bh, const SEGY::TxtHeader* th ) const
{
    return new SEGYHdrCalcSetapplier( *this, is, os, dbpt, bh, th );
}


#define mKeyName(idx) BufferString(toString(idx),".Name")
#define mKeyForm(idx,fidx) BufferString("",idx,".Form.").add(fidx)

void SEGY::HdrCalcSet::getStoredNames( BufferStringSet& nms )
{
    Settings& setts = Settings::fetch( sKeySettsFile() );
    for ( int idx=0; ; idx++ )
    {
	const BufferString nm = setts.find( mKeyName(idx) );
	if ( nm.isEmpty() )
	    break;

	nms.add( nm );
    }
}


void SEGY::HdrCalcSet::getFromSettings( const char* reqnm )
{
    setEmpty(); setName( reqnm );

    Settings& setts = Settings::fetch( sKeySettsFile() );
    int iopidx = -1;
    for ( int idx=0; ; idx++ )
    {
	const BufferString nm( setts.find( mKeyName(idx) ) );
	if ( nm.isEmpty() )
	    break;

	if ( nm == reqnm )
	{
	    iopidx = idx;
	    break;
	}
    }

    if ( iopidx < 0 )
	return;

    for ( int idx=0; ; idx++ )
    {
	const BufferString form( setts.find( mKeyForm(iopidx,idx) ) );
	if ( form.isEmpty() )
	    break;
	add( form );
    }
}


bool SEGY::HdrCalcSet::storeInSettings() const
{
    if ( name().isEmpty() )
	return false;

    Settings& setts = Settings::fetch( sKeySettsFile() );
    int iopidx = 0;
    for ( ; ; iopidx++ )
    {
	const BufferString nm( setts.find( mKeyName(iopidx) ) );
	if ( nm.isEmpty() )
	    break;
	else if ( nm == name() )
	{
	    setts.removeSubSelection( iopidx );
	    break;
	}
    }

    setts.set( mKeyName(iopidx), name() );
    for ( int idx=0; idx<size(); idx++ )
	setts.set( mKeyForm(iopidx,idx), (*this)[idx]->getDispStr() );

    return setts.write();
}
