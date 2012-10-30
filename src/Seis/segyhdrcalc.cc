/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2011
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "segyhdrcalc.h"
#include "segyhdr.h"
#include "mathexpression.h"
#include "executor.h"
#include "strmoper.h"
#include "settings.h"


SEGY::HdrCalcSet::HdrCalcSet( const SEGY::HdrDef& hd )
    : hdef_(hd)
{
    trcidxhe_.setUdf();
    trcidxhe_.setName("INDEXNR");
    trcidxhe_.setDescription("Trace index (sequence) number in file");
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
    char* ptrdef = strchr( str.buf(), '=' );
    if ( !ptrdef ) return false;
    *ptrdef++ = '\0'; mTrimBlanks(ptrdef);

    removeTrailingBlanks( str.buf() );
    const int heidx = hdef_.indexOf( str );
    if ( heidx < 0 ) return false;

    return add( *hdef_[heidx], ptrdef );
}


MathExpression* SEGY::HdrCalcSet::gtME( const char* def, TypeSet<int>& heidxs,
					BufferString* emsg ) const
{
    MathExpressionParser mep( def );
    MathExpression* me = mep.parse();
    if ( !me )
    {
	if ( emsg ) *emsg = mep.errMsg();
	return 0;
    }

    heidxs.erase();
    const int nrvars = me->nrVariables();
    for ( int ivar=0; ivar<nrvars; ivar++ )
    {
	const MathExpression::VarType vt = me->getType( ivar );
	const BufferString varnm( me->fullVariableExpression(ivar) );
	if ( vt != MathExpression::Variable )
	{
	    if ( emsg )
	    {
		*emsg = "'";
		emsg->add( varnm ).add( "' is a ")
		    .add( vt == MathExpression::Constant
			? "named constant" : "recursive expression" )
		    .add( "\nThese are not supported." );
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
		    *emsg = " Found variable: '";
		    emsg->add( varnm ).add( "', which is not a header field");
		}
		delete me; return 0;
	    }
	}
	heidxs += heidx;
    }

    return me;
}


bool SEGY::HdrCalcSet::add( const SEGY::HdrEntry& he, const char* def,
			    BufferString* emsg )
{
    TypeSet<int>* heidxs = new TypeSet<int>;
    MathExpression* me = gtME( def, *heidxs, emsg );
    if ( !me )
	{ delete heidxs; return false; }

    *this += new SEGY::HdrCalc( he, def );
    exprs_ += me;
    heidxs_ += heidxs;
    return true;
}


bool SEGY::HdrCalcSet::set( int heidx, const char* def, BufferString* emsg )
{
    if ( heidx >= size() )
	return false;

    TypeSet<int>* heidxs = new TypeSet<int>;
    MathExpression* me = gtME( def, *heidxs, emsg );
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
	MathExpression& me = *(const_cast<MathExpression*>(exprs_[iexpr]));
	const TypeSet<int>& heidxs = *heidxs_[iexpr];
	for ( int ivar=0; ivar<heidxs.size(); ivar++ )
	{
	    float val = seqnr_;
	    const int heidx = heidxs[ivar];
	    if ( heidx >= 0 )
		val = hdef_[heidx]->getValue( buf );
	    me.setVariableValue( ivar, val );
	}

	const float meval = me.getValue();
	(*this)[iexpr]->he_.putValue( buf, mNINT32(meval) );
    }

    seqnr_++;
}

#define mSEGYFileHdrSize 3600

class SEGYHdrCalcSetapplier : public Executor
{
public:

SEGYHdrCalcSetapplier( const SEGY::HdrCalcSet& cs,
			std::istream& is, std::ostream& os, int dbpt,
			const SEGY::BinHeader* bh, const SEGY::TxtHeader* th )
    : Executor("Manipulate SEG-Y file")
    , cs_(cs)
    , inpstrm_(is)
    , outstrm_(os)
    , bptrc_(dbpt+240)
    , nrdone_(-1)
    , msg_("Handling traces")
    , needswap_(bh ? bh->isSwapped() : false)
{
    StrmOper::seek( inpstrm_, 0, std::ios::end );
    totalnr_ = StrmOper::tell( inpstrm_ );
    StrmOper::seek( inpstrm_, 0 );

    buf_ = new unsigned char [bptrc_>mSEGYFileHdrSize?bptrc_:mSEGYFileHdrSize];
    if ( !StrmOper::readBlock(inpstrm_,buf_,mSEGYFileHdrSize) )
	msg_ = "Cannot read file headers";
    else
    {
	if ( th )
	    memcpy( buf_, th->txt_, 3200 );
	if ( bh )
	{
	    memcpy( buf_+3200, bh->buf(), 400 );
	    if ( needswap_ )
		SEGY::BinHeader::hdrDef().swapValues( buf_+3200 );
	}

	if ( !StrmOper::writeBlock(outstrm_,buf_,mSEGYFileHdrSize) )
	    msg_ = "Cannot write to output file";
	else
	    nrdone_ = 0;
    }

    totalnr_ -= mSEGYFileHdrSize;
    totalnr_ /= bptrc_;
}

const char* message() const		{ return msg_; }
const char* nrDoneText() const		{ return "Traces handled"; }
od_int64 nrDone() const			{ return nrdone_; }
od_int64 totalNr() const		{ return totalnr_; }

int nextStep()
{
    if ( nrdone_ < 0 )
	return ErrorOccurred();

    if ( nrdone_ >= totalnr_ )
	return Finished();

    if ( !StrmOper::readBlock(inpstrm_,buf_,bptrc_) )
    {
	msg_ = "Unexpected early end of input file encountered";
	return ErrorOccurred();
    }
    cs_.apply( buf_, needswap_ );
    if ( !StrmOper::writeBlock(outstrm_,buf_,bptrc_) )
    {
	msg_ = "Cannot write to output file.";
	msg_.add( "\nWrote " ).add( nrdone_ )
	.add( " traces.\nTotal: " ).add( totalnr_ )
	.add( " available in input file" );
	return ErrorOccurred();
    }

    nrdone_++;
    return nrdone_ == totalnr_ ? Finished() : MoreToDo();
}

    const SEGY::HdrCalcSet& cs_;
    std::istream&	inpstrm_;
    std::ostream&	outstrm_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    const unsigned int	bptrc_;
    unsigned char*	buf_;
    bool		needswap_;
    BufferString	msg_;

};


Executor* SEGY::HdrCalcSet::getApplier( std::istream& is, std::ostream& os,
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
	    setts.removeWithKey( BufferString("",iopidx,".*") );
	    break;
	}
    }

    setts.set( mKeyName(iopidx), name() );
    for ( int idx=0; idx<size(); idx++ )
	setts.set( mKeyForm(iopidx,idx), (*this)[idx]->getDispStr() );

    return setts.write();
}
