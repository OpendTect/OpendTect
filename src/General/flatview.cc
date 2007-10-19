/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2000
 RCS:           $Id: flatview.cc,v 1.25 2007-10-19 12:19:06 cvsnanne Exp $
________________________________________________________________________

-*/

#include "flatview.h"
#include "arrayndimpl.h"
#include "settings.h"
#include "survinfo.h"
#include "keystrs.h"
#include "colortab.h"
#include "datapackbase.h"


namespace FlatView
{

const char* Annotation::sKeyAxes = "Axes";
const char* Annotation::sKeyShwAnnot = "Show annotation";
const char* Annotation::sKeyShwGridLines = "Show grid lines";
const char* Annotation::sKeyIsRev = "Reversed";
const char* Annotation::sKeyShwAux = "Show aux data";
const char* DataDispPars::sKeyVD = "VD";
const char* DataDispPars::sKeyWVA = "WVA";
const char* DataDispPars::sKeyShow = "Show";
const char* DataDispPars::sKeyDispRg = "Range";
const char* DataDispPars::sKeyColTab = "Color Table";
const char* DataDispPars::sKeyBlocky = "Blocky";
const char* DataDispPars::sKeyClipPerc = "Percentage Clip";
const char* DataDispPars::sKeyWiggCol = "Wiggle color";
const char* DataDispPars::sKeyMidCol = "Mid color";
const char* DataDispPars::sKeyLeftCol = "Left color";
const char* DataDispPars::sKeyRightCol = "Right color";
const char* DataDispPars::sKeyOverlap = "Overlap";
const char* DataDispPars::sKeyMidValue = "Mid value";

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
	memcpy( x1pos_, fpd.x1pos_, sz * sizeof( float ) );
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
    res.setCapacity( nrtimes );
    for ( int idx=0; idx<nrtimes; idx++ )
	res += position( isx1, idx );
}


float* FlatPosData::getPositions( bool isx1 ) const
{
    const int sz = nrPts( isx1 );
    if ( sz < 1 ) return 0;

    float* ret = new float [sz];
    if ( isx1 && x1pos_ )
	memcpy( ret, x1pos_, sz * sizeof(float) );
    else
    {
	const StepInterval<double>& xrg = range( isx1 );
	for ( int idx=0; idx<sz; idx++ )
	    ret[idx] = xrg.atIndex( idx );
    }
    return ret;
}


FlatView::DataDispPars::Common::Common()
    : show_(true)
    , rg_(mUdf(float),mUdf(float))
    , clipperc_(ColorTable::defPercClip())
    , blocky_(false)
    , midvalue_( mUdf(float) )
{
}


void FlatView::Viewer::getAuxInfo( const Point& pt, IOPar& iop ) const
{
    BufferString txt( appearance().annot_.x1_.name_ );
    txt += " vs "; txt += appearance().annot_.x2_.name_;
    iop.set( "Positioning", txt );
    if ( data().isEmpty() )
	return;

    addAuxInfo( true, pt, iop );
    addAuxInfo( false, pt, iop );
}


void FlatView::Viewer::addAuxInfo( bool iswva, const Point& pt,
				   IOPar& iop ) const
{
    const Array2D<float>* arr = iswva ? data().wvaArr() : data().vdArr();
    if ( !arr ) return;

    const char* nm = iswva ? data().wvaName() : data().vdName();
    iop.set( iswva ? "Wiggle/VA data" : "Variable density data", nm );

    const Array2DInfoImpl& info = arr->info();
    const FlatPosData& pd = data().pos( iswva );
    const IndexInfo ix = pd.indexInfo( true, pt.x );
    const IndexInfo iy = pd.indexInfo( false, pt.y );
    if ( !ix.inundef_ && !iy.inundef_ )
    {
	if ( arr )
	    iop.set( nm, arr->get( ix.nearest_, iy.nearest_ ) );
	data().addAuxInfo( iswva, ix.nearest_, iy.nearest_, iop );
	if ( getPack(iswva) )
	    getPack(iswva)->getAuxInfo( ix.nearest_, iy.nearest_, iop );
    }
}


FlatView::Annotation::Annotation( bool drkbg )
    : color_(drkbg ? Color::White : Color::Black)
    , showaux_(true)
{
    x1_.name_ = "X1";
    x2_.name_ = "X2";
}


FlatView::Annotation::~Annotation()
{
    deepErase( auxdata_ );
}


bool FlatView::Annotation::haveAux() const
{
    if ( !showaux_ ) return false;

    for ( int idx=auxdata_.size()-1; idx>=0; idx-- )
    {
	if ( auxdata_[idx]->enabled_ )
	{
	    return true;
	}
    }

    return false;
}

#define mIOPDoAxes(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyAxes,keynm), memb )
#define mIOPDoAxes2(fn,keynm,memb1,memb2) \
    iop.fn( IOPar::compKey(sKeyAxes,keynm), memb1, memb2 )


void FlatView::Annotation::fillPar( IOPar& iop ) const
{
    mIOPDoAxes( set, sKey::Color, color_ );
    mIOPDoAxes2( set, sKey::Name, x1_.name_, x2_.name_ );
    mIOPDoAxes2( setYN, sKeyShwAnnot, x1_.showannot_, x2_.showannot_ );
    mIOPDoAxes2( setYN, sKeyShwGridLines,x1_.showgridlines_,x2_.showgridlines_);
    mIOPDoAxes2( setYN, sKeyIsRev, x1_.reversed_, x2_.reversed_ );
    iop.setYN( sKeyShwAux, showaux_ );
}


void FlatView::Annotation::usePar( const IOPar& iop )
{
    mIOPDoAxes( get, sKey::Color, color_ );
    mIOPDoAxes2( get, sKey::Name, x1_.name_, x2_.name_ );
    mIOPDoAxes2( getYN, sKeyShwAnnot, x1_.showannot_, x2_.showannot_ );
    mIOPDoAxes2( getYN, sKeyShwGridLines,x1_.showgridlines_,x2_.showgridlines_);
    mIOPDoAxes2( getYN, sKeyIsRev, x1_.reversed_, x2_.reversed_ );
    iop.getYN( sKeyShwAux, showaux_ );
}


FlatView::Annotation::AuxData::EditPermissions::EditPermissions()
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



FlatView::Annotation::AuxData::AuxData( const char* nm )
    : name_( nm )
    , namepos_( mUdf(int) )
    , linestyle_( LineStyle::None, 1, Color::NoColor )
    , fillcolor_( Color::NoColor )
    , markerstyle_( MarkerStyle2D::None )
    , close_( false )
    , x1rg_( 0 )
    , x2rg_( 0 )
    , enabled_( true )
    , editpermissions_( 0 )
{}


FlatView::Annotation::AuxData::AuxData(const FlatView::Annotation::AuxData& aux)
    : name_( aux.name_ )
    , namepos_( aux.namepos_ )
    , linestyle_( aux.linestyle_ )
    , fillcolor_( aux.fillcolor_ )
    , markerstyle_( aux.markerstyle_ )
    , close_( aux.close_ )
    , x1rg_( aux.x1rg_ ? new Interval<double>( *aux.x1rg_ ) : 0 )
    , x2rg_( aux.x2rg_ ? new Interval<double>( *aux.x2rg_ ) : 0 )
    , enabled_( aux.enabled_ )
    , editpermissions_( aux.editpermissions_
	    ? new EditPermissions(*aux.editpermissions_) : 0 )
    , poly_( aux.poly_ )
{}


FlatView::Annotation::AuxData::~AuxData()
{
    delete x1rg_;
    delete x2rg_;
    delete editpermissions_;
}


bool FlatView::Annotation::AuxData::isEmpty() const
{ return poly_.isEmpty(); }


void FlatView::Annotation::AuxData::empty()
{ poly_.erase(); }


#define mIOPDoWVA(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyWVA,keynm), memb )
#define mIOPDoWVA2(fn,keynm,memb1,memb2) \
    iop.fn( IOPar::compKey(sKeyWVA,keynm), memb1, memb2 )
#define mIOPDoVD(fn,keynm,memb) \
    iop.fn( IOPar::compKey(sKeyVD,keynm), memb )
#define mIOPDoVD2(fn,keynm,memb1,memb2) \
    iop.fn( IOPar::compKey(sKeyVD,keynm), memb1, memb2 )

void FlatView::DataDispPars::fillPar( IOPar& iop ) const
{
    mIOPDoVD( setYN, sKeyShow, vd_.show_ );
    mIOPDoVD2( set, sKeyDispRg, vd_.rg_.start, vd_.rg_.stop );
    mIOPDoVD( set, sKeyColTab, vd_.ctab_ );
    mIOPDoVD( setYN, sKeyBlocky, vd_.blocky_ );
    mIOPDoVD( set, sKeyClipPerc, vd_.clipperc_ );
    mIOPDoVD( set, sKeyMidValue, vd_.midvalue_ );

    mIOPDoWVA( setYN, sKeyShow, wva_.show_ );
    mIOPDoWVA2( set, sKeyDispRg, wva_.rg_.start, wva_.rg_.stop );
    mIOPDoWVA( setYN, sKeyBlocky, wva_.blocky_ );
    mIOPDoWVA( set, sKeyClipPerc, wva_.clipperc_ );
    mIOPDoWVA( set, sKeyWiggCol, wva_.wigg_ );
    mIOPDoWVA( set, sKeyMidCol, wva_.mid_ );
    mIOPDoWVA( set, sKeyLeftCol, wva_.left_ );
    mIOPDoWVA( set, sKeyRightCol, wva_.right_ );
    mIOPDoWVA( set, sKeyOverlap, wva_.overlap_ );
    mIOPDoWVA( set, sKeyMidValue, wva_.midvalue_ );
}


void FlatView::DataDispPars::usePar( const IOPar& iop )
{
    mIOPDoVD( getYN, sKeyShow, vd_.show_ );
    mIOPDoVD2( get, sKeyDispRg, vd_.rg_.start, vd_.rg_.stop );
    mIOPDoVD( get, sKeyColTab, vd_.ctab_ );
    mIOPDoVD( getYN, sKeyBlocky, vd_.blocky_ );
    mIOPDoVD( get, sKeyClipPerc, vd_.clipperc_ );
    mIOPDoVD( get, sKeyMidValue, vd_.midvalue_ );

    mIOPDoWVA( getYN, sKeyShow, wva_.show_ );
    mIOPDoWVA2( get, sKeyDispRg, wva_.rg_.start, wva_.rg_.stop );
    mIOPDoWVA( getYN, sKeyBlocky, wva_.blocky_ );
    mIOPDoWVA( get, sKeyClipPerc, wva_.clipperc_ );
    mIOPDoWVA( get, sKeyWiggCol, wva_.wigg_ );
    mIOPDoWVA( get, sKeyMidCol, wva_.mid_ );
    mIOPDoWVA( get, sKeyLeftCol, wva_.left_ );
    mIOPDoWVA( get, sKeyRightCol, wva_.right_ );
    mIOPDoWVA( get, sKeyOverlap, wva_.overlap_ );
    mIOPDoWVA( get, sKeyMidValue, wva_.midvalue_ );
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
    annot_.color_ = yn ? Color::White : Color::Black;
    ddpars_.wva_.wigg_ = annot_.color_;
}


FlatView::Appearance& FlatView::Viewer::appearance()
{
    if ( !defapp_ )
	defapp_ = new FlatView::Appearance;
    return *defapp_;
}


namespace FlatView
{
struct FlDPackData : public Data
{

FlDPackData() : wvapack_(0), vdpack_(0)
{
}

void addAuxInfo( bool wva, int i0, int i1, IOPar& par ) const
{
    const FlatDataPack* pack = wva ? wvapack_ : vdpack_;
    if ( pack )
	pack->getAuxInfo( i0, i1, par );
}

    const FlatDataPack*	wvapack_;
    const FlatDataPack*	vdpack_;

};
}


FlatView::Data& FlatView::Viewer::data()
{
    if ( !defdata_ )
	defdata_ = new FlatView::FlDPackData;
    return *defdata_;
}


void FlatView::Viewer::setPack( bool wva, DataPack::ID id, bool usedefs )
{
    mObtainDataPackToLocalVar(fdp,const FlatDataPack*,DataPackMgr::FlatID,id);
    doSetPack( wva, fdp, usedefs );
}


void FlatView::Viewer::setPack( bool wva, const FlatDataPack* newpack,
				bool usedefs )
{
    doSetPack( wva, newpack, usedefs );
}


void FlatView::Viewer::chgPack( bool wva, const FlatDataPack* newpack )
{
    doSetPack( wva, newpack, false );
}


void FlatView::Viewer::doSetPack( bool wva, const FlatDataPack* newpack,
				  bool usedefs )
{
    FlatView::Data& dta = data();
    mDynamicCastGet(FlatView::FlDPackData*,fddpdata,&dta)
    bool issame = !newpack && !fddpdata;
    if ( fddpdata )
    {
	const FlatDataPack*& pack = wva ? fddpdata->wvapack_ :fddpdata->vdpack_;
	issame = newpack == pack;
	if ( pack && !issame )
	    DPM(DataPackMgr::FlatID).release(pack);
	pack = newpack;
    }

    if ( issame ) return;

    if ( !newpack )
	dta.setEmpty( wva );
    else
    {
	if ( usedefs )
	    useStoredDefaults( newpack->category() );

	FlatView::PackData& pd = wva ? dta.wva_ : dta.vd_;
	pd.pos_ = newpack->posData();
	pd.arr_ = &newpack->data();
	pd.name_ = newpack->name();
	if ( fddpdata )
	{
	    const FlatDataPack& fdp = wva ? *fddpdata->wvapack_
					  : *fddpdata->vdpack_;
	    FlatView::Annotation& annot = appearance().annot_;
	    annot.x1_.name_ = fdp.dimName( true );
	    annot.x2_.name_ = fdp.dimName( false );
	}
    }

    handleChange( wva ? WVAData : VDData );
}


bool FlatView::Viewer::isVisible( bool wva ) const
{
    if ( wva )
	return appearance().ddpars_.wva_.show_ && data().wvaArr();
    else
        return appearance().ddpars_.vd_.show_ && data().vdArr();
}


void FlatView::Viewer::syncDataPacks()
{
    const FlatDataPack* wvapack = getPack( true );
    const FlatDataPack* vdpack = getPack( false );
    if ( !wvapack && !vdpack ) return;
    FlatView::Data& dta = data();
    mDynamicCastGet(FlatView::FlDPackData*,fddpdata,&dta)
    if ( !fddpdata )
	return;

    DataPackMgr& dpm = DPM( DataPackMgr::FlatID );
    const Array2D<float>* curwvaarr = data().arr(true);
    const Array2D<float>* curvdarr = data().arr(false);
    const Array2D<float>* packwvaarr = wvapack ? &wvapack->data() : 0;
    const Array2D<float>* packvdarr = vdpack ? &vdpack->data() : 0;
    if ( (curwvaarr == packwvaarr && curvdarr == packvdarr)
      || (!packwvaarr && !packvdarr) )
	return;
    else if ( !curwvaarr && !curvdarr )
    {
	// No packs used
	dpm.release(fddpdata->wvapack_); dpm.release(fddpdata->vdpack_);
	chgPack( true, 0 ); chgPack( false, 0 );
	return;
    }
    else if ( curwvaarr == packvdarr && curvdarr == packwvaarr )
    {
	// Packs have switched
	const FlatDataPack* newwvapack = fddpdata->vdpack_;
	const FlatDataPack* newvdpack = fddpdata->wvapack_;
	chgPack( true, newwvapack ); chgPack( false, newvdpack );
	return;
    }

    if ( curwvaarr != packwvaarr && curvdarr != packwvaarr )
    {
	dpm.release( fddpdata->wvapack_ );
	chgPack( true, curwvaarr == packvdarr ? fddpdata->vdpack_ : 0 );
    }
    if ( curwvaarr != packvdarr && curvdarr != packvdarr )
    {
	dpm.release( fddpdata->vdpack_ );
	chgPack( false, curvdarr == packwvaarr ? fddpdata->wvapack_ :0 );
    }
}


const FlatDataPack* FlatView::Viewer::getPack( bool wva ) const
{
    const FlatView::Data& dta = data();
    mDynamicCastGet(const FlatView::FlDPackData*,fddpdata,&dta)
    return fddpdata ? (wva ? fddpdata->wvapack_ : fddpdata->vdpack_) : 0;
}


void FlatView::Viewer::storeDefaults( const char* ky ) const
{
    Settings& setts = Settings::fetch( "flatview" );
    IOPar iop; fillPar( iop );
    setts.mergeComp( iop, ky );
    setts.write();
}


void FlatView::Viewer::useStoredDefaults( const char* ky )
{
    Settings& setts = Settings::fetch( "flatview" );
    IOPar* iop = setts.subselect( ky );
    if ( iop && iop->size() )
	usePar( *iop );
    delete iop;
}
