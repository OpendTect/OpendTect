/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.83 2007-09-07 12:27:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "arrayndimpl.h"
#include "binidselimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "filegen.h"
#include "horizonscanner.h"
#include "ioobj.h"
#include "keystrs.h"
#include "randcolor.h"
#include "scaler.h"
#include "streamconn.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uiarray2dchg.h"
#include "uibinidsubsel.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uicursor.h"
#include "uiexecutor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uitable.h"


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


uiImportHorizon::uiImportHorizon( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Horizon",
				 "Specify horizon parameters","104.0.0"))
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , attribnames_(*new BufferStringSet)
    , emobjid_(-1)
{
    infld = new uiFileInput( this, "Input Ascii file", 
	    		     uiFileInput::Setup().withexamine(true) );
    infld->setSelectMode( uiFileDialog::ExistingFiles );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    infld->valuechanged.notify( mCB(this,uiImportHorizon,inputCB) );

    uiPushButton* scanbut = new uiPushButton( this, "Scan &file", 
					mCB(this,uiImportHorizon,scanFile),
					false );
    scanbut->attach( alignedBelow, infld );

    uiSeparator* sep = new uiSeparator( this, "Separator1" );
    sep->attach( stretchedBelow, scanbut );

    midgrp = new uiGroup( this, "Middle Group" );
    midgrp->setSensitive( false );
    midgrp->attach( alignedBelow, scanbut );
    midgrp->attach( ensureBelow, sep );

    xyfld = new uiGenInput( midgrp, "Positions in:",
                            BoolInpSpec(true,"X/Y","Inl/Crl") );
    midgrp->setHAlignObj( xyfld );

    subselfld = new uiBinIDSubSel( midgrp, uiBinIDSubSel::Setup()
			       .withz(false).withstep(true).rangeonly(true) );
    subselfld->attach( alignedBelow, xyfld );

    BufferString scalelbl( SI().zIsTime() ? "Z " : "Depth " );
    scalelbl += "scaling";
    scalefld = new uiScaler( midgrp, scalelbl, true );
    scalefld->attach( alignedBelow, subselfld );

    udfvalfld = new uiGenInput( midgrp, "Value representing 'Undefined'",
	    		     StringInpSpec(sKey::FloatUdf) );
    udfvalfld->attach( alignedBelow, scalefld );

    filludffld = new uiGenInput( midgrp, "Fill undefined parts",
	    			 BoolInpSpec(true) );
    filludffld->valuechanged.notify( mCB(this,uiImportHorizon,fillUdfSel) );
    filludffld->setValue(false);
    filludffld->attach( alignedBelow, udfvalfld );

    arr2dinterpfld = new uiImpHorArr2DInterpPars( midgrp );
    arr2dinterpfld->attach( alignedBelow, filludffld );

    attribbut = new uiPushButton( midgrp, "Attribute &info",
	    			  mCB(this,uiImportHorizon,attribSel), false );
    attribbut->attach( alignedBelow, arr2dinterpfld );

    uiSeparator* sep2 = new uiSeparator( this, "Separator2" );
    sep2->attach( stretchedBelow, midgrp );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    ctio_.ctxt.forread = false;
    outfld = new uiIOObjSel( botgrp, ctio_, "Output Horizon" );

    stratlvlfld_ = new uiStratLevelSel( botgrp );
    stratlvlfld_->attach( alignedBelow, outfld );
    stratlvlfld_->selchanged_.notify( mCB(this,uiImportHorizon,stratLvlChg) );

    colbut = new uiColorInput( botgrp, getRandStdDrawColor(), "Base color" );
    colbut->attach( alignedBelow, stratlvlfld_ );

    displayfld = new uiCheckBox( botgrp, "Display after import" );
    displayfld->attach( alignedBelow, colbut );
    botgrp->setHAlignObj( outfld );
    botgrp->attach( alignedBelow, midgrp );
    botgrp->attach( ensureBelow, sep2 );

    fillUdfSel(0);
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &attribnames_;
}


void uiImportHorizon::inputCB( CallBacker* )
{
    midgrp->setSensitive( false );
    button( OK )->setSensitive( false );
}


void uiImportHorizon::fillUdfSel( CallBacker* )
{
    const bool dodisp = filludffld->getBoolValue();
    arr2dinterpfld->display( filludffld->getBoolValue() );
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    MultiID mid = ctio_.ioobj ? ctio_.ioobj->key() : -1;
    return mid;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && doWork();
    return ret;
}


#define mErrRet(s)	{ uiMSG().error(s); return false; }
#define mErrRetUnRef(s)	{ horizon->unRef(); mErrRet(s) }


bool uiImportHorizon::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    if ( !outfld->commitInput(true) )
	mErrRet( "Please select the output" )

    return true;
}


bool uiImportHorizon::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*infld->fileName() )
	mErrRet( "Please select input file(s)" )

    infld->getFileNames( filenames );
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


bool uiImportHorizon::doWork()
{
    const char* horizonnm = outfld->getInput();
    EM::EMManager& em = EM::EMM();
    emobjid_ = em.createObject( EM::Horizon3D::typeStr(), horizonnm );
    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(emobjid_));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    NotifyStopper stopper( horizon->change );
    horizon->setMultiID( ctio_.ioobj->key() );
    horizon->setTiedToLvl( stratlvlfld_->getLvlName() );
    horizon->setPreferredColor( colbut->color() );
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    HorizonScanner scanner( filenames );
    const bool isxy = scanner.posIsXY();
    const bool doxy = xyfld->getBoolValue();
    if ( doxy != isxy )
    {
	BufferString msg( "Coordinates in inputfile seem to be " );
	msg += isxy ? "X/Y.\n" : "Inl/Crl.\n";
	msg += "Continue importing as "; msg += doxy ? "X/Y?" : "Inl/Crl?";
	if ( !uiMSG().askGoOn(msg) ) return false;
    }

    HorSampling hs = subselfld->getInput().cs_.hrg;
    if ( hs.step.inl==0 ) hs.step.inl = SI().inlStep();
    if ( hs.step.crl==0 ) hs.step.crl = SI().crlStep();
/*  TODO: ask Kris why this has been implemented
    if ( hs.step.inl%filehs_.step.inl || hs.step.crl%filehs_.step.crl )
    {
	BufferString msg( "Inline/Crossline steps must be dividable by ");
	msg += filehs_.step.inl; msg += "/"; msg += filehs_.step.crl;
	msg += ".";
	uiMSG().error( msg );
	return false;
    }
*/

    if ( !filehs_.includes(hs.start) )
    {
	BufferString msg( "Ranges are not compatible with the file.\n"
			  "Snap to closest position (" );
	const StepInterval<int> inlrg( filehs_.inlRange() );
	const StepInterval<int> crlrg( filehs_.crlRange() );
	BinID newstart = BinID(inlrg.atIndex(inlrg.nearestIndex(hs.start.inl)),
			       crlrg.atIndex(crlrg.nearestIndex(hs.start.crl)));
	newstart.inl = mMAX( inlrg.start, newstart.inl );
	newstart.crl = mMAX( crlrg.start, newstart.crl );
	msg += newstart.inl; msg += "/";
	msg += newstart.crl; msg += ")?";
	if ( !uiMSG().askGoOn( msg ) )
	    return false;

	hs.start = newstart;
	uiBinIDSubSel::Data data = subselfld->getInput();
	data.cs_.hrg = hs;
	subselfld->setInput(data); //important since other may read it later
    }

    ObjectSet<BinIDValueSet> sections;
    if ( !readFiles(sections,scanner.needZScaling(),&hs) ) return false;
    if ( sections.isEmpty() )
	mErrRet( "Nothing to import" );

    const bool dofill = filludffld->getBoolValue();
    if ( dofill )
	fillUdfs( sections );

    const RowCol step( hs.step.inl, hs.step.crl );
    horizon->ref();
    ExecutorGroup importer( "Horizon importer" );
    importer.setNrDoneText( "Nr inlines imported" );
    importer.add( horizon->importer(sections,step) );

    ObjectSet<BinIDValueSet> attribvals;
    if ( !dofill && scanner.nrAttribValues()>0 )
    {
	for ( int sidx=0; sidx<sections.size(); sidx++ )
	{
	    BinIDValueSet* set = new BinIDValueSet( sections[sidx]->nrVals(),
						    false );
	    attribvals += set;
	    BinID bid;
	    TypeSet<float> vals;
	    BinIDValueSet::Pos pos;
	    while( sections[sidx]->next(pos,true) )
	    {
		sections[sidx]->get( pos, bid, vals );
		set->add( bid, vals );
	    }
	}
	importer.add( 
		horizon->auxDataImporter(attribvals,attribnames_,attribsel_) );
    }

    uiExecutor impdlg( this, importer );
    const bool success = impdlg.go();
    deepErase( sections );
    deepErase( attribvals );
    if ( !success )
	mErrRetUnRef("Cannot import horizon")

    Executor* exec = horizon->saver();
    if ( !exec )
    {
	delete exec;
	horizon->unRef();
	return false;
    }

    uiExecutor dlg( this, *exec );
    const bool rv = dlg.execute();
    delete exec;
    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();
    return rv;
}


bool uiImportHorizon::readFiles( ObjectSet<BinIDValueSet>& sections,
				 bool doscale, const HorSampling* hs )
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fname = filenames.get( idx );
	BinIDValueSet* bvs = getBidValSet( fname, doscale, hs );
	if ( bvs && !bvs->isEmpty() )
	    sections += bvs;
	else
	{
	    delete bvs;
	    BufferString msg( "Cannot read input file:\n" ); msg += fname;
	    mErrRet( msg );
	}
    }

    return true;
}


void uiImportHorizon::scanFile( CallBacker* )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return;

    HorizonScanner scanner( filenames );
    scanner.execute();
    scanner.launchBrowser();
    if ( scanner.nrPositions() == 0 )
    {
	uiMSG().error( "No valid positions found in input file." );
	return;
    }

    xyfld->setValue( scanner.posIsXY() );
    filehs_.set( scanner.inlRg(), scanner.crlRg() );
    if ( filehs_.step.inl==0 ) filehs_.step.inl = SI().inlStep();
    if ( filehs_.step.crl==0 ) filehs_.step.crl = SI().crlStep();
    uiBinIDSubSel::Data subseldata = subselfld->getInput();
    subseldata.cs_.hrg = filehs_; subselfld->setInput( subseldata );

    filludffld->setValue( scanner.gapsFound(true) || scanner.gapsFound(false) );
    fillUdfSel(0);
    midgrp->setSensitive( true );
    button( OK )->setSensitive( true );

    const int nrattrvals = scanner.nrAttribValues();
    attribbut->setSensitive( nrattrvals > 0 );
    attribnames_.erase();
    attribsel_.erase();
    for ( int idx=0; idx<nrattrvals; idx++ )
    {
	BufferString nm( "Imported attribute " );
	nm += idx+1;
	attribnames_.add( nm );
	attribsel_ += true;
    }
}


BinIDValueSet* uiImportHorizon::getBidValSet( const char* fnm, bool doscale,
					      const HorSampling* hs )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return 0;

    BinIDValueSet* set = new BinIDValueSet(1,false);
    const Scaler* scaler = scalefld->getScaler();
    const float udfval = udfvalfld->getfValue();
    const bool doxy = xyfld->getBoolValue();
    float factor = 1;
    if ( doscale )
	factor = SI().zIsTime() ? 0.001 : (SI().zInMeter() ? .3048 : 3.28084);

    Coord crd;
    BinID bid;
    char buf[1024]; char valbuf[80];
    while ( *sd.istrm )
    {
	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	if ( !ptr || !*ptr ) 
	    continue;
	crd.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );
	crd.y = atof( valbuf );
	bid = doxy ? SI().transform( crd ) : BinID(mNINT(crd.x),mNINT(crd.y));
	if ( hs && !hs->isEmpty() && !hs->includes(bid) ) continue;

	TypeSet<float> values;
	while ( *ptr )
	{
	    ptr = getNextWord( ptr, valbuf );
	    values += atof( valbuf );
	}
	
	if ( values.isEmpty() ) continue;
	if ( set->nrVals() != values.size() )
	    set->setNrVals( values.size() );

	const bool validz = SI().zRange(false).includes(
				doscale ? values[0]*factor : values[0] );
	if ( mIsEqual(values[0],udfval,mDefEps) || !validz )
	    mSetUdf(values[0]);

	if ( doscale && !mIsUdf(values[0]) )
	    values[0] *= factor;

	if ( scaler )
	    values[0] = scaler->scale( values[0] );

	set->add( bid, values );
    }

    sd.close();
    return set;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinIDValueSet>& sections )
{
    HorSampling hs = subselfld->getInput().cs_.hrg;

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
	interpolator.pars() = arr2dinterpfld->pars_;
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


static const char* sYesNo[] = { sKey::Yes, sKey::No, 0 };

class AttribNameEditor : public uiDialog
{

public:
AttribNameEditor( uiParent* p, const BufferStringSet& names )
    : uiDialog(p,Setup("Attribute import","Specify attribute names",""))
{
    table_ = new uiTable( this, uiTable::Setup().rowdesc("Attribute") );
    table_->setNrCols( 2 );
    table_->setColumnLabel( 0, "Attribute Name" );
    table_->setColumnLabel( 1, "Import" );
    table_->setNrRows( names.size() );
    table_->setDefaultRowLabels();

    for ( int idx=0; idx<names.size(); idx++ )
    {
	table_->setText( RowCol(idx,0), names.get(idx) );
	uiComboBox* ynbox = new uiComboBox(0);
	ynbox->addItems( sYesNo );
	table_->setCellObject( RowCol(idx,1), ynbox );
    }
}


void getAttribNames( BufferStringSet& names,
		     BoolTypeSet& doimp ) const
{
    names.erase();
    doimp.erase();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	names.add( table_->text(RowCol(idx,0)) );
	mDynamicCastGet(uiComboBox*,box,table_->getCellObject(RowCol(idx,1)))
	doimp += !strcmp(box->text(),sKey::Yes);
    }
}

    uiTable*	table_;
};


void uiImportHorizon::attribSel( CallBacker* )
{
    AttribNameEditor dlg( this, attribnames_ );
    if ( !dlg.go() ) return;
    dlg.getAttribNames( attribnames_, attribsel_ );
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( strcmp( stratlvlfld_->getLvlName(), "" ) )
	colbut->setColor( *stratlvlfld_->getLvlColor() );
}
