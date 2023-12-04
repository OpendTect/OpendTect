/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiexport2dhorizon.h"

#include "ctxtioobj.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "ptrman.h"
#include "surfaceinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tableconvimpl.h"
#include "unitofmeasure.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimultisurfaceread.h"
#include "uimsg.h"
#include "uiiosurface.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"
#include "od_helpids.h"

#include <stdio.h>


static uiStringSet hdrtyps()
{
    uiStringSet hdrtypes;
    hdrtypes.add( uiStrings::sNo() );
    hdrtypes.add( od_static_tr("hdrtyps", "Single Line") );
    hdrtypes.add( od_static_tr("hdrtyps", "Multi Line") );
    return hdrtypes;
}


uiExport2DHorizon::uiExport2DHorizon( uiParent* p, bool isbulk )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("2D Horizon") ),
	       mNoDlgTitle, mODHelpKey(mExportHorizonHelpID) ))
    , isbulk_(isbulk)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    uiObject* attachobj = nullptr;
    const char* surftype = EMHorizon2DTranslatorGroup::sGroupName();
    auto* horselgrp = new uiGroup( this, "Horizon Selection Group" );
    if ( isbulk_ )
    {
	horzdomypefld_ = new uiGenInput( this, tr("Depth Domain"),
	    BoolInpSpec(true, uiStrings::sTime(),uiStrings::sDepth()) );
	mAttachCB( horzdomypefld_->valueChanged,
					uiExport2DHorizon::zDomainTypeChg );

	horselgrp->attach( alignedBelow, horzdomypefld_ );
	const ZDomain::Info& depthinfo = SI().depthsInFeet() ?
	    ZDomain::DepthFeet() : ZDomain::DepthMeter();
	multisurfdepthread_ = new uiMultiSurfaceRead( horselgrp, surftype,
	    &depthinfo );
	multisurfdepthread_->display( false );
	multisurftimeread_ = new uiMultiSurfaceRead( horselgrp, surftype,
	    &ZDomain::TWT() );
	multisurftimeread_->display( true );
    }
    else
    {
	uiSurfaceRead::Setup su( EMHorizon2DTranslatorGroup::sGroupName() );
	su.withattribfld( false ).withsectionfld( false );
	surfread_ = new uiSurfaceRead( this, su, nullptr );
	mAttachCB( surfread_->inpChange, uiExport2DHorizon::horChg );

	horselgrp->attach( alignedBelow, surfread_ );
	uiListBox::Setup listbxsu( OD::ChooseZeroOrMore,
	    uiStrings::phrSelect(uiStrings::sLine(mPlural).toLower()) );
	linenmfld_ = new uiListBox( horselgrp, listbxsu );
    }

    attachobj = horselgrp->attachObj();
    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, attachobj );
	attachobj = coordsysselfld_->attachObj();
    }

    unitsel_ = new uiUnitSel( this, uiUnitSel::Setup(tr("Z Unit")) );
    unitsel_->setUnit( UnitOfMeasure::surveyDefZUnit() );
    unitsel_->attach( alignedBelow, attachobj );

    const uiStringSet headertyoes = hdrtyps();
    headerfld_ = new uiGenInput( this, tr("Header"),
				 StringListInpSpec(headertyoes) );
    headerfld_->setText( headertyoes[1].getFullString() );
    headerfld_->attach( alignedBelow, unitsel_ );

    writeudffld_ = new uiCheckBox( this, tr("Write undefined parts") );
    writeudffld_->setChecked( true );
    mAttachCB( writeudffld_->activated, uiExport2DHorizon::undefCB );
    writeudffld_->attach( alignedBelow, headerfld_ );

    udffld_ = new uiGenInput( this, tr("Undefined value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->attach( rightTo, writeudffld_ );

    writelinenmfld_ = new uiCheckBox( this, tr("Write line name") );
    writelinenmfld_->setChecked( true );
    writelinenmfld_->attach( alignedBelow, writeudffld_ );

    outfld_ = new uiASCIIFileInput( this, false );
    outfld_->attach( alignedBelow, writelinenmfld_ );

    if ( !isbulk )
	mAttachCB( postFinalize(), uiExport2DHorizon::horChg );
}


uiExport2DHorizon::~uiExport2DHorizon()
{
    detachAllNotifiers();
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

    const bool wrlinenms = writelinenmfld_->isChecked();
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
    nrzdec += 3; // extra precision

    const UnitOfMeasure* uom = unitsel_->getUnit();
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
	    const BufferString linename = hi->linenames_.get( lidx );
	    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
	    BufferString hdrlnm = linename;
	    hdrlnm.quote('\"');
	    const StepInterval<int> trcrg = hor->geometry().colRange( geomid );
	    mDynamicCastGet(const Survey::Geometry2D*,survgeom2d,
			    Survey::GM().getGeometry(geomid))
	    if ( !survgeom2d || trcrg.isUdf() || !trcrg.step )
		continue;

	    TrcKey tk( geomid, -1 );
	    Coord crd; float spnr = mUdf(float);
	    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
	    {
		tk.setTrcNr( trcnr );
		const float z = hor->getZ( tk );
		const bool zudf = mIsUdf(z);
		if ( zudf && !wrudfs )
		    continue;

		BufferString zstr;
		if ( zudf )
		    zstr = undefstr.buf();
		else
		{
		    float newz = z;
		    if ( uom )
			newz = uom->userValue( newz );

		    zstr = toString( newz, nrzdec );
		}

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
				horname.buf(), hdrlnm.buf(),
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
				hdrlnm.buf(),
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

    const bool wrtlnm = writelinenmfld_->isChecked();
    const UnitOfMeasure* uom = unitsel_->getUnit();
    const BufferString zstr( "Z (", uom->symbol(), ")" );
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
	{
	    const IOObj* selobj = surfread_->selIOObj();

	    headerstr.addNewLine().add( "# Horizon: " )
		     .add( selobj->name() );
	}

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


void uiExport2DHorizon::zDomainTypeChg( CallBacker* )
{
    if ( !isbulk_ )
	return;

    const bool istime = isTime();
    multisurfdepthread_->display( !istime );
    multisurftimeread_->display( istime );
    unitsel_->setUnit( UnitOfMeasure::zUnit(zDomain()) );

}


const ZDomain::Info& uiExport2DHorizon::zDomain() const
{
    if ( isTime() )
	return ZDomain::TWT();
    else if ( SI().depthsInFeet() )
	return ZDomain::DepthFeet();

    return ZDomain::DepthMeter();
}


bool uiExport2DHorizon::isTime() const
{
    if ( isbulk_ )
	return horzdomypefld_->getBoolValue();
    else
    {
	const IOObj* selobj = surfread_->selIOObj();
	if ( !selobj )
	    return SI().zIsTime();

	EM::SurfaceIOData emdata;
	const EM::IOObjInfo oi( *selobj );
	uiString errmsg;
	if ( !oi.getSurfaceData(emdata,errmsg) )
	    return SI().zIsTime();

	const UnitOfMeasure* uom = oi.getZUoM();
	if ( !uom )
	    return SI().zIsTime();

	return uom->propType() == Mnemonic::Time;
    }
}


bool uiExport2DHorizon::getInputMultiIDs( TypeSet<MultiID>& midset )
{
    if ( isbulk_ )
    {
	if ( isTime() )
	    multisurftimeread_->getSurfaceIds( midset );
	else
	    multisurfdepthread_->getSurfaceIds( midset );
    }
    else
    {
	const IOObj* selobj = surfread_->selIOObj();
	if ( selobj )
	    midset.add( selobj->key() );
    }

    return true;
}


void uiExport2DHorizon::horChg( CallBacker* )
{
    if ( isbulk_ )
	return;

    BufferStringSet sellines;
    linenmfld_->getChosen( sellines );
    linenmfld_->setEmpty();

    const IOObj* ioobj = surfread_->selIOObj();
    if ( !ioobj )
	return;

    EM::SurfaceIOData emdata;
    const EM::IOObjInfo oi( *ioobj );
    uiString errmsg;
    if ( !oi.getSurfaceData(emdata,errmsg) )
	return;

    unitsel_->setUnit( oi.getZUoM() );
    linenmfld_->addItems( emdata.linenames );
    linenmfld_->setChosen( sellines );
    if ( linenmfld_->nrChosen() == 0 )
	linenmfld_->chooseAll();

    const FilePath fp( ioobj->mainFileName() );
    outfld_->setFileName( fp.baseName() );
}


void uiExport2DHorizon::undefCB(CallBacker *)
{
    udffld_->display( writeudffld_->isChecked() );
}
