/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.47 2005-03-02 08:57:08 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "strmdata.h"
#include "strmprov.h"
#include "uiexecutor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "filegen.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uibutton.h"
#include "uibinidsubsel.h"
#include "scaler.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "binidselimpl.h"

#include "streamconn.h"
#include "binidvalset.h"
#include "uicursor.h"


uiImportHorizon::uiImportHorizon( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Horizon",
				 "Specify horizon parameters","104.0.0"))
    , ctio(*mMkCtxtIOObj(EMHorizon))
    , emobjid(-1)
{
    infld = new uiFileInput( this, "Input Ascii file", 
	    		     uiFileInput::Setup().withexamine() );
    infld->setSelectMode( uiFileDialog::ExistingFiles );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );

    uiPushButton* scanbut = new uiPushButton( this, "Scan file ...", 
					mCB(this,uiImportHorizon,scanFile) );
    scanbut->attach( alignedBelow, infld );

    xyfld = new uiGenInput( this, "Positions in:",
                            BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, scanbut );

    subselfld = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
	    			   .withz(false).withstep(true) );
    subselfld->attach( alignedBelow, xyfld );

    BufferString scalelbl( SI().zIsTime() ? "Z " : "Depth " );
    scalelbl += "scaling";
    scalefld = new uiScaler( this, scalelbl, true );
    scalefld->attach( alignedBelow, subselfld );

    udffld = new uiGenInput( this, "Undefined value",
	    		     StringInpSpec(sUndefValue) );
    udffld->attach( alignedBelow, scalefld );

    fillholesfld = new uiGenInput( this, "Try to fill small holes:",
                            BoolInpSpec() );
    fillholesfld->setValue(false);
    fillholesfld->attach( alignedBelow, udffld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Horizon" );
    outfld->attach( alignedBelow, fillholesfld );

    displayfld = new uiCheckBox( this, "Display after import" );
    displayfld->attach( alignedBelow, outfld );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio.ioobj; delete &ctio;
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    if ( emobjid<0 ) return -1;

    MultiID mid = IOObjContext::getStdDirData(ctio.ctxt.stdseltype)->id;
    mid.add(emobjid);
    return mid;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && doWork();
    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mErrRetUnRef(s) \
{ horizon->unRef(); mErrRet(s) }


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
    emobjid = em.createObject( EM::Horizon::typeStr(), horizonnm );
    mDynamicCastGet(EM::Horizon*,horizon,em.getObject(emobjid));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    bool isxy, doscale;
    if ( !analyzeData(isxy,doscale) )
	mErrRet("Cannot analyze data");

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
    if ( !readFiles(sections,doscale,&hs) ) return false;
    if ( !sections.size() )
	mErrRet( "Nothing to import" );

    const RowCol step( hs.step.inl, hs.step.crl );
    horizon->ref();
    ExecutorGroup exgrp( "Horizon importer" );
    exgrp.setNrDoneText( "Nr inlines imported" );
    exgrp.add( horizon->importer(sections,step,fillholesfld->getBoolValue()) );
    exgrp.add( horizon->auxDataImporter(sections) );
    uiExecutor impdlg( this, exgrp );
    if ( !impdlg.go() ) 
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
    HorSampling hs( false );
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return;

    bool isxy, doscale;
    if ( !analyzeData(isxy,doscale) ) return;
    xyfld->setValue( isxy );

    uiCursorChanger cursorlock( uiCursor::Wait );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fname = filenames.get( idx );
	BinIDValueSet& bvs = *getBidValSet( fname, false, 0 );
	Interval<int> inlrg = bvs.inlRange();
	Interval<int> crlrg = bvs.crlRange();
	if ( !idx )
	    hs.set( inlrg, crlrg );
	else
	{
	    hs.include( BinID(inlrg.start,crlrg.start) );
	    hs.include( BinID(inlrg.stop,crlrg.stop) );
	}

	hs.step.inl = mNINT( (float)inlrg.width() / (bvs.nrInls()-1) );
	int inl0 = inlrg.start;
	hs.step.crl = mNINT( (float)bvs.crlRange(inl0).width() / 
				(bvs.nrCrls(inl0)-1) );
	delete &bvs;
    }

    subselfld->setInput( hs );
}


bool uiImportHorizon::analyzeData( bool& isxy, bool& doscale )
{
    uiCursorChanger cursorlock( uiCursor::Wait );
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    StreamProvider sp( filenames.get(0) );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return false;

    const float fac = SI().zIsTime() ? 0.001
				     : (SI().zInMeter() ? .3048 : 3.28084);
    Interval<float> validrg( SI().zRange(false) );
    const float zwidth = validrg.width();
    validrg.sort();
    validrg.start -= zwidth;
    validrg.stop += zwidth;

    int maxcount = 100;
    int count, nrxy, nrbid, nrscale, nrnoscale;
    count = nrxy = nrbid = nrscale = nrnoscale = 0;
    Coord crd;
    BinID bid;
    float val;
    char buf[1024]; char valbuf[80];
    while ( *sd.istrm )
    {
	if ( count > maxcount ) 
	{
	    if ( nrscale == nrnoscale ) maxcount *= 2;
	    else break;
	}

	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	if ( !ptr || !*ptr ) 
	    continue;
	crd.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );
	crd.y = atof( valbuf );
	BinID bid( mNINT(crd.x), mNINT(crd.y) );
	if ( SI().isReasonable(crd) ) nrxy++;
	if ( SI().isReasonable(bid) ) nrbid++;

	ptr = getNextWord( ptr, valbuf );
	val = atof( valbuf );
	if ( mIsUndefined(val) ) continue;

	if ( validrg.includes(val) ) nrnoscale++;
	else if ( validrg.includes(val*fac) ) nrscale++;
	count++;
    }

    isxy = nrxy > nrbid;
    doscale = nrscale > nrnoscale;
    return true;
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

/*
class HorizonScanner
{
public:

HorizonScanner(const char* fnm)
    : posgeomdetect(new PosGeomDetector)
{
    filenames.add( fnm );
}


HorizonScanner(const BufferStringSet& fnms)
    : posgeomdetect(new PosGeomDetector)
{
    filenames = fnms;
}


bool scan()
{
    if ( !examine() ) return false;

    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fname = filenames.get( idx );
	StreamProvider sp( fname );
	StreamData sd = sp.makeIStream();
	if ( !sd.usable() ) continue;

	const float udfval = udffld->getfValue();
	const bool doxy = xyfld->getBoolValue();

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
	    bid = doxy ? SI().transform( crd ) 
		       : BinID(mNINT(crd.x),mNINT(crd.y));
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
    }

    return true;
}


bool examine()
{
    StreamProvider sp( filenames.get(0) );
    StreamData sd = sp.makeIStream();
    if ( !sd.usable() )
	return false;

    const float fac = SI().zIsTime() ? 0.001
				     : (SI().zInMeter() ? .3048 : 3.28084);
    Interval<float> validrg( SI().zRange(false) );
    const float zwidth = validrg.width();
    validrg.sort();
    validrg.start -= zwidth;
    validrg.stop += zwidth;

    int maxcount = 100;
    int count, nrxy, nrbid, nrscale, nrnoscale;
    count = nrxy = nrbid = nrscale = nrnoscale = 0;
    Coord crd;
    BinID bid;
    float val;
    char buf[1024]; char valbuf[80];
    while ( *sd.istrm )
    {
	if ( count > maxcount ) 
	{
	    if ( nrscale == nrnoscale ) maxcount *= 2;
	    else break;
	}

	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	if ( !ptr || !*ptr ) 
	    continue;
	crd.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );
	crd.y = atof( valbuf );
	BinID bid( mNINT(crd.x), mNINT(crd.y) );
	if ( SI().isReasonable(crd) ) nrxy++;
	if ( SI().isReasonable(bid) ) nrbid++;

	ptr = getNextWord( ptr, valbuf );
	val = atof( valbuf );
	if ( mIsUndefined(val) ) continue;

	if ( validrg.includes(val) ) nrnoscale++;
	else if ( validrg.includes(val*fac) ) nrscale++;
	count++;
    }

    sd.close();
    isxy = nrxy > nrbid;
    doscale = nrscale > nrnoscale;
    return true;
}


    bool	isxy;
    bool	doscale;

};
*/
