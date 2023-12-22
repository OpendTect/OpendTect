/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiexphorizon.h"

#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "transl.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjselgrp.h"
#include "uiiosurface.h"
#include "uimultisurfaceread.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uit2dconvsel.h"
#include "uiunitsel.h"

#include "hiddenparam.h"

#include <stdio.h> // for sprintf


static uiStringSet exptyps()
{
    uiStringSet exptyps;
    exptyps.add( toUiString("%1/%2").arg(uiStrings::sX())
						    .arg(uiStrings::sY()) );
    exptyps.add( toUiString("%1/%2").arg(uiStrings::sInline())
						.arg(uiStrings::sCrossline()) );
    exptyps.add( toUiString("%1/%2 and %3/%4")
		 .arg(uiStrings::sX())
		 .arg(uiStrings::sY())
		 .arg(uiStrings::sInline())
		 .arg(uiStrings::sCrossline()));
    exptyps.add( toUiString("IESX (3d_ci7m)") );
    return exptyps;
}

static uiStringSet hdrtyps()
{
    uiStringSet hdrtyps;
    hdrtyps.add( uiStrings::sNo() );
    hdrtyps.add( od_static_tr("hdrtyps", "Single Line") );
    hdrtyps.add( od_static_tr("hdrtyps", "Multi Line") );
    return hdrtyps;
}


#define mHdr1GFLineLen 102
#define mDataGFLineLen 148

class uiBulkHorHandlerGroup : public uiGroup
{ mODTextTranslationClass(uiBulkHorHandlerGroup)
public:
    uiBulkHorHandlerGroup( uiParent* p )
	: uiGroup(p,"Bulk Horizon List Group")
	, zdomtypupdated(this)
    {
	horzdomypefld_ = new uiGenInput( this, tr("Depth Domain"),
			    BoolInpSpec(true, uiStrings::sTime(),
			    uiStrings::sDepth()) );
	mAttachCB( horzdomypefld_->valueChanged,
	    uiBulkHorHandlerGroup::zDomainTypeChg );

	auto* multigrp = new uiGroup( this, "Multi Surface Read" );
	multigrp->attach( alignedBelow, horzdomypefld_ );
	const char* surftype = EMHorizon3DTranslatorGroup::sGroupName();
	const ZDomain::Info& depthinfo = SI().depthsInFeet() ?
	    ZDomain::DepthFeet() : ZDomain::DepthMeter();
	multisurfdepthread_ = new uiMultiSurfaceRead( multigrp, surftype,
								&depthinfo );
	multisurfdepthread_->display( false );
	multisurftimeread_ = new uiMultiSurfaceRead( multigrp, surftype,
							&ZDomain::TWT() );
	multisurftimeread_->display( true );
    }


    ~uiBulkHorHandlerGroup()
    {
	detachAllNotifiers();
    }

    void zDomainTypeChg( CallBacker* )
    {
	const bool istime = isTime();
	multisurfdepthread_->display( !istime );
	multisurftimeread_->display( istime );
	zdomtypupdated.trigger();
    }


    const ZDomain::Info& zDomain() const
    {
	if ( isTime() )
	    return ZDomain::TWT();
	else if ( SI().depthsInFeet() )
	    return ZDomain::DepthFeet();

	return ZDomain::DepthMeter();
    }

    void getSelectedMIDs( TypeSet<MultiID>& selmids )
    {
	if ( isTime() )
	    multisurftimeread_->getSurfaceIds( selmids );
	else
	    multisurfdepthread_->getSurfaceIds( selmids );
    }

    void getSurfaceSelection( EM::SurfaceIODataSelection& sel )
    {
	if ( isTime() )
	    multisurftimeread_->getSurfaceSelection( sel );
	else
	    multisurfdepthread_->getSurfaceSelection( sel );
    }

    Notifier<uiBulkHorHandlerGroup> zdomtypupdated;

protected:
    uiMultiSurfaceRead*		    multisurfdepthread_;
    uiMultiSurfaceRead*		    multisurftimeread_;
    uiGenInput*			    horzdomypefld_;

    bool isTime() const
    {
	return horzdomypefld_->getBoolValue();
    }
};


class Write3DHorASCII : public Executor
{ mODTextTranslationClass(Write3DHorASCII)
public:
	struct Setup
	{
	    Setup()
		: doxy_(true)
		, doic_(true)
		, addzpos_(true)
		, dogf_(false)
		, issingle_(true)
		, nrattrib_(1)
	    {
		udfstr_ = mUdf(float);
	    }

	  mDefSetupMemb(bool,doxy)
	  mDefSetupMemb(bool,doic)
	  mDefSetupMemb(bool,addzpos)
	  mDefSetupMemb(bool,dogf)
	  mDefSetupMemb(bool,issingle)
	  mDefSetupMemb(int,nrattrib)
	  mDefSetupMemb(BufferString,udfstr)
      };

Write3DHorASCII( od_ostream&, int sectionidx, int sidx,
		 const EM::Horizon3D*, const UnitOfMeasure*,
		 const Coords::CoordSystem*, const Setup&);

    int				nextStep() override;
    const char*			message() const override    { return msg_; }
    const char*			nrDoneText() const override;
    od_int64			nrDone() const override;
    od_int64			totalNr() const override;

protected:
    od_ostream&			stream_;
    const int			sidx_;
    int				maxsize_;
    const EM::Horizon3D*	hor_;
    EM::EMObjectIterator*	it_;
    const UnitOfMeasure*	unit_;
    BufferString		msg_;
    int				counter_;
    ConstRefMan<Coords::CoordSystem>  coordsys_;
    const Setup			setup_;
};



Write3DHorASCII::Write3DHorASCII( od_ostream& stream, const int sectionidx,
    const int sidx, const EM::Horizon3D* hor, const UnitOfMeasure* unit,
    const Coords::CoordSystem* crs, const Setup& su )
    : Executor(hor->name())
    , stream_(stream)
    , sidx_(sidx)
    , hor_(hor)
    , unit_(unit)
    , counter_(0)
    , coordsys_(crs)
    , setup_(su)
{
    it_ = hor->createIterator();
    maxsize_ = it_->maximumSize();
}


od_int64 Write3DHorASCII::nrDone() const
{
    return counter_ < maxsize_ ? counter_ : -1;
}


const char* Write3DHorASCII::nrDoneText() const
{
    return "Number of points processed";
}


od_int64 Write3DHorASCII::totalNr() const
{
    return maxsize_;
}


static void writeGF( od_ostream& strm, const BinID& bid, float z,
	      float val, const Coord& crd, int segid )
{
    char buf[mDataGFLineLen+2];
    const double crl = double( bid.crl() );
    const double gfval = double( mIsUdf(val) ? MAXFLOAT : val );
    const double depth = double( mIsUdf(z) ? MAXFLOAT : z );
    od_sprintf( buf, mDataGFLineLen+2,
	  "%16.8E%16.8E%3d%3d%9.2f%10.2f%10.2f%5d%14.7E I%7d %52s\n",
	  crd.x, crd.y, segid, 14, depth,
	  crl, crl, bid.crl(), gfval, bid.inl(),
	     "" );
    buf[96] = buf[97] = 'X';
    strm << buf;
}


int Write3DHorASCII::nextStep()
{
    BufferString str;
    const EM::PosID posid = it_->next();
    if ( !posid.objectID().isValid() || counter_ > maxsize_ )
	return Executor::Finished();

    if ( !setup_.issingle_ )
    {
	BufferString hornm( hor_->name() );
	stream_ << hornm.quote('\"') << od_tab;
    }

    Coord3 crd = hor_->getPos( posid );
    const BinID bid = SI().transform( crd.coord() );

    if ( !mIsUdf(crd.z) && unit_ )
	crd.z = unit_->userValue( crd.z );

    if ( coordsys_ && !(*coordsys_ == *SI().getCoordSystem()) )
    {
	const Coord crdxy =
		coordsys_->convertFrom( crd.coord(), *SI().getCoordSystem() );
	crd.setXY( crdxy.x, crdxy.y );
    }

    if ( setup_.dogf_ )
    {
	const float auxvalue = setup_.nrattrib_ > 0
	    ? hor_->auxdata.getAuxDataVal(0,posid) : mUdf(float);
	writeGF( stream_, bid, (float) crd.z, auxvalue,
		 crd.coord(), sidx_ );
	Executor::MoreToDo();
    }

    str.setEmpty();
    if ( setup_.doxy_ )
	str.add( crd.x, 2 ).addTab().add( crd.y, 2 );

    if ( setup_.doxy_ && setup_.doic_ )
	str.addTab();

    if ( setup_.doic_ )
	str.add( bid.inl() ).addTab().add( bid.crl() );

    // ostreams print doubles awfully
    stream_ << str;

    str.setEmpty();
    if ( setup_.addzpos_ )
    {
	if ( mIsUdf(crd.z) )
	    stream_ << od_tab << setup_.udfstr_;
	else
	{
	    str.addTab().add( crd.z );
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

    if ( setup_.dogf_ )
	stream_ << "EOD";

    counter_++;
    stream_.flush();
    if ( stream_.isBad() )
    {
	msg_ =  "Cannot write output file";
	Executor::ErrorOccurred();
    }

    return Executor::MoreToDo();
}


// uiExportHorizon
static HiddenParam<uiExportHorizon,uiBulkHorHandlerGroup*>
						uiexporthorizonhpmgr_(nullptr);

uiExportHorizon::uiExportHorizon( uiParent* p, bool isbulk )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport(uiStrings::sHorizon()),
			mNoDlgTitle,mODHelpKey(mExportHorizonHelpID)))
    , isbulk_(isbulk)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    setDeleteOnClose( false );
    uiexporthorizonhpmgr_.setParam( this, nullptr );

    uiObject* attachobj = nullptr;
    if ( !isbulk )
    {
	infld_ = new uiSurfaceRead( this, uiSurfaceRead::Setup(
				    EMHorizon3DTranslatorGroup::sGroupName())
		    .withsubsel(true).withsectionfld(false) );
	mAttachCB( infld_->inpChange, uiExportHorizon::inpSel );
	mAttachCB( infld_->attrSelChange, uiExportHorizon::attrSel );
	attachobj = infld_->attachObj();
    }
    else
    {
	auto* bulkgrp = new uiBulkHorHandlerGroup( this );
	mAttachCB( bulkgrp->zdomtypupdated,
					uiExportHorizon::zDomainBulkTypeChg );
	attachobj = bulkgrp->attachObj();
	uiexporthorizonhpmgr_.setParam( this, bulkgrp );
    }

    typfld_ = new uiGenInput( this, uiStrings::phrOutput( uiStrings::sType() ),
			      StringListInpSpec(exptyps()) );
    mAttachCB( typfld_->valuechanged, uiExportHorizon::typChg );
    typfld_->attach( alignedBelow, attachobj );
    attachobj = typfld_->attachObj();

    settingsbutt_ = new uiPushButton( this, uiStrings::sSettings(),
				      mCB(this,uiExportHorizon,settingsCB),
				      false);
    settingsbutt_->attach( rightOf, typfld_ );

    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, attachobj );
	attachobj = coordsysselfld_->attachObj();
    }

    writezfld_ = new uiGenInput( this, tr("Write Z"), BoolInpSpec(true) );
    writezfld_->attach( alignedBelow, attachobj );
    mAttachCB( writezfld_->valueChanged, uiExportHorizon::addZChg );

    unitsel_ = new uiUnitSel( this, uiUnitSel::Setup(tr("Z Unit")) );
    unitsel_->setUnit( UnitOfMeasure::surveyDefZUnit() );
    unitsel_->attach( alignedBelow, writezfld_ );

    const uiStringSet& hdrtypes = hdrtyps();
    headerfld_ = new uiGenInput( this, tr("Header"),
						StringListInpSpec(hdrtypes) );
    headerfld_->setText( hdrtypes[1].getFullString() );
    headerfld_->attach( alignedBelow, unitsel_ );

    udffld_ = new uiGenInput( this, tr("Undefined value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->attach( alignedBelow, headerfld_ );

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->attach( alignedBelow, udffld_ );

    mAttachCB( postFinalize(), uiExportHorizon::typChg );
    if ( !isbulk )
	mAttachCB( postFinalize(), uiExportHorizon::inpSel );
}


uiExportHorizon::~uiExportHorizon()
{
    detachAllNotifiers();
    uiexporthorizonhpmgr_.removeParam( this );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


static void initGF( od_ostream& strm, const char* hornm,
		    const char* comment )
{
    char gfbuf[mHdr1GFLineLen+2];
    gfbuf[mHdr1GFLineLen] = '\0';
    BufferString hnm( hornm );
    hnm.clean();
    od_sprintf( gfbuf, mHdr1GFLineLen+2,
		"PROFILE %17sTYPE 1  4 %45s3d_ci7m.ifdf     %s ms\n",
		"", "", SI().getXYUnitString(false) );
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

    const int typ = typfld_->getIntValue();
    const bool doxy = typ==0 || typ==2;
    const bool doic = typ==1 || typ==2;
    const bool addzpos = writezfld_->getBoolValue();
    if ( headerfld_->getIntValue() == 1 )
    {
	BufferString posstr;
	if ( isbulk_ )
	    posstr.add( sKey::Name() ).addTab();

	if ( doxy )
	    posstr += "\"X\"\t\"Y\"";

	if ( doxy && doic )
	    posstr += "\t";

	if ( doic )
	    posstr += "\"Inline\"\t\"Crossline\"";

	if ( addzpos )
	    posstr += "\t\"Z\"";

	for ( int idx=0; idx<selattribs.size(); idx++ )
	{
	    posstr += "\t\"";
	    posstr += selattribs.get( idx ); posstr += "\"";
	}

	strm << "# " << posstr.buf();
    }
    else
    {
	if ( doxy && doic )
	    strm << "# 1: X\n# 2: Y\n# 3: Inline\n# 4: Crossline";
	else if ( doxy )
	    strm << "# 1: X\n# 2: Y";
	else if ( doic )
	    strm << "# 1: Inline\n# 2: Crossline";

	int colidx = doxy && doic ? 5 : 3;
	if ( addzpos )
	{
	    strm << "\n# " << colidx << ": Z";
	    colidx++;
	}

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
    TypeSet<MultiID> midset;
    if ( !getInputMIDs(midset) )
	mErrRet(tr("Cannot find object in database"))

    const int typ = typfld_->getIntValue();
    const bool doxy = typ==0 || typ==2;
    const bool doic = typ==1 || typ==2;
    const bool addzpos = writezfld_->getBoolValue();
    const bool dogf = exportToGF();
    BufferString udfstr = udffld_->text();
    if ( udfstr.isEmpty() )
	udfstr = sKey::FloatUdf();

    BufferString basename = outfld_->fileName();

    EM::EMManager& em = EM::EMM();
    EM::SurfaceIOData sd;
    uiString errmsg;
    BufferString fname( basename );
    od_ostream stream( fname );
    if ( stream.isBad() )
	mErrRet( uiStrings::sCantOpenOutpFile() );

    if ( !dogf )
    {
	stream.stdStream() << std::fixed;
	writeHeader( stream );
    }

    for ( int horidx=0; horidx<midset.size(); horidx++ )
    {
	ConstPtrMan<IOObj> ioobj = IOM().get( midset[horidx] );
	if ( !ioobj )
	    mErrRet(uiStrings::phrCannotFind( tr("horizon object")));

	if ( !em.getSurfaceData(ioobj->key(),sd,errmsg) )
	    mErrRet( errmsg )

	EM::SurfaceIODataSelection sels( sd );
	if ( isbulk_ )
	{
	    auto* bulkgrp = uiexporthorizonhpmgr_.getParam( this );
	    bulkgrp->getSurfaceSelection( sels );
	}
	else
	    infld_->getSelection( sels );


	sels.selvalues.erase();
	RefMan<EM::EMObject> emobj = em.createTempObject( ioobj->group() );
	if ( !emobj )
	    mErrRet(uiStrings::sCantCreateHor())

	emobj->setMultiID( ioobj->key() );
	mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
	PtrMan<Executor> loader = hor->geometry().loader( &sels );
	if ( !loader )
	    mErrRet( uiStrings::phrCannotRead( uiStrings::sHorizon() ) )

	uiTaskRunner taskrunner( this );
	if ( !TaskRunner::execute(&taskrunner,*loader) )
	    return false;

	if ( dogf && sels.selvalues.size() > 1 && uiMSG().askContinue(
			tr("Only the first selected attribute will be used\n"
				 "Do you wish to continue?")) )
	    return false;

	if ( !isbulk_ && !sels.selvalues.isEmpty() )
	{
	    ExecutorGroup exgrp( "Reading aux data" );
	    for ( int idx=0; idx<sels.selvalues.size(); idx++ )
		exgrp.add( hor->auxdata.auxDataLoader(sels.selvalues[idx]) );

	    if ( !TaskRunner::execute( &taskrunner, exgrp ) ) return false;
	}

	MouseCursorChanger cursorlock( MouseCursor::Wait );
	const UnitOfMeasure* unit = unitsel_->getUnit();
	const int nrattribs = hor->auxdata.nrAuxData();
	BufferString dispstr("Writing Horizon ");
	dispstr.add(hor->name());
	ExecutorGroup exphorgrp( dispstr );
	if ( stream.isBad() )
	    mErrRet( uiStrings::sCantOpenOutpFile() );

	if ( dogf )
	    initGF( stream, gfname_.buf(), gfcomment_.buf() );

	Write3DHorASCII::Setup su;
	su.addzpos( addzpos ).doxy( doxy ).doic( doic ).dogf( dogf )
	    .issingle( !isbulk_ ).nrattrib( nrattribs ).udfstr( udfstr );

	const Coords::CoordSystem* crs =
	    coordsysselfld_ ? coordsysselfld_->getCoordSystem() : nullptr;
	Write3DHorASCII* executor = new Write3DHorASCII( stream, 0, 0,
						    hor, unit, crs, su);
	exphorgrp.add(executor);
	if ( !TaskRunner::execute(&taskrunner,exphorgrp) )
	    return false;
    }

    return true;
}


bool uiExportHorizon::acceptOK( CallBacker* )
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    if ( File::exists(outfnm) &&
	 !uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	return false;

    const FilePath fnmfp( outfnm );
    SetExportToDir( fnmfp.pathOnly() );

    const bool res = writeAscii();
    if ( !res )
    {
      uiMSG().error( uiStrings::phrCannotWrite(tr("output file.")));
      return false;
    }

    uiString msg = tr("%1 successfully exported."
			"\n\nDo you want to export more %2?").arg(
				  uiStrings::sHorizon(!isbulk_?1:mPlural))
				  .arg(uiStrings::sHorizon(mPlural));
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


void uiExportHorizon::zDomainBulkTypeChg(CallBacker*)
{
    auto* bulkgrp = uiexporthorizonhpmgr_.getParam( this );
    const ZDomain::Info& zinfo = bulkgrp->zDomain();
    unitsel_->setUnit( UnitOfMeasure::zUnit(zinfo) );
}


bool uiExportHorizon::getInputMIDs( TypeSet<MultiID>& midset )
{
    if ( !isbulk_ )
    {
	const IOObj* ioobj = infld_->selIOObj();
	if ( !ioobj ) return false;

	MultiID mid = ioobj->key();
	midset.add(mid);
    }
    else
    {
	auto* bulkgrp = uiexporthorizonhpmgr_.getParam( this );
	bulkgrp->getSelectedMIDs( midset );
    }

    return true;
}


void uiExportHorizon::typChg( CallBacker* cb )
{
    if ( coordsysselfld_ )
    {
	const int typesel = typfld_->getIntValue();
	const bool expxy = typesel==0 || typesel==2;
	coordsysselfld_->display( expxy );
    }

    attrSel( cb );
    addZChg( cb );

    const bool isgf = exportToGF();
    headerfld_->display( !isgf );
    if ( isgf && gfname_.isEmpty() )
	settingsCB( cb );
}


void uiExportHorizon::inpSel( CallBacker* )
{
    const IOObj* ioobj = infld_ ? infld_->selIOObj() : nullptr;
    if ( ioobj )
    {
	gfname_ = ioobj->name();
	EM::IOObjInfo info( ioobj->key() );
	unitsel_->setUnit( info.getZUoM() );
    }
}


void uiExportHorizon::addZChg( CallBacker* )
{
    const bool isgf = exportToGF();
    settingsbutt_->display( isgf );
    writezfld_->display( !isgf );
}


bool uiExportHorizon::exportToGF() const
{
    return typfld_->getIntValue()==3;
}


void uiExportHorizon::attrSel( CallBacker* )
{
    const bool isgf = exportToGF();
    udffld_->display( !isgf && infld_ && infld_->haveAttrSel() );
}


StringView uiExportHorizon::getZDomain() const
{
    return StringView::empty();
}


void uiExportHorizon::settingsCB( CallBacker* )
{
    if ( typfld_->getIntValue() != 3 )
	return;

    uiDialog dlg( this, uiDialog::Setup(tr("IESX details"),
					mNoDlgTitle,mNoHelpKey) );
    uiGenInput* namefld = new uiGenInput( &dlg, tr("Horizon name in file") );
    uiGenInput* commentfld = new uiGenInput( &dlg, tr("[Comment]") );
    commentfld->attach( alignedBelow, namefld );
    namefld->setText( gfname_.buf() );
    commentfld->setText( gfcomment_.buf() );

    while ( dlg.go() )
    {
	StringView nm = namefld->text();
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
