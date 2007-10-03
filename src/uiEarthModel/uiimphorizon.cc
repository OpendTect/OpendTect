/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimphorizon.cc,v 1.92 2007-10-03 11:53:37 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"
#include "uiarray2dchg.h"
#include "uibinidsubsel.h"

#include "uicombobox.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uiexecutor.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uitblimpexpdatasel.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "ioobj.h"
#include "pickset.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "filegen.h"
#include "emhorizon3d.h"

#include <math.h>

static const char* sZVals = "Z values";


class uiImpHorArr2DInterpPars : public uiCompoundParSel
{
public:

uiImpHorArr2DInterpPars( uiParent* p, const Array2DInterpolatorPars* ps=0 )
    : uiCompoundParSel( p, "Parameters for fill", "Set" )
{
    if ( ps ) pars_ = *ps;
    butPush.notify( mCB(this,uiImpHorArr2DInterpPars,selChg) );
}

BufferString getSummary() const
{
    BufferString ret;
    const bool havemaxsz = pars_.maxholesize_ > 0;
    const bool havemaxsteps = pars_.maxnrsteps_ > 0;

    if ( !havemaxsz && !havemaxsteps )
	ret = pars_.extrapolate_ ? "Fill all" : "No extrapolation";
    else
    {
	if ( havemaxsz )
	    { ret = "Holes < "; ret += pars_.maxholesize_; ret += "; "; }
	if ( havemaxsteps )
	{
	    ret += pars_.maxnrsteps_; ret += " iteration";
	    if ( pars_.maxnrsteps_ > 1 ) ret += "s"; ret += "; ";
	}
	ret += pars_.extrapolate_ ? "everywhere" : "inside";
    }
    return ret;
}


void selChg( CallBacker* )
{
    uiArr2DInterpolParsDlg dlg( this, &pars_ );
    if ( dlg.go() )
	pars_ = dlg.getInput();
}

    Array2DInterpolatorPars	pars_;

};


uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,uiDialog::Setup("Import Horizon",
					   "Specify parameters",
					   "104.0.0"))
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , isgeom_(isgeom)
    , scalefld_(0)
    , filludffld_(0)
    , arr2dinterpfld_(0)
    , colbut_(0)
    , stratlvlfld_(0)
    , displayfld_(0)
    , fd_(*EM::Horizon3DAscIO::getDesc(true))
{
    inpfld_ = new uiFileInput( this, "Input ASCII File", uiFileInput::Setup()
					    .withexamine(true)
					    .forread(true) );
    inpfld_->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Surf) );

    ctio_.ctxt.forread = !isgeom_;
    ctio_.ctxt.maychdir = false;

    xyfld_ = new uiGenInput( this, "Positions in:",
			    BoolInpSpec(true,"X/Y","Inl/Crl") );
    xyfld_->valuechanged.notify( mCB(this,uiImportHorizon,formatSel) );
    xyfld_->attach( alignedBelow, inpfld_ );

    attrlistfld_ = new uiLabeledListBox( this, "Select Attribute(s) to import",
	   				 true );
    attrlistfld_->box()->setLines( 4, true );
    attrlistfld_->attach( alignedBelow, xyfld_ );
    attrlistfld_->box()->selectionChanged.notify( mCB(this,uiImportHorizon,
						      formatSel) );

    addbut_ = new uiPushButton( this, "Add new",
	    			mCB(this,uiImportHorizon,addAttrib), false );
    addbut_->attach( rightTo, attrlistfld_ );

    subselfld_ = new uiBinIDSubSel( this, uiBinIDSubSel::Setup().withz(false)
	    			    .withstep(true).rangeonly(true) );
    subselfld_->attach( alignedBelow, attrlistfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    if ( isgeom_ )
    {
	scalefld_ = new uiScaler( this, "Z scaling", true );
	scalefld_->attach( alignedBelow, subselfld_ );

	filludffld_ = new uiGenInput( this, "Fill undefined parts",
				      BoolInpSpec(true) );
	filludffld_->valuechanged.notify(mCB(this,uiImportHorizon,fillUdfSel));
	filludffld_->setValue(false);
	filludffld_->attach( alignedBelow, scalefld_ );

	arr2dinterpfld_ = new uiImpHorArr2DInterpPars( this );
	arr2dinterpfld_->attach( alignedBelow, filludffld_ );

	sep->attach( stretchedBelow, arr2dinterpfld_ );
    }
    else
	sep->attach( stretchedBelow, subselfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "100.0.0" );
    dataselfld_->attach( alignedBelow, attrlistfld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    outputfld_ = new uiIOObjSel( this, ctio_ );
    outputfld_->setLabelText( isgeom_ ? "OutPut Horizon" : "Add to Horizon" );
    outputfld_->attach( alignedBelow, attrlistfld_ );
    outputfld_->attach( ensureBelow, sep );

    if ( isgeom_ )
    {
	stratlvlfld_ = new uiStratLevelSel( this );
	stratlvlfld_->attach( alignedBelow, outputfld_ );
	stratlvlfld_->selchanged_.notify(mCB(this,uiImportHorizon,stratLvlChg));

	colbut_ = new uiColorInput( this, getRandStdDrawColor(), "Base color" );
	colbut_->attach( alignedBelow, stratlvlfld_ );

	displayfld_ = new uiCheckBox( this, "Display after import" );
	displayfld_->attach( alignedBelow, colbut_ );
	
	fillUdfSel(0);
    }
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImportHorizon::formatSel( CallBacker* cb )
{
    const bool isxy = xyfld_->getBoolValue();
    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    EM::Horizon3DAscIO::updateDesc( fd_, attrnms, isxy );
    dataselfld_->updateSummary();
}


void uiImportHorizon::addAttrib( CallBacker* cb )
{
    uiGenInputDlg* dlg = new uiGenInputDlg( this, "Add Attribute",
	    				    "Name", new StringInpSpec() );
    if ( !dlg->go() ) return;

    const char* attrnm = dlg->text();
    attrlistfld_->box()->addItem( attrnm );
}


void uiImportHorizon::fillUdfSel( CallBacker* )
{
    if ( arr2dinterpfld_ )
	arr2dinterpfld_->display( filludffld_->getBoolValue() );
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    MultiID mid = ctio_.ioobj ? ctio_.ioobj->key() : -1;
    return mid;
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const char* lvlname = stratlvlfld_->getLvlName();
    if ( lvlname && *lvlname )
	colbut_->setColor( *stratlvlfld_->getLvlColor() );
}
    
#define mErrRet(s) { uiMSG().error(s); return 0; }
#define mErrRetUnRef(s) { horizon->unRef(); mErrRet(s) }
#define mSave() \
    if ( !exec ) \
    { \
	delete exec; \
	horizon->unRef(); \
	return false; \
    } \
    uiExecutor dlg( this, *exec ); \
    rv = dlg.execute(); \
    delete exec; 

bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    if ( !attrnms.size() ) mErrRet( "No Attributes Selected" );

    EM::Horizon3D* horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon ) return false;

    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    const bool isxy = xyfld_->getBoolValue();
    const Scaler* scaler = scalefld_ ? scalefld_->getScaler() : 0;
    HorSampling hs = subselfld_->getInput().cs_.hrg;
    ObjectSet<BinIDValueSet> sections;
    EM::Horizon3DAscIO aio( fd_, attrnms );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fname = filenames.get( idx ); 
	StreamData sdi = StreamProvider( fname ).makeIStream();
	if ( !sdi.usable() ) 
	{ 
	    sdi.close();
	    mErrRet( "Could not open input file" )
	}

	BinIDValueSet* bvs = aio.get( *sdi.istrm, scaler, hs, isxy );
	if ( bvs && !bvs->isEmpty() )
	    sections += bvs;
	else
	{
	    delete bvs;
	    BufferString msg( "Cannot read input file:\n" ); msg += fname;
	    mErrRet( msg );
	}
    }

    if ( sections.isEmpty() )
    {
	horizon->unRef();
	mErrRet( "Nothing to import" );
    }

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
	fillUdfs( sections );

    ExecutorGroup importer( "Horizon importer" );
    importer.setNrDoneText( "Nr inlines imported" );
    int startidx = 0;
    if ( isgeom_ )
    {
	const RowCol step( hs.step.inl, hs.step.crl );
	importer.add( horizon->importer(sections,step) );
	attrnms.remove( 0 );
	startidx = 1;
    }

    if ( attrnms.size() )
	importer.add( horizon->auxDataImporter(sections,attrnms,startidx) );

    uiExecutor impdlg( this, importer );
    const bool success = impdlg.go();
    deepErase( sections );
    if ( !success )
	mErrRetUnRef("Cannot import horizon")

    bool rv;
    if ( isgeom_ )
    {
	Executor* exec = horizon->saver();
	mSave();
    }
    else
    {
	mDynamicCastGet(ExecutorGroup*,exec,horizon->auxdata.auxDataSaver(-1))
	mSave();
    }
    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();
    return rv;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    return doImport();
}


bool uiImportHorizon::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( "Please select input file(s)" )
    
    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File_exists(fnm) )
	{
	    BufferString errmsg( "Cannot find input file:\n" );
	    errmsg += fnm;
	    deepErase( filenames );
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    const char* outpnm = outputfld_->getInput();
    if ( !outpnm || !*outpnm )
	mErrRet( "Please select output horizon" )

    if ( !outputfld_->commitInput(true) )
	return false;

    return true;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinIDValueSet>& sections )
{
    HorSampling hs = subselfld_->getInput().cs_.hrg;

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	BinIDValueSet& data = *sections[idx];
	Array2DImpl<float>* arr = new Array2DImpl<float>(hs.nrInl(),hs.nrCrl());
	BinID bid;
	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl = hs.start.inl + inl*hs.step.inl;
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl = hs.start.crl + crl*hs.step.crl;
		BinIDValueSet::Pos pos = data.findFirst( bid );
		if ( pos.j < 0 )
		    arr->set( inl, crl, mUdf(float) );
		else
		{
		    const float* vals = data.getVals( pos );
		    arr->set( inl, crl, vals ? vals[0] : mUdf(float) );
		}
	    }
	}

	Array2DInterpolator<float> interpolator( *arr );
	interpolator.pars() = arr2dinterpfld_->pars_;
	interpolator.setColDistRatio( SI().crlDistance()*hs.step.crl/
		(hs.step.inl*SI().inlDistance() ));
	uiExecutor uiex( this, interpolator );
	if ( !uiex.execute() )
	    return false;

	data.empty();
	data.setNrVals( 1, false );
	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl = hs.start.inl + inl*hs.step.inl;
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl = hs.start.crl + crl*hs.step.crl;
		data.add( bid, arr->get(inl,crl) );
	    }
	}

	delete arr;
    }

    return true;
}


EM::Horizon3D* uiImportHorizon::createHor() const
{
    const char* horizonnm = outputfld_->getInput();
    EM::EMManager& em = EM::EMM();
    const MultiID mid = getSelID();
    EM::ObjectID objid = em.getObjectID( mid );
    if ( objid < 0 )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    horizon->setMultiID( mid );
    horizon->setTiedToLvl( stratlvlfld_->getLvlName() );
    horizon->setPreferredColor( colbut_->color() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setMultiID( ctio_.ioobj->key() );
    Executor* loader = emobj->loader();
    if ( !loader ) mErrRet( "Cannot load horizon");

    uiExecutor loaddlg( this, *loader );
    if ( !loaddlg.go() )
	return 0;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon ) mErrRet( "Error loading horizon");

    horizon->ref();
    delete loader;
    return horizon;
}



