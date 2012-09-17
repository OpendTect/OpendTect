/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiexphorizon.cc,v 1.78 2011/05/09 05:42:38 cvssatyaki Exp $";

#include "uiexphorizon.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"
#include "uiunitsel.h"

#include <stdio.h>


static const char* zmodes[] = { sKey::Yes, sKey::No, "Transformed", 0 };
static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };
static const char* hdrtyps[] = { "No", "Single line", "Multi line", 0 };


uiExportHorizon::uiExportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Export Horizon",
				     "Specify output format","104.0.1"))
{
    setCtrlStyle( DoAndStay );
    setModal( false );
    setDeleteOnClose( false );

    infld_ = new uiSurfaceRead( this,
	    	     uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::keyword())
		     		    .withsubsel(true) );
    infld_->inpChange.notify( mCB(this,uiExportHorizon,inpSel) );
    infld_->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );

    typfld_ = new uiGenInput( this, "Output type", StringListInpSpec(exptyps) );
    typfld_->attach( alignedBelow, infld_ );
    typfld_->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    settingsbutt_ = new uiPushButton( this, "Settings",
	    			      mCB(this,uiExportHorizon, settingsCB),
				      false);
    settingsbutt_->attach( rightOf, typfld_ );

    zfld_ = new uiGenInput( this, "Output Z", StringListInpSpec(zmodes) );
    zfld_->valuechanged.notify( mCB(this,uiExportHorizon,addZChg ) );
    zfld_->attach( alignedBelow, typfld_ );

    uiT2DConvSel::Setup su( 0, false );
    su.ist2d( SI().zIsTime() );
    transfld_ = new uiT2DConvSel( this, su );
    transfld_->display( false );
    transfld_->attach( alignedBelow, zfld_ );

    unitsel_ = new uiUnitSel( this, SI().zIsTime() ? PropertyRef::Time
	    					   : PropertyRef::Dist,
				    "Z Unit" );
    unitsel_->attach( alignedBelow, transfld_ );

    headerfld_ = new uiGenInput( this, "Header", StringListInpSpec(hdrtyps) );
    headerfld_->attach( alignedBelow, unitsel_ );

    udffld_ = new uiGenInput( this, "Undefined value",
	    		      StringInpSpec(sKey::FloatUdf) );
    udffld_->attach( alignedBelow, headerfld_ );

    outfld_ = new uiFileInput( this, "Output Ascii file",
	    		       uiFileInput::Setup().forread(false) );
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
    cleanupString( hnm.buf(), false, false, false );
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

static void writeGF( std::ostream& strm, const BinID& bid, float z,
		     float val, const Coord& crd, int segid )
{
    static char buf[mDataGFLineLen+2];
    const float crl = bid.crl;
    const float gfval = mIsUdf(val) ? mGFUndefValue : val;
    const float depth = mIsUdf(z) ? mGFUndefValue : z;
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	     crd.x, crd.y, segid, 14, depth, crl, crl, bid.crl, gfval, bid.inl,
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


void uiExportHorizon::writeHeader( std::ostream& strm )
{
    if ( headerfld_->getIntValue() == 0 )
	return;

    BufferStringSet selattribs;
    if ( infld_->haveAttrSel() )
	infld_->getSelAttributes( selattribs );

    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getIntValue() != 1;
    if ( headerfld_->getIntValue() == 1 )
    {
	BufferString posstr = doxy ? "\"X\"\t\"Y\""
				   : "\"Inline\"\t\"Crossline\"";
	if ( addzpos ) posstr += "\t\"Z\"";

	for ( int idx=0; idx<selattribs.size(); idx++ )
	{
	    posstr += "\t\"";
	    posstr += selattribs.get( idx ); posstr += "\"";
	}

	strm << "# " << posstr.buf();
    }
    else
    {
	if ( doxy )
	    strm << "# 1: X\n# 2: Y";
	else
	    strm << "# 1: Inline\n# 2: Crossline";
	if ( addzpos )
	    strm << "\n# 3: Z";

	int colidx = addzpos ? 4 : 3;
	for ( int idx=0; idx<selattribs.size(); idx++ )
	{
	    strm << "\n# " << colidx << ": ";
	    strm << selattribs.get( idx );
	    colidx++;
	}
    }

    strm << "\n# - - - - - - - - - -\n";
}


bool uiExportHorizon::writeAscii()
{
    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getIntValue() != 1;
    const bool dogf = typfld_->getIntValue() == 2;

    RefMan<ZAxisTransform> zatf = 0;
    IOPar iop;
    if ( zfld_->getIntValue()==2 )
    {
	if ( !transfld_->fillPar( iop ) )
	    return false;

	zatf = ZAxisTransform::create( iop );
	if ( !zatf )
	{
	    uiMSG().message("Transform of selected option is not implemented");
	    return false;
	}
    }

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

    const UnitOfMeasure* unit = unitsel_->getUnit();
    TypeSet<int>& sections = sels.selsections;
    int zatvoi = -1;
    if ( zatf && zatf->needsVolumeOfInterest() ) //Get BBox
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
 
 	if ( !first && zatf->needsVolumeOfInterest() )
 	{
 	    zatvoi = zatf->addVolumeOfInterest( bbox, false );
 	    if ( !zatf->loadDataIfMissing( zatvoi, &taskrunner ) )
 	    {
 		uiMSG().error( "Cannot load data for z-transform" );
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
	else
	{
	    *sdo.ostrm << std::fixed;
	    writeHeader( *sdo.ostrm );
	}

	const EM::SectionID sectionid = hor->sectionID( sectionidx );
	PtrMan<EM::EMObjectIterator> it = hor->createIterator( sectionid );
	BufferString str;
	while ( true )
	{
	    const EM::PosID posid = it->next();
	    if ( posid.objectID()==-1 )
		break;

	    Coord3 crd = hor->getPos( posid );
	    if ( zatf )
		crd.z = zatf->transform( crd );

	    if ( zatf && SI().depthsInFeetByDefault() )
	    {
		const UnitOfMeasure* uom = UoMR().get( "ft" );
		crd.z = uom->getSIValue( crd.z );
	    }

	    if ( !mIsUdf(crd.z) && unit )
		crd.z = unit->userValue( crd.z );

	    if ( dogf )
	    {
		const BinID bid = SI().transform( crd );
		const float auxvalue = nrattribs > 0
		    ? hor->auxdata.getAuxDataVal(0,posid) : mUdf(float);
		writeGF( *sdo.ostrm, bid, crd.z, auxvalue, crd, sidx );
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
		    str = "\t"; str += crd.z;
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

    if ( zatf && zatvoi>=0 )
	zatf->removeVolumeOfInterest( zatvoi );

    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( !strcmp(outfld_->fileName(),"") )
	mErrRet( "Please select output file" );

    if ( File::exists(outfld_->fileName()) &&
	    	      !uiMSG().askOverwrite("Output file exists. Overwrite?") )
	return false;

    const bool res = writeAscii();
    if ( res )
	uiMSG().message( "Horizon successfully exported" );
    return false;
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    attrSel( cb );
    addZChg( cb );

    const bool isgf = typfld_->getIntValue() == 2;
    headerfld_->display( !isgf );
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
    const bool isgf = typfld_->getIntValue() == 2;
    settingsbutt_->display( isgf );
    zfld_->display( !isgf );
    transfld_->display( !isgf && zfld_->getIntValue()==2 );

    const bool displayunit = !isgf && zfld_->getIntValue()!=1;
    if ( displayunit )
    {
	FixedString zdomain = getZDomain();
	if ( zdomain==ZDomain::sKeyDepth() )
	{
	    unitsel_->setPropType( PropertyRef::Dist );
	    if ( SI().zInFeet() || SI().depthsInFeetByDefault() )
		unitsel_->setUnit( "Feet" );
	}
	else if ( zdomain==ZDomain::sKeyTime() )
	{
	    unitsel_->setPropType( PropertyRef::Time );
	    unitsel_->setUnit( "Milliseconds" );
	}
    }

    unitsel_->display( displayunit );
}


FixedString uiExportHorizon::getZDomain() const
{
    FixedString zdomain = ZDomain::SI().key();
    RefMan<ZAxisTransform> zat = 0;
    if ( typfld_->getIntValue()==2 || zfld_->getIntValue()==2 )
    {
	IOPar iop;
	transfld_->fillPar( iop, true );
	zat = ZAxisTransform::create( iop );
	if ( zat )
	    zdomain = zat->toZDomainKey();
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

    uiDialog dlg( this, uiDialog::Setup("IESX details",mNoDlgTitle,mNoHelpID) );
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
