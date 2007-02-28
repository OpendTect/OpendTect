/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2000
 RCS:           $Id: flatview.cc,v 1.4 2007-02-28 12:49:20 cvsbert Exp $
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
	{ delete [] x1pos_; x1pos_ = 0; }
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
    if ( x1sz < 1 )
	return IndexInfo( x1rg_, x );

    return IndexInfo( x1pos_, x1sz, (float)(x-x1offs_) );
}


double FlatPosData::position( bool isx1, int idx ) const
{
    return !isx1 || !x1pos_ || idx >= nrPts(true) ? range(isx1).atIndex(idx)
						  : x1pos_[idx] + x1offs_;
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
    : rg_(mUdf(float),mUdf(float))
    , clipperc_(ColorTable::defPercClip())
    , blocky_(false)
{
}


void FlatView::Viewer::getAuxInfo( const Point& pt, IOPar& iop ) const
{
    BufferString txt( context().annot_.x1_.name_ );
    txt += " vs "; txt += context().annot_.x2_.name_;
    iop.set( "Positioning", txt );
    if ( data().isEmpty() )
	return;

    addAuxInfo( true, pt, iop );
    addAuxInfo( false, pt, iop );
}


void FlatView::Viewer::addAuxInfo( bool iswva, const Point& pt,
				   IOPar& iop ) const
{
    const Array2D<float>* arr = iswva ? data().wvaarr() : data().vdarr();
    const char* nm = iswva ? data().wvaname() : data().vdname();
    if ( !arr ) return;

    iop.set( iswva ? "Wiggle/VA data" : "Variable density data", nm );

    const Array2DInfoImpl& info = arr->info();
    const FlatPosData& pd = iswva ? context().wvaposdata_ :context().vdposdata_;
    const IndexInfo ix = pd.indexInfo( true, pt.x );
    const IndexInfo iy = pd.indexInfo( false, pt.y );
    if ( !ix.inundef_ && !iy.inundef_ )
    {
	if ( arr )
	    iop.set( nm, arr->get( ix.nearest_, iy.nearest_ ) );
	data().addAuxInfo( iswva, ix.nearest_, iy.nearest_, iop );
    }
}


FlatView::Annotation::Annotation( bool drkbg )
    : color_(drkbg ? Color::White : Color::Black)
    , showaux_(true)
{
    x1_.name_ = "X1";
    x2_.name_ = "X2";
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
    mIOPDoVD( setYN, sKeyShow, dispvd_ );
    mIOPDoVD2( set, sKeyDispRg, vd_.rg_.start, vd_.rg_.stop );
    mIOPDoVD( set, sKeyColTab, vd_.ctab_ );
    mIOPDoVD( setYN, sKeyBlocky, vd_.blocky_ );
    mIOPDoVD( set, sKeyClipPerc, vd_.clipperc_ );

    mIOPDoWVA( setYN, sKeyShow, dispwva_ );
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
    mIOPDoVD( getYN, sKeyShow, dispvd_ );
    mIOPDoVD2( get, sKeyDispRg, vd_.rg_.start, vd_.rg_.stop );
    mIOPDoVD( get, sKeyColTab, vd_.ctab_ );
    mIOPDoVD( getYN, sKeyBlocky, vd_.blocky_ );
    mIOPDoVD( get, sKeyClipPerc, vd_.clipperc_ );

    mIOPDoWVA( getYN, sKeyShow, dispwva_ );
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


void FlatView::Context::fillPar( IOPar& iop ) const
{
    annot_.fillPar( iop );
    ddpars_.fillPar( iop );
}


void FlatView::Context::usePar( const IOPar& iop )
{
    annot_.usePar( iop );
    ddpars_.usePar( iop );
}


void FlatView::Context::setDarkBG( bool yn )
{
    darkbg_ = yn;
    annot_.color_ = yn ? Color::White : Color::Black;
    ddpars_.wva_.wigg_ = annot_.color_;
}


bool FlatView::Data::set( bool wva, const Array2D<float>* a, const char* nm )
{
    bool chgd = false;

    if ( wva )
    {
	chgd = a != wvaarr_;
	wvaarr_ = a;
	wvaname_ = nm;
    }
    else
    {
	chgd = a != vdarr_;
	vdarr_ = a;
	vdname_ = nm;
    }

    return chgd;
}


FlatView::Context& FlatView::Viewer::context()
{
    if ( !defctxt_ )
	defctxt_ = new FlatView::Context;
    return *defctxt_;
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


void FlatView::Viewer::setPack( bool wva, DataPack::ID id )
{
    const DataPack* pack = DPM(DataPackMgr::FlatID).obtain(id);
    mDynamicCastGet(const FlatDataPack*,fdp,pack)
    setPack( wva, fdp );
}


void FlatView::Viewer::setPack( bool wva, const FlatDataPack* newpack )
{
    FlatView::Data& dd = data();
    mDynamicCastGet(FlatView::FlDPackData*,fddpdata,&dd)
    bool issame = !newpack && !fddpdata;
    if ( fddpdata )
    {
	const FlatDataPack*& pack = wva ? fddpdata->wvapack_ :fddpdata->vdpack_;
	issame = newpack == pack;
	if ( pack && !issame )
	    DPM(DataPackMgr::FlatID).release(pack);
	pack = newpack;
    }

    if ( !issame )
    {
	if ( !newpack )
	    dd.set( wva, 0, "" );
	else
	{
	    dd.set( wva, &newpack->data(), newpack->name().buf() );
	    FlatView::Context& ctxt = context();
	    (wva ? ctxt.wvaposdata_ : ctxt.vdposdata_) = newpack->posData();
	    if ( fddpdata )
	    {
		const FlatDataPack& fdp = wva	? *fddpdata->wvapack_
		    				: *fddpdata->vdpack_;
		FlatView::Annotation& annot = ctxt.annot_;
		annot.x1_.name_ = fdp.dimName( true );
		annot.x2_.name_ = fdp.dimName( false );
	    }
	    useStoredDefaults( newpack->category() );
	}
    }

    handleChange( wva ? WVAData : VDData );
}


const FlatDataPack* FlatView::Viewer::getPack( bool wva ) const
{
    const FlatView::Data& dd = data();
    mDynamicCastGet(const FlatView::FlDPackData*,fddpdata,&dd)
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
