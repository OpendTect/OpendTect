/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "flatview.h"

#include "flatposdata.h"
#include "keystrs.h"
#include "randcolor.h"
#include "settings.h"
#include "survinfo.h"
#include "uistrings.h"
#include "zaxistransform.h"

#define mNrExtraZDec 3

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
const char* DataDispPars::sKeyDispRg()  { return "Range"; }
const char* DataDispPars::sKeyColTab()  { return "Color Table"; }
const char* DataDispPars::sKeyFlipSequence() { return "Flip Sequence"; }
const char* DataDispPars::sKeyLinearInter()  { return "Linear Interpolation"; }
const char* DataDispPars::sKeyBlocky()	 { return "Blocky"; }
const char* DataDispPars::sKeyAutoScale(){ return "Auto scale"; }
const char* DataDispPars::sKeyClipPerc() { return "Percentage Clip"; }
const char* DataDispPars::sKeyWiggCol()  { return "Wiggle color"; }
const char* DataDispPars::sKeyRefLineCol()	{ return "Ref line color"; }
const char* DataDispPars::sKeyLowFillCol()	{ return "Low fill color"; }
const char* DataDispPars::sKeyHighFillCol()	{ return "High fill color"; }
const char* DataDispPars::sKeyOverlap()  { return "Overlap"; }
const char* DataDispPars::sKeySymMidValue()  { return "Sym Mid value"; }
const char* DataDispPars::sKeyRefLineValue() { return "Ref Line value"; }

const char* Viewer::sKeyDefCategory() { return "General"; };
const char* Viewer::sKeyWVAData() { return "Wiggle/VA data"; };
const char* Viewer::sKeyVDData() { return "Variable density data"; };
const char* Viewer::sKeyWVAVal() { return "WVA Value"; };
const char* Viewer::sKeyVDVal() { return "VD Value"; };
const char* Viewer::sKeyViewZnrDec() { return "Decimal places for Z Value"; };

}


FlatPosData::FlatPosData()
    : x1rg_(0,0,1)
    , x2rg_(0,0,1)
{}


FlatPosData::FlatPosData( const FlatPosData& fpd )
{
    *this = fpd;
}


FlatPosData::~FlatPosData()
{}


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


bool FlatPosData::operator==( const FlatPosData& oth ) const
{
    return oth.x1rg_==x1rg_ && oth.x2rg_==x2rg_ && oth.x1offs_==x1offs_;
}


bool FlatPosData::operator!=( const FlatPosData& oth ) const
{
    return !operator==( oth );
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
    x1rg_.start_ = pos[0] + offs; x1rg_.stop_ = pos[sz-1] + offs;
    x1rg_.step_ = sz > 1 ? (x1rg_.stop_ - x1rg_.start_) / (sz - 1) : 1;
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
    return !isx1 || !x1pos_ || idx<0 || idx>=nrPts(true) ?
		range(isx1).atIndex(idx) : sCast(double,x1pos_[idx]) + x1offs_;
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
{}


FlatView::DataDispPars::Common::~Common()
{}


FlatView::DataDispPars::VD::VD()
{}


FlatView::DataDispPars::VD::~VD()
{}


FlatView::DataDispPars::WVA::WVA()
{}


FlatView::DataDispPars::WVA::~WVA()
{}

/*void FlatView::DataDispPars::Common::fill( ColTab::MapperSetup& setup ) const
{
    if ( autoscale_ )
    {
	if ( histeq_ ) setup.type_ = ColTab::MapperSetup::HistEq;
	else
	{
	    setup.type_ = ColTab::MapperSetup::Auto;
	    setup.cliprate_ = Interval<float>( clipperc_.start*0.01,
					       clipperc_.stop*0.01 );
	    if ( mIsUdf(clipperc_.stop) )
		setup.cliprate_ = clipperc_.start*0.01;
	    else
		setup.cliprate_ = clipperc_.center()*0.01;
	}

	setup.symmidval_ = symmidvalue_;
	setup.autosym0_ = false;
    }
    else
    {
	setup.type_ = ColTab::MapperSetup::Fixed;
	setup.start_ = rg_.start;
	setup.width_ = rg_.width();
    }
}*/


FlatView::Annotation::AxisData::AxisData()
    : sampling_(mUdf(float),mUdf(float))
    , auxhllinestyle_( OD::LineStyle::Dot,2,OD::getRandStdDrawColor() )
{}


FlatView::Annotation::AxisData::~AxisData()
{}

void FlatView::Annotation::AxisData::showAll( bool yn )
{
    showannot_ = showgridlines_ = yn;
}


int FlatView::Annotation::AxisData::auxPosIdx( float atpos, float eps ) const
{
    for ( int auxidx=0; auxidx<auxannot_.size(); auxidx++ )
    {
	const PlotAnnotation& annot = auxannot_[auxidx];
	if ( mIsEqual(annot.pos_,atpos,eps) )
	    return auxidx;
    }

    return -1;
}


FlatView::Annotation::Annotation( bool drkbg )
    : color_(drkbg ? OD::Color::White() :OD:: Color::Black())
{
    x1_.name_ = uiStrings::sX1();
    x2_.name_ = uiStrings::sX2();
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


FlatView::AuxData::AuxData( const char* nm )
    : name_(nm)
    , linestyle_(OD::LineStyle::None,1,OD::Color::NoColor())
{
}


FlatView::AuxData::AuxData( const FlatView::AuxData& aux )
    : name_(aux.name_)
    , namepos_(aux.namepos_)
    , namealignment_(aux.namealignment_)
    , linestyle_(aux.linestyle_)
    , fillcolor_(aux.fillcolor_)
    , zvalue_(aux.zvalue_)
    , markerstyles_(aux.markerstyles_)
    , close_(aux.close_)
    , x1rg_(aux.x1rg_ ? new Interval<double>(*aux.x1rg_) : nullptr)
    , x2rg_(aux.x2rg_ ? new Interval<double>( *aux.x2rg_ ) : nullptr)
    , enabled_(aux.enabled_)
    , editpermissions_(aux.editpermissions_
			? new EditPermissions(*aux.editpermissions_) : nullptr)
    , poly_(aux.poly_)
    , turnon_(true)
    , needsupdatelines_(aux.needsupdatelines_)
    , fitnameinview_(aux.fitnameinview_)
{
}


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


FlatView::AuxData::FillGradient::FillGradient()
{}


FlatView::AuxData::FillGradient::~FillGradient()
{}


void FlatView::AuxData::FillGradient::set( const Point& fr, const Point& to,
		const TypeSet<float>& stops, const TypeSet<OD::Color>& colors )
{
    from_ = fr;
    to_ = to;
    stops_ = stops;
    colors_ = colors;
}


void FlatView::AuxData::FillGradient::set( const OD::Color& col1,
					   const OD::Color& col2, bool hor )
{
    from_ = Point( 0., 0. );
    to_ = Point( hor ? 1. : 0., hor ? 0. : 1. );
    stops_.erase();
    colors_.erase();
    stops_ += 0.f;
    stops_ += 1.f;
    colors_ += col1;
    colors_ += col2;
}


FlatView::DataDispPars::DataDispPars()
{}


FlatView::DataDispPars::~DataDispPars()
{}

#define mIOPDoWVA(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyWVA(),keynm), memb )
#define mIOPDoVD(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyVD(),keynm), memb )

void FlatView::DataDispPars::fillPar( IOPar& iop ) const
{
    mIOPDoVD( setYN, sKeyShow(), vd_.show_ );
    mIOPDoVD( set, sKeyDispRg(), vd_.mappersetup_.range_ );
    mIOPDoVD( set, sKeyColTab(), vd_.ctab_ );
    mIOPDoVD( setYN, sKeyFlipSequence(), vd_.mappersetup_.flipseq_ );
    mIOPDoVD( setYN, sKeyLinearInter(), vd_.lininterp_ );
    mIOPDoVD( setYN, sKeyBlocky(), vd_.blocky_ );
    mIOPDoVD( setYN, sKeyAutoScale(),
	      vd_.mappersetup_.type_ == ColTab::MapperSetup::Auto );
    Interval<float> clipperc( vd_.mappersetup_.cliprate_.start_*100,
			      vd_.mappersetup_.cliprate_.stop_*100 );
    mIOPDoVD( set, sKeyClipPerc(), clipperc );
    mIOPDoVD( set, sKeySymMidValue(), vd_.mappersetup_.symmidval_ );

    mIOPDoWVA( setYN, sKeyShow(), wva_.show_ );
    mIOPDoWVA( set, sKeyDispRg(), wva_.mappersetup_.range_ );
    mIOPDoWVA( setYN, sKeyBlocky(), wva_.blocky_ );
    mIOPDoWVA( setYN, sKeyAutoScale(),
	       wva_.mappersetup_.type_ == ColTab::MapperSetup::Auto );
    mIOPDoWVA( set, sKeyClipPerc(), wva_.mappersetup_.cliprate_ );
    mIOPDoWVA( set, sKeyWiggCol(), wva_.wigg_ );
    mIOPDoWVA( set, sKeyRefLineCol(), wva_.refline_ );
    mIOPDoWVA( set, sKeyLowFillCol(), wva_.lowfill_ );
    mIOPDoWVA( set, sKeyHighFillCol(), wva_.highfill_ );
    mIOPDoWVA( set, sKeyOverlap(), wva_.overlap_ );
    mIOPDoWVA( set, sKeySymMidValue(), wva_.mappersetup_.symmidval_ );
    mIOPDoWVA( set, sKeyRefLineValue(), wva_.reflinevalue_ );
}


void FlatView::DataDispPars::usePar( const IOPar& iop )
{
    mIOPDoVD( getYN, sKeyShow(), vd_.show_ );
    Interval<float> range;
    mIOPDoVD( get, sKeyDispRg(), range );
    mIOPDoVD( get, sKeyColTab(), vd_.ctab_ );
    mIOPDoVD( getYN, sKeyFlipSequence(), vd_.mappersetup_.flipseq_ );
    mIOPDoVD( getYN, sKeyLinearInter(), vd_.lininterp_ );
    mIOPDoVD( getYN, sKeyBlocky(), vd_.blocky_ );
    bool autoscale = true;
    mIOPDoVD( getYN, "Auto Scale", autoscale ); // legacy
    mIOPDoVD( getYN, sKeyAutoScale(), autoscale );
    vd_.mappersetup_.range_ = autoscale ? Interval<float>::udf() : range;
    vd_.mappersetup_.setAutoScale( autoscale );
    mIOPDoVD( get, sKeyClipPerc(), vd_.mappersetup_.cliprate_ );
    vd_.mappersetup_.cliprate_.start_ *= 0.01;
    if ( mIsUdf(vd_.mappersetup_.cliprate_.stop_) )
	vd_.mappersetup_.cliprate_.stop_ = vd_.mappersetup_.cliprate_.start_;
    else
	vd_.mappersetup_.cliprate_.stop_ *= 0.01;

    mIOPDoVD( get, sKeySymMidValue(), vd_.mappersetup_.symmidval_ );

    mIOPDoWVA( getYN, sKeyShow(), wva_.show_ );
    mIOPDoWVA( get, sKeyDispRg(), range );
    wva_.mappersetup_.range_ = autoscale ? Interval<float>::udf() : range;
    mIOPDoWVA( getYN, sKeyBlocky(), wva_.blocky_ );
    autoscale = true;
    mIOPDoWVA( getYN, "Auto Scale", autoscale ); // legacy
    mIOPDoWVA( getYN, sKeyAutoScale(), autoscale );
    wva_.mappersetup_.setAutoScale( autoscale );
    mIOPDoWVA( get, sKeyClipPerc(), wva_.mappersetup_.cliprate_ );
    mIOPDoWVA( get, sKeyWiggCol(), wva_.wigg_ );
    mIOPDoWVA( get, sKeyRefLineCol(), wva_.refline_ );
    mIOPDoWVA( get, sKeyLowFillCol(), wva_.lowfill_ );
    mIOPDoWVA( get, sKeyHighFillCol(), wva_.highfill_ );
    mIOPDoWVA( get, sKeyOverlap(), wva_.overlap_ );
    mIOPDoWVA( get, sKeySymMidValue(), wva_.mappersetup_.symmidval_ );
    mIOPDoWVA( get, sKeyRefLineValue(), wva_.reflinevalue_ );
}


FlatView::Appearance::Appearance( bool drkbg )
    : darkbg_(drkbg)
    , annot_(drkbg)
    , secondsetaxes_(drkbg)
{}


FlatView::Appearance::~Appearance()
{}


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
    annot_.color_ = yn ? OD::Color::White() : OD::Color::Black();
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
void theCB( CallBacker* dp )
{
    DataPackID dpid = ((DataPack*)dp)->id();
    bool dowva = dpid == vwr_.packID(true);
    bool dovd = dpid == vwr_.packID(false);
    vwr_.removePack( FlatView::Viewer::getDest(dowva, dovd) );
}

FlatView::Viewer& vwr_;
};


FlatView::Viewer::Viewer()
    : cbrcvr_(new FlatView_CB_Rcvr(*this))
    , dpm_(DPM(DataPackMgr::FlatID()))
{
    dpm_.packToBeRemoved.notifyIfNotNotified(
			    mCB(cbrcvr_,FlatView_CB_Rcvr,theCB) );
}


FlatView::Viewer::~Viewer()
{
    dpm_.packToBeRemoved.remove( mCB(cbrcvr_,FlatView_CB_Rcvr,theCB) );
    delete defapp_;
    delete cbrcvr_;

    unRefPtr( datatransform_ );
}


bool FlatView::Viewer::setZAxisTransform( ZAxisTransform* zat )
{
    unRefPtr( datatransform_ );
    datatransform_ = zat;
    refPtr( datatransform_ );

    return true;
}


void FlatView::Viewer::setZDomain( const ZDomain::Info& zinfo, bool display )
{
    if ( display )
	displayzdinfo_ = &zinfo;
    else
	zdinfo_ = &zinfo;
}


const ZDomain::Info* FlatView::Viewer::zDomain( bool display ) const
{
    if ( display && displayzdinfo_ )
    {
	if ( datatransform_ )
	{
	    const ZDomain::Info& transzinfo = datatransform_->zDomain( false );
	    if ( transzinfo.isDepth() )
		return SI().depthsInFeet() ? &ZDomain::DepthFeet()
					   : &ZDomain::DepthMeter();

	    return &transzinfo;
	}

	return displayzdinfo_;
    }

    return datatransform_ ? &datatransform_->zDomain( false )
			  : zdinfo_;
}


void FlatView::Viewer::getAuxInfo( const Point& pt, IOPar& iop ) const
{
    addAuxInfo( true, pt, iop );
    addAuxInfo( false, pt, iop );
}


void FlatView::Viewer::addAuxInfo( bool iswva, const Point& pt,
				   IOPar& iop ) const
{
    const WeakPtr<FlatDataPack> datapack = getPack( iswva );
    if ( !datapack )
    {
	iop.removeWithKey( iswva ? sKeyWVAData() : sKeyVDData() );
	iop.removeWithKey( iswva ? sKeyWVAVal() : sKeyVDVal() );
	return;
    }

    ConstRefMan<FlatDataPack> dp = datapack.get();
    const Array2D<float>& arr = dp->data();

    const char* nm = dp->name();
    iop.set( iswva ? sKeyWVAData() : sKeyVDData(), nm );
    const Array2DInfo& info = arr.info();
    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, pt.x_ );
    const IndexInfo iy = pd.indexInfo( false, pt.y_ );

    if ( !info.validPos(ix.nearest_,iy.nearest_) )
	return;

    const float val = arr.get( ix.nearest_, iy.nearest_ );
    iop.set( iswva ? sKeyWVAVal() : sKeyVDVal(), val );
    dp->getAuxInfo( ix.nearest_, iy.nearest_, iop );
}


bool FlatView::Viewer::isVertical() const
{
    const bool usewva = !isVisible( false );
    const WeakPtr<FlatDataPack> datapack = getPack( usewva, true );
    if ( !datapack )
	return true;

    ConstRefMan<FlatDataPack> fdp = datapack.get();
    return fdp ? fdp->isVertical() : true;
}


float FlatView::Viewer::annotUserFactor( bool x2 ) const
{
    if ( !x2 )
	return 1.f;

    const ZDomain::Info* datazdom = zDomain( false );
    const ZDomain::Info* displayzdom = zDomain( true );
    return datazdom ? userFactor( *datazdom, displayzdom ) : 1.f;
}


int FlatView::Viewer::nrXYDec() const
{
    return SI().nrXYDecimals();
}


int FlatView::Viewer::nrZDec() const
{
    const ZDomain::Info* datazdom = zDomain( false );
    const ZDomain::Info* displayzdom = zDomain( true );
    return nrDec( displayzdom ? *displayzdom
			      : (datazdom ? *datazdom : SI().zDomainInfo()) );
}


int FlatView::Viewer::nrDec( const ZDomain::Info& zdom )
{
    return zdom.nrDecimals( mUdf(float) );
}


float FlatView::Viewer::userFactor( const ZDomain::Info& datazdom,
				    const ZDomain::Info* displayzdom )
{
    if ( !displayzdom || displayzdom == &datazdom || !datazdom.isDepth() )
	return mCast(float,datazdom.userFactor());

    float userfac = mCast(float,displayzdom->userFactor());
    if ( displayzdom->isDepthMeter() )
	userfac *= mFromFeetFactorF;
    else if ( displayzdom->isDepthFeet() )
	userfac *= mToFeetFactorF;

    return userfac;
}


Coord3 FlatView::Viewer::getCoord( const Point& wp ) const
{
    const WeakPtr<FlatDataPack> datapack = getPack( false, true );
    if ( !datapack )
	return Coord3::udf();

    ConstRefMan<FlatDataPack> fdp = datapack.get();
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
    const double xfac = ( wp.x_ - pd.position(true,floorx) ) /
		       ( pd.position(true,ceilx) - pd.position(true,floorx) );
    const double yfac = ( wp.y_ - pd.position(false,floory) ) /
		       ( pd.position(false,ceily) - pd.position(false,floory) );
    Coord3 realpos = pos1*(1-xfac)*(1-yfac) + pos2*(1-xfac)*yfac
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


bool FlatView::Viewer::setAnnotChoiceByIdx( int sel, bool dim0 )
{
    ConstRefMan<FlatDataPack> fdp = getPack( false, true ).get();
    if ( !fdp )
	return false;

    FlatView::Annotation::AxisData& axisdata = dim0 ? appearance().annot_.x1_
						    : appearance().annot_.x2_;
    uiStringSet altdimkeys, altdimunitlbls;
    fdp->getAltDimKeys( altdimkeys, dim0 );
    fdp->getAltDimKeysUnitLbls( altdimunitlbls, dim0 );
    axisdata.name_ = altdimkeys.validIdx(sel) ? altdimkeys.get( sel )
					      : fdp->dimName( dim0 );
    axisdata.annotinint_ = fdp->dimValuesInInt( axisdata.name_, dim0 );
    axisdata.altdim_ = altdimkeys.isPresent( axisdata.name_ )
		     ? altdimkeys.indexOf( axisdata.name_ ) : sel;

    const uiString unitlbl = altdimunitlbls.validIdx(sel)
			   ? altdimunitlbls.get( sel )
			   : fdp->dimUnitLbl( dim0, true );
    if ( !unitlbl.isEmpty() )
	axisdata.name_.appendPhrase( unitlbl, uiString::Space,
				     uiString::OnSameLine );

    return true;
}


bool FlatView::Viewer::setAnnotChoice( const uiString& annotnm, bool dim0 )
{
    uiStringSet nms;
    getAnnotChoices( nms, dim0 );
    const int annotidx = nms.indexOf( annotnm );
    return setAnnotChoiceByIdx( annotidx, dim0 );
}


int FlatView::Viewer::getAnnotChoices( uiStringSet& ss, bool dim0 ) const
{
    ConstRefMan<FlatDataPack> fdp = getPack( false, true ).get();
    if ( fdp )
	fdp->getAltDimKeys( ss, dim0 );

    const uiString firstdimnm = fdp->dimName( dim0 );
    if ( !ss.isEmpty() && !ss.isPresent(firstdimnm) )
	{ pErrMsg("Should not happen"); }

    const uiString& xuinm = dim0 ? appearance().annot_.x1_.name_
				 : appearance().annot_.x2_.name_;
    const BufferString xnm = xuinm.getString();
    for ( int idx=0; idx<ss.size(); idx++ )
    {
	const BufferString optstr = ss.get(idx).getString();
	if ( xnm.startsWith(optstr) )
	    return idx;
    }

    return -1;
}


FlatView::Appearance& FlatView::Viewer::appearance()
{
    if ( !defapp_ )
	defapp_ = new FlatView::Appearance;
    return *defapp_;
}


WeakPtr<FlatDataPack> FlatView::Viewer::getPack( bool wva,
						 bool checkother ) const
{
    Threads::Locker locker( lock_ );
    WeakPtr<FlatDataPack> res = wva ? wvapack_ : vdpack_;
    if ( !res && checkother )
	res = wva ? vdpack_ : wvapack_;

    return res;
}


bool FlatView::Viewer::hasPack( bool wva ) const
{
    Threads::Locker locker( lock_ );
    const WeakPtr<FlatDataPack>& res = wva ? wvapack_ : vdpack_;
    return bool(res);
}


DataPackID FlatView::Viewer::packID( bool wva ) const
{
    ConstRefMan<FlatDataPack> dp = getPack( wva ).get();
    return dp ? dp->id() : ::DataPack::cNoID();
}


void FlatView::Viewer::clearAllPacks()
{
    removePack( Both );
}


void FlatView::Viewer::removePack( VwrDest dest )
{
    setPack( dest, nullptr, false );
}


bool FlatView::Viewer::enableChange( bool yn )
{
    const bool ret = canhandlechange_;
    canhandlechange_ = yn;
    return ret;
}


void FlatView::Viewer::setPack( VwrDest dest, FlatDataPack* fdp,
				bool usedefs )
{
    if ( dest == None )
	return;

    const bool wva = dest == WVA || dest == Both;
    const bool vd = dest == VD || dest == Both;
    ConstRefMan<FlatDataPack> curwva, curvd;
    if ( wva )
	curwva = getPack( true ).get();
    if ( vd )
	curvd = getPack( false ).get();

    if ( (dest == WVA && fdp == curwva.ptr()) ||
	 (dest == VD && fdp == curvd.ptr()) ||
	 (dest == Both && fdp == curwva.ptr() && fdp == curvd.ptr()) )
	return;

    BufferString category;
    if ( !fdp )
    {
	if ( wva )
	    wvapack_ = nullptr;
	if ( vd )
	    vdpack_ = nullptr;

	handleChange( BitmapData );
	return;
    }
    else
    {
	if ( wva )
	    wvapack_ = WeakPtr<FlatDataPack>( fdp );
	if ( vd )
	    vdpack_ = WeakPtr<FlatDataPack>( fdp );
    }

    if ( usedefs )
	useStoredDefaults( fdp->category() );

    FlatView::Annotation& annot = appearance().annot_;
    if ( annot.x1_.name_.isEmpty() || annot.x1_.name_ == uiStrings::sX1() )
	setAnnotChoiceByIdx( -1, true );

    if ( annot.x2_.name_.isEmpty() || annot.x2_.name_ == uiStrings::sX2() )
	setAnnotChoiceByIdx( -1, false );

    handleChange( BitmapData );
}


bool FlatView::Viewer::isVisible( bool wva ) const
{
    const FlatView::DataDispPars& ddp = appearance().ddpars_;
    return wva ? ddp.wva_.show_ : ddp.vd_.show_;
}


bool FlatView::Viewer::isVisible( VwrDest dest ) const
{
    const FlatView::DataDispPars& ddp = appearance().ddpars_;
    if ( dest == WVA )
	return ddp.wva_.show_;
    if ( dest == VD )
	return ddp.vd_.show_;
    if ( dest == Both )
	return ddp.wva_.show_ && ddp.vd_.show_;

    return false;
}


bool FlatView::Viewer::setVisible( VwrDest dest, bool visibility,
				   od_uint32* ctyp )
{
    if ( dest == None )
	return false;

    FlatView::DataDispPars& ddp = appearance().ddpars_;
    ObjectSet<DataDispPars::Common> allddpars;
    if ( dest == WVA || dest == Both )
	allddpars.add( &(DataDispPars::Common&)ddp.wva_ );
    if ( dest == VD || dest == Both )
	allddpars.add( &(DataDispPars::Common&)ddp.vd_ );

    bool donotif = false;
    for ( auto* vwrddpars : allddpars )
    {
	bool& show = vwrddpars->show_;
	if ( show != visibility )
	{
	    show = visibility;
	    donotif = true;
	}
    }

    if ( donotif )
    {
	const od_uint32 ctype = sCast(od_uint32,BitmapData);
	if ( ctyp )
	    *ctyp = Math::SetBits( *ctyp, ctype, true );
	else
	    handleChange( ctype );
    }

    return donotif;
}


void FlatView::Viewer::storeDefaults( const char* ky ) const
{
    Settings& setts = Settings::fetch( "flatview" );
    IOPar iop;
    fillAppearancePar( iop );
    setts.mergeComp( iop, ky ? ky : sKeyDefCategory() );
    setts.write();
}


void FlatView::Viewer::useStoredDefaults( const char* ky )
{
    Settings& setts = Settings::fetch( "flatview" );
    PtrMan<IOPar> iop = setts.subselect( ky );
    if ( !iop || iop->isEmpty() )
	iop = setts.subselect( sKeyDefCategory() );

    if ( iop && iop->size() )
	useAppearancePar( *iop );
}


StepInterval<double> FlatView::Viewer::getDataPackRange( bool forx1 ) const
{
    const bool wva = appearance().ddpars_.wva_.show_;
    const WeakPtr<FlatDataPack> datapack = getPack( wva, true );
    if ( !datapack )
	return StepInterval<double>(mUdf(double),mUdf(double),1);
    ConstRefMan<FlatDataPack> dp = datapack.get();
    return dp->posData().range( forx1 );
}


Interval<float> FlatView::Viewer::getDataRange( bool iswva ) const
{
    Interval<float> rg( mUdf(float), mUdf(float) );
    const ColTab::MapperSetup mapper =
	iswva ? appearance().ddpars_.wva_.mappersetup_
	      : appearance().ddpars_.vd_.mappersetup_;
    Interval<float> mapperrange = mapper.range_;
    return mapperrange;
}



void FlatView::Viewer::setSeisGeomidsToViewer( TypeSet<Pos::GeomID>& geomids )
{
    geom2dids_ = geomids;
}


const TypeSet<Pos::GeomID>& FlatView::Viewer::getAllSeisGeomids() const
{
    return geom2dids_;
}


FlatView::Viewer::VwrDest FlatView::Viewer::getDest( bool dowva, bool dovd )
{
    if ( !dowva && !dovd )
	return None;

    return dowva && dovd ? Both : (dowva ? WVA : VD);
}


void FlatView::Viewer::addPack( DataPackID id )
{
}


const FlatDataPack* FlatView::Viewer::obtainPack(
				      bool wva, bool checkother ) const
{
    Threads::Locker locker( lock_ );
    ConstRefMan<FlatDataPack> res = wva ? wvapack_.get() : vdpack_.get();
    if ( !res && checkother )
	res = wva ? vdpack_.get() : wvapack_.get();

    refPtr( res.ptr() );
    dpm_.add<FlatDataPack>( res.ptr() );
    return res.ptr();
}


void FlatView::Viewer::removePack( DataPackID id )
{
    const bool wva = hasPack(true) && packID(true)==id;
    const bool vd = hasPack(false) && packID(false)==id;
    const VwrDest dest = getDest( wva, vd );
    removePack( dest );
}


void FlatView::Viewer::removeUnusedPacks()
{
}


void FlatView::Viewer::setPack( bool wva, ::DataPackID id, bool usedefs )
{
    setPack( wva ? WVA : VD, dpm_.get<FlatDataPack>(id).ptr() , usedefs );
}


void FlatView::Viewer::setPack( VwrDest dest, ::DataPackID id, bool usedefs )
{
    if ( dest == None )
	return;

    setPack( dest, dpm_.get<FlatDataPack>(id).ptr() , usedefs );
}


void FlatView::Viewer::usePack( bool wva, ::DataPackID id, bool usedefs )
{
    setPack( wva ? WVA : VD, dpm_.get<FlatDataPack>(id).ptr() , usedefs );
}


void FlatView::Viewer::usePack( VwrDest dest, DataPackID id, bool usedefs )
{
    if ( dest == None )
	return;

    setPack( dest, dpm_.get<FlatDataPack>(id).ptr() , usedefs );
}


void FlatView::Viewer::setVisible( bool wva, bool visibility )
{
    setVisible( wva ? WVA : VD, visibility );
}


const TypeSet< ::DataPackID>& FlatView::Viewer::availablePacks() const
{
    return ids_;
}


bool FlatView::Viewer::isAvailable( ::DataPackID id ) const
{
    return packID(true)==id || packID(false)==id;
}

