/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmarkerdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistratlvlsel.h"
#include "uistrattreewin.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uitoolbutton.h"
#include "uilabel.h"

#include "ctxtioobj.h"
#include "file.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "od_istream.h"
#include "randcolor.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "tabledef.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welldisp.h"
#include "wellman.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "wellreader.h"
#include "od_helpids.h"

static const int cNrEmptyRows = 5;

static const char* sKeyMD()             { return sKey::MD(); }
static const char* sKeyTVD()            { return sKey::TVD(); }
static const char* sKeyTVDSS()          { return sKey::TVDSS(); }
static const int cNameCol  = 0;
static const int cDepthCol = 1;
static const int cTVDCol = 2;
static const int cTVDSSCol = 3;
static const int cTWTCol = 4;
static const int cColorCol = 5;
static const int cLevelCol = 6;


static void getColumnLabels( uiStringSet& lbls, uiCheckBox* unfld,
			     bool withlvls )
{
    lbls.add( uiStrings::sName() );
    if ( unfld )
    {
	const bool zinfeet = unfld ? unfld->isChecked() : SI().depthsInFeet();
	const uiString zunituistr = toUiString(getDistUnitString(zinfeet,true));
	lbls.add( uiStrings::phrJoinStrings(uiStrings::sMD(),zunituistr) );
	lbls.add( uiStrings::phrJoinStrings(uiStrings::sTVD(),zunituistr) );
	lbls.add( uiStrings::phrJoinStrings(uiStrings::sTVDSS(),zunituistr) );
    }
    else
    {
	lbls.add( uiStrings::sMD().withSurvDepthUnit() )
	    .add( uiStrings::sTVD().withSurvDepthUnit() )
	    .add( uiStrings::sTVDSS().withSurvDepthUnit() );
    }

    if ( SI().zIsTime() )
	lbls.add( uiStrings::sTWT().withSurvZUnit() );
    else
	lbls.add( uiString::empty() );

    lbls.add( uiStrings::sColor() );
    if ( withlvls )
	lbls.add( uiStrings::sRegionalMarker() );
}


static uiTable* createMarkerTable( uiParent* p, int nrrows, bool editable )
{
    uiTable* ret = new uiTable( p, uiTable::Setup().rowdesc("Marker")
						.rowgrow(editable).defrowlbl("")
						.selmode(uiTable::Multi),
			  "Well Marker Table" );
    uiStringSet lbls;
    getColumnLabels( lbls, nullptr, editable );
    ret->setColumnLabels( lbls );
    ret->setColumnResizeMode( uiTable::ResizeToContents );
    ret->setColumnStretchable( cLevelCol, true );
    ret->setNrRows( nrrows );
    ret->setColumnReadOnly( cTWTCol, true );
    ret->hideColumn( cTWTCol, !SI().zIsTime() );
    ret->setColumnReadOnly( cColorCol, true );
    ret->setPrefWidth( 650 );

    return ret;
}


static float uiMarkerDlgzFactor( uiCheckBox* cb=0 )
{
    const bool isinm = cb ? !cb->isChecked() : !SI().depthsInFeet();

    if ( SI().zIsTime() )
	return isinm ? 1 : mToFeetFactorF;

    return ((SI().zInFeet() && !isinm) ||
	    (SI().zInMeter() && isinm)) ? 1
					  : ( SI().zInFeet() && isinm )
					      ? mFromFeetFactorF
					      : mToFeetFactorF;
}


void uiMarkerDlg::exportMarkerSet( uiParent* p, const Well::MarkerSet& mset,
			const Well::Track& trck, const Well::D2TModel* d2t,
			uiCheckBox* cb )
{
    uiFileDialog fdlg( p, false, 0, 0, tr("%1 for export")
				       .arg(uiStrings::sFileName()) );
    fdlg.setDirectory( GetDataDir() );
    if ( !fdlg.go() )
	return;

    od_ostream strm( fdlg.fileName() );
    if ( !strm.isOK() )
    {
	BufferString msg( "Cannot open '", fdlg.fileName(), "' for write" );
	strm.addErrMsgTo( msg );
	return;
    }

    uiStringSet colnms;
    getColumnLabels( colnms, cb, false );
    strm << colnms.get( cDepthCol ).getFullString() << od_tab
	 << colnms.get( cTVDCol ).getFullString() << od_tab
	 << colnms.get( cTVDSSCol ).getFullString() << od_tab;
    if ( SI().zIsTime() && d2t )
	 strm << colnms.get( cTWTCol ).getFullString() << od_tab;

    strm << colnms.get( cNameCol ).getFullString() << od_newline;

    const float kbelev = trck.getKbElev();
    const float zfac = uiMarkerDlgzFactor( cb );
    for ( int idx=0; idx<mset.size(); idx++ )
    {
	const Well::Marker& mrkr = *mset[idx];
	const float dah = mrkr.dah();
	const float tvdss = sCast(float,trck.getPos(dah).z);
	const float tvd = tvdss + kbelev;
	strm << dah * zfac << od_tab
	     << tvd * zfac << od_tab
	     << tvdss * zfac << od_tab;
	if ( SI().zIsTime() && d2t )
	{
	    const float twt = d2t->getTime( dah, trck );
	    strm << twt * SI().zDomain().userFactor() << od_tab;
	}
	strm << mrkr.name() << od_newline;
    }
}


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& t,
			  const Well::D2TModel* d2t )
    : uiDialog(p,uiDialog::Setup(tr("Edit Well Markers"),mNoDlgTitle,
				 mODHelpKey(mMarkerDlgHelpID)))
    , track_(t)
    , d2tmodel_(d2t)
{
    uiString title( toUiString("%1: %2") );
    title.arg( uiStrings::sWell() ).arg( t.name() );
    setTitleText( title );

    table_ = new uiTable( this, uiTable::Setup().rowdesc("Marker")
						.rowgrow(true)
						.defrowlbl(true)
						.selmode(uiTable::Multi),
			  "Well Marker Table" );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( cLevelCol, true );
    table_->setNrRows( cNrEmptyRows );
    table_->setColumnReadOnly( cColorCol, true );
    table_->setSelectionBehavior( uiTable::SelectRows );
    mAttachCB( table_->doubleClicked, uiMarkerDlg::mouseClick );
    mAttachCB( table_->valueChanged, uiMarkerDlg::markerChangedCB );
    mAttachCB( table_->rowInserted, uiMarkerDlg::markerAddedCB );

    auto* updatebut = new uiPushButton( this, tr("Update display"),
			mCB(this,uiMarkerDlg,updateDisplayCB), true );
    updatebut->attach( leftAlignedBelow, table_ );

    auto* rfbut = new uiPushButton( this, uiStrings::sImport(),
			mCB(this,uiMarkerDlg,rdFile), false );
    rfbut->attach( rightOf, updatebut );

    auto* expbut = new uiPushButton( this, uiStrings::sExport(),
			mCB(this,uiMarkerDlg,exportCB), false );
    expbut->attach( rightOf, rfbut );

    auto* setregmarker = new uiPushButton( this, tr("Set as regional markers"),
			mCB(this,uiMarkerDlg,setAsRegMarkersCB), false );
    setregmarker->attach( alignedBelow, updatebut );

    auto* randclrbut = new uiToolButton( this, "random_color",
			tr("Assign random colors to markers"),
			mCB(this,uiMarkerDlg,assignRandomColorsCB) );
    randclrbut->attach( rightOf, setregmarker );

    unitfld_ = new uiCheckBox( this, tr("Z in Feet"),
				mCB(this,uiMarkerDlg,unitChangedCB) );
    unitfld_->attach( rightAlignedBelow, table_ );
    unitfld_->setChecked( SI().depthsInFeet() );

    uiStringSet header;
    getColLabels( header );
    table_->setColumnLabels( header );
    table_->setPrefWidth( 700 );
    table_->setPrefHeightInRows( cNrEmptyRows*2 );
    table_->hideColumn( cTWTCol, !SI().zIsTime() );
}


uiMarkerDlg::~uiMarkerDlg()
{
    detachAllNotifiers();
    delete oldmrkrs_;
    delete olddisps_;
}


void uiMarkerDlg::getColLabels( uiStringSet& lbls ) const
{
    getColumnLabels( lbls, unitfld_, true );
}


int uiMarkerDlg::getNrRows() const
{
    for ( int idx=table_->nrRows()-1; idx>=0; idx-- )
    {
	const StringView txt = table_->text( RowCol(idx,cNameCol) );
	if ( !txt.isEmpty() )
	    return idx+1;
    }

    return 0;
}


bool uiMarkerDlg::getFromScreen()
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) )
	return false;

    setMarkerSet( markers );
    return true;
}


void uiMarkerDlg::markerAddedCB( CallBacker* )
{
    auto* levelsel = new uiStratLevelSel( 0, true, uiStrings::sEmptyString() );
    mAttachCB( levelsel->selChange, uiMarkerDlg::stratLvlChg );
    const int currentrow = table_->currentRow();
    const OD::Color defgreycol( 128, 128, 128 );
    table_->setCellGroup( RowCol(currentrow,cLevelCol), levelsel );
    table_->setColor( RowCol(currentrow,cColorCol), defgreycol );
    table_->setCellReadOnly( RowCol(currentrow,cColorCol), true );
    levelsel->setSensitive( true );
}


void uiMarkerDlg::markerChangedCB( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    const int col = rc.col();
    const bool depthchg = col == cDepthCol || col == cTVDCol || col==cTVDSSCol;
    const bool nmchg = col == cNameCol;

    if ( depthchg )
    {
	if ( mIsUdf(table_->getFValue(rc)) )
	{
	    uiMSG().error( uiStrings::phrEnter(tr("a valid number")) );
	    return;
	}
	if ( !updateMarkerDepths(row,col==cDepthCol) )
	    return;
    }
    else if ( nmchg )
    {
	const char* markernm = table_->text( rc );
	if ( !markernm || !*markernm )
	{
	    uiMSG().error( uiStrings::sEnterValidName() );
	    return;
	}
    }

    uiObject* obj = table_->getCellObject( RowCol(row,cLevelCol) );
    if ( obj )
	obj->setSensitive( true );
    else
	markerAddedCB( nullptr );
}


float uiMarkerDlg::zFactor() const
{
    return uiMarkerDlgzFactor( unitfld_ );
}


void uiMarkerDlg::unitChangedCB( CallBacker* )
{
    NotifyStopper notifystop( table_->valueChanged );
    uiStringSet colnms;
    getColLabels( colnms );
    table_->setColumnLabels( colnms );
    const float zfac = unitfld_->isChecked() ? mToFeetFactorF
					     : mFromFeetFactorF;

    TypeSet<int> depthcols;
    depthcols += cDepthCol;
    depthcols += cTVDCol;
    depthcols += cTVDSSCol;
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	for ( int idx=0; idx<depthcols.size(); idx++ )
	{
	    const RowCol rc( rowidx, depthcols[idx] );
	    const float val = table_->getFValue( rc );
	    if ( mIsUdf(val) )
		continue;

	    table_->setValue( rc, val*zfac, 2 );
	}
    }
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    if ( rc.col() != cColorCol ) return;

    uiGroup* grp = table_->getCellGroup( RowCol(rc.row(),cLevelCol) );
    mDynamicCastGet(uiStratLevelSel*,levelsel,grp)
    const bool havelvl = levelsel && levelsel->getID().isValid();
    if ( havelvl )
    {
	uiMSG().error( tr("Cannot change color of regional marker") );
	return;
    }

    OD::Color newcol = table_->getColor( rc );
    if ( selectColor(newcol, this, tr("Marker color")) )
	table_->setColor( rc, newcol );

    table_->setSelected( rc, false );
}


void uiMarkerDlg::doStrat( CallBacker* )
{
    StratTWin().popUp();
}


int uiMarkerDlg::rowNrFor( uiStratLevelSel* lvlsel ) const
{
    const int nrrows = table_->nrRows();
    for ( int irow=0; irow<nrrows; irow++ )
    {
	if ( lvlsel == table_->getCellGroup( RowCol(irow,cLevelCol) ) )
	    return irow;
    }

    return -1;
}


void uiMarkerDlg::setMarkerSet( const Well::MarkerSet& markers, bool add )
{
    MultiID mid;
    if ( !getKey(mid) )
	return;

    const Well::LoadReqs reqs( Well::Mrkrs, Well::DispProps3D );
    ConstRefMan<Well::Data> wd = Well::MGR().get( mid, reqs );
    if ( !wd )
    {
	uiMSG().error( mToUiStringTodo(Well::MGR().errMsg()) );
	return;
    }

    if ( !olddisps_ )
	olddisps_ = new Well::DisplayProperties( wd->displayProperties() );

    const int nrnew = markers.size();
    NotifyStopper notifystop( table_->valueChanged );
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + startrow + cNrEmptyRows;
    if ( !add )
	table_->clearTable();

    table_->setNrRows( nrrows );
    const float zfac = zFactor();
    const float kbelev = track_.getKbElev();
    for ( int idx=0; idx<nrnew; idx++ )
    {
	const int irow = startrow + idx;
	uiGroup* grp = table_->getCellGroup( RowCol(irow,cLevelCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,grp);
	if ( !levelsel )
	{
	    levelsel = new uiStratLevelSel( nullptr, true, uiString::empty() );
	    mAttachCB( levelsel->selChange, uiMarkerDlg::stratLvlChg );
	    table_->setCellGroup( RowCol(irow,cLevelCol), levelsel );
	}

	const Well::Marker* marker = markers.validIdx(idx) ? markers[idx]
							   : nullptr;
	if ( marker )
	{
	    if ( !Strat::LVLS().isPresent( marker->levelID() ) )
		const_cast<Well::Marker*>(markers[idx])->setLevelID(
							Strat::LevelID::udf() );

	    levelsel->setID( marker->levelID() );
	    const float dah = marker->dah();
	    table_->setValue( RowCol(irow,cDepthCol), dah*zfac, 2 );
	    const float tvdss = sCast(float,track_.getPos(dah).z);
	    table_->setValue( RowCol(irow,cTVDCol), (tvdss+kbelev)*zfac, 2 );
	    table_->setValue( RowCol(irow,cTVDSSCol), tvdss*zfac, 2 );
	    if ( SI().zIsTime() && d2tmodel_ )
	    {
		const float twt = d2tmodel_->getTime( dah, track_ );
		table_->setValue( RowCol(irow,cTWTCol),
					 twt * SI().zDomain().userFactor(),2  );
	    }

	    table_->setText( RowCol(irow,cNameCol), marker->name() );
	    if ( wd->displayProperties().getMarkers()
					.isSelected( marker->name()) )
		table_->setCellChecked( RowCol(irow,cNameCol), true );
	    else
		table_->setCellChecked( RowCol(irow,cNameCol), false );

	    table_->setColor( RowCol(irow,cColorCol), marker->color() );
	    if ( marker->levelID().isValid() )
		updateFromLevel( irow, levelsel );

	    continue;
	}

	Well::Marker mrk;
	levelsel->setSensitive( true );
	table_->setText( RowCol(irow,cDepthCol), "" );
	table_->setText( RowCol(irow,cTVDCol), "" );
	table_->setText( RowCol(irow,cTVDSSCol), "" );
	table_->setText( RowCol(irow,cNameCol), "" );
	table_->setColor( RowCol(irow,cColorCol), mrk.color() );
    }

    table_->setColumnReadOnly( cColorCol, true );
    table_->resizeColumnsToContents();
    table_->setColumnStretchable( cLevelCol, true );

    if ( !oldmrkrs_ )
    {
	oldmrkrs_ = new Well::MarkerSet();
	getMarkerSet( *oldmrkrs_ );
    }
}


void uiMarkerDlg::stratLvlChg( CallBacker* cb )
{
    mDynamicCastGet(uiStratLevelSel*,levelsel,cb)
    if ( !levelsel ) return;

    const int irow = rowNrFor( levelsel );
    if ( irow < 0 ) return;

    updateFromLevel( irow, levelsel );
}


void uiMarkerDlg::updateFromLevel( int irow, uiStratLevelSel* levelsel )
{
    if ( !levelsel ) return;

    NotifyStopper notifystop( table_->valueChanged );
    const bool havelvl = levelsel->getID().isValid();
    if ( havelvl )
    {
	table_->setColor( RowCol(irow,cColorCol), levelsel->getColor() );
	table_->setText( RowCol(irow,cNameCol), levelsel->getLevelName() );
    }

    table_->setCellReadOnly( RowCol(irow,cNameCol), havelvl );
}


class uiReadMarkerFile : public uiDialog
{ mODTextTranslationClass(uiReadMarkerFile);
public:

uiReadMarkerFile( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Markers"),mNoDlgTitle,
				 mODHelpKey(mReadMarkerFileHelpID) ))
    , fd_(*Well::MarkerSetAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );
    fnmfld_ = new uiASCIIFileInput( this, true );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
		      mODHelpKey(mTableImpDataSelmarkersHelpID) );
    dataselfld_->attach( alignedBelow, fnmfld_ );

    replfld_ = new uiGenInput( this, tr("Existing markers (if any)"),
			      BoolInpSpec(true,tr("Replace"),tr("Keep")) );
    replfld_->attach( alignedBelow, dataselfld_ );

    colorfld_ = new uiCheckBox( this, tr("Assign random colors to markers") );
    colorfld_->setChecked( false );
    colorfld_->attach( alignedBelow, replfld_ );
}

~uiReadMarkerFile()
{
    delete &fd_;
}

bool acceptOK( CallBacker* ) override
{
    fnm_ = fnmfld_->fileName();
    if ( File::isEmpty(fnm_) )
	{ uiMSG().error( uiStrings::sInvInpFile() ); return false; }

    if ( !dataselfld_->commit() )
	return false;

    keep_ = !replfld_->getBoolValue();
    assignrandcolors_ = colorfld_->isChecked();
    return true;
}

    Table::FormatDesc&	fd_;
    BufferString	fnm_;
    bool		keep_;
    bool		assignrandcolors_;

    uiFileInput*	fnmfld_;
    uiGenInput*		replfld_;
    uiTableImpDataSel*	dataselfld_;
    uiCheckBox*		colorfld_;
};


void uiMarkerDlg::assignRandomColorsCB( CallBacker* )
{
    Well::MarkerSet mrkrs;
    getMarkerSet( mrkrs );
    assignRandomColors( mrkrs );
    setMarkerSet( mrkrs, false );
}


void uiMarkerDlg::assignRandomColors( Well::MarkerSet& mrkrs )
{
    for ( int midx=0; midx<mrkrs.size(); midx++ )
    {
	if ( !mrkrs[midx] || mrkrs[midx]->levelID().isValid() )
	    continue;

	mrkrs[midx]->setColor( OD::getRandomColor() );
    }
}


void uiMarkerDlg::rdFile( CallBacker* )
{
    uiReadMarkerFile dlg( this );
    if ( !dlg.go() ) return;

    od_istream strm( dlg.fnm_ );
    if ( !strm.isOK() )
	{ uiMSG().error( tr("Input file exists but cannot be read") ); return; }

    Well::MarkerSetAscIO aio( dlg.fd_ );
    Well::MarkerSet mrkrs;
    aio.get( strm, mrkrs, track_ );
    if ( mrkrs.isEmpty() )
    {
	uiMSG().error( tr("No valid markers found") );
	return;
    }

    if ( dlg.assignrandcolors_ )
	assignRandomColors( mrkrs );

    setMarkerSet( mrkrs, dlg.keep_ );
}


bool uiMarkerDlg::getMarkerSet( Well::MarkerSet& markers ) const
{
    deepErase( markers );
    BufferStringSet markernms;
    uiString errmsg;
    const float zfac = zFactor();
    for ( int rowidx=0; rowidx<table_->nrRows(); rowidx++ )
    {
	float dah = table_->getFValue( RowCol(rowidx,cDepthCol) );
	if ( mIsUdf(dah) )
	    continue;

	const char* markernm = table_->text( RowCol(rowidx,cNameCol) );
	if ( !markernm || !*markernm )
	    continue;

	if ( !markernms.addIfNew(markernm) )
	{
	    errmsg = tr("Marker name '%1' is present several times,"
			" please make sure it is unique")
		   .arg(markernm);
	    uiMSG().error( errmsg );
	    return false;
	}

	dah /= zfac;
	auto* marker = new Well::Marker( markernm, dah );
	marker->setColor( table_->getColor(RowCol(rowidx,cColorCol)) );
	uiGroup* grp = table_->getCellGroup( RowCol(rowidx,cLevelCol) );
	mDynamicCastGet(uiStratLevelSel*,levelsel,grp)
	marker->setLevelID( levelsel ? levelsel->getID()
				     : Strat::LevelID::udf() );
	markers += marker;
    }

    return true;
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) ) return false;

    Interval<float> dahrg( track_.dahRange() );
    const float zfac = zFactor();
    dahrg.scale( zFactor() );
    uiString errmsg;
    for ( int midx=0; midx<markers.size(); midx++ )
    {
	const float val = markers[midx]->dah() * zfac;
	if ( !dahrg.includes(val,true) )
	    errmsg = tr("'%1'%2").arg(markers[midx]->name());
    }

    if ( !errmsg.isEmpty() )
    {
	errmsg.arg(tr("depth value(s) is/are out of well track range [%1-%2]. "
		      "Press Abort if you want to re-enter the depth.")
		 .arg(dahrg.start).arg(dahrg.stop));
      const bool res = uiMSG().askContinue( errmsg );
      if ( !res ) return false;
    }

    return true;
}


class uiMarkersList : public uiDialog
{ mODTextTranslationClass(uiMarkersList);
public:

uiMarkersList( uiParent* p, const Well::MarkerSet& mset )
    : uiDialog(p,uiDialog::Setup(tr("Select Regional Markers"),
				 mNoDlgTitle,mNoHelpKey))
{
    list_ = new uiListBox( this, "Markers", OD::ChooseZeroOrMore );
    list_->setHSzPol( uiObject::Wide );
    for ( int idx=0; idx<mset.size(); idx++ )
	list_->addItem( toUiString(mset[idx]->name()), mset[idx]->color() );
}

void getSelIDs( TypeSet<int>& items )
{ list_->getChosen( items ); }

protected:
	uiListBox*	list_;
};


void uiMarkerDlg::setAsRegMarkersCB( CallBacker* )
{
    Well::MarkerSet mset;
    if ( !getMarkerSet( mset ) ) return;

    if ( !mset.size() )
    {
	uiMSG().message( tr("No markers available") );
	return;
    }

    uiMarkersList dlg( this, mset );
    if ( !dlg.go() ) return;

    TypeSet<int> selitems;
    dlg.getSelIDs( selitems );
    if ( !selitems.size() )
    {
	uiMSG().message( tr("No markers selected.") );
	return;
    }

    Strat::LevelSet& lvls = Strat::eLVLS();
    uiString msg;
    int mid = 0;
    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const int selidx = selitems[idx];
	if ( lvls.isPresent(mset[selidx]->name()) )
	{
	    msg = tr( "'%1' %2" ).arg( mset[selidx]->name() );
	    mid++;
	}
    }

    if ( !msg.isEmpty() )
    {
	msg.arg(tr("%1already set as regional marker(s)."
		   "Press Continue to update properties.")
	      .arg(mid > 1 ? tr("are ") : tr("is ")));
	const bool res = uiMSG().askContinue( msg );
	if ( !res ) return;
    }

    for ( int idx=0; idx<selitems.size(); idx++ )
    {
	const int selidx = selitems[idx];
	const Strat::LevelID lvlid = lvls.add( mset[selidx]->name(),
						 mset[selidx]->color() );
	lvls.write();
	mset[selidx]->setLevelID( lvlid );
    }

    setMarkerSet( mset, false );
}


void uiMarkerDlg::exportCB( CallBacker* )
{
    Well::MarkerSet mset;
    if ( !getMarkerSet( mset ) )
	return;

    if ( mset.isEmpty() )
    {
	uiMSG().message( tr("No data available to export") );
	return;
    }

    exportMarkerSet( this, mset, track_, d2tmodel_, unitfld_ );
}


bool uiMarkerDlg::getKey( MultiID& mid ) const
{
    IOM().to( IOObjContext::WllInf );
    ConstPtrMan<IOObj> obj = IOM().getLocal( track_.name(), "Well" );
    if ( !obj )
	return false;

    mid = obj->key();
    return true;
}


void uiMarkerDlg::getUnselMarkerNames( BufferStringSet& nms ) const
{
    for ( int row=0; row<table_->nrRows(); row++ )
    {
	if ( !table_->isCellChecked(RowCol(row,cNameCol)) )
	    nms.add( table_->text(RowCol(row,cNameCol)) );
    }
}


void uiMarkerDlg::updateDisplayCB( CallBacker* )
{
    MultiID mid;
    if ( !getKey(mid) )
	return;

    RefMan<Well::Data> wd = Well::MGR().get( mid, Well::LoadReqs(Well::Mrkrs) );
    if ( !wd )
    {
	uiMSG().error( mToUiStringTodo(Well::MGR().errMsg()) );
	return;
    }

    getMarkerSet( wd->markers() );
    BufferStringSet markernms, selmarkernms, unselmarkernms;
    wd->markers().getNames( markernms );
    getUnselMarkerNames( unselmarkernms );
    if ( unselmarkernms.isEmpty() )
    {
	wd->displayProperties().setMarkerNames( BufferStringSet(), true );
	wd->displayProperties().setMarkerNames( BufferStringSet(), false );
    }
    else
	wd->displayProperties().setMarkerNames( unselmarkernms, false );

    wd->markerschanged.trigger();
}


bool uiMarkerDlg::rejectOK( CallBacker* )
{
    MultiID mid;
    if ( !getKey(mid) )
	return true;

    RefMan<Well::Data> wd = Well::MGR().get( mid );
    if ( !wd )
	return true;

    if ( oldmrkrs_ )
    {
	deepCopy<Well::Marker,Well::Marker>( wd->markers(),*oldmrkrs_ );
	wd->markerschanged.trigger();
    }

    if ( olddisps_ )
    {
	wd->displayProperties() = *olddisps_;
	wd->disp3dparschanged.trigger();
    }

    return true;
}


#define mDelRet { if (marker) delete marker; return false; }


bool uiMarkerDlg::updateMarkerDepths( int rowidx, bool md2tvdss )
{
    NotifyStopper ns( table_->valueChanged );
    const RowCol rcin = table_->notifiedCell();
    const int row = rcin.row();
    const bool istvd = rcin.col() == cTVDCol;
    const float kbelev = track_.getKbElev();
    const float zfac = zFactor();

    float inval = table_->getFValue( rcin ) / zfac;
    if ( istvd ) inval -= kbelev;

    Interval<float> trckrg( md2tvdss ? track_.dahRange() : track_.zRange() );
    if ( !trckrg.includes(inval,true) )
    {
	uiString errmsg = tr("The entered depth %1 is outside of "
			     "track range\n[%2, %3] %4%5")
			.arg( inval * zfac ).arg( trckrg.start * zfac )
			.arg( trckrg.stop * zfac )
			.arg( !unitfld_->isChecked() ? "m" : "ft" )
			.arg( md2tvdss ? sKeyMD() : istvd ? sKeyTVD()
							  : sKeyTVDSS() );
	Well::Marker* marker = getMarker( row, true );
	uiMSG().error( errmsg );
	if ( marker )
	    table_->setValue( rcin, getOldMarkerVal(marker) );
	else
	    table_->setText(rcin, uiStrings::sEmptyString());

	mDelRet;
    }

    const float dah = md2tvdss ? inval : track_.getDahForTVD( inval );
    const float tvdss = md2tvdss ? mCast(float,track_.getPos(inval).z) : inval;
    const float tvd = tvdss + kbelev;
    if ( !md2tvdss )
	table_->setValue( RowCol(row,cDepthCol), dah * zfac );

    if ( md2tvdss || istvd )
	table_->setValue( RowCol(row,cTVDSSCol), tvdss * zfac );

    if ( md2tvdss || !istvd )
	table_->setValue( RowCol(row,cTVDCol), tvd * zfac );

    if ( SI().zIsTime() && d2tmodel_ )
    {
	const float twt = d2tmodel_->getTime( dah, track_ );
	table_->setValue( RowCol(row,cTWTCol),
			  twt * SI().zDomain().userFactor(), 2 );
    }

    return true;
}


Well::Marker* uiMarkerDlg::getMarker( int row, bool fromname ) const
{
    Well::MarkerSet markers;
    if ( !getMarkerSet(markers) )
	return nullptr;

    const char* markernm = table_->text(RowCol(row,cNameCol));
    const float dah = table_->getFValue( RowCol(row,cDepthCol) );
    if ( (fromname && (!markernm || !*markernm)) ||
	 (!fromname && mIsUdf(dah)) )
	return nullptr;

    const int markeridx = markers.getIdxAbove( dah / zFactor() );
    if ( ( fromname && !markers.getByName(markernm) ) ||
	 (!fromname && !markers.validIdx(markeridx) ) )
	return nullptr;

    auto* marker = new Well::Marker();
    *marker = fromname ? *(markers.getByName( markernm ))
		       : *(markers[markeridx]);

    return marker;
}


float uiMarkerDlg::getOldMarkerVal( Well::Marker* marker ) const
{
    if ( !marker )
	return mUdf(float);

    const RowCol rc = table_->notifiedCell();
    const bool ismd = rc.col() == cDepthCol;
    const bool istvd = rc.col() == cTVDCol;
    const float kbelev = track_.getKbElev();

    const float olddah = marker->dah();
    const float oldtvdss = mCast(float,track_.getPos(olddah).z);
    const float oldtvd = oldtvdss + kbelev;
    const float oldval = ismd ? olddah : istvd ? oldtvd : oldtvdss;

    return oldval * zFactor();
}



uiMarkerViewDlg::uiMarkerViewDlg( uiParent* p, const Well::Data& wd )
    : uiDialog(p,Setup(mJoinUiStrs(sWell(),sMarker(mPlural)),mNoDlgTitle,
		       mODHelpKey(mMarkerViewDlgHelpID)))
    , wd_(&wd)
{
    if ( !wd_ )
	return;

    setCtrlStyle( CloseOnly );
    setTitleText( tr("Markers for well '%1'").arg(toUiString(wd_->name())) );

    const Well::MarkerSet& mset = wd_->markers();
    if ( mset.isEmpty() )
	{ new uiLabel( this, tr("No markers for this well") ); return; }

    const int nrmrks = mset.size();
    table_ = createMarkerTable( this, nrmrks, false );

    const float zfac = uiMarkerDlgzFactor();
    const Well::Track& trck = wd_->track();
    const float kbelev = trck.getKbElev();

    for ( int irow=0; irow<nrmrks; irow++ )
    {
	const Well::Marker& mrkr = *mset[irow];
	table_->setText( RowCol(irow,cNameCol), mrkr.name() );
	table_->setColor( RowCol(irow,cColorCol), mrkr.color() );

	const float dah = mrkr.dah();
	table_->setValue( RowCol(irow,cDepthCol), dah*zfac, 2 );
	const float tvdss = (float)trck.getPos(dah).z;
	table_->setValue( RowCol(irow,cTVDCol), (tvdss+kbelev)*zfac, 2 );
	table_->setValue( RowCol(irow,cTVDSSCol), tvdss*zfac, 2 );
    }


    auto* expbut = new uiPushButton( this, uiStrings::sExport(),
				    mCB(this,uiMarkerViewDlg,exportCB), false );
    expbut->setIcon( "export" );
    expbut->attach( centeredBelow, table_ );
}


uiMarkerViewDlg::~uiMarkerViewDlg()
{
    detachAllNotifiers();
}


void uiMarkerViewDlg::exportCB( CallBacker* )
{
    if ( !wd_ )
	return;

    uiMarkerDlg::exportMarkerSet( this, wd_->markers(), wd_->track(),
				  wd_->d2TModel() );
}
