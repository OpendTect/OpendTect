/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "od_ostream.h"
#include "od_helpids.h"

#include <stdio.h> // for sprintf


static const char* zmodes[] = { sKey::Yes(), sKey::No(), "Transformed", 0 };
static const char* exptyps[] = { "X/Y", "Inl/Crl", "IESX (3d_ci7m)", 0 };
static const char* hdrtyps[] = { "No", "Single line", "Multi line", 0 };


uiExportHorizon::uiExportHorizon( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Export Horizon"),mNoDlgTitle,
                                 mODHelpKey(mExportHorizonHelpID) ))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    setModal( false );
    setDeleteOnClose( false );

    infld_ = new uiSurfaceRead( this,
		uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::keyword())
		.withsubsel(true).withsectionfld(false) );
    infld_->inpChange.notify( mCB(this,uiExportHorizon,inpSel) );
    infld_->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );

    typfld_ = new uiGenInput( this, tr("Output type"), 
                              StringListInpSpec(exptyps) );
    typfld_->attach( alignedBelow, infld_ );
    typfld_->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    settingsbutt_ = new uiPushButton( this, uiStrings::sSettings(),
				      mCB(this,uiExportHorizon,settingsCB),
				      false);
    settingsbutt_->attach( rightOf, typfld_ );

    zfld_ = new uiGenInput( this, tr("Output Z"), StringListInpSpec(zmodes) );
    zfld_->valuechanged.notify( mCB(this,uiExportHorizon,addZChg ) );
    zfld_->attach( alignedBelow, typfld_ );

    uiT2DConvSel::Setup su( 0, false );
    su.ist2d( SI().zIsTime() );
    transfld_ = new uiT2DConvSel( this, su );
    transfld_->display( false );
    transfld_->attach( alignedBelow, zfld_ );

    unitsel_ = new uiUnitSel( this, "Z Unit" );
    unitsel_->attach( alignedBelow, transfld_ );

    headerfld_ = new uiGenInput( this, tr("Header"), 
                                 StringListInpSpec(hdrtyps) );
    headerfld_->attach( alignedBelow, unitsel_ );

    udffld_ = new uiGenInput( this, tr("Undefined value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->attach( alignedBelow, headerfld_ );

    outfld_ = new uiFileInput( this, "Output ASCII file",
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

static void initGF( od_ostream& strm, const char* hornm,
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    hnm.clean();
    sprintf( gfbuf, "PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		    "", "", SI().xyInFeet() ? "ft" : "m " );
    int sz = hnm.size(); if ( sz > 17 ) sz = 17;
    OD::memCopy( gfbuf+8, hnm.buf(), sz );
    hnm = comment;
    sz = hnm.size(); if ( sz > 45 ) sz = 45;
    OD::memCopy( gfbuf+35, hnm.buf(), sz );
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << od_endl;
}


#define mGFUndefValue 3.4028235E+38

static void writeGF( od_ostream& strm, const BinID& bid, float z,
		     float val, const Coord& crd, int segid )
{
    char buf[mDataGFLineLen+2];
    const float crl = mCast( float, bid.crl() );
    const float gfval = (float) ( mIsUdf(val) ? mGFUndefValue : val );
    const float depth = (float) ( mIsUdf(z) ? mGFUndefValue : z );
    sprintf( buf, "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	  crd.x, crd.y, segid, 14, depth, crl, crl, bid.crl(), gfval, bid.inl(),
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


void uiExportHorizon::writeHeader( od_ostream& strm )
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
    if ( zfld_->getIntValue()==2 )
    {
	zatf = transfld_->getSelection();
	if ( !zatf )
	{
	 uiMSG().message(tr("Transform of selected option is not implemented"));
	    return false;
	}
    }

    BufferString udfstr = udffld_->text();
    if ( udfstr.isEmpty() ) udfstr = sKey::FloatUdf();

    BufferString basename = outfld_->fileName();

    const IOObj* ioobj = infld_->selIOObj();
    if ( !ioobj ) mErrRet(tr("Cannot find horizon object"));

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !em.getSurfaceData(ioobj->key(),sd,errmsg) )
	mErrRet( errmsg )

    EM::SurfaceIODataSelection sels( sd );
    infld_->getSelection( sels );
    sels.selvalues.erase();

    RefMan<EM::EMObject> emobj = em.createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet(tr("Cannot create horizon"))

    emobj->setMultiID( ioobj->key() );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
    PtrMan<Executor> loader = hor->geometry().loader( &sels );
    if ( !loader ) mErrRet(tr("Cannot read horizon"))

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *loader ) ) return false;

    infld_->getSelection( sels );
    if ( dogf && sels.selvalues.size() > 1 &&
    !uiMSG().askContinue(tr("Only the first selected attribute will be used\n"
			     "Do you wish to continue?")) )
	return false;

    if ( !sels.selvalues.isEmpty() )
    {
	ExecutorGroup exgrp( "Reading aux data" );
	for ( int idx=0; idx<sels.selvalues.size(); idx++ )
	    exgrp.add( hor->auxdata.auxDataLoader(sels.selvalues[idx]) );

	if ( !TaskRunner::execute( &taskrunner, exgrp ) ) return false;
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
		    bbox.zrg.start = bbox.zrg.stop = (float) crd.z;
		}
		else
		{
		    bbox.hrg.include( bid );
		    bbox.zrg.include( (float) crd.z );
		}
	    }

	}

	if ( !first && zatf->needsVolumeOfInterest() )
	{
	    zatvoi = zatf->addVolumeOfInterest( bbox, false );
	    if ( !zatf->loadDataIfMissing( zatvoi, &taskrunner ) )
	    {
		uiMSG().error( tr("Cannot load data for z-transform") );
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

        od_ostream stream( fname );

	if ( stream.isBad() )
	{
            mErrRet( tr("Cannot open output file") );
	}

	if ( dogf )
	    initGF( stream, gfname_.buf(), gfcomment_.buf() );
	else
	{
	    stream.stdStream() << std::fixed;
	    writeHeader( stream );
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

	    if ( zatf && SI().depthsInFeet() )
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
		writeGF( stream, bid, (float) crd.z, auxvalue, crd, sidx );
		continue;
	    }

	    if ( !doxy )
	    {
		const BinID bid = SI().transform( crd );
		stream << bid.inl() << od_tab << bid.crl();
	    }
	    else
	    {
		// ostreams print doubles awfully
		str.setEmpty();
		str += crd.x; str += od_tab; str += crd.y;
		stream << str;
	    }

	    if ( addzpos )
	    {
		if ( mIsUdf(crd.z) )
		    stream << od_tab << udfstr;
		else
		{
		    str = od_tab; str += crd.z;
		    stream << str;
		}
	    }

	    for ( int idx=0; idx<nrattribs; idx++ )
	    {
		const float auxvalue = hor->auxdata.getAuxDataVal( idx, posid );
		if ( mIsUdf(auxvalue) )
		    stream << od_tab << udfstr;
		else
		{
		    str = od_tab; str += auxvalue;
		    stream << str;
		}
	    }

	    stream << od_newline;
	}

	if ( dogf ) stream << "EOD";

        stream.flush();
        if ( stream.isBad() )
            mErrRet( tr("Cannot write output file") );
    }

    if ( zatf && zatvoi>=0 )
	zatf->removeVolumeOfInterest( zatvoi );

    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    if ( zfld_->getIntValue()==2 )
    {
	if ( !transfld_->acceptOK() )
	    return false;
    }

    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( tr("Please select output file") );

    if ( File::exists(outfnm) &&
		  !uiMSG().askOverwrite(tr("Output file exists. Overwrite?")) )
	return false;

    const bool res = writeAscii();
    if ( res )
	uiMSG().message( tr("Horizon successfully exported") );
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
	    unitsel_->setPropType( PropertyRef::Dist );
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

    if ( typfld_->getIntValue()==2 || zfld_->getIntValue()==2 )
    {
	zdomain = transfld_->selectedToDomain();
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

    uiDialog dlg( this, uiDialog::Setup(tr("IESX details"),
                                        mNoDlgTitle,mNoHelpKey));
    uiGenInput* namefld = new uiGenInput( &dlg, tr("Horizon name in file") );
    uiGenInput* commentfld = new uiGenInput( &dlg, tr("[Comment]") );
    commentfld->attach( alignedBelow, namefld );
    namefld->setText( gfname_.buf() );
    commentfld->setText( gfcomment_.buf() );

    while ( dlg.go() )
    {
	FixedString nm = namefld->text();
	if ( nm.isEmpty() )
	{
	    uiMSG().error( tr("No name selected") );
	    continue;
	}

	gfname_ = namefld->text();
	gfcomment_ = commentfld->text();
	return;
    }
}
