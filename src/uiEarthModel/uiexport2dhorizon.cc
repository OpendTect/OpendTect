/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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

#include "uichecklist.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#include <stdio.h>


static const char* hdrtyps[] = { "No", "Single line", "Multi line", 0 };


uiExport2DHorizon::uiExport2DHorizon( uiParent* p,
			const ObjectSet<SurfaceInfo>& hinfos, bool isbulk )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("2D Horizon") ),
	       mNoDlgTitle, mODHelpKey(mExportHorizonHelpID) ))
    , hinfos_(hinfos)
    , isbulk_(isbulk)
{
    setOkText( uiStrings::sExport() );
    IOObjContext ctxt = mIOObjContext( EMHorizon2D );
    uiIOObjSelGrp::Setup stup; stup.choicemode_ = OD::ChooseAtLeastOne;
    if ( !isbulk_ )
    {

	uiLabeledComboBox* lcbox = new uiLabeledComboBox( this,
			     uiStrings::phrSelect( uiStrings::sHorizon() ),
			     "Select 2D Horizon" );
	horselfld_ = lcbox->box();
	horselfld_->setHSzPol( uiObject::MedVar );
	horselfld_->selectionChanged.notify(
					  mCB(this,uiExport2DHorizon,horChg) );
	for ( int idx=0; idx<hinfos_.size(); idx++ )
	    horselfld_->addItem( mToUiStringTodo(hinfos_[idx]->name) );

	uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Select lines") );
	linenmfld_ = new uiListBox( this, su );
	linenmfld_->attach( alignedBelow, lcbox );
    }
    else
	bulkinfld_ = new uiIOObjSelGrp( this, ctxt,
					uiStrings::sHorizon(mPlural), stup );

    headerfld_ = new uiGenInput( this, tr("Header"),
				 StringListInpSpec(hdrtyps) );
    if ( !isbulk_ )
	headerfld_->attach( alignedBelow, linenmfld_ );
    else
	headerfld_->attach( alignedBelow, bulkinfld_ );

    udffld_ = new uiGenInput( this, tr("Write undefined parts? Undef value"),
			      StringInpSpec(sKey::FloatUdf()) );
    udffld_->setChecked( true );
    udffld_->setWithCheck( true );
    udffld_->attach( alignedBelow, headerfld_ );

    optsfld_ = new uiCheckList( this, uiCheckList::Unrel, OD::Horizontal);
    optsfld_->addItem( tr("Write line name") ).addItem(
	uiStrings::phrZIn( SI().zIsTime() ? uiStrings::sMsec()
					  : uiStrings::sFeet()) );
    optsfld_->attach( alignedBelow, udffld_ );
    optsfld_->setChecked( 0, true )
	     .setChecked( 1, !SI().zIsTime() && SI().depthsInFeet() );

    outfld_ = new uiFileInput( this,
		  uiStrings::phrOutput(uiStrings::phrASCII(uiStrings::sFile())),
		  uiFileInput::Setup().forread(false) );
    outfld_->attach( alignedBelow, optsfld_ );
    if ( !isbulk )
	horChg( 0 );
}


uiExport2DHorizon::~uiExport2DHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExport2DHorizon::doExport()
{
    TypeSet<MultiID> midset;
    if ( !getInputMultiIDs(midset) )
	mErrRet(tr("Cannot find object in database"))


    od_ostream strm( outfld_->fileName() );
    const bool wrudfs = udffld_->isChecked();
    BufferString undefstr;
    if ( wrudfs )
    {
	undefstr =  udffld_->text();
	if ( undefstr.isEmpty() )
	    undefstr = "-";
    }

    writeHeader( strm );

    for ( int horidx=0; horidx<midset.size(); horidx++ )
    {
	MultiID horid;
	if ( !isbulk_ )
	{
	    const int selhoridx = horselfld_->currentItem();
	    horid = hinfos_[selhoridx]->multiid;
	}
	else
	    horid = midset[horidx];

	if ( horidx < 0 || horidx > hinfos_.size() )
	    mErrRet(tr("Invalid Horizon"))

	BufferStringSet linenms;

	EM::EMManager& em = EM::EMM();
	EM::EMObject* obj = em.getObject( em.getObjectID(horid) );
	if ( !obj )
	{
	    PtrMan<Executor> exec = em.objectLoader( horid );
	    if ( !exec || !exec->execute() )
		mErrRet(uiStrings::sCantReadHor())

	    obj = em.getObject( em.getObjectID(horid) );
	    if ( !obj ) return false;

	    obj->ref();
	}

	if ( !isbulk_ )
	{
	    linenmfld_->getChosen( linenms );
	    if ( !linenms.size() )
		mErrRet( uiStrings::phrSelect(tr("at least one line to "
								"proceed")) )
	}
	else
	{
	    PtrMan<IOObj> ioobj = IOM().get( horid );
	    if ( !ioobj )
		mErrRet(uiStrings::phrCannotFindDBEntry(
						uiStrings::sEmptyString()));

	    EM::SurfaceIOData emdata; EM::IOObjInfo oi( *ioobj );
	    uiString errmsg;
	    if ( !oi.getSurfaceData(emdata,errmsg) )
		mErrRet( tr("Error in reading data") )
	    linenms = emdata.linenames;
	}
	mDynamicCastGet(EM::Horizon2D*,hor,obj);
	if ( !hor )
	    mErrRet(uiStrings::sCantReadHor())

	EM::SectionID sid = hor->sectionID( 0 );
	const Geometry::Horizon2DLine* geom = hor->geometry().
							sectionGeometry(sid);
	if ( !geom ) mErrRet(tr("Error Reading Horizon"))

	BufferString horname = hor->name();
	horname.quote('\"');

	const float zfac = !optsfld_->isChecked(1) ? 1
			 : (SI().zIsTime() ? 1000 : mToFeetFactorF);
	const bool wrlnms = optsfld_->isChecked( 0 );
	BufferString line( 180, false );

	if ( !strm.isOK() )
	    mErrRet(uiStrings::sCantOpenOutpFile())

	for ( int idx=0; idx<linenms.size(); idx++ )
	{
	    BufferString linename = linenms.get( idx );
	    const Pos::GeomID geomid = Survey::GM().getGeomID( linename );
	    linename.quote('\"');
	    const StepInterval<int> trcrg = hor->geometry().colRange( geomid );
	    mDynamicCastGet(const Survey::Geometry2D*,survgeom2d,
			    Survey::GM().getGeometry(geomid))
	    if ( !survgeom2d || trcrg.isUdf() || !trcrg.step) continue;

	    TrcKey tk( geomid, -1 );
	    Coord crd; int spnr = mUdf(int);
	    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
	    {
		tk.setTrcNr( trcnr );
		const float z = hor->getZ( tk );
		const bool zudf = mIsUdf(z);
		if ( zudf && !wrudfs )
		    continue;

		survgeom2d->getPosByTrcNr( trcnr, crd, spnr );
		BufferString controlstr = !isbulk_ ? "%15s" : "%15s\t%15s";

		if ( zudf )
		{
		    if ( !wrlnms )
		    {
			line.setEmpty();
			if ( isbulk_ )
			    line.add(horname).add("\t");

			line.add( crd.x ).add( "\t" ).add( crd.y )
					 .add("\t" ).add( undefstr );
		    }
		    else
		    {
			controlstr += "%16.2lf%16.2lf%8d%8d%16s";
			if ( isbulk_ )
			    sprintf( line.getCStr(), controlstr.buf(),
				   horname.buf(), linename.buf(), crd.x, crd.y,
				   spnr, trcnr, undefstr.buf() );
			else
			    sprintf( line.getCStr(), controlstr.buf(),
				   linename.buf(), crd.x, crd.y, spnr, trcnr,
				   undefstr.buf() );
		    }
		}
		else
		{
		    const float scaledz = z * zfac;
		    if ( wrlnms )
		    {
			controlstr += "%16.2lf%16.2lf%8d%8d%16.4lf";
			if ( isbulk_ )
			      sprintf( line.getCStr(), controlstr.buf(),
			      horname.buf(), linename.buf(), crd.x, crd.y,spnr,
			      trcnr, scaledz );
			else
			{
			    sprintf( line.getCStr(), controlstr.buf(),
				linename.buf(), crd.x, crd.y, spnr,
				trcnr, scaledz );
			}
		    }
		    else
		    {
			line.setEmpty();
			if ( isbulk_ )
			      line.add(horname).add("\t");
			line.add( crd.x ).add( "\t" ).add( crd.y ).add("\t" )
					 .add( scaledz );
		    }
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
    BufferString zstr( "Z", optsfld_->isChecked(1) ? "(ms)" : "(s)" );
    BufferString headerstr;
    if ( headerfld_->getIntValue() == 1 )
    {
	if ( isbulk_  )
	    headerstr = "\"Horizon Name\"\t";
	wrtlnm ? headerstr.add( "\"Line name\"\t\"X\"\t\"Y\"\t\"ShotPointNr\""
							"\t\"TraceNr\"\t" )
	       : headerstr.add( " \"X\"\t\"Y\"\t" );

	headerstr.add( "\"" ).add( zstr ).add( "\"" );
    }
    else
    {
	int id = 1;
	BufferString str( wrtlnm ? " LineName" : "" );
	if ( isbulk_ )
	{
	    headerstr.add( id ).add( ":" ).add( "Horizon Name" ).add( "\n" )
		    .add( "# " );
	    id++;
	}

	if ( !str.isEmpty() )
	{

	    headerstr.add( id ).add( ":" )
                     .add( str ).add( "\n" ).add( "# " );
	    id++;
	}

	headerstr.add( id ).add( ": " ).add( "X\n" );
	headerstr.add( "# " ).add( ++id ).add( ": " ).add( "Y\n" );
	if ( wrtlnm )
	{
	    headerstr.add( "# " ).add( ++id )
		     .add( ": " ).add( "ShotPointNr\n" );
	    headerstr.add( "# " ).add( ++id )
                     .add( ": " ).add( "TraceNr\n" );
	}

	headerstr.add( "# " ).add( ++id ).add( ": " ).add( zstr );
    }

    strm << "#" << headerstr << od_newline;
    strm << "#-------------------" << od_endl;
}


bool uiExport2DHorizon::acceptOK( CallBacker* )
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

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
		      "\n\nDo you want to export more 2D %1?")
		      .arg(uiStrings::sHorizon(isbulk_ ? mPlural:1));
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}


bool uiExport2DHorizon::getInputMultiIDs( TypeSet<MultiID>& midset )
{
    if ( !isbulk_ )
    {
	const int horidx = horselfld_->currentItem();
	if ( horidx < 0 || horidx > hinfos_.size() )
	    return false;
	MultiID horid = hinfos_[horidx]->multiid;
	midset.add(horid);
    }
    else
	bulkinfld_->getChosen(midset);
    return true;
}


void uiExport2DHorizon::horChg( CallBacker* cb )
{
    BufferStringSet sellines;
    linenmfld_->getChosen( sellines );
    linenmfld_->setEmpty();
    const int horidx = horselfld_->currentItem();
    if ( horidx < 0 || horidx > hinfos_.size() )
	return;

    MultiID horid = hinfos_[horidx]->multiid;

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
