/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitracksettingdlg.cc,v 1.1 2012/02/14 23:20:31 cvsyuancheng Exp $";

#include "uitracksettingdlg.h"

#include "arrayndimpl.h"
#include "datapackbase.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "faultextractor.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
#include "picksettr.h"
#include "seisparallelreader.h"
#include "seistrctr.h"

#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseissubsel.h"


uiTrackSettingDlg::uiTrackSettingDlg( uiParent* p )
    : uiDialog(p,Setup("Fault auto extractor","Tracking settings",mNoHelpID))
{
    setCtrlStyle( DoAndStay );

    modefld_ = new uiGenInput( this, "Mode",
	    BoolInpSpec(true,"Multi-Faults","Seeds based body") );
    modefld_->valuechanged.notify( mCB(this,uiTrackSettingDlg,modeChangeCB) );
    modefld_->setSensitive(false);

    inputselfld_ = new uiSeisSel( this, mIOObjContext(SeisTrc), 
	    uiSeisSel::Setup(Seis::Vol) );
    inputselfld_->attach( alignedBelow, modefld_ );
    inputselfld_->selectionDone.notify( mCB(this,uiTrackSettingDlg,inpSel) );
    subselfld_ = uiSeisSubSel::get( this, Seis::SelSetup(Seis::Vol) );
    subselfld_->attachObj()->attach( alignedBelow, inputselfld_ );
   
    thresholdfld_ = new uiGenInput( this, "Threshold", FloatInpSpec(0) );
    thresholdfld_->attach( leftAlignedBelow, subselfld_ );
    aboveisovaluefld_ = new uiGenInput( this, "Fault value",
	    BoolInpSpec(true,"Above threshold","Below threshold") );
    aboveisovaluefld_->attach( alignedBelow, thresholdfld_ );
    
    seedselfld_ = new uiIOObjSel( this, mIOObjContext(PickSet), "Picked seeds");
    seedselfld_->attach( alignedBelow, aboveisovaluefld_ );

    hlanglergfld_ = new uiGenInput( this, "Line angle range(degree)",
	    FloatInpIntervalSpec(Interval<float>(45,135)) );
    hlanglergfld_->attach( alignedBelow, aboveisovaluefld_ );

    hltoplistfld_ = new uiGenInput( this, "Nr top faults",IntInpSpec(5) );
    hltoplistfld_->attach( alignedBelow, hlanglergfld_ );

    outputfltfld_ = new uiIOObjSel( this, mIOObjContext(EMFault3D),
	    			    "Output fault(s)" );
    outputfltfld_->setForRead( false );
    outputfltfld_->attach( alignedBelow, hltoplistfld_ );
    
    outputbodyfld_ = new uiIOObjSel( this, mIOObjContext(EMBody),
				     "Output fault body" );
    outputbodyfld_->setForRead( false );
    outputbodyfld_->attach( alignedBelow, hltoplistfld_ );

    modeChangeCB(0);
    inpSel(0);
}


void uiTrackSettingDlg::modeChangeCB( CallBacker* )
{
    const bool hlmode = modefld_->getBoolValue();
    seedselfld_->display( !hlmode );
    outputbodyfld_->display( !hlmode );

    outputfltfld_->display( hlmode );
    hltoplistfld_->display( hlmode );
    hlanglergfld_->display( hlmode );
}


void uiTrackSettingDlg::inpSel( CallBacker* )
{
    if ( !inputselfld_->ioobj() )
	return;

    subselfld_->setInput( *(inputselfld_->ioobj()) );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiTrackSettingDlg::acceptOK( CallBacker* cb )
{ 
    if ( !inputselfld_->commitInput() )
	mErrRet("Missing input, please select the input seismics")

    MouseCursorChanger mousecursorchanger( MouseCursor::Wait );	    
    const bool res = modefld_->getBoolValue() ? processHoughLineExtraction()
    					      : processSeedsFloodFill();
    if ( res )
	uiMSG().message("Fault tracking succeed");
    else
	mErrRet("Fault tracking failed");

    return false;
}


bool uiTrackSettingDlg::processHoughLineExtraction()
{
    if ( !outputfltfld_->commitInput() )
	mErrRet("Missing Output, please enter a name for the output fault")

    CubeSampling cs;
    subselfld_->getSampling( cs );

    mDeclareAndTryAlloc(Array3DImpl<float>*,arr,Array3DImpl<float>(
		cs.nrInl(), cs.nrCrl(), cs.nrZ() ));
    if ( !arr ) mErrRet("Not able to allocate memory on disk");
    arr->setAll( mUdf(float) );

    if ( !readInput(*arr) )
    {
	delete arr;
	mErrRet("Input data reading error.");
    }

    Interval<float> anglerg = hlanglergfld_->getFInterval(0);
    //if ( anglerg.start<0 || anglerg.stop<0 )
    //{
//	delete arr;
//	mErrRet("Please select angle between [0,360]")
  //  }
 
    anglerg.start = (mMIN(anglerg.start,anglerg.stop))*M_PI/180.0;
    anglerg.stop = (mMAX(anglerg.start,anglerg.stop))*M_PI/180.0;

    FaultExtractor fe(*arr,cs);
    fe.setThreshold( thresholdfld_->getfValue(), 
	    	     aboveisovaluefld_->getBoolValue() );
    fe.setLineAngleRange( anglerg );
    fe.setTopList( 1 );
    const bool res = fe.compute();
    if ( !res )
    {mErrRet("Fault extraction failed.");}
    else
	uiMSG().message("Fault extraction succeed.");

    ObjectSet<EM::Fault3D> flts = fe.getFaults();
    for ( int idx=0; idx<flts.size(); idx++ )
    {
	BufferString nm = outputfltfld_->getInput();
	nm += idx;
	flts[idx]->setName( nm.buf() );
	flts[idx]->setFullyLoaded( true );
	EM::EMM().addObject( flts[idx] );

	PtrMan<Executor> exec = flts[idx]->saver();
	if ( exec )
	{
	    MultiID key = flts[idx]->multiID();
	    PtrMan<IOObj> eioobj = IOM().get( key );
	    if ( !eioobj->pars().find( sKey::Type ) )
	    {
		eioobj->pars().set( sKey::Type, flts[idx]->getTypeStr() );
		IOM().commitChanges( *eioobj );
	    }
	    
	    exec->execute();
	}
    }

    return true;
}


bool uiTrackSettingDlg::processSeedsFloodFill()
{
    if ( !outputbodyfld_->commitInput() )
	mErrRet("Missing Output, please enter a name for the output body")
    
    return true;
}


bool uiTrackSettingDlg::readInput( Array3D<float>& arr )
{
    if ( !inputselfld_->ioobj() ) return false;

    CubeSampling cs;
    subselfld_->getSampling( cs );

    // Note: For now only the first component
    ObjectSet< Array3D<float> > data; data += &arr;
    TypeSet<int> comps; comps += 0;
    Seis::ParallelReader rdr( *inputselfld_->ioobj(), comps, data, cs );
    return rdr.execute();
}

