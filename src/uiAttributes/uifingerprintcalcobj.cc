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
#include "ioman.h"
#include "binidvalset.h"
#include "picksettr.h"
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
    uiMSG().error( em );\
    return;\
}\


void calcFingParsObject::create2DRandPicks( const MultiID& dsetid,
					    BinIDValueSet& rangesset ) const
{
    PtrMan<IOObj> ioobj = IOM().get( dsetid );
    if ( !ioobj ) mErrRet( tr("2D Dataset ID is not OK") );
    PtrMan<Seis2DDataSet> dset = new Seis2DDataSet( *ioobj );
    const int nrlines = dset->nrLines();
    if ( !nrlines )
	mErrRet( uiStrings::phrInput(tr("Dataset is empty")) );

    while ( rangesset.totalSize() < cNrRandPicks )
    {
	const int lineidx = gen_.getIndex( nrlines );
	const Pos::GeomID geomid = dset->geomID( lineidx );
	mDynamicCastGet( const Survey::Geometry2D*, geom2d,
			 Survey::GM().getGeometry(geomid) );
	if ( !geom2d ) break;

	const PosInfo::Line2DData& geometry = geom2d->data();
	const int nrcoords = geometry.positions().size();
	const int crdidx = gen_.getIndex( nrcoords );
	const Coord& pos = geometry.positions()[crdidx].coord_;

	const BinID bid = SI().transform( pos );
	const float zpos = (float) (geometry.zRange().start +
			    gen_.get()*geometry.zRange().width());
	rangesset.add( bid, zpos );
    }
}

static void create3DRandPicks( Stats::RandGen& gen, BinIDValueSet& rangesset )
{
    BinID bid;
    for ( int ipt=0; ipt<cNrRandPicks; ipt++ )
    {
	StepInterval<int> irg = SI().inlRange( true );
	StepInterval<int> crg = SI().crlRange( true );
	bid.inl() = gen.getInt( irg.start, irg.stop );
	bid.crl() = gen.getInt( crg.start, crg.stop );
	SI().snap( bid );
	const float z = (float) (SI().zRange(true).start
		      + gen.get() * SI().zRange(true).width());
	rangesset.add( bid, z );
    }
}


calcFingParsObject::calcFingParsObject( uiParent* p )
    : parent_( p )
    , gen_(*new Stats::RandGen())
{
    posset_.allowNull(true);
    while ( posset_.size() < 2 )
	posset_ += nullptr;
}


calcFingParsObject::~calcFingParsObject()
{
    deepErase( posset_ );
    reflist_->erase();
    delete &gen_;
}


void calcFingParsObject::setValRgSet( BinIDValueSet* positions, bool isvalset )
{
    delete( posset_.replace( isvalset ? 0 : 1, positions ) );
}


BinIDValueSet* calcFingParsObject::createRangesBinIDSet() const
{
    if ( rgreftype_ == 1 )
    {
	ObjectSet<BinIDValueSet> values;
	BufferStringSet ioobjids;
	ioobjids.add( getRgRefPick().toString() );
	PickSetTranslator::createBinIDValueSets( ioobjids, values );
	BinIDValueSet* rgset = new BinIDValueSet( *(values[0]) );
	deepErase( values );
	return rgset;
    }
    else if ( rgreftype_ == 2 )
    {
	auto* rangesset = new BinIDValueSet( 2, true );
	if ( attrset_->is2D() )
	{
	    MultiID datasetid;
	    findDataSetID( datasetid );
	    create2DRandPicks( datasetid, *rangesset );
	}
	else
	    create3DRandPicks( gen_, *rangesset );

	return rangesset;
    }

    return new BinIDValueSet( 1, false );
}


void calcFingParsObject::findDataSetID( MultiID& linesetid ) const
{
    BufferString firstinp = reflist_->get(0);
    for ( int idxdesc=0; idxdesc<attrset_->size(); idxdesc++ )
    {
	if ( firstinp == attrset_->desc(idxdesc)->userRef() )
	{
	    Desc* dsc = attrset_->desc(idxdesc);
	    const char* key = StorageProvider::keyStr();
	    if ( dsc->isStored() )
		linesetid = MultiID( dsc->getValParam(key)->getStringValue() );
	    else
	    {
		bool foundstored = false;

		while ( !foundstored )
		{
		    Desc* inpdsc = dsc->getInput(0);
		    if ( inpdsc->isStored() )
		    {
			linesetid = MultiID( inpdsc->getValParam(key)
							 ->getStringValue() );
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
    uiString errmsg;
    PtrMan<EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createLocationOutput( errmsg, posset_ );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return false;
    }

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
    BinIDValueSet* valueset = posset_[0];
    BinIDValueSet* rangeset = posset_[1];
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

	Stats::Type styp = (Stats::Type)statstype_;
	fillInStats( valueset, statsset, styp );

	for ( int idx=0; idx<nrattribs; idx++ )
	    vals[idx] = (float) statsset[idx]->getValue(styp);

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

    TypeSet<SelSpec> attribspecs;
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


void calcFingParsObject::fillInStats( BinIDValueSet* bidvalset,
			ObjectSet< Stats::RunCalc<float> >& statsset,
			Stats::Type styp ) const
{
    const int nrattribs = reflist_->size();
    for ( int idx=0; idx<nrattribs; idx++ )
	statsset += new Stats::RunCalc<float>(
			Stats::CalcSetup().require(styp) );

    BinIDValueSet::SPos pos;
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
