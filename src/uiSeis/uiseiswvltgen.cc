/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltgen.cc,v 1.5 2009-09-14 14:36:30 cvsbruno Exp $";


#include "uiseiswvltgen.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "iodirentry.h"
#include "ioman.h"
#include "interpol1d.h"
#include "mathfunc.h"
#include "survinfo.h"
#include "wavelet.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiwindowfuncseldlg.h"
#include "uiworld2ui.h"
#include "uimsg.h"


uiSeisWvltCreate::uiSeisWvltCreate( uiParent* p, uiDialog::Setup su ) 
	: uiDialog(p,su)
	, ctio_(*mMkCtxtIOObj(Wavelet))
{
    ctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, ctio_ );
}
	

uiSeisWvltCreate::~uiSeisWvltCreate()
{
    delete ctio_.ioobj; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisWvltCreate::putWvlt( const Wavelet& wvlt )
{
    if ( !wvltfld_->commitInput() )
	mErrRet( "Please enter a name for the new Wavelet" );

    if ( !wvlt.put(ctio_.ioobj) )
	mErrRet( "Cannot write wavelet" )

    return true;
}


MultiID uiSeisWvltCreate::storeKey() const
{
    return ctio_.ioobj ? ctio_.ioobj->key() : MultiID("");
}




uiSeisWvltGen::uiSeisWvltGen( uiParent* p )
    : uiSeisWvltCreate(p,uiDialog::Setup("Create Wavelet",
				 "Specify wavelet creation parameters",
				 "103.3.2"))
{
    isrickfld_ = new uiGenInput( this, "Wavelet type",
				BoolInpSpec(true,"Ricker","Sinc") );

    const float sisr = SI().zStep();
    float deffrq = 0.1 / sisr; int ideffr = mNINT(deffrq);
    if ( ideffr > 0 && mIsZero(deffrq-ideffr,1e-4) )
	deffrq = ideffr; // avoid awkward 99.999 display
    BufferString txt( "Central " );
    txt += SI().zIsTime() ? "Frequency" : "Wavenumber";
    freqfld_ = new uiGenInput( this, txt, FloatInpSpec(deffrq) );
    freqfld_->attach( alignedBelow, isrickfld_ );

    const float usrsr = sisr * SI().zFactor();
    txt = "Sample interval "; txt += SI().getZUnitString();
    srfld_ = new uiGenInput( this, txt, FloatInpSpec(usrsr) );
    srfld_->attach( alignedBelow, freqfld_ );

    peakamplfld_ = new uiGenInput( this, "Peak amplitude", FloatInpSpec(1) );
    peakamplfld_->attach( alignedBelow, srfld_ );
   
    wvltfld_->attach( alignedBelow, peakamplfld_ );
}


bool uiSeisWvltGen::acceptOK( CallBacker* )
{
    const float sr = srfld_->getfValue();
    const float peakampl = peakamplfld_->getfValue();
    const float freq = freqfld_->getfValue();

    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( "The sample interval is not valid" )
    else if ( peakampl == 0 )
	mErrRet( "The peak amplitude must be non-zero" )
    else if ( mIsUdf(freq) || freq <= 0 )
	mErrRet( "The frequency must be positive" )

    const float realsr = sr / SI().zFactor();
    Wavelet wvlt( isrickfld_->getBoolValue(), freq, realsr, peakampl );
    putWvlt( wvlt );

    return true;
}



uiSeisWvltMerge::uiSeisWvltMerge( uiParent* p, const char* curwvltnm )
    : uiSeisWvltCreate(p,uiDialog::Setup("Merge Wavelets",
				 "Select two ore more wavelets to be stacked",
				 mTODOHelpID))
    , maxwvltsize_(0)					      
    , stackedwvlt_(0)					      
    , wvltdrawer_(0)
{
    IODirEntryList del( IOM().dirPtr(), ctio_.ctxt );

    BufferStringSet namelist;
    float minhght=0; float maxhght=0;
    for ( int delidx=0; delidx<del.size(); delidx++ )
    {
	const IOObj* ioobj = del[delidx]->ioobj;
	if ( !ioobj ) continue;

	wvltset_ += Wavelet::get( ioobj );
	namelist.add( ioobj->name() );
	const int wvltsz = wvltset_[delidx]->size();
	if ( wvltsz > maxwvltsize_ ) maxwvltsize_ = wvltsz-1;
	for ( int idx=0; idx<wvltsz; idx++ )
	{
	    const float val = wvltset_[delidx]->samples()[idx];
	    if ( minhght < val ) minhght = val;
	    if ( maxhght > val ) maxhght = val;
	}
    }
    const StepInterval<float> xaxrg( 0, maxwvltsize_, 1);
    const StepInterval<float> yaxrg( maxhght, minhght, ( maxhght-minhght )/8);
    uiFuncSelDraw::Setup su; su.name_ = "Wavelet Stacking";
    su.xaxrg_ = xaxrg; su.yaxrg_ = yaxrg;

    wvltdrawer_ = new uiFuncSelDraw( this, su );
    for ( int idx=0; idx<wvltset_.size(); idx++ )
    {
	wvltdrawer_->addToList( namelist[idx]->buf() );
	wvltfuncset_ += new WvltMathFunction( wvltset_[idx] );
	wvltdrawer_->addFunction( wvltfuncset_[idx] );
    }
    wvltdrawer_->setAsCurrent( curwvltnm );

    wvltfld_->setLabelText("Save stacked wavelet");
    wvltfld_->attach( ensureBelow, wvltdrawer_ );
    wvltfld_->setSensitive( false );

    wvltdrawer_->funclistselChged.notify(mCB(this,uiSeisWvltMerge,funcSelChg));
}


uiSeisWvltMerge::~uiSeisWvltMerge()
{
    wvltdrawer_->funclistselChged.remove( mCB(this,uiSeisWvltMerge,funcSelChg));
    deepErase( wvltset_ );
    deepErase( wvltfuncset_ );
}


void uiSeisWvltMerge::funcSelChg( CallBacker* )
{
    NotifyStopper nsf( wvltdrawer_->funclistselChged );
    TypeSet<int> selitems;
    wvltdrawer_->getSelectedItems( selitems );
    const int selsz = selitems.size();
    if ( selsz ==1 && wvltdrawer_->isSelected( wvltdrawer_->getListSize()-1 ) ) 
	return;
    clearStackedWvlt();
    wvltfld_->setSensitive( selsz > 1 );
    if ( selsz <= 1 ) return;
    stackWvlts( selitems );
}


void uiSeisWvltMerge::clearStackedWvlt()
{
    if ( stackedwvlt_ )
    {
	wvltset_.remove( wvltset_.size()-1 );
	delete wvltfuncset_.remove( wvltdrawer_->removeLastItem() );
	delete stackedwvlt_; stackedwvlt_=0;
    }
}


void uiSeisWvltMerge::stackWvlts( TypeSet<int>& selitems )
{
    const int selsize = selitems.size();

    const char* wvltname = "Stacked Wavelet";
    stackedwvlt_ = new Wavelet( wvltname );
    wvltset_ += stackedwvlt_;
    stackedwvlt_->reSize( maxwvltsize_ );
    for ( int idx=0; idx<maxwvltsize_; idx++ )
	stackedwvlt_->samples()[idx] = 0;

    for ( int selidx=0; selidx<selsize; selidx++ )
    {
	Wavelet* curwvlt = wvltset_[selitems[selidx]];
	for ( int idx=0; idx<maxwvltsize_; idx++ )
	{
	    const float val = idx<curwvlt->size() ? curwvlt->samples()[idx] : 0;
	    stackedwvlt_->samples()[idx] += val/selsize; 
	}
    }
    WvltMathFunction* stackedfunc = new WvltMathFunction( stackedwvlt_ );
    wvltfuncset_ += stackedfunc;

    wvltdrawer_->addToList( wvltname );
    wvltdrawer_->addFunction( stackedfunc );
    wvltdrawer_->setAsCurrent( wvltname );
}


bool uiSeisWvltMerge::acceptOK( CallBacker* )
{
    if ( !stackedwvlt_ )
	mErrRet( "Please, select 2 or more wavelets to merge wavelet" );
    return putWvlt( *stackedwvlt_ );
}



uiSeisWvltMerge::WvltMathFunction::WvltMathFunction( const Wavelet* wvlt )
    		: samples_(wvlt->samples())
		, size_(wvlt->size())  
		{}  

float uiSeisWvltMerge::WvltMathFunction::getValue( float x ) const
{
    const int x1 = (int)(x);
    if ( x1 > size_-1 )
	return 0;
    else if ( x1 == size_-1 )
	return samples_[x1];
    const float val1 = samples_[x1];
    const int x2 = x1+1;
    const float val2 = samples_[x2];

    const float factor = ( x-x1 )/( x2-x1 ); 
    return Interpolate::linearReg1D( val1, val2, factor );
}

