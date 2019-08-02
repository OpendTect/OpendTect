/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          June 2006

________________________________________________________________________

-*/

#include "uifingerprintcalcobj.h"
#include "attribdesc.h"
#include "attribsel.h"
#include "attribparam.h"
#include "attribdescset.h"
#include "attribstorprovider.h"
#include "attribprocessor.h"
#include "attribengman.h"
#include "uimsg.h"
#include "ioobj.h"
#include "binnedvalueset.h"
#include "picksetmanager.h"
#include "seis2ddata.h"
#include "posinfo2d.h"
#include "posinfo2dsurv.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "uitaskrunner.h"
#include "ptrman.h"
#include "statrand.h"

using namespace Attrib;

static const int cNrRandPicks = 100;


#define mErrRet(msg) \
{\
    uiString em = toUiString("%1 \n %2").arg(emTxt()).arg(msg);	\
    gUiMsg().error( em );\
    return;\
}\


static void create3DRandPicks( BinnedValueSet* rangesset )
{
    BinID bid;
    const auto zrg = SI().zRange( OD::UsrWork );
    const auto zwidth = zrg.width();
    for ( int ipt=0; ipt<cNrRandPicks; ipt++ )
    {
	StepInterval<int> irg = SI().inlRange();
	StepInterval<int> crg = SI().crlRange();
	bid.inl() = mNINT32(irg.start + Stats::randGen().get() * irg.nrSteps());
	bid.crl() = mNINT32(crg.start + Stats::randGen().get() * crg.nrSteps());
	SI().snap( bid );
	const auto z = zrg.start + Stats::randGen().get() * zwidth;
	rangesset->add( bid, z );
    }
}


calcFingParsObject::calcFingParsObject( uiParent* p )
    : parent_( p )
{
    posset_.setNullAllowed( true );
    while ( posset_.size() < 2 )
	posset_ += 0;
}


calcFingParsObject::~calcFingParsObject()
{
    deepErase(posset_);
    reflist_->erase();
}


void calcFingParsObject::create2DRandPicks( const DBKey& dsetid,
					    BinnedValueSet* rangesset )
{
    PtrMan<IOObj> ioobj = dsetid.getIOObj();
    if ( !ioobj )
	mErrRet( tr("2D Dataset ID is not OK") );
    PtrMan<Seis2DDataSet> dset = new Seis2DDataSet( *ioobj );
    const int nrlines = dset->nrLines();
    if ( !nrlines )
	mErrRet( uiStrings::phrInput(tr("Dataset is empty")) );

    while ( rangesset->totalSize() < cNrRandPicks )
    {
	const int lineidx = Stats::randGen().getIndex( nrlines );
	const Pos::GeomID geomid = dset->geomID( lineidx );
	const auto& geom2d = SurvGeom::get2D( geomid );
	if ( geom2d.isEmpty() )
	    break;

	const PosInfo::Line2DData& geometry = geom2d.data();
	const int nrcoords = geometry.positions().size();
	const int crdidx = Stats::randGen().getIndex( nrcoords );
	const Coord& pos = geometry.positions()[crdidx].coord_;

	const BinID bid = SI().transform( pos );
	const float zpos = (float) (geometry.zRange().start +
			    Stats::randGen().get()*geometry.zRange().width());
	rangesset->add( bid, zpos );
    }
}


void calcFingParsObject::setValRgSet( BinnedValueSet* positions, bool isvalset )
{
    delete( posset_.replace( isvalset ? 0 : 1, positions ) );
}


BinnedValueSet* calcFingParsObject::createRangesBinIDSet() const
{
    BinnedValueSet* retset = 0;
    if ( rgreftype_ == 1 )
    {
	const DBKey setid( getRgRefPick() );
	uiRetVal uirv;
	ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( setid, uirv );
	if ( !ps )
	    { gUiMsg(parent_).error( uirv ); return 0; }

	retset = new BinnedValueSet( 1, false );
	Pick::SetIter psiter( *ps );
	while ( psiter.next() )
	{
	    const Pick::Location& pl = psiter.get();
	    retset->add( pl.binID(), pl.z() );
	}
    }
    else if ( rgreftype_ == 2 )
    {
	retset = new BinnedValueSet( 2, true );
	if ( attrset_->is2D() )
	{
	    DBKey datasetid;
	    findDataSetID( datasetid );
	    create2DRandPicks( datasetid, retset );
	}
	else
	    create3DRandPicks( retset );
    }
    else
	retset = new BinnedValueSet( 1, false );

    return retset;
}


void calcFingParsObject::findDataSetID( DBKey& linesetid ) const
{
    BufferString firstinp = reflist_->get(0);
    for ( int idxdesc=0; idxdesc<attrset_->size(); idxdesc++ )
    {
	if ( firstinp == attrset_->desc(idxdesc)->userRef() )
	{
	    Desc* dsc = attrset_->desc(idxdesc);
	    const char* key = StorageProvider::keyStr();
	    if ( dsc->isStored() )
		linesetid = DBKey( dsc->getValParam(key)->getStringValue() );
	    else
	    {
		bool foundstored = false;

		while ( !foundstored )
		{
		    Desc* inpdsc = dsc->getInput(0);
		    if ( inpdsc->isStored() )
		    {
			linesetid = DBKey(
				inpdsc->getValParam(key)->getStringValue() );
			foundstored = true;
		    }
		    else
			dsc = dsc->getInput(0);
		}
	    }
	}
    }
}


bool calcFingParsObject::computeValsAndRanges()
{
    uiRetVal uirv;
    PtrMan<EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createLocationOutput( uirv, posset_ );
    if ( !proc )
	{ gUiMsg(parent_).error( uirv ); return false; }

    proc->setName( "Compute reference values" );
    uiTaskRunner taskrunner( parent_ );
    if ( !TaskRunner::execute( &taskrunner, *proc ) )
	return false;

    extractAndSaveValsAndRanges();
    return true;
}


void calcFingParsObject::extractAndSaveValsAndRanges()
{
    const int nrattribs = reflist_->size();
    BinnedValueSet* valueset = posset_[0];
    BinnedValueSet* rangeset = posset_[1];
    TypeSet<float> vals( nrattribs, mUdf(float) );
    TypeSet< Interval<float> > rgs( nrattribs,
				    Interval<float>(mUdf(float),mUdf(float)) );

    if ( valueset->totalSize() == 1 )
    {
	const float* tmpvals = valueset->getVals( valueset->getPos(0) );
	for ( int idx=0; idx<nrattribs; idx++ )
	    vals[idx] = tmpvals[idx+1];
    }
    else if ( valueset->totalSize() > 1 )
    {
	ObjectSet< Stats::RunCalc<float> > statsset;

	fillInStats( valueset, statsset, statstype_ );

	for ( int idx=0; idx<nrattribs; idx++ )
	    vals[idx] = (float) statsset[idx]->getValue(statstype_);

	deepErase( statsset );
    }

    if ( rangeset->totalSize() >= 1 )
    {
	ObjectSet< Stats::RunCalc<float> > stats;
	fillInStats( rangeset, stats, Stats::Min );

	for ( int idx=0; idx<nrattribs; idx++ )
	    rgs[idx] = Interval<float>( stats[idx]->min(), stats[idx]->max());

	deepErase( stats );
    }

    saveValsAndRanges( vals, rgs );
}


EngineMan* calcFingParsObject::createEngineMan()
{
    EngineMan* aem = new EngineMan;

    SelSpecList attribspecs;
    for ( int idx=0; idx<reflist_->size(); idx++ )
    {
	for ( int idxdesc=0; idxdesc<attrset_->size(); idxdesc++ )
	{
	    if ( reflist_->get(idx) == attrset_->desc(idxdesc)->userRef() )
	    {
		SelSpec sp( 0, attrset_->desc(idxdesc)->id() );
		attribspecs += sp;
	    }
	}
    }

    aem->setAttribSet( attrset_ );
    aem->setAttribSpecs( attribspecs );

    return aem;
}


void calcFingParsObject::fillInStats( BinnedValueSet* bidvalset,
			ObjectSet< Stats::RunCalc<float> >& statsset,
			Stats::Type styp ) const
{
    const int nrattribs = reflist_->size();
    for ( int idx=0; idx<nrattribs; idx++ )
	statsset += new Stats::RunCalc<float>(
			Stats::CalcSetup().require(styp) );

    BinnedValueSet::SPos pos;
    while ( bidvalset->next(pos) )
    {
	const float* values = bidvalset->getVals( pos );
	for ( int idx=0; idx<nrattribs; idx++ )
	    *(statsset[idx]) += values[idx+1];
    }
}


void calcFingParsObject::saveValsAndRanges( const TypeSet<float>& vals,
					const TypeSet< Interval<float> >& rgs )
{
    int index = 0;
    values_.erase(); ranges_.erase();
    for ( int idx=0; idx<reflist_->size(); idx++ )
    {
	BufferString inp = reflist_->get(idx);
	for ( int idxdesc=0; idxdesc<attrset_->size(); idxdesc++ )
	{
	    if ( inp == attrset_->desc(idxdesc)->userRef() )
	    {
		if ( vals.size() > index )
		    values_ += vals[index];
		if ( rgs.size() > index )
		    ranges_ += rgs[index];
		index++;
	    }
	}
    }
}
