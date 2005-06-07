/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: posvecdataset.cc,v 1.5 2005-06-07 16:22:54 cvsbert Exp $";

#include "posvecdataset.h"
#include "posvecdatasetfact.h"
#include "datacoldef.h"
#include "survinfo.h"
#include "iopar.h"
#include "ioobj.h"


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


PosVecDataSet::PosVecDataSet( const char* nm )
	: data_(1,true)
	, pars_(*new IOPar)
	, name_(nm)
{
    empty();
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
    if ( &vds != this )
    {
	name_ = vds.name();
	copyStructureFrom( vds );
	merge( vds );
	pars_ = vds.pars_;
    }
    return *this;
}


void PosVecDataSet::copyStructureFrom( const PosVecDataSet& vds )
{
    empty();
    data_.copyStructureFrom( vds.data_ );
    deepErase( coldefs_ );
    deepCopy( coldefs_, vds.coldefs_ );
    pars_ = vds.pars_;
}


void PosVecDataSet::empty()
{
    deepErase(coldefs_);
    data_.setNrVals( 1 );
    coldefs_ += new DataColDef( "Z" );
}


void PosVecDataSet::add( DataColDef* cd )
{
    coldefs_ += cd;
    data_.setNrVals( data_.nrVals() + 1 );
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
    int colidxs[ vds.coldefs_.size() ];
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
	vals = 0;
	while ( pos.valid() )
	{
	    vals = data_.getVals( pos );
	    const float z = *vals;
	    if ( mIsUndefined(z) || mIsEqual(*vdsvals,z,1e-6) )
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
	      || (ovwpol == OvwIfUdf && mIsUndefined(vals[targidx])) )
		vals[targidx] = vdsvals[idx];
	}
    }
}


bool PosVecDataSet::getFrom( const char* fnm, BufferString& errmsg )
{
    errmsg = "TODO: implement PosVecDataSet::getFrom"; //TODO
    return false;
}


bool PosVecDataSet::putTo( const char* fnm, BufferString& errmsg ) const
{
    errmsg = "TODO: implement PosVecDataSet::putTo"; //TODO
    return false;
}


const IOObjContext& PosVecDataSetTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = false;
	ctxt->stdseltype = IOObjContext::Feat;
    }

    return *ctxt;
}


int PosVecDataSetTranslatorGroup::selector( const char* key )
{
    return defaultSelector( theInst().userName(), key );
}


bool odPosVecDataSetTranslator::read( const IOObj& ioobj, PosVecDataSet& vds )
{
    return vds.getFrom( ioobj.fullUserExpr(true), errmsg_ );
}


bool odPosVecDataSetTranslator::write( const IOObj& ioobj,
					const PosVecDataSet& vds )
{
    return vds.putTo( ioobj.fullUserExpr(false), errmsg_ );
}
