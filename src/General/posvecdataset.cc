/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "posvecdataset.h"

#include "ascstream.h"
#include "datacoldef.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "od_ostream.h"


const DataColDef& DataColDef::unknown()
{
    static DataColDef* def = 0;
    if ( !def )
	def = new DataColDef( "Unknown", 0 );
    return *def;
}


DataColDef::MatchLevel DataColDef::compare( const DataColDef& cd,
					    bool usenm ) const
{
    const BufferString& mystr = usenm ? name_ : ref_;
    const BufferString& cdstr = usenm ? cd.name_ : cd.ref_;
    if ( mystr == cdstr )
	return DataColDef::Exact;

    if ( matchString(mystr.buf(),cdstr.buf())
      || matchString(cdstr.buf(),mystr.buf()) )
	return DataColDef::Start;

    return DataColDef::None;
}


void DataColDef::putTo( BufferString& bs ) const
{
    FileMultiString fms( name_ );
    fms += FileMultiString( ref_ );
    if ( unit_ ) fms += unit_->name();
    bs = fms;
}


void DataColDef::getFrom( const char* s )
{
    FileMultiString fms( s );
    const int sz = fms.size();
    if ( sz < 1 )
	*this = unknown();
    else
    {
	name_ = fms[0];
	if ( sz > 1 )
	    ref_ = fms[1];
	if ( sz > 2 )
	{
	    const char* lastpart = fms.from(2);
	    if ( isdigit(*lastpart) )
		ref_ = fms.from( 1 );
	    else
		unit_ = UoMR().get( lastpart );
	}
    }
}


PosVecDataSet::PosVecDataSet( const char* nm )
	: data_(1,true)
	, pars_(*new IOPar)
	, name_(nm)
{
    setEmpty();
}


PosVecDataSet::PosVecDataSet( const PosVecDataSet& vds )
	: data_(1,true)
	, pars_(*new IOPar)
{
    *this = vds;
}


PosVecDataSet::~PosVecDataSet()
{
    deepErase( coldefs_ );
    delete &pars_;
}


PosVecDataSet& PosVecDataSet::operator =( const PosVecDataSet& vds )
{
    if ( &vds == this ) return *this;

    copyStructureFrom( vds );
    name_ = vds.name();
    pars_ = vds.pars_;

    const BinIDValueSet& bvs = vds.data();
    BinIDValueSet::SPos pos;
    while ( bvs.next(pos) )
	data_.add( bvs.getBinID(pos), bvs.getVals( pos ) );

    return *this;
}


void PosVecDataSet::copyStructureFrom( const PosVecDataSet& vds )
{
    setEmpty();
    data_.copyStructureFrom( vds.data_ );
    deepErase( coldefs_ );
    deepCopy( coldefs_, vds.coldefs_ );
    pars_ = vds.pars_;
}


void PosVecDataSet::setEmpty()
{
    deepErase(coldefs_);
    data_.setNrVals( 1 );
    coldefs_ += new DataColDef( "Z" );
}


int PosVecDataSet::add( DataColDef* cd )
{
    coldefs_ += cd;
    data_.setNrVals( data_.nrVals() + 1 );
    return data_.nrVals()-2;
}


bool PosVecDataSet::insert( int idx, DataColDef* cd )
{
    if ( !coldefs_.validIdx(idx) )
	return false;
    coldefs_.insertAt( cd, idx );;
    data_.insertVal( idx );
    return true;
}


void PosVecDataSet::removeColumn( int colidx )
{
    if ( colidx > 0 && colidx < coldefs_.size() )
    {
	DataColDef* cd = coldefs_[colidx];
	coldefs_.removeSingle( colidx );
	delete cd;
	data_.removeVal( colidx );
    }
}


int PosVecDataSet::findColDef( const DataColDef& mtchcd, ColMatchPol pol ) const
{
    const bool use_name = pol == NameExact || pol == NameStart;
    const bool match_start = pol == NameStart || pol == RefStart;
    for ( int idx=0; idx<coldefs_.size(); idx++ )
    {
	const DataColDef& cd = *coldefs_[idx];
	DataColDef::MatchLevel ml = cd.compare(mtchcd,use_name);
	if ( ml == DataColDef::Exact
	  || (ml == DataColDef::Start && match_start) )
	    return idx;
    }
    return -1;
}


void PosVecDataSet::mergeColDefs( const PosVecDataSet& vds, ColMatchPol cmpol,
				 int* colidxs )
{
    const bool use_name = cmpol == NameExact || cmpol == NameStart;
    const bool match_start = cmpol == NameStart || cmpol == RefStart;
    const int orgcdsz = coldefs_.size();
    colidxs[0] = 0;
    for ( int idxvds=1; idxvds<vds.coldefs_.size(); idxvds++ )
    {
	const DataColDef& cdvds = *vds.coldefs_[idxvds];
	int matchidx = -1;
	for ( int idx=1; idx<orgcdsz; idx++ )
	{
	    DataColDef::MatchLevel ml = cdvds.compare(*coldefs_[idx],use_name);
	    if ( ml == DataColDef::Exact
	      || (ml == DataColDef::Start && match_start) )
		{ matchidx = idx; break; }
	}
	if ( matchidx >= 0 )
	    colidxs[idxvds] = matchidx;
	else
	{
	    add( new DataColDef(cdvds) );
	    colidxs[idxvds] = coldefs_.size() - 1;
	}
    }
}


void PosVecDataSet::merge( const PosVecDataSet& vds, OvwPolicy ovwpol,
			   ColMatchPol cmpol )
{
    ArrPtrMan<int> colidxs = new int [ vds.coldefs_.size() ];
    const int orgnrcds = coldefs_.size();
    mergeColDefs( vds, cmpol, colidxs );
    pars_.merge( vds.pars_ );

    if ( vds.data_.isEmpty() )
	return;

    BinIDValueSet::SPos vdspos;
    const int vdsnrvals = vds.data_.nrVals();
    BinID bid; float* vals;
    while ( vds.data_.next(vdspos) )
    {
	const float* vdsvals = vds.data_.getVals(vdspos);
	vds.data_.get( vdspos, bid );
	BinIDValueSet::SPos pos = data_.find( bid );
	data_.prev( pos );
	vals = 0;
	while ( data_.next(pos) )
	{
	    vals = data_.getVals( pos );
	    const float z = *vals;
	    if ( mIsUdf(z) || mIsEqual(*vdsvals,z,1e-6) )
		break;
	}
	if ( !pos.isValid() )
	    vals = 0;

	const bool newpos = !vals;
	if ( newpos )
	{
	    pos = data_.add( bid );
	    vals = data_.getVals( pos );
	}

	for ( int idx=0; idx<vdsnrvals; idx++ )
	{
	    int targidx = colidxs[ idx ];
	    if ( newpos || targidx >= orgnrcds // new column
	      || ovwpol == Ovw
	      || (ovwpol == OvwIfUdf && mIsUdf(vals[targidx])) )
		vals[targidx] = vdsvals[idx];
	}
    }
}


#define mErrRet(s) { errmsg = s; return strm; }


static od_istream getInpStrm( const char* fnm, BufferString& errmsg,
			    bool& tabstyle )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet("Cannot open input file")
    BufferString firstword; strm >> firstword;
    strm.setPosition( 0 );
    tabstyle = firstword != "dTect" && firstword != "dGB-GDI";
    if ( !tabstyle )
    {
	ascistream astrm( strm );
	if ( !astrm.isOfFileType(mPosVecDataSetFileType) )
	    mErrRet("Invalid input file")
    }
    return strm;
}


static const UnitOfMeasure* parseColName( const char* inp, BufferString& nm )
{
    nm = inp;
    char* ptrstart = strrchr( nm.buf(), '(' );
    const UnitOfMeasure* ret = 0;
    if ( ptrstart && ptrstart != nm.buf() && *(ptrstart-1) == ' ' )
    {
	BufferString unsymb = ptrstart + 1;
	char* ptrend = strchr( unsymb.buf(), ')' );
	if ( ptrend )
	{
	    *ptrend = '\0';
	    ret = UoMR().get( unsymb );
	    if ( ret )
		*(ptrstart-1) = '\0';
	}
    }
    return ret;
}


bool PosVecDataSet::getColNames( const char* fnm, BufferStringSet& bss,
				 BufferString& errmsg, bool refs )
{
    bool tabstyle = false;
    od_istream strm = getInpStrm( fnm, errmsg, tabstyle );
    if ( !strm.isOK() )
	return false;

    if ( tabstyle )
    {
	BufferString buf; strm.getLine( buf );
	SeparString ss( buf, '\t' );
	const int nrcols = ss.size();
	for ( int idx=2; idx<nrcols; idx++ )
	{
	    BufferString nm;
	    parseColName( ss[idx], nm );
	    bss.add( nm );
	}
    }
    else
    {
	ascistream astrm( strm, false );
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.type() == ascistream::Keyword )
	    {
		DataColDef cd( "" );
		cd.getFrom( astrm.keyWord() );
		bss.add( refs ? cd.ref_ : cd.name_ );
	    }
	}
    }

    return true;
}


bool PosVecDataSet::getIOPar( const char* fnm, IOPar& iop, BufferString& emsg )
{
    bool tabstyle = false;
    od_istream strm = getInpStrm( fnm, emsg, tabstyle );
    if ( !strm.isOK() )
	return false;
    iop.setEmpty();
    if ( tabstyle )
	return true;

    ascistream astrm( strm, false );
    while ( !atEndOfSection(astrm.next()) )
	/* read away column defs */;

    iop.getFrom( astrm );
    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }


bool PosVecDataSet::getFrom( const char* fnm, BufferString& errmsg )
{
    bool tabstyle = false;
    od_istream strm = getInpStrm( fnm, errmsg, tabstyle );
    if ( !strm.isOK() )
	return false;

    setEmpty(); pars_.setEmpty(); setName( "" );
    int valstartcol = 2;
    if ( tabstyle )
    {
	BufferString buf; strm.getLine( buf );
	SeparString ss( buf, '\t' );
	const int nrcols = ss.size();
	const BufferString xcolnm( ss[2] ); const BufferString ycolnm( ss[3] );
	if ( xcolnm=="X-coord" && ycolnm=="Y-corrd" )
	    valstartcol = 4;
	for ( int idx=valstartcol; idx<nrcols; idx++ )
	{
	    BufferString nm;
	    DataColDef* cd = new DataColDef( "" );
	    cd->unit_ = parseColName( ss[idx], nm );
	    cd->name_ = nm;
	    add( cd );
	}
    }
    else
    {
	ascistream astrm( strm, false );
	while ( !atEndOfSection(astrm.next()) )
	{
	    if ( astrm.type() == ascistream::Keyword )
	    {
		DataColDef* cd = new DataColDef( "" );
		cd->getFrom( astrm.keyWord() );
		if ( cd->name_ != "Z" )
		    add( cd );
		else
		{
		    colDef(0) = *cd;
		    delete cd;
		}
	    }
	    else if ( astrm.hasKeyword( sKey::Name() ) )
		setName( astrm.value() );
	}
	if ( !atEndOfSection(astrm.next()) )
	    pars().getFrom( astrm );
    }

    const int nrvals = nrCols();
    if ( nrvals < 1 )
    {
	add( new DataColDef("Z") );
	data().setNrVals(1);
	return true;
    }

    data().setNrVals( nrvals );
    BinID bid; float* vals = new float [ nrvals ];
    while ( strm.isOK() )
    {
	bid.inl() = bid.crl() = 0;
	strm >> bid.inl() >> bid.crl();
	if ( !bid.inl() && !bid.crl() )
	    { strm.skipUntil( '\n' ); continue; }

	if ( valstartcol == 4 ) // also has X, Y coordinates.
	{
	    float x, y;
	    strm >> x >> y;
	}

	for ( int idx=0; idx<nrvals; idx++ )
	    strm >> vals[idx];
	if ( !strm.isBad() )
	    data().add( bid, vals );
	if ( !strm.isOK() )
	    break;
    }
    delete [] vals;

    return true;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }

bool PosVecDataSet::putTo( const char* fnm, BufferString& errmsg,
			   bool tabstyle ) const
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	mErrRet("Cannot open output file")

    BufferString str;
    if ( tabstyle )
    {
	strm << "\"In-line\"\t\"X-line\"\t\"X-coord\"\t\"Y-Coord\"";
	for ( int idx=0; idx<nrCols(); idx++ )
	{
	    const DataColDef& cd = colDef(idx);
	    strm << "\t\"" << cd.name_;
	    if ( cd.unit_ )
		strm << " (" << cd.unit_->symbol() << ")";
	    strm << '"';
	}
	strm << '\n';
    }
    else
    {
	ascostream astrm( strm );
	if ( !astrm.putHeader(mPosVecDataSetFileType) )
	    mErrRet("Cannot write header to output file")

	if ( *name() )
	    astrm.put( sKey::Name(), name() );
	astrm.put( "--\n-- Column definitions:" );
	for ( int idx=0; idx<nrCols(); idx++ )
	{
	    colDef(idx).putTo( str );
	    astrm.put( str );
	}
	astrm.newParagraph();
	pars().putTo(astrm);
	// iopar does a newParagraph()
    }

    const int nrvals = data().nrVals();
    BinIDValueSet::SPos pos;
    float* vals = new float [nrvals];
    BinID bid;
    while ( data().next(pos) )
    {
	data().get( pos, bid, vals );
	strm << bid.inl() << '\t' << bid.crl();
	if ( tabstyle )
	{
	    Coord crd = SI().transform( bid );
	    if ( nrvals>=2 && nrCols()>=2 && colDef(0).name_=="X Offset"
					  && colDef(1).name_=="Y Offset" )
	    {
		crd.x += vals[0];
		crd.y += vals[1];
	    }

	    strm << '\t' << toString(crd.x) << '\t' << toString(crd.y);
	}

	for ( int idx=0; idx<nrvals; idx++ )
	{
	    str = vals[idx];
	    strm << '\t' << str;
	}
	strm << '\n';
	if ( !strm.isOK() )
	    mErrRet("Error during write of data")
    }
    delete [] vals;

    return true;
}
