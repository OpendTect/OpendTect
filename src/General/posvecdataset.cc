/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "posvecdataset.h"

#include "ascstream.h"
#include "datacoldef.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include <iostream>


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
    BinIDValueSet::Pos pos;
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


void PosVecDataSet::removeColumn( int colidx )
{
    if ( colidx > 0 && colidx < coldefs_.size() )
    {
	DataColDef* cd = coldefs_[colidx];
	coldefs_.remove( colidx );
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

    BinIDValueSet::Pos vdspos;
    const int vdsnrvals = vds.data_.nrVals();
    BinID bid; float* vals;
    while ( vds.data_.next(vdspos) )
    {
	const float* vdsvals = vds.data_.getVals(vdspos);
	vds.data_.get( vdspos, bid );
	BinIDValueSet::Pos pos = data_.findFirst( bid );
	data_.prev( pos );
	vals = 0;
	while ( data_.next(pos) )
	{
	    vals = data_.getVals( pos );
	    const float z = *vals;
	    if ( mIsUdf(z) || mIsEqual(*vdsvals,z,1e-6) )
		break;
	}
	if ( !pos.valid() )
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


#define mErrRet(s) { errmsg = s; sd.close(); return sd; }


static StreamData getInpSD( const char* fnm, BufferString& errmsg,
			    bool& tabstyle )
{
    StreamData sd = StreamProvider(fnm).makeIStream();
    if ( !sd.usable() )
	mErrRet("Cannot open input file")
    std::string buf; *sd.istrm >> buf;
    sd.istrm->seekg( 0, std::ios::beg );
    tabstyle = buf != "dTect" && buf != "dGB-GDI"; // For legacy data
    if ( !tabstyle )
    {
	ascistream strm( *sd.istrm );
	if ( !strm.isOfFileType(mPosVecDataSetFileType) )
	    mErrRet("Invalid input file")
    }
    return sd;
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
    StreamData sd = getInpSD( fnm, errmsg, tabstyle );
    if ( !sd.usable() )
	return false;

    if ( tabstyle )
    {
	char buf[65536]; sd.istrm->getline( buf, 65536 );
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
	ascistream strm( *sd.istrm, false );
	while ( !atEndOfSection(strm.next()) )
	{
	    if ( strm.type() == ascistream::Keyword )
	    {
		DataColDef cd( "" );
		cd.getFrom( strm.keyWord() );
		bss.add( refs ? cd.ref_ : cd.name_ );
	    }
	}
    }

    sd.close();
    return true;
}


bool PosVecDataSet::getIOPar( const char* fnm, IOPar& iop, BufferString& emsg )
{
    bool tabstyle = false;
    StreamData sd = getInpSD( fnm, emsg, tabstyle );
    if ( !sd.usable() )
	return false;
    iop.setEmpty();
    if ( tabstyle )
	return true;

    ascistream strm( *sd.istrm, false );
    while ( !atEndOfSection(strm.next()) )
	/* read away column defs */;

    iop.getFrom( strm );
    sd.close();
    return true;
}

#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }


bool PosVecDataSet::getFrom( const char* fnm, BufferString& errmsg )
{
    bool tabstyle = false;
    StreamData sd = getInpSD( fnm, errmsg, tabstyle );
    if ( !sd.usable() )
	return false;

    setEmpty(); pars_.setEmpty(); setName( "" );
    if ( tabstyle )
    {
	char buf[65536]; sd.istrm->getline( buf, 65536 );
	SeparString ss( buf, '\t' );
	const int nrcols = ss.size();
	for ( int idx=2; idx<nrcols; idx++ )
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
	ascistream strm( *sd.istrm, false );
	while ( !atEndOfSection(strm.next()) )
	{
	    if ( strm.type() == ascistream::Keyword )
	    {
		DataColDef* cd = new DataColDef( "" );
		cd->getFrom( strm.keyWord() );
		if ( cd->name_ != "Z" )
		    add( cd );
		else
		{
		    colDef(0) = *cd;
		    delete cd;
		}
	    }
	    else if ( strm.hasKeyword( sKey::Name() ) )
		setName( strm.value() );
	}
	if ( !atEndOfSection(strm.next()) )
	    pars().getFrom( strm );
    }

    const int nrvals = nrCols();
    if ( nrvals < 1 )
    {
	add( new DataColDef("Z") );
	data().setNrVals(1);
	sd.close(); return true;
    }

    data().setNrVals( nrvals );
    BinID bid; float* vals = new float [ nrvals ];
    while ( *sd.istrm )
    {
	bid.inl = bid.crl = 0;
	*sd.istrm >> bid.inl >> bid.crl;
	if ( !bid.inl && !bid.crl )
	    { sd.istrm->ignore( 10000, '\n' ); continue; }

	for ( int idx=0; idx<nrvals; idx++ )
	    *sd.istrm >> vals[idx];
	if ( !sd.istrm->good() )
	    break;

	data().add( bid, vals );
    }
    delete [] vals;

    sd.close();
    return true;
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; sd.close(); return false; }

bool PosVecDataSet::putTo( const char* fnm, BufferString& errmsg,
			   bool tabstyle ) const
{
    StreamData sd = StreamProvider(fnm).makeOStream();
    if ( !sd.usable() )
	mErrRet("Cannot open output file")

    BufferString str;
    if ( tabstyle )
    {
	*sd.ostrm << "\"In-line\"\t\"X-line\"";
	for ( int idx=0; idx<nrCols(); idx++ )
	{
	    const DataColDef& cd = colDef(idx);
	    *sd.ostrm << "\t\"" << cd.name_;
	    if ( cd.unit_ )
		*sd.ostrm << " (" << cd.unit_->symbol() << ")";
	    *sd.ostrm << '"';
	}
	*sd.ostrm << '\n';
    }
    else
    {
	ascostream strm( *sd.ostrm );
	if ( !strm.putHeader(mPosVecDataSetFileType) )
	    mErrRet("Cannot write header to output file")

	if ( *name() )
	    strm.put( sKey::Name(), name() );
	strm.put( "--\n-- Column definitions:" );
	for ( int idx=0; idx<nrCols(); idx++ )
	{
	    colDef(idx).putTo( str );
	    strm.put( str );
	}
	strm.newParagraph();
	pars().putTo(strm);
	// iopar does a newParagraph()
    }

    const int nrvals = data().nrVals();
    BinIDValueSet::Pos pos;
    float* vals = new float [nrvals];
    BinID bid;
    while ( data().next(pos) )
    {
	data().get( pos, bid, vals );
	*sd.ostrm << bid.inl << '\t' << bid.crl;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    str = vals[idx];
	    *sd.ostrm << '\t' << str;
	}
	*sd.ostrm << '\n';
	if ( !sd.ostrm->good() )
	    mErrRet("Error during write of data")
    }
    delete [] vals;

    sd.close();
    return true;
}
