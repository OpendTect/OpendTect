/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2000
________________________________________________________________________

-*/

#include "flatview.h"
#include "flatposdata.h"
#include "arrayndimpl.h"
#include "settings.h"
#include "zaxistransform.h"
#include "zdomain.h"
#include "randcolor.h"
#include "survinfo.h"
#include "coltabseqmgr.h"
#include "datadistributionextracter.h"
#include "posidxsubsel.h"
#include "keystrs.h"

namespace FlatView
{

const char* Annotation::sKeyAxes()	   { return "Axes"; }
const char* Annotation::sKeyX1Sampl()	   { return "Axis 1 Sampling"; }
const char* Annotation::sKeyX2Sampl()	   { return "Axis 2 Sampling"; }
const char* Annotation::sKeyShwAnnot()	   { return "Show annotation"; }
const char* Annotation::sKeyShwGridLines() { return "Show grid lines"; }
const char* Annotation::sKeyIsRev()	   { return "Reversed"; }
const char* Annotation::sKeyShwAux()	   { return "Show aux data"; }

const char* DataDispPars::sKeyVD()	{ return "VD"; }
const char* DataDispPars::sKeyWVA()	{ return "WVA"; }
const char* DataDispPars::sKeyShow()	{ return "Show"; }
const char* DataDispPars::sKeyDispRg()	{ return sKey::Range(); }
const char* DataDispPars::sKeyColTab()	{ return sKey::ColTab(); }
const char* DataDispPars::sKeyFlipSequence() { return "Flip Sequence"; }
const char* DataDispPars::sKeyCyclicSequence() { return "Cyclic Sequence"; }
const char* DataDispPars::sKeyLinearInter()  { return "Linear Interpolation"; }
const char* DataDispPars::sKeyBlocky()	 { return "Blocky"; }
const char* DataDispPars::sKeyAutoScale(){ return "Auto scale"; }
const char* DataDispPars::sKeyClipPerc() { return "Percentage Clip"; }
const char* DataDispPars::sKeyWiggCol()  { return "Wiggle color"; }
const char* DataDispPars::sKeyRefLineCol()	{ return "Ref line color"; }
const char* DataDispPars::sKeyLowFillCol()	{ return "Low fill color"; }
const char* DataDispPars::sKeyHighFillCol()	{ return "High fill color"; }
const char* DataDispPars::sKeyOverlap()  { return "Overlap"; }
const char* DataDispPars::sKeyRefLineValue() { return "Ref Line value"; }

}


FlatPosData& FlatPosData::operator =( const FlatPosData& fpd )
{
    if ( this == &fpd ) return *this;

    x1rg_ = fpd.x1rg_; x2rg_ = fpd.x2rg_;
    x1offs_ = fpd.x1offs_;
    delete [] x1pos_; x1pos_ = 0;
    if ( fpd.x1pos_ )
    {
	const int sz = fpd.nrPts(true);
	x1pos_ = new float[ sz ];
	OD::memCopy( x1pos_, fpd.x1pos_, sz * sizeof( float ) );
    }

    return *this;
}


void FlatPosData::setRange( bool isx1, const StepInterval<double>& newrg )
{
    rg( isx1 ) = newrg;
    if ( isx1 )
	{ delete [] x1pos_; x1pos_ = 0; x1offs_ = 0; }
}


void FlatPosData::setX1Pos( float* pos, int sz, double offs )
{
    delete [] x1pos_; x1pos_ = 0;
    x1offs_ = offs;
    if ( !pos || sz < 1 ) return;

    x1pos_ = pos;
    x1rg_.start = pos[0] + offs; x1rg_.stop = pos[sz-1] + offs;
    x1rg_.step = sz > 1 ? (x1rg_.stop - x1rg_.start) / (sz - 1) : 1;
}


void FlatPosData::set( const Pos::IdxSubSel2D& subsel )
{
    StepInterval<double> drg;
    assign( drg, subsel.inlRange() );
    setRange( true, drg );
    assign( drg, subsel.crlRange() );
    setRange( false, drg );
}


IndexInfo FlatPosData::indexInfo( bool isx1, double x ) const
{
    if ( !isx1 )
	return IndexInfo( x2rg_, x );
    const int x1sz = nrPts(true);
    if ( x1sz < 1 || !x1pos_ )
	return IndexInfo( x1rg_, x );

    return IndexInfo( x1pos_, x1sz, (float)(x-x1offs_) );
}


double FlatPosData::position( bool isx1, int idx ) const
{
    return !isx1 || !x1pos_ || idx >= nrPts(true) ? range(isx1).atIndex(idx)
						  : x1pos_[idx] + x1offs_;
}


void FlatPosData::getPositions( bool isx1, TypeSet<float>& res ) const
{
    res.erase();

    const int nrtimes = nrPts( isx1 );
    res.setCapacity( nrtimes, false );
    for ( int idx=0; idx<nrtimes; idx++ )
	res += ( float )position( isx1, idx );
}


float* FlatPosData::getPositions( bool isx1 ) const
{
    const int sz = nrPts( isx1 );
    if ( sz < 1 ) return 0;

    float* ret = new float [sz];
    if ( isx1 && x1pos_ )
	OD::memCopy( ret, x1pos_, sz * sizeof(float) );
    else
    {
	const StepInterval<double>& xrg = range( isx1 );
	for ( int idx=0; idx<sz; idx++ )
	    ret[idx] = ( float )xrg.atIndex( idx );
    }
    return ret;
}


FlatView::DataDispPars::Common::Common()
    : show_(true)
    , blocky_(false)
    , allowuserchange_(true)
    , allowuserchangedata_(true)
    , mapper_(new ColTab::Mapper)
{
}


FlatView::DataDispPars::Common::Common( const Common& oth )
    : show_(oth.show_)
    , blocky_(oth.blocky_)
    , allowuserchange_(oth.allowuserchange_)
    , allowuserchangedata_(oth.allowuserchangedata_)
    , mapper_(new ColTab::Mapper)
{
    *mapper_ = *oth.mapper_;
}


FlatView::DataDispPars::Common& FlatView::DataDispPars::Common::operator =(
					const Common& oth )
{
    if ( this != &oth )
    {
	show_ = oth.show_;
	blocky_ = oth.blocky_;
	allowuserchange_ = oth.allowuserchange_;
	allowuserchangedata_ = oth.allowuserchangedata_;
	*mapper_ = *oth.mapper_;
    }
    return *this;
}


FlatView::DataDispPars::VD::VD()
    : lininterp_(false)
{
    mAttachCB( ColTab::SeqMGR().nameChange, VD::tabNmChg );
}


void FlatView::DataDispPars::VD::tabNmChg( CallBacker* cb )
{
    if ( colseqname_.isEmpty() )
	return;

    mGetMonitoredChgDataWithAux( cb, cd, NamedMonitorable::NameChgData, ncd );
    if ( ncd && colseqname_ == ncd->oldnm_ )
	colseqname_ = ncd->newnm_;
}



FlatView::Annotation::AxisData::AxisData()
    : reversed_(false)
    , sampling_( mUdf(float), mUdf(float) )
    , showannot_( false )
    , showgridlines_( false )
    , annotinint_( false )
    , showauxannot_(true)
    , factor_( 1 )
    , auxlinestyle_( OD::LineStyle(OD::LineStyle(OD::LineStyle::Dot)) )
    , auxhllinestyle_( OD::LineStyle(OD::LineStyle(OD::LineStyle::Dot,2,
					   getRandStdDrawColor())) )
{}


void FlatView::Annotation::AxisData::showAll( bool yn )
{ showannot_ = showgridlines_ = yn; }



int FlatView::Annotation::AxisData::auxPosIdx( float atpos, float eps ) const
{
    for ( int auxidx=0; auxidx<auxannot_.size(); auxidx++ )
    {
	const OD::PlotAnnotation& annot = auxannot_[auxidx];
	if ( mIsEqual(annot.pos_,atpos,eps) )
	    return auxidx;
    }

    return -1;
}


FlatView::Annotation::Annotation( bool drkbg )
    : color_(drkbg ? Color::White() : Color::Black())
    , showaux_(true)
    , editable_(false)
    , showscalebar_(false)
    , allowuserchange_(true)
    , allowuserchangereversedaxis_(true)
{
    x1_.name_ = toUiString("X1");
    x2_.name_ = toUiString("X2");
}


FlatView::Annotation::~Annotation()
{
}


#define mIOPDoAxes(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyAxes(),keynm), memb )
#define mIOPDoAxes2(fn,keynm,memb1,memb2) \
    iop.fn( IOPar::compKey(sKeyAxes(),keynm), memb1, memb2 )


void FlatView::Annotation::fillPar( IOPar& iop ) const
{
    mIOPDoAxes( set, sKey::Color(), color_ );
    mIOPDoAxes( set, sKeyX1Sampl(), x1_.sampling_ );
    mIOPDoAxes( set, sKeyX2Sampl(), x2_.sampling_ );
    mIOPDoAxes2( setYN, sKeyShwAnnot(), x1_.showannot_, x2_.showannot_ );
    mIOPDoAxes2(setYN,sKeyShwGridLines(),x1_.showgridlines_,x2_.showgridlines_);
    mIOPDoAxes2( setYN, sKeyIsRev(), x1_.reversed_, x2_.reversed_ );
    iop.setYN( sKeyShwAux(), showaux_ );
}


void FlatView::Annotation::usePar( const IOPar& iop )
{
    mIOPDoAxes( get, sKey::Color(), color_ );
    mIOPDoAxes( get, sKeyX1Sampl(), x1_.sampling_ );
    mIOPDoAxes( get, sKeyX2Sampl(), x2_.sampling_ );
    mIOPDoAxes2( getYN, sKeyShwAnnot(), x1_.showannot_, x2_.showannot_ );
    mIOPDoAxes2( getYN, sKeyShwGridLines(),x1_.showgridlines_,
		 x2_.showgridlines_);
    mIOPDoAxes2( getYN, sKeyIsRev(), x1_.reversed_, x2_.reversed_ );
    iop.getYN( sKeyShwAux(), showaux_ );
}


FlatView::AuxData::EditPermissions::EditPermissions()
    : onoff_( true )
    , namepos_( true )
    , linestyle_( true )
    , linecolor_( true )
    , fillcolor_( true )
    , markerstyle_( true )
    , markercolor_( true )
    , x1rg_( true )
    , x2rg_( true )
{}



FlatView::AuxData::AuxData( const char* nm )
    : name_( toUiString(nm) )
    , namepos_( mUdf(int) )
    , namealignment_(mAlignment(Center,Center))
    , linestyle_( OD::LineStyle::None, 1, Color::NoColor() )
    , fillcolor_( Color::NoColor() )
    , zvalue_( 1 )
    , close_( false )
    , x1rg_( 0 )
    , x2rg_( 0 )
    , enabled_( true )
    , turnon_( true )
    , editpermissions_( 0 )
    , needsupdatelines_( true )
{}


FlatView::AuxData::AuxData(const FlatView::AuxData& aux)
    : name_( aux.name_ )
    , namepos_( aux.namepos_ )
    , namealignment_( aux.namealignment_ )
    , linestyle_( aux.linestyle_ )
    , fillcolor_( aux.fillcolor_ )
    , zvalue_( aux.zvalue_ )
    , markerstyles_( aux.markerstyles_ )
    , close_( aux.close_ )
    , x1rg_( aux.x1rg_ ? new Interval<double>( *aux.x1rg_ ) : 0 )
    , x2rg_( aux.x2rg_ ? new Interval<double>( *aux.x2rg_ ) : 0 )
    , enabled_( aux.enabled_ )
    , turnon_( aux.turnon_ )
    , editpermissions_( aux.editpermissions_
	    ? new EditPermissions(*aux.editpermissions_) : 0 )
    , poly_( aux.poly_ )
    , needsupdatelines_( aux.needsupdatelines_ )
{}


FlatView::AuxData::~AuxData()
{
    delete x1rg_;
    delete x2rg_;
    delete editpermissions_;
}


bool FlatView::AuxData::isEmpty() const
{ return poly_.isEmpty(); }


void FlatView::AuxData::empty()
{ poly_.erase(); }


#define mIOPDoWVA(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyWVA(),keynm), memb )
#define mIOPDoVD(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyVD(),keynm), memb )

void FlatView::DataDispPars::fillPar( IOPar& iop ) const
{
    mIOPDoVD( setYN, sKeyShow(), vd_.show_ );
    mIOPDoVD( set, sKeyColTab(), vd_.colseqname_ );
    mIOPDoVD( setYN, sKeyFlipSequence(),
	      ColTab::isFlipped(vd_.mapper_->setup().seqUseMode()) );
    mIOPDoVD( setYN, sKeyCyclicSequence(),
	      ColTab::isCyclic(vd_.mapper_->setup().seqUseMode()) );
    mIOPDoVD( setYN, sKeyLinearInter(), vd_.lininterp_ );
    mIOPDoVD( setYN, sKeyBlocky(), vd_.blocky_ );

    mIOPDoVD( set, sKeyDispRg(), vd_.mapper_->getRange() );
    mIOPDoVD( setYN, sKeyAutoScale(), vd_.mapper_->setup().isFixed() );
    ColTab::ClipRatePair clipperc = vd_.mapper_->setup().clipRate();
    ColTab::convToPerc( clipperc );
    mIOPDoVD( set, sKeyClipPerc(), clipperc );

    mIOPDoWVA( setYN, sKeyShow(), wva_.show_ );
    mIOPDoWVA( setYN, sKeyBlocky(), wva_.blocky_ );
    mIOPDoWVA( set, sKeyWiggCol(), wva_.wigg_ );
    mIOPDoWVA( set, sKeyRefLineCol(), wva_.refline_ );
    mIOPDoWVA( set, sKeyLowFillCol(), wva_.lowfill_ );
    mIOPDoWVA( set, sKeyHighFillCol(), wva_.highfill_ );
    mIOPDoWVA( set, sKeyOverlap(), wva_.overlap_ );
    mIOPDoWVA( set, sKeyRefLineValue(), wva_.reflinevalue_ );

    mIOPDoWVA( set, sKeyDispRg(), wva_.mapper_->getRange() );
    mIOPDoWVA( setYN, sKeyAutoScale(), wva_.mapper_->setup().isFixed() );
    clipperc = wva_.mapper_->setup().clipRate();
    ColTab::convToPerc( clipperc );
    mIOPDoWVA( set, sKeyClipPerc(), clipperc );
}


void FlatView::DataDispPars::usePar( const IOPar& iop )
{
    mIOPDoVD( getYN, sKeyShow(), vd_.show_ );
    mIOPDoVD( get, sKeyColTab(), vd_.colseqname_ );
    bool flipseq = ColTab::isFlipped( vd_.mapper_->setup().seqUseMode() );
    bool seqcyclic = ColTab::isCyclic( vd_.mapper_->setup().seqUseMode() );
    mIOPDoVD( getYN, sKeyFlipSequence(), flipseq );
    mIOPDoVD( getYN, sKeyCyclicSequence(), seqcyclic );
    vd_.mapper_->setup().setSeqUseMode(
			    ColTab::getSeqUseMode(flipseq,seqcyclic) );
    mIOPDoVD( getYN, sKeyLinearInter(), vd_.lininterp_ );
    mIOPDoVD( getYN, sKeyBlocky(), vd_.blocky_ );

    bool autoscale = !vd_.mapper_->setup().isFixed();
    mIOPDoVD( getYN, "Auto Scale", autoscale ); // legacy
    mIOPDoVD( getYN, sKeyAutoScale(), autoscale );
    Interval<float> range = vd_.mapper_->getRange();
    mIOPDoVD( get, sKeyDispRg(), range );
    if ( autoscale )
	vd_.mapper_->setup().setNotFixed();
    else
	vd_.mapper_->setup().setFixedRange( range );
    ColTab::ClipRatePair clpperc = vd_.mapper_->setup().clipRate();
    clpperc.first() *= 100.f; clpperc.second() *= 100.f;
    mIOPDoVD( get, sKeyClipPerc(), clpperc );
    if ( mIsUdf(clpperc.second()) )
	clpperc.second() = clpperc.first();
    ColTab::convFromPerc( clpperc );
    vd_.mapper_->setup().setClipRate( clpperc );

    mIOPDoWVA( getYN, sKeyShow(), wva_.show_ );
    mIOPDoWVA( getYN, sKeyBlocky(), wva_.blocky_ );
    mIOPDoWVA( get, sKeyWiggCol(), wva_.wigg_ );
    mIOPDoWVA( get, sKeyRefLineCol(), wva_.refline_ );
    mIOPDoWVA( get, sKeyLowFillCol(), wva_.lowfill_ );
    mIOPDoWVA( get, sKeyHighFillCol(), wva_.highfill_ );
    mIOPDoWVA( get, sKeyOverlap(), wva_.overlap_ );
    mIOPDoWVA( get, sKeyRefLineValue(), wva_.reflinevalue_ );

    autoscale = !wva_.mapper_->setup().isFixed();
    mIOPDoWVA( getYN, "Auto Scale", autoscale ); // legacy
    mIOPDoWVA( getYN, sKeyAutoScale(), autoscale );
    range = wva_.mapper_->getRange();
    mIOPDoWVA( get, sKeyDispRg(), range );
    if ( autoscale )
	wva_.mapper_->setup().setNotFixed();
    else
	wva_.mapper_->setup().setFixedRange( range );
    clpperc = wva_.mapper_->setup().clipRate();
    clpperc.first() *= 100.f; clpperc.second() *= 100.f;
    mIOPDoWVA( get, sKeyClipPerc(), clpperc );
    if ( mIsUdf(clpperc.second()) )
	clpperc.second() = clpperc.first();
    ColTab::convFromPerc( clpperc );
    wva_.mapper_->setup().setClipRate( clpperc );
}


void FlatView::Appearance::fillPar( IOPar& iop ) const
{
    annot_.fillPar( iop );
    ddpars_.fillPar( iop );
}


void FlatView::Appearance::usePar( const IOPar& iop )
{
    annot_.usePar( iop );
    ddpars_.usePar( iop );
}


void FlatView::Appearance::setDarkBG( bool yn )
{
    darkbg_ = yn;
    annot_.color_ = yn ? Color::White() : Color::Black();
    ddpars_.wva_.wigg_ = annot_.color_;
}


void FlatView::Appearance::setGeoDefaults( bool isvert )
{
    annot_.x2_.reversed_ = isvert || SI().isRightHandSystem();
    annot_.x2_.annotinint_ = !isvert;
    ddpars_.wva_.allowuserchange_ = isvert;
}


class FlatView_CB_Rcvr : public CallBacker
{
public:
FlatView_CB_Rcvr( FlatView::Viewer& vwr ) : vwr_(vwr)	{}
void theCB( CallBacker* dp ) { vwr_.removePack( ((DataPack*)dp)->id() ); }

FlatView::Viewer& vwr_;
};


FlatView::Viewer::Viewer()
    : cbrcvr_(new FlatView_CB_Rcvr(*this))
    , dpm_(DPM(DataPackMgr::FlatID()))
    , defapp_(0)
    , datatransform_(0)
    , zdinfo_(new ZDomain::Info(SI().zDomain()) )
    , wvapack_(0)
    , vdpack_(0)
    , needstatusbarupd_(true)
{
    dpm_.packToBeRemoved.notifyIfNotNotified(
			    mCB(cbrcvr_,FlatView_CB_Rcvr,theCB) );
}


FlatView::Viewer::~Viewer()
{
    dpm_.packToBeRemoved.remove( mCB(cbrcvr_,FlatView_CB_Rcvr,theCB) );
    delete defapp_;
    delete cbrcvr_;

    if ( datatransform_ )
	datatransform_->unRef();

    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	dpm_.unRef( ids_[idx] );
    }

    delete zdinfo_;
}


bool FlatView::Viewer::setZAxisTransform( ZAxisTransform* zat )
{
    if ( datatransform_ )
	datatransform_->unRef();

    datatransform_ = zat;
    if ( datatransform_ )
	datatransform_->ref();

    return true;
}


void FlatView::Viewer::setZDomain( const ZDomain::Def& zdef )
{
    delete zdinfo_;
    zdinfo_ = new ZDomain::Info( zdef );
}


const ZDomain::Info& FlatView::Viewer::zDomain() const
{
    return datatransform_ ? datatransform_->toZDomainInfo()
			  : *zdinfo_;
}


void FlatView::Viewer::getAuxInfo( const Point& pt, IOPar& iop ) const
{
    BufferString txt( mFromUiStringTodo(appearance().annot_.x1_.name_) );
    txt += " vs "; txt += mFromUiStringTodo(appearance().annot_.x2_.name_);
    iop.set( "Positioning", txt );
    addAuxInfo( true, pt, iop );
    addAuxInfo( false, pt, iop );
}


void FlatView::Viewer::addAuxInfo( bool iswva, const Point& pt,
				   IOPar& iop ) const
{
    ConstRefMan<FlatDataPack> dp = getPack( iswva );
    if ( !dp )
    {
	iswva ? iop.removeWithKey( "Wiggle/VA data" )
	      : iop.removeWithKey( "Variable density data" );
	iswva ? iop.removeWithKey( "WVA Value" )
	      : iop.removeWithKey( "VD Value" );
	    return;
    }
    const Array2D<float>& arr = dp->data();

    const char* nm = dp->name();
    iop.set( iswva ? "Wiggle/VA data" : "Variable density data", nm );

    const Array2DInfo& info = arr.info();
    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, pt.x_ );
    const IndexInfo iy = pd.indexInfo( false, pt.y_ );

    if ( info.validPos(ix.nearest_, iy.nearest_) )
    {
	const float val = arr.get( ix.nearest_, iy.nearest_ );
	iop.set( iswva ? "WVA Value" : "VD Value", val );
	dp->getAuxInfo( ix.nearest_, iy.nearest_, iop );
    }
}


void FlatView::Viewer::setMapper( bool wva, ColTab::Mapper& mpr )
{
    auto& ddpars = appearance().ddpars_;
    if ( wva )
	ddpars.wva_.mapper_ = &mpr;
    else
	ddpars.vd_.mapper_ = &mpr;
}


RefMan<ColTab::Mapper> FlatView::Viewer::mapper( bool wva )
{
    return const_cast<ColTab::Mapper*>(
	    (const_cast<const Viewer*>(this)->mapper( wva )).ptr() );
}


ConstRefMan<ColTab::Mapper> FlatView::Viewer::mapper( bool wva ) const
{
    auto& ddpars = appearance().ddpars_;
    if ( wva )
	return ddpars.wva_.mapper_;
    else
	return ddpars.vd_.mapper_;
}


Coord3 FlatView::Viewer::getCoord( const Point& wp ) const
{
    ConstRefMan<FlatDataPack> fdp = getPack( false, true );
    if ( !fdp )
	return Coord3::udf();

    const FlatPosData& pd = fdp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x_ );
    const IndexInfo iy = pd.indexInfo( false, wp.y_ );
    if ( !fdp->data().info().validPos(ix.nearest_,iy.nearest_) )
	return Coord3::udf();

    const int floorx = ix.roundedtolow_ ? ix.nearest_ : ix.nearest_ - 1;
    const int floory = iy.roundedtolow_ ? iy.nearest_ : iy.nearest_ - 1;
    const int ceilx = ix.roundedtolow_ ? ix.nearest_ + 1 : ix.nearest_;
    const int ceily = iy.roundedtolow_ ? iy.nearest_ + 1 : ix.nearest_;
    if ( !fdp->data().info().validPos(floorx,floory) ||
	    !fdp->data().info().validPos(ceilx,ceily) )
	return fdp->getCoord( ix.nearest_, iy.nearest_ );

    Coord3 pos1 = fdp->getCoord( floorx, floory );
    Coord3 pos2 = fdp->getCoord( floorx, ceily );
    Coord3 pos3 = fdp->getCoord( ceilx, floory );
    Coord3 pos4 = fdp->getCoord( ceilx, ceily );
    const double xfac = ceilx==floorx
	? 0
	: ( wp.x_ - pd.position(true,floorx) )/
		       ( pd.position(true,ceilx) - pd.position(true,floorx) );

    const double yfac = ceily==floory
	? 0
	: ( wp.y_ - pd.position(false,floory))/
		       ( pd.position(false,ceily) - pd.position(false,floory) );

    const Coord3 realpos = pos1*(1-xfac)*(1-yfac) + pos2*(1-xfac)*yfac
				+ pos3*xfac*(1-yfac) + pos4*xfac*yfac;
    return realpos;
}


void FlatView::Viewer::removeAllAuxData()
{
    while ( nrAuxData() )
	delete removeAuxData( 0 );
}


void FlatView::Viewer::removeAuxDatas( ObjectSet<AuxData>& ads )
{
    const bool ismanaged = ads.isManaged();
    for ( int idx=ads.size()-1; idx>=0; idx -- )
    {
	AuxData* ad = removeAuxData( ads[idx] );
	if ( !ismanaged ) delete ad;
    }
    ads.erase();
}


FlatView::Appearance& FlatView::Viewer::appearance()
{
    if ( !defapp_ )
	defapp_ = new FlatView::Appearance;
    return *defapp_;
}


void FlatView::Viewer::addPack( DataPack::ID id )
{
    if ( !id.isValid() || ids_.isPresent(id) )
	return;

#ifdef __debug__
    RefMan<FlatDataPack> rm = dpm_.get<FlatDataPack>( id );
    if ( !rm )
    {
	if ( !dpm_.isPresent(id) )
	    { pErrMsg("DataPack not added to Flat DPM"); return; }
	else
	    { pErrMsg("DataPack added to Flat DPM is not Flat"); return; }
    }
#endif

    ids_ += id;
    dpm_.ref( id );
}


ConstRefMan<FlatDataPack>
FlatView::Viewer::getPack( bool wva, bool checkother ) const
{
    Threads::Locker locker( lock_ );
    ConstRefMan<FlatDataPack> res = wva ? wvapack_.get() : vdpack_.get();
    if ( !res && checkother )
	res = wva ? vdpack_.get() : wvapack_.get();

    return res;

}


const FlatDataPack* FlatView::Viewer::obtainPack(
				bool wva, bool checkother ) const
{
    Threads::Locker locker( lock_ );
    ConstRefMan<FlatDataPack> res = wva ? wvapack_.get() : vdpack_.get();

    if ( !res && checkother )
	res = wva ? vdpack_.get() : wvapack_.get();

    refPtr( res );
    dpm_.add( const_cast<FlatDataPack*>(res.ptr()) );
    return res;
}


DataPack::ID FlatView::Viewer::packID( bool wva ) const
{
    ConstRefMan<FlatDataPack> dp = getPack( wva );
    return dp ? dp->id() : ::DataPack::cNoID();
}


void FlatView::Viewer::clearAllPacks()
{
    while ( !ids_.isEmpty() )
	removePack( ids_[0] );
}


void FlatView::Viewer::removePack( DataPack::ID id )
{
    const int idx = ids_.indexOf( id );
    if ( idx < 0 ) return;

    if ( hasPack(true) && packID(true)==id )
	usePack( true, DataPack::cNoID(), false );

    if ( hasPack(false) && packID(false)==id )
	usePack( false, DataPack::cNoID(), false );

    ids_.removeSingle( idx );
    dpm_.unRef( id );
}


void FlatView::Viewer::usePack( bool wva, DataPack::ID id, bool usedefs )
{
    DataPack::ID curid = packID( wva );
    if ( id == curid )
	return;

    if ( id == DataPack::cNoID() )
	(wva ? wvapack_ : vdpack_) = 0;
    else if ( !ids_.isPresent(id) )
	{ pErrMsg("Requested usePack, but ID not added"); return; }
    else
	(wva ? wvapack_ : vdpack_) = dpm_.observe<FlatDataPack>( id );

    ConstRefMan<FlatDataPack> fdp = getPack( wva );
    if ( !fdp )
	return;

    if ( usedefs )
	useStoredDefaults( fdp->category() );

    FlatView::Annotation& annot = appearance().annot_;
    if ( annot.x1_.name_.isEmpty() ||
		    BufferString(mFromUiStringTodo(annot.x1_.name_)) == "X1" )
    {
	annot.x1_.name_ = toUiString(fdp->dimName( true ));
	BufferStringSet altdimnms; fdp->getAltDim0Keys( altdimnms );
	setAnnotChoice( altdimnms.indexOf(
					 mFromUiStringTodo(annot.x1_.name_)) );
    }

    if ( annot.x2_.name_.isEmpty() ||
		    BufferString(mFromUiStringTodo(annot.x2_.name_)) == "X2" )
    {
	annot.x2_.name_ = toUiString(fdp->dimName( false ));
	annot.x2_.annotinint_ = fdp->dimValuesInInt(
				    mFromUiStringTodo(annot.x2_.name_) );
    }

    handleChange( BitmapData );
}


bool FlatView::Viewer::isVisible( bool wva ) const
{
    const FlatView::DataDispPars& ddp = appearance().ddpars_;
    return wva ? ddp.wva_.show_ : ddp.vd_.show_;
}


void FlatView::Viewer::setVisible( bool wva, bool visibility )
{
    FlatView::DataDispPars& ddp = appearance().ddpars_;
    bool& show = ( wva ? ddp.wva_.show_ : ddp.vd_.show_ );

    if ( show!=visibility )
    {
	show = visibility;
	handleChange( BitmapData );
    }
}


void FlatView::Viewer::storeDefaults( const char* ky ) const
{
    Settings& setts = Settings::fetch( "flatview" );
    IOPar iop; fillAppearancePar( iop );
    setts.mergeComp( iop, ky );
    setts.write();
}


void FlatView::Viewer::useStoredDefaults( const char* ky )
{
    Settings& setts = Settings::fetch( "flatview" );
    ConstPtrMan<IOPar> iop = setts.subselect( ky );
    if ( iop && !iop->isEmpty() )
	useAppearancePar( *iop );
}


StepInterval<double> FlatView::Viewer::getDataPackRange( bool forx1 ) const
{
    const bool wva = appearance().ddpars_.wva_.show_;
    ConstRefMan<FlatDataPack> dp = getPack( wva, true );
    if ( !dp )
	return StepInterval<double>(mUdf(double),mUdf(double),1);
    return dp->posData().range( forx1 );
}


Interval<float> FlatView::Viewer::getDataRange( bool iswva ) const
{
    if ( iswva )
	return appearance().ddpars_.wva_.mapper_->getRange();
    else
	return appearance().ddpars_.vd_.mapper_->getRange();
}


void FlatView::Viewer::setMapperDistribFromDataPack( bool iswva )
{
    ConstRefMan<FlatDataPack> fdp = getPack( iswva );
    if ( !fdp )
	return;

    RangeLimitedDataDistributionExtracter<float> extr( fdp->data(),
                                                SilentTaskRunnerProvider() );
    auto& ddpars = appearance().ddpars_;
    auto& mpr = *(iswva ? ddpars.wva_.mapper_ : ddpars.vd_.mapper_);
    mpr.distribution() = *extr.getDistribution();
}


void FlatView::Viewer::setSeisGeomidsToViewer( GeomIDSet& geomids )
{
    geom2dids_ = geomids;
}
