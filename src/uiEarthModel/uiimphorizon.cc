/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.57 2006-01-25 07:13:40 cvskris Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon.h"

#include "array2dinterpol.h"
#include "binidselimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "filegen.h"
#include "horizonscanner.h"
#include "ioobj.h"
#include "scaler.h"
#include "streamconn.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "userinputobj.h"

#include "uiexecutor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uibinidsubsel.h"
#include "uicolor.h"
#include "uicursor.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uitable.h"


static int sDefStepout = 3;

uiImportHorizon::uiImportHorizon( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Horizon",
				 "Specify horizon parameters","104.0.0"))
    , ctio_(*mMkCtxtIOObj(EMHorizon))
    , attribnames_(*new BufferStringSet)
    , emobjid_(-1)
{
    infld = new uiFileInput( this, "Input Ascii file", 
	    		     uiFileInput::Setup().withexamine(true) );
    infld->setSelectMode( uiFileDialog::ExistingFiles );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    infld->valuechanged.notify( mCB(this,uiImportHorizon,inputCB) );

    uiPushButton* scanbut = new uiPushButton( this, "Scan file ...", 
					mCB(this,uiImportHorizon,scanFile) );
    scanbut->attach( alignedBelow, infld );

    uiSeparator* sep = new uiSeparator( this, "Separator1" );
    sep->attach( stretchedBelow, scanbut );

    midgrp = new uiGroup( this, "Middle Group" );
    midgrp->setSensitive( false );
    midgrp->attach( alignedBelow, scanbut );
    midgrp->attach( ensureBelow, sep );

    xyfld = new uiGenInput( midgrp, "Positions in:",
                            BoolInpSpec("X/Y","Inl/Crl") );
    midgrp->setHAlignObj( xyfld );

    subselfld = new uiBinIDSubSel( midgrp, uiBinIDSubSel::Setup()
			       .withz(false).withstep(true).rangeonly(true) );
    subselfld->attach( alignedBelow, xyfld );

    BufferString scalelbl( SI().zIsTime() ? "Z " : "Depth " );
    scalelbl += "scaling";
    scalefld = new uiScaler( midgrp, scalelbl, true );
    scalefld->attach( alignedBelow, subselfld );

    udffld = new uiGenInput( midgrp, "Undefined value",
	    		     StringInpSpec(sUndefValue) );
    udffld->attach( alignedBelow, scalefld );

    interpolfld = new uiGenInput( midgrp, "Interpolate to make regular grid",
				  BoolInpSpec() );
    interpolfld->valuechanged.notify( mCB(this,uiImportHorizon,interpolSel) );
    interpolfld->setValue(false);
    interpolfld->attach( alignedBelow, udffld );

    stepoutfld = new uiGenInput( midgrp, "Stepout", IntInpSpec() );
    stepoutfld->setValue( sDefStepout );
    stepoutfld->setElemSzPol( uiObject::small );
    stepoutfld->attach( rightTo, interpolfld );

    attribbut = new uiPushButton( midgrp, "Attribute info ...",
	    			  mCB(this,uiImportHorizon,attribSel) );
    attribbut->attach( alignedBelow, interpolfld );

    uiSeparator* sep2 = new uiSeparator( this, "Separator2" );
    sep2->attach( stretchedBelow, midgrp );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    ctio_.ctxt.forread = false;
    outfld = new uiIOObjSel( botgrp, ctio_, "Output Horizon" );

    colbut = new uiColorInput( botgrp, Color::randDrawColor(), "Base color" );
    colbut->attach( alignedBelow, outfld );

    displayfld = new uiCheckBox( botgrp, "Display after import" );
    displayfld->attach( alignedBelow, colbut );
    botgrp->setHAlignObj( outfld );
    botgrp->attach( alignedBelow, midgrp );
    botgrp->attach( ensureBelow, sep2 );

    interpolSel(0);
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


void uiImportHorizon::interpolSel( CallBacker* )
{
    stepoutfld->display( interpolfld->getBoolValue() );
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

    if ( !outfld->commitInput( true ) )
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
    emobjid_ = em.createObject( EM::Horizon::typeStr(), horizonnm );
    mDynamicCastGet(EM::Horizon*,horizon,em.getObject(emobjid_));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    horizon->setPreferredColor( colbut->color() );
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    HorizonScanner scanner( filenames );
    bool isxy = scanner.posIsXY();

    const bool doxy = xyfld->getBoolValue();
    if ( doxy != isxy )
    {
	BufferString msg( "Coordinates in inputfile seem to be " );
	msg += isxy ? "X/Y.\n" : "Inl/Crl.\n";
	msg += "Continue importing as "; msg += doxy ? "X/Y?" : "Inl/Crl?";
	if ( !uiMSG().askGoOn(msg) ) return false;
    }

    HorSampling hs; subselfld->getHorSampling( hs );
    ObjectSet<BinIDValueSet> sections;
    if ( !readFiles(sections,scanner.needZScaling(),&hs) ) return false;
    if ( !sections.size() )
	mErrRet( "Nothing to import" );

    const bool dointerpolate = interpolfld->getBoolValue();
    if ( dointerpolate )
	interpolateGrid( sections );

    const RowCol step( hs.step.inl, hs.step.crl );
    horizon->ref();
    ExecutorGroup importer( "Horizon importer" );
    importer.setNrDoneText( "Nr inlines imported" );
    importer.add( horizon->importer(sections,step) );

    ObjectSet<BinIDValueSet> attribvals;
    if ( !dointerpolate && scanner.nrAttribValues()>0 )
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

    PtrMan<Executor> exec = horizon->geometry.saver();
    if ( !exec )
    {
	horizon->unRef();
	return false;
    }

    uiExecutor dlg( this, *exec );
    bool rv = dlg.execute();
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
    HorSampling hs( false );
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return;

    HorizonScanner scanner( filenames );
    scanner.execute();
    scanner.launchBrowser();

    xyfld->setValue( scanner.posIsXY() );
    hs.set( scanner.inlRg(), scanner.crlRg() );
    subselfld->setInput( hs );
    interpolfld->setValue(scanner.gapsFound(true) || scanner.gapsFound(false));
    interpolSel(0);
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
    const float udfval = udffld->getfValue();
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
	
	if ( !values.size() ) continue;
	if ( set->nrVals() != values.size() )
	    set->setNrVals( values.size() );

	if ( mIsEqual(values[0],udfval,mDefEps) )
	    values[0] = mUndefValue;

	if ( doscale && !mIsUndefined(values[0]) )
	    values[0] *= factor;

	if ( scaler )
	    values[0] = scaler->scale( values[0] );

	set->add( bid, values );
    }

    sd.close();
    return set;
}


bool uiImportHorizon::interpolateGrid( ObjectSet<BinIDValueSet>& sections )
{
    HorSampling hs; subselfld->getHorSampling( hs );

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
		    arr->set( inl, crl, mUndefValue );
		else
		{
		    const float* vals = data.getVals( pos );
		    arr->set( inl, crl, vals ? vals[0] : mUndefValue );
		}
	    }
	}

	Array2DInterpolator<float> interpolator( arr );
	interpolator.setInverseDistance( true );
	interpolator.setAperture( stepoutfld->getIntValue() );
	while ( true )
	{
	    const int res = interpolator.nextStep();
	    if ( !res ) break;
	    else if ( res == -1 ) return false;
	}

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
	UserInputObj* uiobj = table_->mkUsrInputObj( RowCol(idx,1) );
	uiobj->addItems( sYesNo );
    }
}

void AttribNameEditor::getAttribNames( BufferStringSet& names,
				       BoolTypeSet& doimp ) const
{
    names.erase();
    doimp.erase();
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	names.add( table_->text(RowCol(idx,0)) );
	doimp += !strcmp(table_->text(RowCol(idx,1)),sKey::Yes);
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
