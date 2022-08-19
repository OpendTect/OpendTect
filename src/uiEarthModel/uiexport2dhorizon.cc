/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiexport2dhorizon.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "ptrman.h"
#include "od_ostream.h"
#include "surfaceinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tableconvimpl.h"

#include "uichecklist.h"
#include "uicombobox.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#include <stdio.h>


static const char* hdrtyps[] = { "No", "Single line", "Multi line", nullptr };


uiExport2DHorizon::uiExport2DHorizon( uiParent* p,
			const ObjectSet<SurfaceInfo>& hinfos, bool isbulk )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("2D Horizon") ),
	       mNoDlgTitle, mODHelpKey(mExportHorizonHelpID) ))
    , hinfos_(hinfos)
    , isbulk_(isbulk)
{
    setOkText( uiStrings::sExport() );
    if ( isbulk_ )
    {
	IOObjContext ctxt = mIOObjContext( EMHorizon2D );
	uiIOObjSelGrp::Setup stup; stup.choicemode_ = OD::ChooseAtLeastOne;
	bulkinfld_ = new uiIOObjSelGrp( this, ctxt,
	    uiStrings::sHorizon( mPlural ), stup );
    }
    else
    {
	auto* lcbox = new uiLabeledComboBox( this,
			     uiStrings::phrSelect( uiStrings::sHorizon() ),
			     "Select 2D Horizon" );
	horselfld_ = lcbox->box();
	horselfld_->setHSzPol( uiObject::MedVar );
	mAttachCB( horselfld_->selectionChanged, uiExport2DHorizon::horChg );
	for ( int idx=0; idx<hinfos_.size(); idx++ )
	    horselfld_->addItem( mToUiStringTodo(hinfos_[idx]->name) );

	uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Select lines") );
	linenmfld_ = new uiListBox( this, su );
	linenmfld_->attach( alignedBelow, lcbox );
    }

    headerfld_ = new uiGenInput( this, tr("Header"),
				 StringListInpSpec(hdrtyps) );
    if ( isbulk_ )
	headerfld_->attach( alignedBelow, bulkinfld_ );
    else
	headerfld_->attach( alignedBelow, linenmfld_ );

    udffld_ = new uiGenInput( this, tr("Write undefined parts? Undef value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->setChecked( true );
    udffld_->setWithCheck( true );
    udffld_->attach( alignedBelow, headerfld_ );

    optsfld_ = new uiCheckList( this, uiCheckList::Unrel, OD::Horizontal );
    optsfld_->addItem( tr("Write line name") ).addItem(
	uiStrings::phrZIn( SI().zIsTime() ? uiStrings::sMsec().toLower()
					  : uiStrings::sFeet().toLower()) );
    optsfld_->attach( alignedBelow, udffld_ );
    optsfld_->setChecked( 0, true )
	     .setChecked( 1, !SI().zIsTime() && SI().depthsInFeet() );

    uiObject* attachobj = optsfld_->attachObj();
    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, optsfld_);
	attachobj = coordsysselfld_->attachObj();
    }

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->attach( alignedBelow, attachobj );

    if ( !isbulk )
	horChg( nullptr );
}


uiExport2DHorizon::~uiExport2DHorizon()
{
    detachAllNotifiers();
    deepErase( hinfos_ );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExport2DHorizon::doExport()
{
    TypeSet<MultiID> midset;
    if ( !getInputMultiIDs(midset) )
	mErrRet(tr("Cannot find object in database"))

    od_ostream strm( outfld_->fileName() );
    if ( !strm.isOK() )
	mErrRet( strm.errMsg() );

    const bool wrudfs = udffld_->isChecked();
    BufferString undefstr;
    if ( wrudfs )
    {
	undefstr =  udffld_->text();
	if ( undefstr.isEmpty() )
	    undefstr = "-";
    }

    const float zfac = !optsfld_->isChecked(1) ? 1
		     : (SI().zIsTime() ? 1000 : mToFeetFactorF);
    const bool wrlinenms = optsfld_->isChecked( 0 );

    writeHeader( strm );
    if ( !strm.isOK() )
	mErrRet( strm.errMsg() );

    struct HorInfo
    {
	HorInfo( const MultiID& mid )
	    : horid_(mid)	{}

	MultiID		horid_;
	BufferStringSet linenames_;

	void updateMax( od_uint32& maxhornm, od_uint32& maxlinenm )
	{
	    PtrMan<IOObj> ioobj = IOM().get( horid_ );
	    if ( !ioobj )
		return;

	    maxhornm = mMAX( maxhornm, ioobj->name().size() );

	    for ( int lidx=0; lidx<linenames_.size(); lidx++ )
		maxlinenm = mMAX( maxlinenm, linenames_.get(lidx).size() );
	}

	bool fillLineNames()
	{
	    PtrMan<IOObj> ioobj = IOM().get( horid_ );
	    if ( !ioobj )
		return false;

	    EM::SurfaceIOData emdata; EM::IOObjInfo oi( *ioobj );
	    uiString errmsg;
	    if ( !oi.getSurfaceData(emdata,errmsg) )
		mErrRet( tr("Error in reading data") )

	    linenames_ = emdata.linenames;
	    return true;
	}
    };

    ManagedObjectSet<HorInfo> horinfos;

    od_uint32 maxhornm = 15;
    od_uint32 maxlinenm = 15;

    if ( isbulk_ )
    {
	for ( int idx=0; idx<midset.size(); idx++ )
	{
	    auto* hi = new HorInfo( midset[idx] );
	    hi->fillLineNames();
	    hi->updateMax( maxhornm, maxlinenm );
	    horinfos += hi;
	}
    }
    else
    {
	auto* hi = new HorInfo( midset.first() );
	linenmfld_->getChosen( hi->linenames_ );
	if ( hi->linenames_.isEmpty() )
	    mErrRet( tr("Select at least one line to proceed") )

	hi->updateMax( maxhornm, maxlinenm );
	horinfos += hi;
    }

    const bool wrhornms = isbulk_;
    BufferString controlstr;
    Table::FormatProvider prov;
    if ( wrhornms )
	controlstr.add( prov.string(maxhornm+2) );
    if ( wrlinenms )
	controlstr.add( prov.string(maxlinenm+3) );

    controlstr.add( prov.xy() )
	      .add( prov.trcnr() ).add( prov.spnr() ).add( prov.string() );

    int nrzdec = SI().nrZDecimals();
    nrzdec += 2; // extra precision
    if ( !optsfld_->isChecked(1) && SI().zIsTime() )
	nrzdec += 3; // z in seconds

    EM::EMManager& em = EM::EMM();
    for ( int horidx=0; horidx<horinfos.size(); horidx++ )
    {
	auto* hi = horinfos[horidx];
	const MultiID& horid = hi->horid_;

	RefMan<EM::EMObject> obj = em.getObject( em.getObjectID(horid) );
	if ( !obj )
	{
	    PtrMan<Executor> exec = em.objectLoader( horid );
	    if ( !exec || !exec->execute() )
		mErrRet(uiStrings::sCantReadHor())

	    obj = em.getObject( em.getObjectID(horid) );
	}

	mDynamicCastGet(EM::Horizon2D*,hor,obj.ptr())
	if ( !hor )
	    mErrRet(uiStrings::sCantReadHor())

	const Geometry::Horizon2DLine* geom = hor->geometry().geometryElement();
	if ( !geom ) mErrRet(tr("Error Reading Horizon"))

	BufferString horname = hor->name();
	horname.quote('\"');

	BufferString line( 180, false );

	if ( !strm.isOK() )
	    mErrRet( uiStrings::sCannotWrite() );

	for ( int lidx=0; lidx<hi->linenames_.size(); lidx++ )
	{
	    BufferString linename = hi->linenames_.get( lidx );
	    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
	    linename.quote('\"');
	    const StepInterval<int> trcrg = hor->geometry().colRange( geomid );
	    mDynamicCastGet(const Survey::Geometry2D*,survgeom2d,
			    Survey::GM().getGeometry(geomid))
	    if ( !survgeom2d || trcrg.isUdf() || !trcrg.step) continue;

	    TrcKey tk( geomid, -1 );
	    Coord crd; float spnr = mUdf(float);
	    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
	    {
		tk.setTrcNr( trcnr );
		const float z = hor->getZ( tk );
		const bool zudf = mIsUdf(z);
		if ( zudf && !wrudfs )
		    continue;

		const BufferString zstr =
			zudf ? undefstr.buf() : toString( z*zfac, nrzdec );

		survgeom2d->getPosByTrcNr( trcnr, crd, spnr );
		Coords::CoordSystem* coordsys = coordsysselfld_ ?
			coordsysselfld_->getCoordSystem() : nullptr;
		if ( coordsys && !(*coordsys == *SI().getCoordSystem()) )
		{
		    const Coord crd2d =
			coordsys->convertFrom( crd, *SI().getCoordSystem() );
		    crd.setXY( crd2d.x, crd2d.y );
		}

		if ( wrhornms && wrlinenms )
		{
		    od_sprintf( line.getCStr(), line.bufSize(),
				controlstr.buf(),
				horname.buf(), linename.buf(),
				crd.x, crd.y,
				trcnr, double(spnr), zstr.buf() );
		}
		else if ( wrhornms )
		{
		    od_sprintf( line.getCStr(), line.bufSize(),
				controlstr.buf(),
				horname.buf(),
				crd.x, crd.y,
				trcnr, double(spnr), zstr.buf() );
		}
		else if ( wrlinenms )
		{
		    od_sprintf( line.getCStr(), line.bufSize(),
				controlstr.buf(),
				linename.buf(),
				crd.x, crd.y,
				trcnr, double(spnr), zstr.buf() );
		}
		else
		{
		    od_sprintf( line.getCStr(), line.bufSize(),
				controlstr.buf(),
				crd.x, crd.y,
				trcnr, double(spnr), zstr.buf() );
		}

		strm << line << od_newline;
		if ( !strm.isOK() )
		{
		    uiString msg = tr( "Error writing to the output file." );
		    strm.addErrMsgTo( msg );
		    mErrRet(msg)
		}
	    }
	}
    }

    return true;
}


void uiExport2DHorizon::writeHeader( od_ostream& strm )
{
    if ( headerfld_->getIntValue() == 0 )
	return;

    const bool wrtlnm = optsfld_->isChecked( 0 );
    const BufferString zstr( "Z ", optsfld_->isChecked(1) ? "(ms)" : "(s)" );
    BufferString headerstr;
    if ( headerfld_->getIntValue() == 1 )
    {
	headerstr = "# ";
	if ( isbulk_  )
	    headerstr.add( "\"Horizon Name\" " );
	if ( wrtlnm )
	    headerstr.add( "\"Line name\" " );

	headerstr.add( "\"X\" \"Y\" \"TraceNr\" \"ShotPointNr\" " )
		 .add( "\"" ).add( zstr ).add( "\"" );
    }
    else
    {
	int id = 0;
	BufferString str( wrtlnm ? " Line Name" : "" );
	if ( isbulk_ )
	    headerstr.add( "# " ).add( ++id ).add( ": Horizon Name\n" );

	if ( wrtlnm )
	    headerstr.add( "# " ).add( ++id ).add( ": Line Name\n" );

	headerstr.add( "# " ).add( ++id ).add( ": " ).add( "X\n" );
	headerstr.add( "# " ).add( ++id ).add( ": " ).add( "Y\n" );
	headerstr.add( "# " ).add( ++id ).add( ": " ).add( "Trace Nr\n" );
	headerstr.add( "# " ).add( ++id ).add( ": " ).add( "ShotPoint Nr\n" );
	headerstr.add( "# " ).add( ++id ).add( ": " ).add( zstr );
	if ( coordsysselfld_ )
	    headerstr.addNewLine().add( "# " )
		     .add( coordsysselfld_->getCoordSystem()->summary() );
	if ( !isbulk_ )
	    headerstr.addNewLine().add( "# Horizon: " )
		     .add( horselfld_->text() );
	headerstr.addNewLine().add( "#-------------------" );
    }

    strm << headerstr << od_endl;
}


bool uiExport2DHorizon::acceptOK( CallBacker* )
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() )

    if ( File::exists(outfnm) &&
	!uiMSG().askOverwrite(uiStrings::sOutputFileExistsOverwrite()) )
	return false;

    const bool res = doExport();
    if (!res)
    {
	uiMSG().error( uiStrings::phrCannotWrite( uiStrings::sHorizon() ) );
	return false;
    }

    uiString msg = tr("2D %1 successfully exported."
		      "\n\nDo you want to export more 2D %2?")
		      .arg(uiStrings::sHorizon(isbulk_ ? mPlural:1))
		      .arg(uiStrings::sHorizon(mPlural));
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}


bool uiExport2DHorizon::getInputMultiIDs( TypeSet<MultiID>& midset )
{
    if ( !isbulk_ )
    {
	const int horidx = horselfld_->currentItem();
	if ( !hinfos_.validIdx(horidx) )
	    return false;

	midset.add( hinfos_[horidx]->multiid );
    }
    else
	bulkinfld_->getChosen( midset );

    return true;
}


void uiExport2DHorizon::horChg( CallBacker* )
{
    BufferStringSet sellines;
    linenmfld_->getChosen( sellines );
    linenmfld_->setEmpty();
    const int horidx = horselfld_->currentItem();
    if ( !hinfos_.validIdx(horidx) )
	return;

    const MultiID horid = hinfos_[horidx]->multiid;
    PtrMan<IOObj> ioobj = IOM().get( horid );
    if ( !ioobj ) return;

    EM::SurfaceIOData emdata; EM::IOObjInfo oi( *ioobj );
    uiString errmsg;
    if ( !oi.getSurfaceData(emdata,errmsg) )
	return;

    linenmfld_->addItems( emdata.linenames );
    linenmfld_->setChosen( sellines );
    if ( linenmfld_->nrChosen() == 0 )
	linenmfld_->chooseAll();
}
