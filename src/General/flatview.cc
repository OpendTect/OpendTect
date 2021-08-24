/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2000
________________________________________________________________________

-*/
#define mNrExtraZDec 3

#include "flatview.h"
#include "flatposdata.h"
#include "arrayndimpl.h"
#include "settings.h"
#include "zaxistransform.h"
#include "randcolor.h"
#include "survinfo.h"

using namespace OD;

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

const char* Viewer::sKeyIsZSlice() { return "isZSlice"; };
const char* Viewer::sKeyWVAData() { return "Wiggle/VA data"; };
const char* Viewer::sKeyVDData() { return "Variable density data"; };
const char* Viewer::sKeyWVAVal() { return "WVA Value"; };
const char* Viewer::sKeyVDVal() { return "VD Value"; };
const char* Viewer::sKeyViewZnrDec() { return "Decimal places for Z Value"; };

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
    , hasannot_(true)
    , showannot_(false)
    , showgridlines_(false)
    , reversed_(false)
    , annotinint_(false)
    , factor_(1)
    , showauxannot_(true)
    , auxlinestyle_( OD::LineStyle(OD::LineStyle(OD::LineStyle::Dot)) )
    , auxhllinestyle_( OD::LineStyle(OD::LineStyle(OD::LineStyle::Dot,2,
					   getRandStdDrawColor())) )
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
    : dynamictitle_(true)
    , color_(drkbg ? Color::White() : Color::Black())
    , showaux_(true)
    , showscalebar_(false)
    , showcolorbar_(false)
    , editable_(false)
    , allowuserchange_(true)
    , allowuserchangereversedaxis_(true)
{
    x1_.name_ = "X1";
    x2_.name_ = "X2";
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
    : name_( nm )
    , namepos_( mUdf(int) )
    , namealignment_(mAlignment(Center,Center))
    , linestyle_( OD::LineStyle::None, 1, Color::NoColor() )
    , fillcolor_( Color::NoColor() )
    , zvalue_( 1 )
    , close_( false )
    , x1rg_( 0 )
    , x2rg_( 0 )
    , enabled_( true )
    , editpermissions_( 0 )
    , turnon_(true)
    , needsupdatelines_(true)
    , fitnameinview_(true)
{
}


FlatView::AuxData::AuxData( const FlatView::AuxData& aux )
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
    , editpermissions_( aux.editpermissions_
	    ? new EditPermissions(*aux.editpermissions_) : 0 )
    , poly_( aux.poly_ )
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

void FlatView::AuxData::FillGradient::set( const Point& fr, const Point& to,
		const TypeSet<float>& stops, const TypeSet<Color>& colors )
{
    from_ = fr; to_ = to;
    stops_ = stops; colors_ = colors;
}


void FlatView::AuxData::FillGradient::set( const Color& col1,
					   const Color& col2, bool hor )
{
    from_ = Point( 0., 0. );
    to_ = Point( hor ? 1. : 0., hor ? 0. : 1. );
    stops_.erase(); colors_.erase();
    stops_ += 0.f; stops_ += 1.f;
    colors_ += col1; colors_ += col2;
}


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
    Interval<float> clipperc( vd_.mappersetup_.cliprate_.start*100,
			      vd_.mappersetup_.cliprate_.stop*100 );
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
    vd_.mappersetup_.cliprate_.start *= 0.01;
    if ( mIsUdf(vd_.mappersetup_.cliprate_.stop) )
	vd_.mappersetup_.cliprate_.stop = vd_.mappersetup_.cliprate_.start;
    else
	vd_.mappersetup_.cliprate_.stop *= 0.01;

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
    , wvapack_(0)
    , vdpack_(0)
    , needstatusbarupd_(true)
    , zdinfo_(new ZDomain::Info(ZDomain::SI()))
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
	dpm_.release( ids_[idx] );
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
    BufferString txt( appearance().annot_.x1_.name_ );
    txt += " vs "; txt += appearance().annot_.x2_.name_;
    iop.set( "Positioning", txt );
    addAuxInfo( true, pt, iop );
    addAuxInfo( false, pt, iop );
}


void FlatView::Viewer::addAuxInfo( bool iswva, const Point& pt,
				   IOPar& iop ) const
{
    ConstDataPackRef<FlatDataPack> dp = obtainPack( iswva );
    if ( !dp )
    {
	iswva ? iop.removeWithKey( sKeyWVAData() )
	      : iop.removeWithKey( sKeyVDData() );
	iswva ? iop.removeWithKey( sKeyWVAVal() )
	      : iop.removeWithKey( sKeyVDVal() );
	    return;
    }
    const Array2D<float>& arr = dp->data();

    const char* nm = dp->name();
    iop.set( iswva ? sKeyWVAData() : sKeyVDData(), nm );
    const Array2DInfo& info = arr.info();
    const FlatPosData& pd = dp->posData();
    const IndexInfo ix = pd.indexInfo( true, pt.x );
    const IndexInfo iy = pd.indexInfo( false, pt.y );

    if ( !info.validPos(ix.nearest_, iy.nearest_) )
	return;

    const float val = arr.get( ix.nearest_, iy.nearest_ );
    iop.set( iswva ? sKeyWVAVal() : sKeyVDVal(), val );
    dp->getAuxInfo( ix.nearest_, iy.nearest_, iop );
    bool isvertical = false;
    iop.getYN( sKeyIsZSlice(), isvertical );
    if ( isvertical )
    {
	int nrdec = nrDec();
	iop.get( sKeyViewZnrDec(), nrdec );
	BufferString zstr;
	zstr.set( mCast(float,pt.y*zDomain().userFactor()), nrdec );
	iop.set( sKey::ZCoord(), zstr );
    }
}


int FlatView::Viewer::nrDec() const
{
    const float zstep = SI().zStep() * SI().zDomain().userFactor();
    if ( zstep > 1 ) return 0;
    return Math::NrSignificantDecimals( zstep ) + 1;
}


Coord3 FlatView::Viewer::getCoord( const Point& wp ) const
{
    ConstDataPackRef<FlatDataPack> fdp = obtainPack( false, true );
    if ( !fdp ) return Coord3::udf();

    const FlatPosData& pd = fdp->posData();
    const IndexInfo ix = pd.indexInfo( true, wp.x );
    const IndexInfo iy = pd.indexInfo( false, wp.y );
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
    const double xfac = ( wp.x - pd.position(true,floorx) ) /
		       ( pd.position(true,ceilx) - pd.position(true,floorx) );
    const double yfac = ( wp.y - pd.position(false,floory) ) /
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


FlatView::Appearance& FlatView::Viewer::appearance()
{
    if ( !defapp_ )
	defapp_ = new FlatView::Appearance;
    return *defapp_;
}


void FlatView::Viewer::addPack( DataPack::ID id )
{
    if ( ids_.isPresent(id) ) return;
    ids_ += id;
    dpm_.obtain( id );
}


const FlatDataPack* FlatView::Viewer::obtainPack(
				bool wva, bool checkother ) const
{
    Threads::Locker locker( lock_ );
    const FlatDataPack* res = wva ? wvapack_ : vdpack_;
    if ( !res && checkother )
	res = wva ? vdpack_ : wvapack_;
    dpm_.addAndObtain( const_cast<FlatDataPack*>(res) );
    return res;
}


DataPack::ID FlatView::Viewer::packID( bool wva ) const
{
    ConstDataPackRef<FlatDataPack> dp = obtainPack( wva );
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
    dpm_.release( id );
}


void FlatView::Viewer::usePack( bool wva, DataPack::ID id, bool usedefs )
{
    DataPack::ID curid = packID( wva );
    if ( id == curid ) return;

    if ( id == DataPack::cNoID() )
	(wva ? wvapack_ : vdpack_) = 0;
    else if ( !ids_.isPresent(id) )
    {
	pErrMsg("Requested usePack, but ID not added");
	return;
    }
    else
	(wva ? wvapack_ : vdpack_) = (FlatDataPack*)dpm_.observe( id );

    ConstDataPackRef<FlatDataPack> fdp = obtainPack( wva );
    if ( !fdp ) return;

    if ( usedefs )
	useStoredDefaults( fdp->category() );

    FlatView::Annotation& annot = appearance().annot_;
    if ( annot.x1_.name_.isEmpty() || annot.x1_.name_ == "X1" )
    {
	annot.x1_.name_ = fdp->dimName( true );
	BufferStringSet altdimnms; fdp->getAltDim0Keys( altdimnms );
	setAnnotChoice( altdimnms.indexOf(annot.x1_.name_) );
    }

    if ( annot.x2_.name_.isEmpty() || annot.x2_.name_ == "X2" )
    {
	annot.x2_.name_ = fdp->dimName( false );
	annot.x2_.annotinint_ = fdp->dimValuesInInt( annot.x2_.name_ );
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
    IOPar* iop = setts.subselect( ky );
    if ( iop && iop->size() )
	useAppearancePar( *iop );
    delete iop;
}


StepInterval<double> FlatView::Viewer::getDataPackRange( bool forx1 ) const
{
    const bool wva = appearance().ddpars_.wva_.show_;
    ConstDataPackRef<FlatDataPack> dp = obtainPack( wva, true );
    if ( !dp ) return StepInterval<double>(mUdf(double),mUdf(double),1);
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
