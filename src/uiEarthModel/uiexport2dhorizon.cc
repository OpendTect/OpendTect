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

static BufferString bufstr( const char* horname, const char* hdrlnm,
     Coord crd, int trcnr, double spnr, const char* zvalstr )
{
    BufferString ret, crd_xstr, crd_ystr, trcnr_str, spnr_str;
    crd_xstr.set( crd.x_, 12, 'f', 2 );
    crd_ystr.set( crd.y_, 12, 'f', 2 );
    trcnr_str.set( trcnr, 10, 'd', 0);
    spnr_str.set( spnr, 10, 'f', 2 );
    ret.addSpace().add( horname ).add( hdrlnm ).add( crd_xstr ).add( crd_ystr )
       .add( trcnr_str ).add( spnr_str ).addSpace( 3 ).add( zvalstr );
    return ret;
}

uiExport2DHorizon::uiExport2DHorizon( uiParent* p, bool isbulk )
    : uiDialog(p,Setup(uiStrings::phrExport( tr("2D Horizon") ),
		       mODHelpKey(mExportHorizonHelpID)))
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
	const uiListBox::Setup listbxsu( OD::ChooseZeroOrMore,
	    uiStrings::phrSelect(uiStrings::sLine(mPlural).toLower()) );
	linenmfld_ = new uiListBox( horselgrp, listbxsu, "linenames" );
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
	mErrRet( uiStrings::phrSelect(
		 uiStrings::s2DHorizon(isbulk_ ? mPlural : 1)) );

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

    const UnitOfMeasure* zunitout_ = unitsel_->getUnit();
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
	if ( !geom )
	    mErrRet(tr("Error Reading Horizon"))

	BufferString horname = hor->name();
	horname.quote('\"');

	if ( !strm.isOK() )
	    mErrRet( uiStrings::sCannotWrite() );

	const UnitOfMeasure* horzunit = hor->zUnit();
	for ( int lidx=0; lidx<hi->linenames_.size(); lidx++ )
	{
	    const BufferString linename = hi->linenames_.get( lidx );
	    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
	    BufferString hdrlnm = linename;
	    hdrlnm.quote('\"');
	    const StepInterval<int> trcrg = hor->geometry().colRange( geomid );
	    mDynamicCastGet(const Survey::Geometry2D*,survgeom2d,
			    Survey::GM().getGeometry(geomid))
            if ( !survgeom2d || trcrg.isUdf() || !trcrg.step_ )
		continue;

	    TrcKey tk( geomid, (Pos::TraceID)-1 );
	    Coord crd;
	    float spnr = mUdf(float);
	    for ( Pos::TraceID trcnr=trcrg.start_; trcnr<=trcrg.stop_;
						   trcnr+=trcrg.step_)
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
		    if ( zunitout_ )
			convValue( newz, horzunit, zunitout_ );

		    zstr = toString( newz, 0, 'f', nrzdec );
		}

		survgeom2d->getPosByTrcNr( trcnr, crd, spnr );
		ConstRefMan<Coords::CoordSystem> sicoordsys =
					SI().getCoordSystem();
		ConstRefMan<Coords::CoordSystem> coordsys;
		if ( coordsysselfld_ )
		    coordsys = coordsysselfld_->getCoordSystem();

		if ( coordsys && !(*coordsys == *sicoordsys) )
		{
		    const Coord crd2d =
			coordsys->convertFrom( crd, *sicoordsys );
                    crd.setXY( crd2d.x_, crd2d.y_ );
		}

		BufferString line;
		if ( wrhornms && wrlinenms )
		{
			line = bufstr( horname, hdrlnm,
				       crd, trcnr, spnr, zstr );
		}
		else if ( wrhornms )
		{
			line = bufstr( horname, nullptr ,
				       crd, trcnr, spnr, zstr );
		}
		else if ( wrlinenms )
		{
			line = bufstr( nullptr, hdrlnm,
				       crd, trcnr, spnr, zstr );
		}
		else
		{
			line = bufstr( nullptr, nullptr, crd, trcnr,
				       spnr, zstr );
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
	{
	    ConstRefMan<Coords::CoordSystem> coordsys =
					coordsysselfld_->getCoordSystem();
	    if ( coordsys )
		headerstr.addNewLine().add( "# " ).add( coordsys->summary() );
	}

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
    unitsel_->setUnit( UnitOfMeasure::zUnit(zDomain(),false) );

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

    return !midset.isEmpty();
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

    unitsel_->setUnit( UnitOfMeasure::zUnit(oi.zDomain(),false) );
    linenmfld_->addItems( emdata.linenames );
    linenmfld_->setChosen( sellines );
    if ( linenmfld_->nrChosen() == 0 )
	linenmfld_->chooseAll();

    const FilePath prevfnm( outfld_->fileName() );
    FilePath fp( ioobj->mainFileName() );
    fp.setExtension( prevfnm.isEmpty() ? outfld_->defaultExtension()
				       : prevfnm.extension() );
    outfld_->setFileName( fp.fileName() );
}


void uiExport2DHorizon::undefCB(CallBacker *)
{
    udffld_->display( writeudffld_->isChecked() );
}
