/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2002
________________________________________________________________________

-*/

#include "uiexphorizon.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"
#include "uiunitsel.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "uiioobjselgrp.h"
#include "transl.h"
#include "emioobjinfo.h"

#include "od_ostream.h"
#include <ostream>

#include <stdio.h> // for sprintf


static uiStringSet zmodes()
{
    uiStringSet zmodes;
    zmodes.add( uiStrings::sYes() );
    zmodes.add( uiStrings::sNo() );
    zmodes.add( uiStrings::sTransform() );
    return zmodes;
}

static uiStringSet exptyps()
{
    uiStringSet exptyps;
    exptyps.add( toUiString("%1/%2").arg(uiStrings::sX())
						    .arg(uiStrings::sY()) );
    exptyps.add( toUiString("%1/%2").arg(uiStrings::sInl())
						    .arg(uiStrings::sCrl()) );
    exptyps.add( toUiString("IESX (3d_ci7m)") );
    return exptyps;
}

static uiStringSet hdrtyps()
{
    uiStringSet hdrtyps;
    hdrtyps.add(uiStrings::sNo());
    hdrtyps.add( od_static_tr("hdrtyps", "Single Line") );
    hdrtyps.add( od_static_tr("hdrtyps", "Multi Line") );
    return hdrtyps;
}

#define mHdr1GFLineLen 102
#define mDataGFLineLen 148


mExpClass(uiEarthModel) write3DHorASCII : public Executor
{ mODTextTranslationClass(write3DHorASCII)
public:

	struct Setup
	{
	    Setup()
		: doxy_(true)
		, addzpos_(true)
		, dogf_(false)
		, issingle_(true)
		, nrattrib_(1)
		, udfstr_(mUdf(float))
	    {}

	    mDefSetupMemb(bool,doxy)
	    mDefSetupMemb(bool,addzpos)
	    mDefSetupMemb(bool,dogf)
	    mDefSetupMemb(bool,issingle)
	    mDefSetupMemb(int,nrattrib)
	    mDefSetupMemb(BufferString,udfstr)
	};

				write3DHorASCII(od_ostream&,
					const EM::Horizon3D*,
					const ZAxisTransform* zatf,
					const UnitOfMeasure*,
					const Coords::CoordSystem*,
					const Setup&);

    int				nextStep();
    uiString			message() const	{ return msg_; }
    uiString			nrDoneText() const;
    od_int64			nrDone() const;
    od_int64			totalNr() const;

protected:
    od_ostream&			stream_;
    int				maxsize_;
    const EM::Horizon3D*	hor_;
    EM::ObjectIterator*		it_;
    const ZAxisTransform*	zatf_;
    const UnitOfMeasure*	unit_;
    uiString			msg_;
    int				counter_;
    ConstRefMan<Coords::CoordSystem>	coordsys_;
    const Setup			setup_;

};



write3DHorASCII::write3DHorASCII( od_ostream& stream,const EM::Horizon3D* hor,
	const ZAxisTransform* zatf, const UnitOfMeasure* unit,
	const Coords::CoordSystem* crs, const Setup& su )
	: Executor(hor->name())
	, stream_(stream)
	, setup_(su)
	, hor_(hor)
	, zatf_(zatf)
	, unit_(unit)
	, counter_(0)
	, coordsys_(crs)
{
    it_ = hor->createIterator();
    maxsize_ = it_->maximumSize();
}


od_int64 write3DHorASCII::nrDone() const
{
    return counter_ < maxsize_ ? counter_ : -1;
}


uiString write3DHorASCII::nrDoneText() const
{
    return tr("Number of points processed");
}


od_int64 write3DHorASCII::totalNr() const
{
    return maxsize_;
}


static void writeGF( od_ostream& strm, const BinID& bid, float z,
		     float val, const Coord& crd, int segid )
{
    char buf[mDataGFLineLen+2];
    const float crl = mCast( float, bid.crl() );
    const float gfval = (float) ( mIsUdf(val) ? MAXFLOAT : val );
    const float depth = (float) ( mIsUdf(z) ? MAXFLOAT : z );
#ifdef __win__
    sprintf_s( buf, mDataGFLineLen + 2,
#else
    sprintf( buf,
#endif
	    "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	  crd.x_, crd.y_, segid, 14, depth,
	  crl, crl, bid.crl(), gfval, bid.inl(), "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


int write3DHorASCII::nextStep()
{
    BufferString str;
    const EM::PosID posid = it_->next();
    if ( counter_ > maxsize_ )
	return Executor::Finished();

    if ( !setup_.issingle_ )
	stream_ << '"' << hor_->name() << '"' << od_tab;

    Coord3 crd = hor_->getPos( posid );
    if ( coordsys_ != SI().getCoordSystem() )
	crd.setXY( coordsys_->convertFrom(crd.getXY(),*SI().getCoordSystem()) );
    const BinID bid = SI().transform( crd.getXY() );
    if ( zatf_ )
	crd.z_ = zatf_->transformTrc( TrcKey(bid), (float)crd.z_ );

    if ( zatf_ && SI().depthsInFeet() )
    {
	const UnitOfMeasure* uom = UoMR().get( "ft" );
	crd.z_ = uom->getSIValue( crd.z_ );
    }

    if ( !mIsUdf(crd.z_) && unit_ )
	crd.z_ = unit_->userValue( crd.z_ );

    if ( setup_.dogf_ )
    {
	const float auxvalue = setup_.nrattrib_ > 0
	    ? hor_->auxdata.getAuxDataVal(0,posid) : mUdf(float);
	writeGF( stream_, bid, (float) crd.z_, auxvalue, crd.getXY(), 0 );
	Executor::MoreToDo();
    }

    if ( !setup_.doxy_ )
    {
	stream_ << bid.inl() << od_tab << bid.crl();
    }
    else
    {
	// ostreams print doubles awfully
	str.setEmpty();
	str += crd.x_; str += od_tab; str += crd.y_;
	stream_ << str;
    }

    if ( setup_.addzpos_ )
    {
	if ( mIsUdf(crd.z_) )
	    stream_ << od_tab << setup_.udfstr_;
	else
	{
	    str = od_tab; str += crd.z_;
	    stream_ << str;
	}
    }

    for ( int idx=0; idx<setup_.nrattrib_; idx++ )
    {
	const float auxvalue = hor_->auxdata.getAuxDataVal( idx, posid );
	if ( mIsUdf(auxvalue) )
	    stream_ << od_tab << setup_.udfstr_;
	else
	{
	    str = od_tab; str += auxvalue;
	    stream_ << str;
	}
    }

    stream_ << od_newline;

    if ( setup_.dogf_ ) stream_ << "EOD";
    counter_++;
    stream_.flush();
    if ( stream_.isBad() )
    {
	msg_ =	tr("Cannot write output file");
	Executor::ErrorOccurred();
    }
    return Executor::MoreToDo();
}


uiExportHorizon::uiExportHorizon( uiParent* p, bool isbulk)
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport(
			      uiStrings::sHorizon(!isbulk?1:mPlural) ),
			      mNoDlgTitle,
			      mODHelpKey(mExportHorizonHelpID) ))
    , isbulk_(isbulk)
    , bulkinfld_(0)
    , infld_(0)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    setModal( false );
    setDeleteOnClose( false );
    IOObjContext ctxt = mIOObjContext( EMHorizon3D );
    uiIOObjSelGrp::Setup stup; stup.choicemode_ =
					OD::ChoiceMode::ChooseAtLeastOne;
    if ( !isbulk_ )
    {
	infld_ = new uiSurfaceRead( this,
		 uiSurfaceRead::Setup(EMHorizon3DTranslatorGroup::sGroupName())
		   .withsubsel(true).withsectionfld(false).multisubsel(true) );
	infld_->inpChange.notify( mCB(this,uiExportHorizon,inpSel) );
	infld_->attrSelChange.notify( mCB(this,uiExportHorizon,attrSel) );
    }
    else
	bulkinfld_ = new uiIOObjSelGrp( this, ctxt,
					uiStrings::sHorizon(mPlural), stup );

    typfld_ = new uiGenInput( this, uiStrings::phrOutput( uiStrings::sType() ),
			      StringListInpSpec(exptyps()) );
    if ( !isbulk_ )
	typfld_->attach( alignedBelow, infld_ );
    else
	typfld_->attach( alignedBelow, bulkinfld_ );
    typfld_->valuechanged.notify( mCB(this,uiExportHorizon,typChg) );

    settingsbutt_ = uiButton::getStd( this, OD::Settings,
				      mCB(this,uiExportHorizon,settingsCB),
				      false );
    settingsbutt_->attach( rightOf, typfld_ );

    zfld_ = new uiGenInput( this, uiStrings::phrOutput( uiStrings::sZ() ),
			    StringListInpSpec(zmodes()) );
    zfld_->valuechanged.notify( mCB(this,uiExportHorizon,addZChg ) );
    zfld_->attach( alignedBelow, typfld_ );

    coordsysselfld_ = new Coords::uiCoordSystemSel( this );
    coordsysselfld_->attach( alignedBelow, zfld_ );
    coordsysselfld_->display( false );

    uiT2DConvSel::Setup su( 0, false );
    su.ist2d( SI().zIsTime() );
    transfld_ = new uiT2DConvSel( this, su );
    transfld_->display( false );
    transfld_->attach( alignedBelow, coordsysselfld_ );

    unitsel_ = new uiUnitSel( this, tr("Z Unit") );
    unitsel_->attach( alignedBelow, transfld_ );

    headerfld_ = new uiGenInput( this, uiStrings::sHeader(),
				 StringListInpSpec(hdrtyps()) );
    headerfld_->attach( alignedBelow, unitsel_ );

    udffld_ = new uiGenInput( this, tr("Undefined value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->attach( alignedBelow, headerfld_ );

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::sOutputASCIIFile(), fssu );
    outfld_->attach( alignedBelow, udffld_ );

    addZChg(0);

    if ( !isbulk_ )
    {
	typChg( 0 );
	inpSel( 0 );
    }
}


uiExportHorizon::~uiExportHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }


static void initGF( od_ostream& strm, const char* hornm,
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    hnm.clean();
#ifdef __win__
    sprintf_s( gfbuf, mHdr1GFLineLen + 2,
#else
    sprintf( gfbuf,
#endif
		"PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		    "", "", SI().xyInFeet() ? "ft" : "m " );
    int sz = hnm.size(); if ( sz > 17 ) sz = 17;
    OD::memCopy( gfbuf+8, hnm.buf(), sz );
    hnm = comment;
    sz = hnm.size(); if ( sz > 45 ) sz = 45;
    OD::memCopy( gfbuf+35, hnm.buf(), sz );
    strm << gfbuf << "SNAPPING PARAMETERS 5     0 1" << od_endl;
}


void uiExportHorizon::writeHeader( od_ostream& strm )
{
    if ( headerfld_->getIntValue() == 0 )
	return;

    BufferStringSet selattribs;
    if ( !isbulk_ && infld_->haveAttrSel() )
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
    DBKeySet dbkeyset;
    if ( !getInputDBKeys(dbkeyset) )
	mErrRet(tr("Cannot find object in database"))
    ObjectSet<EM::IOObjInfo> emioinfoset;
    for ( int idx=0; idx<dbkeyset.size(); idx++ )
    {
	EM::IOObjInfo* emioinfo =  new EM::IOObjInfo(dbkeyset.get(idx));
	emioinfoset.add(emioinfo);
    }

    const bool doxy = typfld_->getIntValue() == 0;
    const bool addzpos = zfld_->getIntValue() != 1;
    const bool dogf = typfld_->getIntValue() == 2;

    RefMan<ZAxisTransform> zatf = 0;
    if ( zfld_->getIntValue()==2 )
    {
	zatf = transfld_->getSelection();
	if ( !zatf )
	    { uiMSG().error( mINTERNAL("transform not impl.") ); return false; }
    }

    BufferString udfstr = udffld_->text();
    if ( udfstr.isEmpty() ) udfstr = sKey::FloatUdf();

    BufferString basename = outfld_->fileName();

    EM::ObjectManager& mgr = EM::Hor3DMan();
    EM::SurfaceIOData sd;
    uiString errmsg;
    BufferString fname( basename );
    od_ostream stream( fname );
    for ( int horidx=0; horidx<emioinfoset.size(); horidx++ )
    {
	const IOObj* ioobj = !isbulk_ ? infld_->selIOObj() :
					emioinfoset.get(horidx)->ioObj();
	if ( !ioobj ) mErrRet(uiStrings::phrCannotFind( tr("horizon object")));

	if ( !mgr.getSurfaceData(ioobj->key(),sd,errmsg) )
	    mErrRet( errmsg )

	EM::SurfaceIODataSelection sels( sd );

	if ( !isbulk_ )
	    infld_->getSelection( sels );


	sels.selvalues.erase();

	RefMan<EM::Object> emobj = mgr.createTempObject( ioobj->group() );
	if ( !emobj )
	    mErrRet(uiStrings::phrCannotCreateHor())

	emobj->setDBKey( ioobj->key() );
	mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
	PtrMan<Executor> loader = hor->geometry().loader( &sels );
	if ( !loader )
	    mErrRet( uiStrings::phrCannotRead( uiStrings::sHorizon() ) )
	uiTaskRunnerProvider trprov( this );

	if ( !trprov.execute( *loader ) ) return false;

	if ( !isbulk_ )
	    infld_->getSelection( sels );

	uiUserShowWait usw( this, uiStrings::sSavingData() );
	if ( dogf && sels.selvalues.size() > 1 &&
	!uiMSG().askContinue(tr("Only the first selected attribute "
				"will be used.\nDo you wish to continue?")) )
	    return false;

	if ( !isbulk_ && !sels.selvalues.isEmpty() )
	{
	    ExecutorGroup exgrp( "Reading aux data" );
		for ( int idx=0; idx<sels.selvalues.size(); idx++ )
		    exgrp.add( hor->auxdata.auxDataLoader(
						    sels.selvalues[idx]) );

	    if ( !trprov.execute( exgrp ) ) return false;
	}

	const UnitOfMeasure* unit = unitsel_->getUnit();
	int zatvoi = -1;
	if ( zatf && zatf->needsVolumeOfInterest() ) //Get BBox
	{
	    TrcKeyZSampling bbox;
	    bool first = true;
	    PtrMan<EM::ObjectIterator> it = hor->createIterator();
	    while ( true )
	    {
		const EM::PosID posid = it->next();
		if ( posid.isInvalid() )
		    break;

		const Coord3 crd = hor->getPos( posid );
		if ( !crd.isDefined() )
		    continue;

		const BinID bid = SI().transform( crd.getXY() );
		if ( first )
		{
		    first = false;
		    bbox.hsamp_.start_ = bbox.hsamp_.stop_ = bid;
		    bbox.zsamp_.start = bbox.zsamp_.stop = (float) crd.z_;
		}
		else
		{
		    bbox.hsamp_.include( bid );
		    bbox.zsamp_.include( (float) crd.z_ );
		}
	    }

	    if ( !first && zatf->needsVolumeOfInterest() )
	    {
		zatvoi = zatf->addVolumeOfInterest( bbox, false );
		if ( !zatf->loadDataIfMissing( zatvoi, trprov ) )
		{
		    uiMSG().error( tr("Cannot load data for z-transform") );
		    return false;
		}
	    }
	}

	const int nrattribs = hor->auxdata.nrAuxData();

	BufferString dispstr("Writing Horizon ");
	dispstr.add(hor->name());
	ExecutorGroup exphorgrp( dispstr );

	if ( stream.isBad() )
	{
	    mErrRet( uiStrings::phrCannotOpenOutpFile() );
	}

	if ( dogf )
	    initGF( stream, gfname_.buf(), gfcomment_.buf() );
	else
	{
	    stream.stdStream() << std::fixed;
	    writeHeader( stream );
	}

	write3DHorASCII::Setup su;
	su.addzpos( addzpos ).doxy( doxy ).dogf( dogf ).issingle( !isbulk_ )
					.nrattrib( nrattribs ).udfstr( udfstr );

	const Coords::CoordSystem* crs(0);
	if ( !coordsysselfld_->isDisplayed() )
	    crs = SI().getCoordSystem().ptr();
	else
	    crs = coordsysselfld_->getCoordSystem();

	write3DHorASCII* executor = new write3DHorASCII( stream, hor,
		      zatf.ptr(), unit, crs, su );
	exphorgrp.add(executor);
	if ( !trprov.execute(exphorgrp) ) return false;

	if ( zatf && zatvoi>=0 )
	    zatf->removeVolumeOfInterest( zatvoi );
    }

    return true;
}


bool uiExportHorizon::acceptOK()
{
    if ( zfld_->getIntValue()==2 )
    {
	if ( !transfld_->acceptOK() )
	    return false;
    }

    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::phrSelOutpFile() );

    if ( File::exists(outfnm) &&
	 !uiMSG().askOverwrite(uiStrings::phrOutputFileExistsOverwrite()) )
	return false;

    const bool res = writeAscii();
    if ( !res )
    {
	uiMSG().error( uiStrings::phrCannotWrite(tr("output file.")));
	return false;
    }
    uiString msg = tr("%1 successfully exported").arg(
				    uiStrings::sHorizon(!isbulk_?1:mPlural));
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    const bool shoulddisplay = SI().getCoordSystem() &&
	 SI().getCoordSystem()->isProjection() && (typfld_->getIntValue() == 0);
    coordsysselfld_->display(shoulddisplay);

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
    udffld_->display( !isgf && infld_ && infld_->haveAttrSel() );
}


bool uiExportHorizon::getInputDBKeys( DBKeySet& dbkeyset )
{
    if ( !isbulk_ )
    {
	const IOObj* ioobj = infld_->selIOObj();
	if ( !ioobj ) return false;
	dbkeyset.add(ioobj->key());
    }
    else
	bulkinfld_->getChosen(dbkeyset);
    return true;
}


void uiExportHorizon::settingsCB( CallBacker* )
{
    if ( typfld_->getIntValue() != 2 )
	return;

    uiDialog dlg( this, uiDialog::Setup(tr("IESX details"),
                                        mNoDlgTitle,mNoHelpKey));
    uiGenInput* namefld = new uiGenInput( &dlg, tr("Horizon name in file") );
    uiGenInput* commentfld = new uiGenInput( &dlg,
				uiStrings::sComment().optional() );
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
