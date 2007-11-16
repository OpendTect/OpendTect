/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          January 2007
 RCS:           $Id: uirandlinegen.cc,v 1.2 2007-11-16 13:39:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uirandlinegen.h"
#include "emhorizon3d.h"
#include "emrandlinegen.h"
#include "emsurfacetr.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "emmanager.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "survinfo.h"
#include "uiexecutor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uimsg.h"


uiGenRanLinesByContour::uiGenRanLinesByContour( uiParent* p )
    : uiDialog( p, Setup("Create Random Lines","Specify generation parameters",
			 "0.0.0") )
    , horctio_(*mMkCtxtIOObj(EMHorizon3D))
    , rlsctio_(*mMkCtxtIOObj(RandomLineSet))
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    horctio_.setObj( IOM().getIfOnlyOne(mTranslGroupName(EMHorizon3D)) );
    rlsctio_.ctxt.forread = false;

    infld_ = new uiIOObjSel( this, horctio_, "Input Horizon" );

    StepInterval<float> sizrg( SI().zRange(true) );
    sizrg.scale( SI().zFactor() );
    contzrgfld_ = new uiGenInput( this, "Contour Z range",
			FloatInpIntervalSpec(sizrg) );
    contzrgfld_->attach( alignedBelow, infld_ );

    static const char* fldnm = "Random line Z range";
    const float wdth = 30 * sizrg.step;
    relzrgfld_ = new uiGenInput( this, fldnm,
			FloatInpIntervalSpec(Interval<float>(-wdth,wdth)) );
    relzrgfld_->attach( alignedBelow, contzrgfld_ );
    Interval<float> abszrg; assign( abszrg, sizrg );
    abszrgfld_ = new uiGenInput( this, fldnm, FloatInpIntervalSpec(abszrg) );
    abszrgfld_->attach( alignedBelow, contzrgfld_ );
    const CallBack cb( mCB(this,uiGenRanLinesByContour,isrelChg) );
    isrelfld_ = new uiCheckBox( this, "Relative to horizon", cb );
    isrelfld_->setChecked( true );
    isrelfld_->attach( rightOf, abszrgfld_ );

    outfld_ = new uiIOObjSel( this, rlsctio_, "Random Line set" );
    outfld_->attach( alignedBelow, relzrgfld_ );

    finaliseDone.notify( cb );
}


uiGenRanLinesByContour::~uiGenRanLinesByContour()
{
    delete horctio_.ioobj; delete &horctio_;
    delete rlsctio_.ioobj; delete &rlsctio_;
}


const char* uiGenRanLinesByContour::getNewSetID() const
{
    return rlsctio_.ioobj ? rlsctio_.ioobj->key().buf() : 0;
}


void uiGenRanLinesByContour::isrelChg( CallBacker* )
{
    const bool isrel = isrelfld_->isChecked();
    relzrgfld_->display( isrel );
    abszrgfld_->display( !isrel );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGenRanLinesByContour::acceptOK( CallBacker* )
{
    if ( !infld_->commitInput(false) )
       mErrRet("Please select the input horizon")
    if ( !outfld_->commitInput(true) )
       mErrRet("Please select the output random line set")
    
    uiTaskRunner taskrunner( this ); EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.loadIfNotFullyLoaded( horctio_.ioobj->key(),
	    					   &taskrunner );
    mDynamicCastGet( EM::Horizon3D*, hor, emobj )
    if ( !hor ) return false;
    hor->ref();

    StepInterval<float> contzrg = contzrgfld_->getFStepInterval();
    const bool isrel = isrelfld_->isChecked();
    Interval<float> linezrg = (isrel?relzrgfld_:abszrgfld_)->getFStepInterval();
    const float zfac = 1 / SI().zFactor();
    contzrg.scale( zfac ); linezrg.scale( zfac );


    EM::RandomLineSetGenerator::Setup setup( isrel );
    setup.contzrg( contzrg ).linezrg( linezrg );
    EM::RandomLineSetGenerator gen( *hor, setup );
    Geometry::RandomLineSet rls;
    gen.createLines( rls );
    hor->unRef();

    const int rlssz = rls.size();
    if ( rlssz < 1 )
       mErrRet("Could not find a contour in range")
    else
    {
	BufferString emsg;
	if ( !RandomLineSetTranslator::store(rls,rlsctio_.ioobj,emsg) )
	   mErrRet(emsg)
    }

    uiMSG().message( BufferString("Created ", rls.size(),
				  rls.sz>1?" lines":" line") );
    return true;
}
