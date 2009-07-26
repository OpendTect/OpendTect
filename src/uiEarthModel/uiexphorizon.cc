/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexphorizon.cc,v 1.63 2009-07-26 04:01:55 cvskris Exp $";

#include "uiexphorizon.h"

#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filegen.h"
#include "filepath.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uizaxistransform.h"
#include "zaxistransform.h"
#include "zdomain.h"

#include <stdio.h>

static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };


uiExportHorizon::uiExportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format","104.0.1"))
{
    setCtrlStyle( DoAndStay );

    infld_ = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::keyword())
	    .withsubsel(true) );
    infld_->inpChange.notify( mCB(this,uiExportHorizon,inpSel) );
    infld_->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );

    typfld_ = new uiGenInput( this, "Output type", StringListInpSpec(exptyps) );
    typfld_->attach( alignedBelow, infld_ );
    typfld_->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    settingsbutt_ = new uiPushButton( this, "Settings",
 			      mCB(this,uiExportHorizon, settingsCB), false);
    settingsbutt_->attach( rightOf, typfld_ );
    const char* zmodes[] = { sKey::Yes, sKey::No, "Transformed", 0 };
    transfld_ = new uiZAxisTransformSel( this, true, SI().getZDomainString() );
    if ( !transfld_->nrTransforms() )
    {
	zmodes[2] = 0;
 	transfld_->display( false );
 	transfld_ = 0;
    }
 
    zfld_ = new uiGenInput( this, "Output Z", StringListInpSpec( zmodes ) );
    zfld_->valuechanged.notify( mCB(this,uiExportHorizon,addZChg ) );
    zfld_->setText( sKey::No );

    zfld_->attach( alignedBelow, typfld_ );

    if ( transfld_ )
    {
	transfld_->attach( alignedBelow, zfld_ );
     	transfld_->selectionDone()->notify( mCB(this,uiExportHorizon,addZChg) );
    }

    unitsel_ = new uiLabeledComboBox( this, "Z Unit" );
    unitsel_->box()->addItem( 0 );
    unitsel_->box()->addItem( 0 );
    unitsel_->attach( alignedBelow,
	    transfld_ ? (uiGroup*) transfld_ : (uiGroup*) zfld_ );

    udffld_ = new uiGenInput( this, "Undefined value",
	    		     StringInpSpec(sKey::FloatUdf) );
    udffld_->attach( alignedBelow, unitsel_ );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		      uiFileInput::Setup().forread(false) );
    outfld_->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    outfld_->attach( alignedBelow, udffld_ );

    typChg( 0 );
    inpSel( 0 );
}


uiExportHorizon::~uiExportHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mHdr1GFLineLen 102
#define mDataGFLineLen 148

static void initGF( std::ostream& strm, const char* hornm,
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    cleanupString( hnm.buf(), mC_False, mC_False, mC_False );
    sprintf( gfbuf, "PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		    "", "", SI().xyInFeet() ? "ft" : "m " );
    int sz = hnm.size(); if ( sz > 17 ) sz = 17;
    memcpy( gfbuf+8, hnm.buf(), sz );
    hnm = comment;
    sz = hnm.size(); if ( sz > 45 ) sz = 45;
    memcpy( gfbuf+35, hnm.buf(), sz );
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << std::endl;
}


#define mGFUndefValue 3.4028235E+38

static void writeGF( std::ostream& strm, const BinID& bid, float z, float zfac,
		     float val, const Coord& crd, int segid )
{
    static char buf[mDataGFLineLen+2];
    const float crl = bid.crl;
    const float gfval = mIsUdf(val) ? mGFUndefValue : val;
    const float depth = mIsUdf(z) ? mGFUndefValue : z * zfac;
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	     crd.x, crd.y, segid, 14, depth, crl, crl, bid.crl, gfval, bid.inl,
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


bool uiExportHorizon::writeAscii()
{
    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getIntValue() != 1;
    const bool dogf = typfld_->getIntValue() == 2;
    BufferString udfstr = udffld_->text();
    if ( udfstr.isEmpty() ) udfstr = sKey::FloatUdf;

    BufferString basename = outfld_->fileName();

    const IOObj* ioobj = infld_->selIOObj();
    if ( !ioobj ) mErrRet("Cannot find horizon object");

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    em.getSurfaceData( ioobj->key(), sd );
    EM::SurfaceIODataSelection sels( sd );
    infld_->getSelection( sels );
    sels.selvalues.erase();

    RefMan<EM::EMObject> emobj = em.createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot create horizon")

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
    PtrMan<Executor> loader = hor->geometry().loader( &sels );
    if ( !loader ) mErrRet("Cannot read horizon")

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) ) return false;

    infld_->getSelection( sels );
    if ( dogf && sels.selvalues.size() > 1 &&
	!uiMSG().askContinue("Only the first selected attribute will be used\n"
			     "Do you wish to continue?") )
	return false;

    if ( !sels.selvalues.isEmpty() )
    {
	ExecutorGroup exgrp( "Reading aux data" );
	for ( int idx=0; idx<sels.selvalues.size(); idx++ )
	    exgrp.add( hor->auxdata.auxDataLoader(sels.selvalues[idx]) );

	if ( !taskrunner.execute(exgrp) ) return false;
    }

    MouseCursorChanger cursorlock( MouseCursor::Wait );

    const FixedString zdomain = getZDomain();
    float zfac = 1;
    RefMan<ZAxisTransform> zat = 0;
    if ( zfld_->getIntValue()==2 && transfld_ )
	zat = transfld_->getSelection();

    if ( unitsel_->box()->getIntValue()==1 )
    {
	if ( zdomain==ZDomain::sKeyDepth() )
	    zfac = mToFeetFactor;
	else if ( zdomain==ZDomain::sKeyTWT() )
	    zfac = 1000;
    }
    else if ( dogf )
    {
	if ( zdomain==ZDomain::sKeyDepth() && SI().xyInFeet() )
	    zfac = mToFeetFactor;
	else if ( zdomain==ZDomain::sKeyTWT() )
	    zfac = 1000;
    }

    TypeSet<int>& sections = sels.selsections;
    int zatvoi = -1;
    if ( zat && zat->needsVolumeOfInterest() ) //Get BBox
    {
	CubeSampling bbox;
	bool first = true;
	for ( int sidx=0; sidx<sections.size(); sidx++ )
	{
	    const EM::SectionID sectionid = hor->sectionID( sections[sidx] );
 	    PtrMan<EM::EMObjectIterator> it = hor->createIterator( sectionid );
 	    while ( true )
 	    {
 		const EM::PosID posid = it->next();
 		if ( posid.objectID()==-1 )
 		    break;
 
		const Coord3 crd = hor->getPos( posid );
 		if ( !crd.isDefined() )
 		    continue;
 
 		const BinID bid = SI().transform( crd );
 		if ( first )
 		{
 		    first = false;
 		    bbox.hrg.start = bbox.hrg.stop = bid;
 		    bbox.zrg.start = bbox.zrg.stop = crd.z;
 		}
 		else
 		{
 		    bbox.hrg.include( bid );
 		    bbox.zrg.include( crd.z );
 		}
 	    }
 
 	}
 
 	if ( !first )
 	{
 	    zatvoi = zat->addVolumeOfInterest( bbox, false );
 	    if ( !zat->loadDataIfMissing( zatvoi, &taskrunner ) )
 	    {
 		uiMSG().error("Cannot load data for z-transform");
 		return false;
 	    }
 	}
    }

    const int nrattribs = hor->auxdata.nrAuxData();
    const bool writemultiple = sections.size() > 1;
    for ( int sidx=0; sidx<sections.size(); sidx++ )
    {
	const int sectionidx = sections[sidx];
	BufferString fname( basename ); 
	if ( writemultiple )
	{
	    FilePath fp( fname );
	    BufferString ext( fp.extension() );
	    if ( ext.isEmpty() )
		{ fname += "_"; fname += sidx; }
	    else
	    {
		fp.setExtension( 0 );
		BufferString fnm = fp.fileName();
		fnm += "_"; fname += sidx;
		fp.setFileName( fnm );
		fp.setExtension( ext );
		fname = fp.fullPath();
	    }
	}

	StreamData sdo = StreamProvider( fname ).makeOStream();
	if ( !sdo.usable() )
	{
	    sdo.close();
	    mErrRet( "Cannot open output file" );
	}

	if ( dogf )
	    initGF( *sdo.ostrm, gfname_.buf(), gfcomment_.buf() );

	const EM::SectionID sectionid = hor->sectionID( sectionidx );
	PtrMan<EM::EMObjectIterator> it = hor->createIterator( sectionid );
	BufferString str;
	while ( true )
	{
	    const EM::PosID posid = it->next();
	    if ( posid.objectID()==-1 )
		break;

	    Coord3 crd = hor->getPos( posid );
	    if ( zat )
		crd.z = zat->transform( crd );

	    if ( dogf )
	    {
		const BinID bid = SI().transform( crd );
		const float auxvalue = nrattribs > 0
		    ? hor->auxdata.getAuxDataVal(0,posid) : mUdf(float);
		writeGF( *sdo.ostrm, bid, crd.z, zfac, auxvalue, crd, sidx );
		continue;
	    }

	    if ( !doxy )
	    {
		const BinID bid = SI().transform( crd );
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    }
	    else
	    {
		// ostreams print doubles awfully
		str.setEmpty();
		str += crd.x; str += "\t"; str += crd.y;
		*sdo.ostrm << str;
	    }

	    if ( addzpos )
	    {
		if ( mIsUdf(crd.z) ) 
		    *sdo.ostrm << '\t' << udfstr;
		else
		{
		    str = "\t"; str += zfac * crd.z;
		    *sdo.ostrm << str;
		}
	    }

	    for ( int idx=0; idx<nrattribs; idx++ )
	    {
		const float auxvalue = hor->auxdata.getAuxDataVal( idx, posid );
		if ( mIsUdf(auxvalue) )
		    *sdo.ostrm << '\t' << udfstr;
		else
		{
		    str = "\t"; str += auxvalue;
		    *sdo.ostrm << str;
		}
	    }

	    *sdo.ostrm << '\n';
	}

	if ( dogf ) *sdo.ostrm << "EOD";
	sdo.close();
    }

    if ( zat && zatvoi>=0 )
	zat->removeVolumeOfInterest( zatvoi );

    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( zfld_->getIntValue()==2 && transfld_ )
    {
	if ( !transfld_->acceptOK() )
	    return false;
    }

    if ( !strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File_exists(outfld_->fileName()) && 
		    !uiMSG().askOverwrite("Output file exists. Overwrite?") )
	return false;

    writeAscii();
    return false;
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    const bool isgf = typfld_->getIntValue() == 2;
    attrSel( cb );
    addZChg( cb );

    if ( isgf && gfname_.isEmpty() )
	settingsCB( cb );
}


void uiExportHorizon::inpSel( CallBacker* )
{
    const IOObj* ioobj = infld_->selIOObj();
    if ( ioobj )
	gfname_ = ioobj->name();
}


void uiExportHorizon::addZChg( CallBacker* )
{
    settingsbutt_->display( typfld_->getIntValue()==2 );
    zfld_->display( typfld_->getIntValue() != 2 );
    if ( transfld_ )
	transfld_->display(typfld_->getIntValue()==2||zfld_->getIntValue()==2);

    bool displayunit = typfld_->getIntValue() != 2 && zfld_->getIntValue()!=1;
    if ( displayunit )
    {
	FixedString zdomain = getZDomain();

	if ( zdomain==ZDomain::sKeyDepth() )
	{
	    if ( unitsel_->box()->textOfItem(0)[0]!='M' )
	    {
		unitsel_->box()->setItemText( 0, "Meter" );
		unitsel_->box()->setItemText( 1, "Feet" );
		unitsel_->box()->setCurrentItem( 
			SI().depthsInFeetByDefault() ? 1 : 0 );
	    }
	    displayunit = true;
	}
	else if ( zdomain==ZDomain::sKeyTWT() )
	{
	    unitsel_->box()->setItemText( 0, "Second" );
	    unitsel_->box()->setItemText( 1, "Millisecond" );
	    displayunit = true;
	}
    }

    unitsel_->display( displayunit );
}


FixedString uiExportHorizon::getZDomain() const
{
    FixedString zdomain = SI().getZDomainString();
    if ( zfld_->getIntValue()==2 && transfld_ )
    {
	FixedString transdomain = transfld_->getZDomain();
	if ( !transdomain.isEmpty() )
	    zdomain = transdomain;
    }

    return zdomain;
}


void uiExportHorizon::attrSel( CallBacker* )
{
    const bool isgf = typfld_->getIntValue() == 2;
    udffld_->display( !isgf && infld_->haveAttrSel() );
}


void uiExportHorizon::settingsCB( CallBacker* )
{
    if ( typfld_->getIntValue() != 2 )
	return;

    uiDialog::Setup setup( "IESX Setup", 0, mNoHelpID );
    uiDialog dlg( this, setup );

    uiGenInput* namefld = new uiGenInput( &dlg, "Horizon name in file" );
    uiGenInput* commentfld = new uiGenInput( &dlg, "[Comment]" );
    commentfld->attach( alignedBelow, namefld );
    namefld->setText( gfname_.buf() );
    commentfld->setText( gfcomment_.buf() );

    while ( dlg.go() )
    {
	FixedString nm = namefld->text();
	if ( nm.isEmpty() )
	{
	    uiMSG().error( "No name selected" );
	    continue;
	}
 
	gfname_ = namefld->text();
	gfcomment_ = commentfld->text();
 	return;
    }
}
