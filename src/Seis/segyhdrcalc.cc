/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2011
-*/



#include "segyhdrcalc.h"
#include "segyhdr.h"
#include "mathexpression.h"
#include "executor.h"
#include "od_iostream.h"
#include "settings.h"


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
	if ( emsg ) *emsg = mep.errMsg();
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

class SEGYHdrCalcSetApplier : public Executor
{ mODTextTranslationClass(SEGYHdrCalcSetApplier);
public:

SEGYHdrCalcSetApplier( const SEGY::HdrCalcSet& cs,
			od_istream& is, od_ostream& os,
			const SEGY::TxtHeader* th, const SEGY::BinHeader* bh )
    : Executor("Manipulate SEG-Y file")
    , hcs_(cs)
    , inpstrm_(is)
    , outstrm_(os)
    , nrdone_(-1)
    , bptrc_(240)
    , msg_(tr("Handling traces"))
    , needswap_(bh ? bh->isSwapped() : false)
    , rdbuf_(nullptr)
{
    totalnr_ = inpstrm_.endPosition();
    inpstrm_.setReadPosition( 0 );

    auto* fhdrbuf = new unsigned char [mSEGYFileHdrSize];
    if ( !inpstrm_.getBin(fhdrbuf,mSEGYFileHdrSize) )
	msg_ = tr("Cannot read file headers");
    else
    {
	if ( th )
	    OD::memCopy( fhdrbuf, th->txt_, 3200 );

	SEGY::BinHeader filebh;
	if ( !bh )
	{
	    filebh.setInput( fhdrbuf+3200 );
	    bh = &filebh;
	}
	else
	{
	    OD::memCopy( fhdrbuf+3200, bh->buf(), 400 );
	    if ( needswap_ )
		SEGY::BinHeader::hdrDef().swapValues( fhdrbuf+3200 );
	}
	bptrc_ = 240 + bh->nrSamples() * bh->bytesPerSample();

	if ( !outstrm_.addBin(fhdrbuf,mSEGYFileHdrSize) )
	    msg_ = tr("Cannot write to output file");
	else
	    nrdone_ = 0;
    }

    totalnr_ -= mSEGYFileHdrSize;
    totalnr_ /= bptrc_;

    rdbuf_ = new unsigned char [bptrc_];
}

~SEGYHdrCalcSetApplier()
{
    delete rdbuf_;
}

uiString message() const		{ return msg_; }
uiString nrDoneText() const		{ return tr("Traces handled"); }
od_int64 nrDone() const			{ return nrdone_; }
od_int64 totalNr() const		{ return totalnr_; }

int nextStep()
{
    if ( nrdone_ < 0 || !rdbuf_ )
	return ErrorOccurred();

    if ( nrdone_ >= totalnr_ )
	return Finished();

    if ( !inpstrm_.getBin(rdbuf_,bptrc_) )
    {
	msg_ = tr("Unexpected early end of input file encountered");
	return ErrorOccurred();
    }
    hcs_.apply( rdbuf_, needswap_ );
    if ( !outstrm_.addBin(rdbuf_,bptrc_) )
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

    const SEGY::HdrCalcSet& hcs_;
    od_istream&		inpstrm_;
    od_ostream&		outstrm_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    unsigned int	bptrc_;
    unsigned char*	rdbuf_;
    bool		needswap_;
    uiString		msg_;

};


Executor* SEGY::HdrCalcSet::getApplier( od_istream& is, od_ostream& os,
			const TxtHeader* th, const BinHeader* bh ) const
{
    return new SEGYHdrCalcSetApplier( *this, is, os, th, bh );
}


#define mKeyName(idx) BufferString(toString(idx),".Name")
#define mKeyForm(idx,fidx) BufferString("",idx,".Form.").add(fidx)

void SEGY::HdrCalcSet::getStoredNames( BufferStringSet& nms )
{
    Settings& setts = Settings::fetch( sKeySettsFile() );
    for ( int idx=0; ; idx++ )
    {
	const char* nm = setts.find( mKeyName(idx) );
	if ( !nm || !*nm )
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
	    { iopidx = idx; break; }
    }
    if ( iopidx < 0 ) return;

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
	    setts.removeWithKeyPattern( BufferString("",iopidx,".*") );
	    break;
	}
    }

    setts.set( mKeyName(iopidx), name() );
    for ( int idx=0; idx<size(); idx++ )
	setts.set( mKeyForm(iopidx,idx), (*this)[idx]->getDispStr() );

    return setts.write();
}
