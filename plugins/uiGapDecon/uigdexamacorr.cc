/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep  2006
 RCS:           $Id: uigdexamacorr.cc,v 1.7 2006-10-06 07:21:29 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigdexamacorr.h"
#include "uigapdeconattrib.h"
#include "gapdeconattrib.h"

#include "attribparam.h"
#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "attribdatacubes.h"
#include "uiexecutor.h"
#include "arrayndimpl.h"
#include "uimsg.h"
#include "ptrman.h"
#include "uiflatvertview.h"
#include "colortab.h"

using namespace Attrib;

GapDeconACorrView::GapDeconACorrView( uiParent* p )
    : uiMainWin( p, "Auto-correlation viewer", 0, false, true )
    , attribid_( DescID::undef() )
    , autocorr2darr_( 0 )
    , viewer2d_( 0 )
    , dset_( 0 )
{
}


GapDeconACorrView::~GapDeconACorrView()
{
    if ( autocorr2darr_ )
	delete autocorr2darr_;
    
    if ( viewer2d_ )
	delete viewer2d_;
    
    if ( dset_ )
	delete dset_;
}


bool GapDeconACorrView::computeAutocorr()
{
    BufferString errmsg;
    PtrMan<EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createDataCubesOutput( errmsg, 0  );
    if ( !proc )
    {
	uiMSG().error( errmsg );
	return false;
    }

    proc->setName( "Compute autocorrelation values" );
    uiExecutor dlg( this, *proc );
    if ( !dlg.go() )
	return false;

    const Attrib::DataCubes* output = aem->getDataCubesOutput( *proc );
    if ( !output )
	return false;
    
    output->ref();
    extractAndSaveVals( output );
    output->unRef();
    return true;
}


EngineMan* GapDeconACorrView::createEngineMan()
{
    EngineMan* aem = new EngineMan;

    TypeSet<SelSpec> attribspecs;
    SelSpec sp( 0, attribid_ );
    attribspecs += sp;

    aem->setAttribSet( dset_ );
    aem->setAttribSpecs( attribspecs );
//    CubeSampling cs = cs_;
//    cs.hrg.step.inl = cs.hrg.step.inl*5;
    aem->setCubeSampling( cs_ );

    return aem;
}


void GapDeconACorrView::extractAndSaveVals( const DataCubes* dtcube )
{
    const Array3D<float>& data3darr = dtcube->getCube( 0 );
    int firstdimsz = cs_.nrInl()==1 ? cs_.nrCrl() : cs_.nrInl();
    autocorr2darr_ = new Array2DImpl<float>( firstdimsz, cs_.nrZ() );
    for ( int idfdim=0; idfdim<firstdimsz; idfdim++ )
    {
	for ( int idz=0; idz<cs_.nrZ(); idz++ )
	{
	    float val = cs_.nrInl()==1 ? data3darr.get( 0, idfdim, idz )
				       : data3darr.get( idfdim, 0, idz );
	    autocorr2darr_->set( idfdim, idz, val );
	}
    }
}


void GapDeconACorrView::createAndDisplay2DViewer()
{
    viewer2d_ = new uiFlatDisp::VertViewer(this);
    displayWiggles( false );

    FlatDisp::Context& ctxt = viewer2d_->context();
    ctxt.darkbg_ = true;
    ColorTable defctab( 0 );
    ctxt.ddpars_.vd_.ctab_ = defctab.name();
    ctxt.ddpars_.wva_.overlap_ = 0.8;
    ctxt.ddpars_.wva_.left_= Color(255,0,0);
    ctxt.ddpars_.wva_.right_= Color(0,0,255);
    ctxt.ddpars_.wva_.wigg_= Color(255,255,255);
    StepInterval<int> x1rg = cs_.nrInl()==1 ? cs_.hrg.crlRange() 
					    : cs_.hrg.inlRange();
    ctxt.posdata_.x1rg_ = StepInterval<double>(x1rg.start,x1rg.stop,x1rg.step);
    ctxt.posdata_.x2rg_ = 
		StepInterval<double>(0,cs_.zrg.stop-cs_.zrg.start, 
				     cs_.zrg.step);
    viewer2d_->setData( false, autocorr2darr_, "Seismic data");
    show();
}


void GapDeconACorrView::displayWiggles( bool yn )
{
    viewer2d_->context().ddpars_.dispvd_ = !yn;
    viewer2d_->context().ddpars_.dispwva_ = yn;
}


void GapDeconACorrView::setDescSet( Attrib::DescSet* ds )
{
    if ( dset_ )
	delete dset_;

    dset_ = ds;
}
