/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          June 2008
________________________________________________________________________

-*/

#include "uiexport2dhorizon.h"

#include "ioobjctxt.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "emioobjinfo.h"
#include "executor.h"
#include "file.h"
#include "ioobj.h"
#include "keystrs.h"
#include "ptrman.h"
#include "od_ostream.h"
#include "surfaceinfo.h"
#include "survinfo.h"

#include "uichecklist.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#include <stdio.h>


static const char* hdrtyps[] = { "No", "Single line", "Multi line", 0 };


uiExport2DHorizon::uiExport2DHorizon( uiParent* p,
		const ObjectSet<SurfaceInfo>& hinfos, bool isbulk )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( tr("2D %1").
		arg(uiStrings::sHorizon(isbulk?mPlural:1)) ),
		mNoDlgTitle, mODHelpKey(mExportHorizonHelpID) ))
    , hinfos_(hinfos)
    , isbulk_(isbulk)
{
    setOkText( uiStrings::sExport() );
    IOObjContext ctxt = mIOObjContext( EMHorizon2D );
    uiIOObjSelGrp::Setup stup; stup.choicemode_ =
					OD::ChoiceMode::ChooseAtLeastOne;
    if ( !isbulk_ )
    {
	uiLabeledComboBox* lcbox = new uiLabeledComboBox( this,
			 uiStrings::phrSelect( uiStrings::sHorizon() ),
			 "Select 2D Horizon" );
	horselfld_ = lcbox->box();
	horselfld_->setHSzPol( uiObject::MedVar );
	horselfld_->selectionChanged.notify( mCB(this,uiExport2DHorizon,
								    horChg) );
	for ( int idx=0; idx<hinfos_.size(); idx++ )
	    horselfld_->addItem( toUiString(hinfos_[idx]->name) );

	uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Select lines") );
	linenmfld_ = new uiListBox( this, su );
	linenmfld_->attach( alignedBelow, lcbox );
    }
    else
	bulkinfld_ = new uiIOObjSelGrp( this, ctxt,
					uiStrings::sHorizon(mPlural), stup );
    headerfld_ = new uiGenInput( this, uiStrings::sHeader(),
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
    optsfld_->addItem( tr("Write line name") )
	      .addItem( uiStrings::phrZIn( SI().zIsTime()
		      ? uiStrings::sMSec(false) : uiStrings::sFeet(false)) );
    optsfld_->attach( alignedBelow, udffld_ );
    optsfld_->setChecked( 0, true )
	     .setChecked( 1, !SI().zIsTime() && SI().depthsInFeet() );

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::sOutputASCIIFile(), fssu );
    outfld_->attach( alignedBelow, optsfld_ );
    if ( !isbulk_ )
	horChg( 0 );
}


uiExport2DHorizon::~uiExport2DHorizon()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiExport2DHorizon::doExport()
{
    DBKeySet dbkeyset;
    if ( !getInputDBKeys(dbkeyset) )
	mErrRet(tr("Cannot find object in database"))

    od_ostream strm( outfld_->fileName() );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpenForWrite(strm.fileName()))
    const bool wrudfs = udffld_->isChecked();
    BufferString undefstr;
    if ( wrudfs )
    {
	undefstr =  udffld_->text();
	if ( undefstr.isEmpty() )
	    undefstr = "-";
    }

    uiTaskRunnerProvider trprov( this );
    for ( int horidx=0; horidx<dbkeyset.size(); horidx++ )
    {
	DBKey horid;
	if ( !isbulk_ )
	{
	    const int selhoridx = horselfld_->currentItem();
	    horid = hinfos_[selhoridx]->dbkey;
	}
	else
	    horid = dbkeyset[horidx];
	if ( horidx < 0 || horidx > hinfos_.size() )
	    mErrRet(tr("Invalid Horizon"))

	BufferStringSet linenms;

	EM::ObjectManager& mgr = EM::Hor2DMan();
	ConstRefMan<EM::Object> obj = mgr.getObject( horid );
	if ( !obj )
	{
	    obj = mgr.fetch( horid, trprov );
	    if ( !obj ) return false;
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
	    PtrMan<IOObj> ioobj = horid.getIOObj();
	    if ( !ioobj )
		mErrRet(uiStrings::phrCannotFindDBEntry( horid ));

	    EM::SurfaceIOData emdata; EM::IOObjInfo oi( *ioobj );
	    uiString errmsg;
	    if ( !oi.getSurfaceData(emdata,errmsg) )
		mErrRet( uiStrings::phrErrDuringRead()  )
	    linenms = emdata.linenames;
	}
	mDynamicCastGet(const EM::Horizon2D*,hor,obj.ptr());
	if ( !hor )
	    mErrRet(uiStrings::phrCannotReadHor())

	const Geometry::Horizon2DLine* geom = hor->geometry().geometryElement();
	if ( !geom )
	    mErrRet(uiStrings::phrErrDuringRead(uiStrings::sHorizon()) )

	BufferString horname = hor->name();
	horname.quote( '"' );

	const float zfac = !optsfld_->isChecked(1) ? 1
			 : (SI().zIsTime() ? 1000 : mToFeetFactorF);
	const bool wrlnms = optsfld_->isChecked( 0 );
	BufferString line( 180, false );

	writeHeader( strm );

	for ( int idx=0; idx< linenms.size(); idx++ )
	{
	    BufferString linename = linenms.get( idx );

	    const int lineidx = hor->geometry().lineIndex( linename );
	    linename.quote('\"');
	    StepInterval<int> trcrg = geom->colRange( lineidx );
	    if ( trcrg.isUdf() || !trcrg.step ) continue;

	    for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+=trcrg.step )
	    {
		Coord3 pos = geom->getKnot( RowCol(lineidx,trcnr) );

		if ( mIsUdf(pos.x_) || mIsUdf(pos.y_) )
		    continue;
		const bool zudf = mIsUdf(pos.z_);
		if ( zudf && !wrudfs )
		    continue;

		BufferString controlstr = !isbulk_ ? "%15s" : "%15s\t%15s";
		if ( zudf )
		{
		    line.setEmpty();
		    if ( !wrlnms )
		    {
			if ( isbulk_ )
			    line.add(horname).add("\t");

			line.add( pos.x_ ).add( "\t" ).add( pos.y_ )
			    .add( "\t" ).add( undefstr );
		    }
		    else
		    {
			controlstr += "%16.2lf%16.2lf%8d%16s";
			if ( isbulk_ )
#ifdef __win__
			    sprintf_s( line.getCStr(), line.bufSize(),
#else
			    sprintf( line.getCStr(),
#endif
				    controlstr.buf(), horname.buf(),
				    linename.buf(), pos.x_, pos.y_,
				    trcnr, undefstr.buf() );
			else
#ifdef __win__
			    sprintf_s( line.getCStr(), line.bufSize(),
#else
			    sprintf( line.getCStr(),
#endif
				controlstr.buf(), linename.buf(),
				pos.x_, pos.y_, trcnr, undefstr.buf() );
		    }
		}
		else
		{
		    pos.z_ *= zfac;
		    if ( wrlnms )
		    {
			 controlstr += "%16.2lf%16.2lf%8d%16.4lf";
			if ( isbulk_ )
#ifdef __win__
			      sprintf_s( line.getCStr(), line.bufSize(),
#else
			      sprintf( line.getCStr(),
#endif
				controlstr.buf(), horname.buf(), linename.buf(),
				pos.x_, pos.y_, trcnr, pos.z_ );
			else
			{
#ifdef __win__
			    sprintf_s( line.getCStr(), line.bufSize(),
#else
			    sprintf( line.getCStr(),
#endif
				     controlstr.buf(), linename.buf(),
				     pos.x_, pos.y_, trcnr, pos.z_ );
			}
		    }
		    else
		    {
			line.setEmpty();
			if ( isbulk_ )
			    line.add(horname).add("\t");
			line.add( pos.x_ ).add( "\t" ).add( pos.y_ )
			    .add( "\t" ).add( pos.z_ );
		    }
		}

		strm << line << od_newline;
		if ( !strm.isOK() )
		{
		    uiString msg
			= uiStrings::phrErrDuringWrite(strm.fileName());
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
	    headerstr.add( id++ ).add( ":" ).add( "Horizon Name" ).add( "\n" )
		    .add( "# " );
	}

	if ( !str.isEmpty() )
	{
	    headerstr.add( id++ ).add( ":" )
                     .add( str ).add( "\n" ).add( "# " );
	}

	headerstr.add( id++ ).add( ": " ).add( "X\n" );
	headerstr.add( "# " ).add( id++ ).add( ": " ).add( "Y\n" );
	if ( wrtlnm )
	{
	    headerstr.add( "# " ).add( id++ ).add( ": " );
	    // don't make one .add because of pfx oper vs order of evaluation
	    headerstr.add( "ShotPointNr\n" ).add( "# " )
		.add( id++ ).add( ": " ).add( "TraceNr\n" );
	}

	headerstr.add( "#" ).add( id++ ).add( ": " ).add( zstr );
    }

    strm << "#" << headerstr << od_newline;
    strm << "#-------------------" << od_endl;
}


bool uiExport2DHorizon::acceptOK()
{
    const BufferString outfnm( outfld_->fileName() );
    if ( outfnm.isEmpty() )
	mErrRet( uiStrings::phrSelOutpFile() );

    if ( File::exists(outfnm) &&
	!uiMSG().askOverwrite(uiStrings::phrOutputFileExistsOverwrite()) )
	return false;

    const bool res = doExport();
    if (!res)
    {
	uiMSG().error( uiStrings::phrCannotWrite( uiStrings::sHorizon() ) );
	return false;
    }

    uiString msg = tr("2D Horizon successfully exported."
		      "\n\nDo you want to export more horizons?");
    return !uiMSG().askGoOn(msg, uiStrings::sYes(), tr("No, close window"));
}


bool uiExport2DHorizon::getInputDBKeys( DBKeySet& dbkeyset )
{
    if ( !isbulk_ )
    {
	const int horidx = horselfld_->currentItem();
	if ( horidx < 0 || horidx > hinfos_.size() )
	    return false;
	DBKey horid = hinfos_[horidx]->dbkey;
	dbkeyset.add(horid);
    }
    else
	bulkinfld_->getChosen(dbkeyset);
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

    DBKey horid = hinfos_[horidx]->dbkey;

    PtrMan<IOObj> ioobj = horid.getIOObj();
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
