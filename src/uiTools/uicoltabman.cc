/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicoltabman.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "color.h"
#include "coltabsequence.h"
#include "draw.h"
#include "hiddenparam.h"
#include "mouseevent.h"
#include "od_helpids.h"
#include "settings.h"
#include "timer.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicoltabexport.h"
#include "uicoltabimport.h"
#include "uicoltabmarker.h"
#include "uicoltabtools.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uisplitter.h"
#include "uistring.h"
#include "uistrings.h"
#include "uistrings.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitreeview.h"
#include "uiworld2ui.h"

#define mTransHeight	300
#define mTransWidth	600

static StringView sKeyAskBeforeSettingToSeg()
{ return "Ask before setting a transparency line to segments"; }

static StringView sKeyAskBeforeNotSavingCT()
{ return "Ask before closing without saving Color Table"; }

static const int sPosCol = 0;
static const int sDataCol = 1;
static const int sTranspCol = 2;
static const int sPosColorCol = 3;

static const int sColorCol = 1;

static HiddenParam<uiColorTableMan,Interval<float>*> hp_ctabrange( nullptr );
static HiddenParam<uiColorTableMan,uiLineEdit*> hp_minfld(nullptr);
static HiddenParam<uiColorTableMan,uiLineEdit*> hp_maxfld(nullptr);


mClass(uiTools) uiTranspValuesDlgPlus : public uiDialog
{ mODTextTranslationClass(uiTranspValuesDlgPlus);
public:
    uiTranspValuesDlgPlus(uiParent*,ColTab::Sequence&,
			  const Interval<float>&);
    ~uiTranspValuesDlgPlus();

    Notifier<uiTranspValuesDlgPlus> valuesChanged;
    Notifier<uiTranspValuesDlgPlus>& segmentInserted();
    Notifier<uiTranspValuesDlgPlus>& segmentRemoved();


protected:

    uiTable*			table_;
    uiTable*			anchortable_	    = nullptr;
    uiPushButton*		syncanchors_;
    uiPushButton*		resettransp_;
    ColTab::Sequence&		ctab_;
    Interval<float>		ctabrange_;

    void			doFinalizeCB(CallBacker*);
    void			setPtsToAnchSegCB(CallBacker*);
    void			dataChgdCB(CallBacker*);
    void			pointInsertedCB(CallBacker*);
    void			pointDeletedCB(CallBacker*);
    void			resetTranspPtsCB(CallBacker*);

    void			fillTableWithPts();
    void			fillAnchorTable();
    void			fillTableWithSegments(bool resettransp);
    void			setPtsToAnchSeg(bool extrapolate);
    void			handleColorPos();
    void			handleDataVal();
    void			handleTranspVal();
};


static HiddenParam<uiTranspValuesDlgPlus,Notifier<uiTranspValuesDlgPlus>*>
    hp_segmentinserted(nullptr);

Notifier<uiTranspValuesDlgPlus>& uiTranspValuesDlgPlus::segmentInserted()
{
    return *hp_segmentinserted.getParam( this );
}


static HiddenParam<uiTranspValuesDlgPlus,Notifier<uiTranspValuesDlgPlus>*>
    hp_segmentremoved(nullptr);

Notifier<uiTranspValuesDlgPlus>& uiTranspValuesDlgPlus::segmentRemoved()
{
    return *hp_segmentremoved.getParam( this );
}


uiTranspValuesDlg::uiTranspValuesDlg( uiParent* p, ColTab::Sequence& ctab,
				      const Interval<float>& ctabrange )
    : uiDialog(p,Setup(tr("View Point Values"),mNoHelpKey))
    , valuesChanged(this)
    , ctab_(ctab)
    , ctabrange_(ctabrange)
{
    table_ = new uiTable( this, uiTable::Setup(ctab_.transparencySize(),3)
			     .rowgrow(true)
			     .rowdesc(uiStrings::sPoint())
			     .defrowlbl(true)
			     .manualresize(true)
			     .removeselallowed(false)
			     .fillrow(true),
			     "Position Table");

    const uiString color = tr("Color Positions\n(0 - 1)");
    const uiString data = uiStrings::phrJoinStrings( uiStrings::sData(),
						uiStrings::sValue(mPlural) );
    const uiString transparency = uiStrings::phrJoinStrings(
	uiStrings::sTransparency(),tr("%") );
    const uiStringSet columnlabels = { color, data, transparency };
    table_->setColumnLabels( columnlabels );
    table_->setSelectionMode( uiTable::SingleRow );
    fillTable();

    mAttachCB( table_->valueChanged, uiTranspValuesDlg::colorOrDataChgdCB );
    mAttachCB( table_->valueChanged, uiTranspValuesDlg::transpValChgdCB );
    mAttachCB( table_->rowInserted, uiTranspValuesDlg::pointInsertedCB );
    mAttachCB( table_->rowDeleted, uiTranspValuesDlg::pointDeletedCB );
}


uiTranspValuesDlg::~uiTranspValuesDlg()
{
    detachAllNotifiers();
}


void uiTranspValuesDlg::fillTable()
{
    const int transpsize = ctab_.transparencySize();
    const bool udfrange = ctabrange_.isUdf();
    for ( int cidx=0; cidx<transpsize; cidx++ )
    {
	const Geom::Point2D<float> position = ctab_.transparency( cidx );
	float dataval = mUdf(float);
	if ( !udfrange )
	{
	    const float min = ctabrange_.start_;
	    const float max = ctabrange_.stop_;
	    dataval = position.x_*(max-min)+min;
	}

	const float transperc = position.y_/255*100;
	const float colpos = position.x_;

	table_->setText( RowCol(cidx,sPosCol), toUiStringDec(colpos,2) );
	table_->setText( RowCol(cidx,sDataCol), toUiString(dataval,0,'g',5));
	table_->setText( RowCol(cidx,sTranspCol), toUiStringDec(transperc,0) );
    }

    table_->setCellReadOnly( RowCol(0,0), true );
    table_->setCellReadOnly( RowCol(0,1), true);
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,0), true );
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,1), true );

    if ( udfrange )
	table_->hideColumn( sDataCol, true);

    table_->setMinimumWidthInChar( 52 );
    table_->setPrefHeightInRows( 7 );
}


void uiTranspValuesDlg::colorOrDataChgdCB( CallBacker* )
{
    const RowCol rc = table_->currentCell();
    const int col = rc.col();

    if ( col == 0 )
	handleColorPos();
    else if ( col == 1 )
	handleDataVal();
    else
	return;
}


void uiTranspValuesDlg::handleColorPos()
{
    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();

    const float newcolpos = table_->getFValue( rc );

    const RowCol prevrc = RowCol( row-1, col );
    const float prevval = table_->getFValue( prevrc );

    const RowCol nextrc = RowCol( row+1, col );
    const float nextval = table_->getFValue( nextrc );

    if ( newcolpos>1 || newcolpos<0 )
    {
	const uiString msg = tr("Entered Color Position '%1' is out of range.\n"
				"Please enter a position between 0 and 1.")
				 .arg( newcolpos );
	uiMSG().error( msg );
	table_->setText( rc, toUiStringDec(ctab_.transparency(row).x_,2) );
	return;
    }
    else if ( newcolpos<prevval || newcolpos>nextval )
    {
	const uiString msg = tr("Entered Color Position '%1' is Invalid for "
				"Point %2.\n"
				"Please enter a position between %3 and %4.")
				 .arg( newcolpos )
				 .arg( row+1 )
				 .arg( prevval )
				 .arg( nextval );
	uiMSG().error( msg );
	table_->setText( rc, toUiStringDec(ctab_.transparency(row).x_,2) );
	return;
    }

    if ( !ctabrange_.isUdf() )
    {
	const RowCol rchist = RowCol( row, sDataCol );
	const float min = ctabrange_.start_;
	const float max = ctabrange_.stop_;
	const float dataval = newcolpos*(max-min)+min;
	table_->setText( rchist, toUiString(dataval,0,'g',5) );
    }

    table_->setText( rc, toUiStringDec(newcolpos,2) );
    const float transpval = ctab_.transparency( row ).y_;
    const Geom::Point2D<float> newpoint = { newcolpos, transpval };
    ctab_.changeTransparency( row, newpoint );
    valuesChanged.trigger();
}


void uiTranspValuesDlg::handleDataVal()
{
    if ( ctabrange_.isUdf() )
    {
	pErrMsg("Attempted to change the Data Value column when range is Udf");
	return;
    }

    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();

    const float newdataval = table_->getFValue( rc );

    const RowCol prevrc = RowCol( row-1, col );
    const float prevval = table_->getFValue( prevrc );

    const RowCol nextrc = RowCol( row+1, col );
    const float nextval = table_->getFValue( nextrc );

    const float min = ctabrange_.start_;
    const float max = ctabrange_.stop_;

    if ( newdataval>max || newdataval<min )
    {
	const uiString msg = tr("Entered Data Value '%1' is out of range.\n"
				"Please enter a value between %2 and %3.")
				 .arg( newdataval )
				 .arg( min )
				 .arg( max );
	uiMSG().error( msg );
	const float colorpos = ctab_.transparency(row).x_;
	const float olddataval = colorpos*(max-min)+min;
	table_->setText( rc, toUiString(olddataval,0,'g',5) );
	return;
    }
    else if ( newdataval<prevval || newdataval>nextval )
    {
	const uiString msg = tr("Entered Data Value '%1' is Invalid for "
				"Point %2: "
				"Please enter a value between %3 and %4.")
				 .arg( newdataval )
				 .arg( row+1 )
				 .arg( prevval )
				 .arg( nextval );
	uiMSG().error( msg );
	const float colorpos = ctab_.transparency(row).x_;
	const float olddataval = colorpos*(max-min)+min;
	table_->setText( rc, toUiString(olddataval,0,'g',5) );
	return;
    }

    table_->setText( rc, toUiString(newdataval,0,'g',5) );

    const RowCol colrc = RowCol( row, sPosCol );
    const float newcolorpos = (newdataval-min)/(max-min);
    table_->setText( colrc, toUiStringDec(newcolorpos,2) );

    const float transpval = ctab_.transparency( row ).y_;
    const Geom::Point2D<float> newpoint = { newcolorpos, transpval };
    ctab_.changeTransparency( row, newpoint );
    valuesChanged.trigger();
}


void uiTranspValuesDlg::transpValChgdCB( CallBacker* )
{
    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();
    if ( col < 2 )
	return;

    const float newtranspval = table_->getFValue( rc );

    if ( newtranspval>100 || newtranspval<0 )
    {
	uiMSG().error( tr("Entered Transparency Value '%1' is out of range.\n"
			 "Please enter a percentage between 0 and 100%.")
			  .arg( newtranspval ) );
	const float transperc = ctab_.transparency(row).y_/255*100;
	table_->setText( rc, toUiStringDec(transperc,0) );
	return;
    }

    table_->setText( rc, toUiStringDec(newtranspval,0) );
    const float newcolpos = ctab_.transparency( row ).x_;
    const float convtranspval = newtranspval/100*255;
    const Geom::Point2D<float> newval = { newcolpos, convtranspval };
    ctab_.changeTransparency( row, newval );
    valuesChanged.trigger();
}


void uiTranspValuesDlg::pointInsertedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcpos = table_->newCell();
    const int row = rcpos.row();
    if ( row-1<0 || row>=ctab_.transparencySize() )
    {
	table_->removeRow( rcpos );
	uiMSG().error( tr("Cannot insert new points before or after "
			 "extreme positions.") );
	return;
    }

    const float x1 = ctab_.transparency(row-1).x_;
    const float x2 = ctab_.transparency(row).x_;
    const float y1 = ctab_.transparency(row-1).y_;
    const float y2 = ctab_.transparency(row).y_;

    const float epsilonx = 1e-2;
    if ( x2-x1<epsilonx )
    {
	table_->removeRow( rcpos );
	uiMSG().error( tr("Cannot insert a new point here since the point will "
			 "exceed the proximity threshold between "
			 "the previous and next points.") );
	return;
    }

    const float newposval = (x1+x2)/2;
    const float newtransval = (y1+y2)/2;
    const Geom::Point2D<float> newval = { newposval, newtransval };
    table_->setCurrentCell( RowCol(row,sPosCol) );
    ctab_.setTransparency( newval );
    fillTable();
    valuesChanged.trigger();
}


void uiTranspValuesDlg::pointDeletedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    if ( row==0 || row==ctab_.transparencySize()-1 )
    {
	table_->insertRows( rc, 1 );
	fillTable();
	uiMSG().error( tr("Cannot remove remove points "
			 "from extreme positions.") );
	return;
    }

    ctab_.removeTransparencyAt( row );
    fillTable();
    valuesChanged.trigger();
}


uiTranspValuesDlgPlus::uiTranspValuesDlgPlus( uiParent* p,
					      ColTab::Sequence& ctab,
					      const Interval<float>& ctabrange )
    : uiDialog(p,Setup(tr("View Point Values"),mNoHelpKey))
    , valuesChanged(this)
    , ctab_(ctab)
    , ctabrange_(ctabrange)
{
    hp_segmentinserted.setParam( this,
				 new Notifier<uiTranspValuesDlgPlus>(this));
    hp_segmentremoved.setParam( this,
				new Notifier<uiTranspValuesDlgPlus>(this));
    const bool hasequalseg = ctab_.nrSegments() > 1;

    auto* tablegrp = new uiGroup( this, "Table" );
    table_ = new uiTable( tablegrp, uiTable::Setup(ctab_.transparencySize(),4)
				   .rowdesc(hasequalseg ? tr("Segment")
							: uiStrings::sPoint())
				   .rowgrow(true)
				   .defrowlbl(true)
				   .manualresize(false)
				   .removeselallowed(false)
				   .fillrow(true),
			  "Position Table");
    const uiString pos = hasequalseg ? tr("% Value") : tr("Position\n(0 - 1)");
    const uiString transparency = tr("% Transparency");
    const uiString color = uiStrings::sColor();
    uiStringSet columnlabels;
    columnlabels.add(pos).add(uiStrings::sValue())
	.add(transparency).add(color);
    table_->setColumnLabels( columnlabels );
    table_->setSelectionMode( uiTable::SingleRow );
    table_->setColumnReadOnly( sPosColorCol, true );

    auto* buttongrp = new uiButtonGroup( this, "Buttons", OD::Horizontal );
    syncanchors_ = new uiPushButton( buttongrp, hasequalseg
			? tr("Convert Transparency Points to Segments")
			: tr("Set Transparency Points to Anchor Values"));
    syncanchors_->attach( leftAlignedBelow, table_ );
    mAttachCB( syncanchors_->activated,
	       uiTranspValuesDlgPlus::setPtsToAnchSegCB );

    resettransp_ = new uiPushButton( buttongrp, hasequalseg
				? tr("Clear and set to Initial Anchor Points")
				: tr("Clear Transparency Points"));
    resettransp_->attach( rightAlignedBelow, table_ );
    mAttachCB( resettransp_->activated,
	       uiTranspValuesDlgPlus::resetTranspPtsCB );
    resettransp_->display( false );

    auto* splitter = new uiSplitter( this, "Splitter", false );

    if ( !hasequalseg )
    {
	anchortable_ = new uiTable( tablegrp, uiTable::Setup(ctab_.size(),2)
					     .rowdesc(tr("Anchor"))
					     .defrowlbl(true)
					     .manualresize(false)
					     .removeselallowed(false)
					     .insertcolallowed(false),
				    "Anchor Table");
	uiStringSet anchorcolumnlabels;
	anchorcolumnlabels.add( tr("Position\n(0 - 1)") )
	    .add( uiStrings::sColor() );
	anchortable_->setColumnLabels( anchorcolumnlabels );
	anchortable_->setSelectionMode( uiTable::SingleRow );
	fillAnchorTable();
	anchortable_->setTableReadOnly(true);
	anchortable_->attach( rightOf, table_ );
    }

    splitter->addGroup( tablegrp );
    splitter->addGroup( buttongrp );

    mAttachCB( table_->valueChanged, uiTranspValuesDlgPlus::dataChgdCB );
    mAttachCB( table_->rowInserted, uiTranspValuesDlgPlus::pointInsertedCB );
    mAttachCB( table_->rowDeleted, uiTranspValuesDlgPlus::pointDeletedCB );
    mAttachCB( postFinalize(), uiTranspValuesDlgPlus::doFinalizeCB );
}


uiTranspValuesDlgPlus::~uiTranspValuesDlgPlus()
{
    detachAllNotifiers();
    hp_segmentinserted.removeAndDeleteParam( this );
    hp_segmentremoved.removeAndDeleteParam( this );
}


void uiTranspValuesDlgPlus::doFinalizeCB( CallBacker* )
{
    if ( ctab_.nrSegments() > 1 )
    {
	setPtsToAnchSegCB( nullptr );
    }
    else
	fillTableWithPts();
}


void uiTranspValuesDlgPlus::fillTableWithPts()
{
    NotifyStopper notifstop( table_->valueChanged );
    const int transpsize = ctab_.transparencySize();
    const bool udfrange = ctabrange_.isUdf();
    for ( int cidx=0; cidx<transpsize; cidx++ )
    {
	const Geom::Point2D<float> position = ctab_.transparency( cidx );
	float dataval = mUdf(float);
	if ( !udfrange )
	{
	    const float min = ctabrange_.start_;
	    const float max = ctabrange_.stop_;
	    dataval = position.x_*(max-min)+min;
	}

	const float transperc = position.y_/255*100;
	const float colpos = position.x_;
	if ( ctab_.nrSegments()>1 )
	    table_->setText( RowCol(cidx,sPosCol), toUiStringDec(colpos*100,0));
	else
	    table_->setText( RowCol(cidx,sPosCol), toUiStringDec(colpos,2) );

	table_->setText( RowCol(cidx,sDataCol), toUiString(dataval,0,'g',5));
	table_->setText( RowCol(cidx,sTranspCol), toUiStringDec(transperc,0) );
	table_->setColor( RowCol(cidx,sPosColorCol), ctab_.color(colpos) );
    }

    table_->setCellReadOnly( RowCol(0,0), true );
    table_->setCellReadOnly( RowCol(0,1), true);
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,0), true );
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,1), true );
    table_->setColumnLabel( sDataCol, uiStrings::sValue() );

    if ( udfrange )
	table_->hideColumn( sDataCol, true);

    if ( ctab_.nrSegments() < 2 )
	table_->hideColumn( sPosColorCol, true );

    const bool eqseg = ctab_.nrSegments() > 1;
    table_->setColumnReadOnly( sPosCol, eqseg );
    table_->setColumnReadOnly( sDataCol, eqseg );
    table_->setColumnReadOnly( sTranspCol, eqseg );

    table_->setMinimumWidthInChar( 80 );

    if ( anchortable_ && ctab_.nrSegments() < 2 )
	fillAnchorTable();
}


void uiTranspValuesDlgPlus::fillTableWithSegments( bool resettransp )
{
    NotifyStopper notifstop( table_->valueChanged );
    const int nrsegments = ctab_.nrSegments();
    const bool udfrange = ctabrange_.isUdf();

    const int transpsize = ctab_.transparencySize();

    for ( int cidx=0; cidx<transpsize-2; cidx+=2 )
    {
	const Geom::Point2D<float> pos1 = ctab_.transparency( cidx+1 );
	const Geom::Point2D<float> pos2 = ctab_.transparency( cidx+2 );

	float dataval1 = mUdf(float);
	float dataval2 = mUdf(float);
	if ( !udfrange )
	{
	    const float min = ctabrange_.start_;
	    const float max = ctabrange_.stop_;
	    const float nrseg = ctab_.nrSegments();
	    const float idx = cidx/2;
	    dataval1 = idx/nrseg*(max-min)+min;
	    dataval2 = (idx+1.0f)/nrseg*(max-min)+min;
	}

	float trperc = 0;
	if ( !resettransp )
	{
	    trperc = pos1.y_/255*100;
	}

	const uiString start = toUiString( dataval1, 0, 'g', 5 );
	const uiString stop = toUiString( dataval2, 0, 'g', 5 );

	const uiString segmentrange = tr("( %1, %2 )").arg( start ).arg( stop );
	const uiString posrange = tr("( %1, %2 )").arg( pos1.x_ ).arg( pos2.x_);

	float colpos = 0;
	const int halfcol = nrsegments;
	colpos = halfcol > 6 ? cidx>halfcol ? pos1.x_ : pos2.x_
			     : pos1.x_;

	table_->setText( RowCol(cidx/2,sDataCol), segmentrange );
	table_->setText( RowCol(cidx/2,sTranspCol), toUiStringDec(trperc,0) );
	table_->setColor( RowCol(cidx/2,sPosColorCol), ctab_.color(colpos) );
    }

    table_->setCellReadOnly( RowCol(0,0), true );
    table_->setCellReadOnly( RowCol(0,1), true);
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,0), true );
    table_->setCellReadOnly( RowCol(table_->nrRows()-1,1), true );
    table_->setColumnReadOnly( sPosColorCol, true );
    table_->hideColumn( sPosCol, true );
    table_->hideColumn( sPosColorCol, false );
    table_->setColumnLabel( sDataCol, tr("Value Range") );
    table_->setColumnReadOnly( sDataCol, true );
    table_->setColumnReadOnly( sTranspCol, false );

    if ( udfrange )
	table_->hideColumn( sDataCol, true );

    table_->setMinimumWidthInChar( 80 );
    table_->setMinimumHeightInChar( 28 );
}


void uiTranspValuesDlgPlus::fillAnchorTable()
{
    const int numpos = ctab_.size();
    for ( int cidx=numpos-1; cidx>=0; cidx-- )
    {
	const float position = ctab_.position( cidx );
	anchortable_->setValue( RowCol(cidx,sPosCol), position );
	anchortable_->setColor( RowCol(cidx,sColorCol), ctab_.color(position) );
    }

    anchortable_->setTableReadOnly( true );
    anchortable_->setMinimumWidthInChar( 40 );
    anchortable_->setMinimumHeightInChar( 28 );
}


void uiTranspValuesDlgPlus::setPtsToAnchSegCB( CallBacker * )
{
    uiString synctxt = syncanchors_->text();
    BufferString lb = synctxt.getString();
    const bool settozero = lb.contains("0");

    setPtsToAnchSeg( !settozero );

    if ( ctab_.nrSegments() > 1 )
    {
	syncanchors_->setText( tr("Set All Segments to 0% Transparency") );
    }
}


void uiTranspValuesDlgPlus::dataChgdCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->currentCell();
    const int col = rc.col();

    switch ( col )
    {
	case 0:
	    handleColorPos();
	    break;
	case 1:
	    handleDataVal();
	    break;
	case 2:
	    handleTranspVal();
	    break;
	case 3:
	    break;//TODO: handleColorCol()
	default:
	    pErrMsg("Column is invalid in transparency table");
	    break;
    }

    if ( ctab_.nrSegments() < 2 )
    {
	table_->clearTable();
	fillTableWithPts();
    }
    else
    {
	table_->clearTable();
	fillTableWithSegments(false);
    }
}


void uiTranspValuesDlgPlus::setPtsToAnchSeg( bool extrapolate )
{
    NotifyStopper ns( table_->valueChanged );

    const int nrsegs = ctab_.nrSegments();
    const float nrseg = nrsegs;
    const float segsz = 1.0f/nrseg;
    TypeSet<float> tvals;
    if ( nrsegs<26 && nrsegs>1 )
    {
	const float quarter = segsz/4;
	for ( float i=segsz/2; i<1; i+=segsz )
	{
	    if ( !extrapolate )
	    {
		tvals.add( 0 );
		continue;
	    }

	    float high = 0;
	    float low = 255;
	    for ( float j=(i-quarter); j<(i+quarter); j+=0.002 )
	    {
		const float transp = ctab_.transparencyAt(j);
		if ( transp > high )
		    high = transp;

		if ( transp < low )
		    low = transp;
	    }

	    const float avg = (high+low)/2;
	    tvals.add( avg );
	}
    }

    TypeSet<float> transpvals;
    for ( int cidx=0; cidx<ctab_.size(); cidx++)
    {
	const float position = ctab_.position( cidx );
	const float transp = extrapolate ? ctab_.transparencyAt(position) : 0;
	transpvals.add( transp );
    }

    while ( ctab_.transparencySize() > 2 )
	ctab_.removeTransparencyAt( 1 );

    if ( nrsegs==0 || nrsegs==-1 )
    {
	for ( int cidx=0; cidx<ctab_.size(); cidx++ )
	{
	    const float position = ctab_.position( cidx );
	    const float transp = transpvals.get(cidx);
	    const Geom::Point2D<float> newval = { position, transp };
	    ctab_.setTransparency( newval );
	}

	int trows = table_->nrRows();
	if ( trows < ctab_.size() )
	{
	    table_->insertRows( trows, ctab_.size()-trows );
	}
	else if ( trows > ctab_.size() )
	{
	    while ( trows >= ctab_.size() )
		table_->removeRow( trows-- );
	}
    }
    else if ( nrsegs < 26 )
    {
	ctab_.setTransparency( {0,0} );
	ctab_.setTransparency( {.001,tvals.first()} );

	int rgidx = 0;

	for ( float cidx=segsz; cidx<1; cidx+=segsz )
	{
	    const Geom::Point2D<float> segval = { cidx-.001f,
						  tvals.get(rgidx++)};
	    ctab_.setTransparency( segval );

	    if ( rgidx > tvals.size()-1 )
		rgidx = tvals.size()-1;

	    const Geom::Point2D<float> segval2 = { cidx,
						   tvals.get(rgidx) };
	    ctab_.setTransparency( segval2 );
	}

	ctab_.setTransparency( {.999,tvals.last()} );
	ctab_.setTransparency( {1,0} );

	const int tabsize = nrsegs;

	int trows = table_->nrRows();
	if ( trows < tabsize )
	{
	    table_->insertRows( trows, tabsize-trows );
	}
	else if ( trows > tabsize )
	{
	    while ( trows >= tabsize )
		table_->removeRow( trows-- );
	}
    }
    else
	uiMSG().error(tr("Number of Segments to replace the Points exceeds "
			 "the max threshold."));


    table_->clearTable();
    if ( ctab_.nrSegments() > 1 )//TODO: Change this condition should segment
    //view will be need to be implemented for Variable segmentation
    {
	fillTableWithSegments( false );
    }
    else
    {
	fillTableWithPts();
    }

    valuesChanged.trigger();
    return;
}


void uiTranspValuesDlgPlus::handleColorPos()
{
    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();

    const float newcolpos = table_->getFValue( rc );

    const RowCol prevrc = RowCol( row-1, col );
    const float prevval = table_->getFValue( prevrc );

    const RowCol nextrc = RowCol( row+1, col );
    const float nextval = table_->getFValue( nextrc );
    const bool fixed = ctab_.nrSegments()>1;
    const float posmax = fixed ? 100 : 1;

    if ( newcolpos>posmax || newcolpos<0 )
    {
	const uiString msg = tr("Entered Value '%1' is out of range.\n"
				"Please enter a value between 0 and %1.")
				 .arg( newcolpos )
				 .arg( posmax );
	uiMSG().error( msg );
	table_->setText(rc, toUiStringDec(ctab_.transparency(row).x_*posmax,2));
	return;
    }
    else if ( newcolpos<prevval || newcolpos>nextval )
    {
	const uiString msg = tr("Entered Value '%1' is Invalid for Point %2.\n"
				"Please enter a position between %3 and %4.")
				 .arg( newcolpos )
				 .arg( row+1 )
				 .arg( prevval )
				 .arg( nextval );
	uiMSG().error( msg );
	table_->setText( rc, toUiStringDec(ctab_.transparency(row).x_*posmax,
					   fixed ? 0 : 2) );
	return;
    }

    if ( !ctabrange_.isUdf() )
    {
	const RowCol rchist = RowCol( row, sDataCol );
	const float min = ctabrange_.start_;
	const float max = ctabrange_.stop_;
	const float dataval = newcolpos*(max-min)+min;
	table_->setText( rchist, toUiString(dataval,0,'g',5) );
    }

    table_->setText( rc, toUiStringDec(newcolpos,fixed ? 0 : 2) );
    const float transpval = ctab_.transparency( row ).y_;
    const Geom::Point2D<float> newpoint = { newcolpos/posmax, transpval };
    ctab_.changeTransparency( row, newpoint );
    valuesChanged.trigger();
}


void uiTranspValuesDlgPlus::handleDataVal()
{
    if ( ctabrange_.isUdf() )
    {
	pErrMsg("Attempted to change the Data Value column when range is Udf");
	return;
    }

    const RowCol rc = table_->currentCell();
    const int row = rc.row();
    const int col = rc.col();

    const float newdataval = table_->getFValue( rc );

    const RowCol prevrc = RowCol( row-1, col );
    const float prevval = table_->getFValue( prevrc );

    const RowCol nextrc = RowCol( row+1, col );
    const float nextval = table_->getFValue( nextrc );

    const float min = ctabrange_.start_;
    const float max = ctabrange_.stop_;

    if ( newdataval>max || newdataval<min )
    {
	const uiString msg = tr("Entered Data Value '%1' is out of range.\n"
				"Please enter a value between %2 and %3.")
				 .arg( newdataval )
				 .arg( min )
				 .arg( max );
	uiMSG().error( msg );
	const float colorpos = ctab_.transparency(row).x_;
	const float olddataval = colorpos*(max-min)+min;
	table_->setText( rc, toUiString(olddataval,0,'g',5) );
	return;
    }
    else if ( newdataval<prevval || newdataval>nextval )
    {
	const uiString msg = tr("Entered Data Value '%1' is Invalid for "
				"Point %2: "
				"Please enter a value between %3 and %4.")
				 .arg( newdataval )
				 .arg( row+1 )
				 .arg( prevval )
				 .arg( nextval );
	uiMSG().error( msg );
	const float colorpos = ctab_.transparency(row).x_;
	const float olddataval = colorpos*(max-min)+min;
	table_->setText( rc, toUiString(olddataval,0,'g',5) );
	return;
    }

    table_->setText( rc, toUiString(newdataval,0,'g',5) );

    const RowCol colrc = RowCol( row, sPosCol );
    const float newcolorpos = (newdataval-min)/(max-min);
    table_->setText( colrc, toUiStringDec(newcolorpos,2) );

    const float transpval = ctab_.transparency( row ).y_;
    const Geom::Point2D<float> newpoint = { newcolorpos, transpval };
    ctab_.changeTransparency( row, newpoint );
    valuesChanged.trigger();
}


void uiTranspValuesDlgPlus::handleTranspVal()
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->currentCell();
    const int row = rc.row();

    const float newtranspval = table_->getFValue( rc );

    if ( newtranspval>100 || newtranspval<0 )
    {
	uiMSG().error( tr("Entered Transparency Value '%1' is out of range.\n"
			  "Please enter a percentage between 0 and 100%.")
			    .arg( newtranspval ) );
	const float transperc = ctab_.transparency(row).y_/255*100;
	table_->setText( rc, toUiStringDec(transperc,0) );
	return;
    }

    table_->setText( rc, toUiStringDec(newtranspval,0) );

    const BufferString lb = table_->rowLabel(0);
    if ( ctab_.nrSegments()>1 && lb.contains("Segment") )
    {
	int segidx = row;
	segidx = row*2+1;

	const float newcolpos = ctab_.transparency( segidx ).x_;
	const float convtranspval = newtranspval/100*255;
	const Geom::Point2D<float> newval = { newcolpos, convtranspval };
	ctab_.changeTransparency( segidx, newval );

	const float newcolpos2 = ctab_.transparency( segidx+1 ).x_;
	const Geom::Point2D<float> newval2 = { newcolpos2, convtranspval };
	ctab_.changeTransparency( segidx+1, newval2 );
    }
    else
    {
	const float newcolpos = ctab_.transparency( row ).x_;
	const float convtranspval = newtranspval/100*255;
	const Geom::Point2D<float> newval = { newcolpos, convtranspval };
	ctab_.changeTransparency( row, newval );
    }

    valuesChanged.trigger();
}


void uiTranspValuesDlgPlus::pointInsertedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    RowCol rcpos = table_->newCell();
    const int row = rcpos.row();

    if ( ctab_.nrSegments() > 1 )
    {
	if ( row-1<0 || row>=ctab_.nrSegments() )
	{
	    table_->removeRow( rcpos );
	    uiMSG().error( tr("Cannot insert new segments before or after "
			     "extreme positions.") );
	    return;
	}

	if ( table_->nrRows() > 25 )
	{
	    table_->removeRow( rcpos );
	    uiMSG().error( tr("Cannot have more than 25 segments") );
	    return;
	}

	ctab_.setNrSegments( ctab_.nrSegments()+1 );
	segmentInserted().trigger();
	table_->clearTable();
	setPtsToAnchSeg( true );
	valuesChanged.trigger();
    }
    else
    {
	if ( row-1<0 || row>=ctab_.transparencySize() )
	{
	    table_->removeRow( rcpos );
	    uiMSG().error( tr("Cannot insert new points before or after "
			      "extreme positions.") );
	    return;
	}
    }

    const float x1 = ctab_.transparency(row-1).x_;
    const float x2 = ctab_.transparency(row).x_;
    const float y1 = ctab_.transparency(row-1).y_;
    const float y2 = ctab_.transparency(row).y_;

    const float epsilonx = 1e-2;
    if ( x2-x1<epsilonx )
    {
	table_->removeRow( rcpos );
	uiMSG().error( tr("Cannot insert a new point here since the point will "
			  "exceed the proximity threshold between "
			  "the previous and next points.") );
	return;
    }

    const float newposval = (x1+x2)/2;
    const float newtransval = (y1+y2)/2;
    const Geom::Point2D<float> newval = { newposval, newtransval };
    table_->setCurrentCell( RowCol(row,sPosCol) );
    ctab_.setTransparency( newval );
    fillTableWithPts();
    valuesChanged.trigger();
}


void uiTranspValuesDlgPlus::pointDeletedCB( CallBacker* )
{
    NotifyStopper notifstop( table_->valueChanged );
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    if ( ctab_.nrSegments() > 1 )
    {
	if ( table_->nrRows() < 2 )
	{
	    table_->insertRows( rc, 1 );
	    setPtsToAnchSeg( true );
	    uiMSG().error( tr("Cannot have fewer than 2 segments") );
	    return;
	}

	ctab_.setNrSegments( ctab_.nrSegments()-1 );
	segmentRemoved().trigger();
	table_->clearTable();
	setPtsToAnchSeg( true );
	valuesChanged.trigger();
    }
    else
    {
	if ( row==0 || row==ctab_.transparencySize()-1 )
	{
	    table_->insertRows( rc, 1 );
	    fillTableWithPts();
	    uiMSG().error( tr("Cannot remove remove points "
			      "from extreme positions.") );
	    return;
	}

	ctab_.removeTransparencyAt( row );
	fillTableWithPts();
	valuesChanged.trigger();
    }
}


void uiTranspValuesDlgPlus::resetTranspPtsCB( CallBacker* )
{
    NotifyStopper ns( table_->valueChanged );
    while ( ctab_.transparencySize() > 2 )
	ctab_.removeTransparencyAt(1);

    const int tabsize = ctab_.transparencySize();
    int trows = table_->nrRows();
    if ( trows < tabsize )
    {
	table_->insertRows(trows, tabsize-trows);
    }
    else if ( trows > tabsize )
    {
	while ( trows >= tabsize )
	    table_->removeRow(trows--);
    }

    table_->clearTable();
    fillTableWithPts();
    valuesChanged.trigger();

    if ( ctab_.nrSegments() > 1 )
    {
	table_->setup().insertrowallowed(true).removerowallowed(true);
	syncanchors_->setText(
				tr("Convert Transparency Points to Segments") );
    }
}


static HiddenParam<uiColorTableMan,Notifier<uiColorTableMan>*>
    hp_rangechanged(nullptr);

Notifier<uiColorTableMan>& uiColorTableMan::rangeChanged()
{
    return *hp_rangechanged.getParam( this );
}


uiColorTableMan::uiColorTableMan( uiParent* p, ColTab::Sequence& ctab,
       				  bool enabletrans )
    : uiDialog(p,Setup(uiStrings::phrManage(uiStrings::sColorTable(mPlural)),
		       tr("Add, Remove, and Edit Color Tables"),
		       mODHelpKey(mColorTableManHelpID)).nrstatusflds(2))
    , tableAddRem(this)
    , tableChanged(this)
    , ctab_(ctab)
    , enabletrans_(enabletrans)
{
    hp_ctabrange.setParam( this,
			   new Interval<float>(Interval<float>::udf()) );
    hp_rangechanged.setParam(this, new Notifier<uiColorTableMan>(this));
    setShrinkAllowed( false );

    auto* leftgrp = new uiGroup( this, "Left" );

    coltablistfld_ = new uiTreeView( leftgrp, "Color Table List" );
    BufferStringSet labels;
    labels.add( "Color table" ).add( "Source" );
    coltablistfld_->addColumns( labels );
    coltablistfld_->setRootDecorated( false );
    coltablistfld_->setStretch( 2, 2 );
    coltablistfld_->setSelectionBehavior( uiTreeView::SelectRows );
    mAttachCB( coltablistfld_->selectionChanged, uiColorTableMan::selChg );

    const char* segtypes[] = { "None", "Equal", "Variable", 0 };
    segmentfld_ = new uiGenInput( leftgrp, tr("Segmentation"),
				 StringListInpSpec(segtypes) );
    mAttachCB( segmentfld_->valueChanged, uiColorTableMan::segmentSel );
    segmentfld_->attach( leftAlignedBelow, coltablistfld_, 5 );

    nrsegbox_ = new uiSpinBox( leftgrp, 0, 0 );
    nrsegbox_->setInterval( 2, 25 );
    nrsegbox_->setValue( 8 );
    nrsegbox_->display( false );
    mAttachCB( nrsegbox_->valueChanging, uiColorTableMan::nrSegmentsCB );
    nrsegbox_->attach( rightOf, segmentfld_ );

    uiColorInput::Setup cisetup( ctab_.undefColor(), enabletrans_ ?
		    uiColorInput::Setup::Separate : uiColorInput::Setup::None );
    cisetup.lbltxt( tr("Undefined color") );
    undefcolfld_ = new uiColorInput( leftgrp, cisetup );
    mAttachCB( undefcolfld_->colorChanged, uiColorTableMan::undefColSel );
    undefcolfld_->attach( leftAlignedBelow, segmentfld_ );

    auto* lbl = new uiLabel( leftgrp, tr("Range") );
    lbl->attach( leftAlignedBelow, undefcolfld_ );

    hp_minfld.setParam( this, new uiLineEdit(leftgrp,"Min") );
    auto* minfld = hp_minfld.getParam(this);
    mAttachCB( minfld->returnPressed, uiColorTableMan::rangeChangedCB );
    minfld->attach( rightTo, lbl );

    hp_maxfld.setParam( this, new uiLineEdit(leftgrp,"Max") );
    auto* maxfld = hp_maxfld.getParam(this);
    mAttachCB( maxfld->returnPressed, uiColorTableMan::rangeChangedCB );
    maxfld->attach( rightTo, minfld );

    uiColorInput::Setup ctsu( ctab_.markColor(), uiColorInput::Setup::None );
    ctsu.withdesc( false );
    markercolfld_ = new uiColorInput( leftgrp,
				      ctsu.lbltxt(tr("Marker color")) );
    mAttachCB( markercolfld_->colorChanged, uiColorTableMan::markerColChgd );

    markercolfld_->display( false );

    auto* rightgrp = new uiGroup( this, "Right" );

    ctabcanvas_ =
	new uiColorTableCanvas( rightgrp, ctab_, false, OD::Horizontal );
    mAttachCB( ctabcanvas_->getMouseEventHandler().buttonPressed,
	      uiColorTableMan::rightClick );
    mAttachCB( ctabcanvas_->reSize, uiColorTableMan::reDrawCB );

    w2uictabcanvas_ = new uiWorld2Ui( uiWorldRect(0,0,0,255),
				     uiSize(mTransWidth/5, mTransWidth) );

    ctabcanvas_->setPrefWidth( mTransWidth/25 );
    ctabcanvas_->setPrefHeight( mTransWidth );
    ctabcanvas_->setStretch( 0, 2 );
    ctabcanvas_->setOrientation( OD::Vertical );
    mAttachCB( ctabcanvas_->getMouseEventHandler().doubleClick,
	      uiColorTableMan::markerDialogCB );

    markercanvas_ = new uiColTabMarkerCanvas( rightgrp, ctab_ );
    markercanvas_->setPrefWidth( 60 );
    markercanvas_->setPrefHeight( mTransWidth );
    markercanvas_->setStretch( 0, 2 );
    markercanvas_->attach( rightOf, ctabcanvas_ );

    uiFunctionDisplay::Setup su;
    int nrsegs = ctab_.nrSegments();

    su.border(uiBorder(2,5,3,5)).xrg(Interval<float>(0,1)).editable(true)
      .yrg(Interval<float>(0,255)).canvaswidth(mTransWidth).closepolygon(true)
      .canvasheight(mTransHeight).drawscattery1(true)
      .ylinestyle(OD::LineStyle(OD::LineStyle::Solid,2,OD::Color(255,0,0)))
      .y2linestyle(OD::LineStyle(OD::LineStyle::Solid,2,OD::Color(190,190,190)))
      .drawliney2(true).fillbelowy2(true)
      .pointsz(3).ptsnaptol(nrsegs<10 && nrsegs>1 ? 0.5f/nrsegs : 0.08)
      .noxaxis(true).noxgridline(true)
      .noyaxis(true).noygridline(true)
      .noy2axis(true).noy2gridline(true)
      .drawborder(false).xannotinint(true);

    cttranscanvas_ = new uiFunctionDisplay( rightgrp, su, OD::Vertical );
    cttranscanvas_->setStretch( 2, 2 );
    cttranscanvas_->setTitleColor( OD::Color::Red() );
    cttranscanvas_->attach( rightOf, markercanvas_ );
    cttranscanvas_->setTitleAlignment(Alignment(Alignment::Left,
						Alignment::Top));

    if ( enabletrans_ )
    {
	mAttachCB( cttranscanvas_->pointChanged, uiColorTableMan::transptChg );
	mAttachCB( cttranscanvas_->pointSelected, uiColorTableMan::transptSel );
	mAttachCB( cttranscanvas_->mouseMove,
		   uiColorTableMan::mouseMoveCB );
	mAttachCB( cttranscanvas_->getMouseEventHandler().buttonReleased,
		   uiColorTableMan::rightClickTranspCB );
    }

    auto* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );

    auto* butgrp = new uiButtonGroup( this, "actions", OD::Horizontal );
    butgrp->attach( alignedBelow, splitter );

    auto* renamebut = new uiToolButton( butgrp, "renameobj",
					uiStrings::sRename() );
    mAttachCB( renamebut->activated, uiColorTableMan::renameColTab );

    importbut_ = new uiToolButton( butgrp, "import", uiStrings::sImport() );
    mAttachCB( importbut_->activated, uiColorTableMan::importColTab );

    exportbut_ = new uiToolButton( butgrp, "export", uiStrings::sExport() );
    mAttachCB( exportbut_->activated, uiColorTableMan::exportColTab );

    removebut_ = new uiToolButton( butgrp, "delete", uiStrings::sDelete() );
    mAttachCB( removebut_->activated, uiColorTableMan::removeCB );

    auto* saveasbut = new uiToolButton( this, "saveas", uiStrings::sSaveAs() );
    mAttachCB( saveasbut->activated, uiColorTableMan::saveAsCB );
    saveasbut->attach( rightBorder, 0 );

    auto* savebut = new uiToolButton( this, "save", uiStrings::sSave() );
    mAttachCB( savebut->activated, uiColorTableMan::saveCB );
    savebut->attach( leftOf, saveasbut );

    auto* flipbut = new uiToolButton( this, "revpol", tr("Flip") );
    mAttachCB( flipbut->activated, uiColorTableMan::flipCB );
    flipbut->attach( leftOf, savebut );
    flipbut->attach( rightTo, butgrp );

    mAttachCB( markercanvas_->markerChanged, uiColorTableMan::markerChange );
    mAttachCB( ctab_.colorChanged, uiColorTableMan::sequenceChange );
    mAttachCB( ctab_.transparencyChanged, uiColorTableMan::sequenceChange );
    mAttachCB( postFinalize(), uiColorTableMan::doFinalize );
}


uiColorTableMan::~uiColorTableMan()
{
    detachAllNotifiers();
    hp_ctabrange.removeAndDeleteParam( this );
    hp_maxfld.removeParam( this );
    hp_minfld.removeParam( this );

    delete orgctab_;
    delete w2uictabcanvas_;
}


uiString uiColorTableMan::sKeyDefault()
{ return tr("System"); }
uiString uiColorTableMan::sKeyEdited()
{ return tr("Edited"); }
uiString uiColorTableMan::sKeyOwn()
{ return tr("Own"); }

void uiColorTableMan::doFinalize( CallBacker* )
{
    refreshColTabList( ctab_.name() );
    sequenceChange( 0 );
    toStatusBar( uiString::emptyString(), 1 );

    markercanvas_->reDrawNeeded.trigger();
}


void uiColorTableMan::refreshColTabList( const char* selctnm )
{
    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    coltablistfld_->clear();

    BufferString defaultct;
    Settings::common().get( "dTect.Color table.Name", defaultct );
    for ( int idx=0; idx<allctnms.size(); idx++ )
    {
	const int seqidx = ColTab::SM().indexOf( allctnms.get(idx) );
	if ( seqidx<0 ) continue;
	const ColTab::Sequence* seq = ColTab::SM().get( seqidx );

	uiString status;
	if ( seq->type() == ColTab::Sequence::System )
	    status = sKeyDefault();
	else if ( seq->type() == ColTab::Sequence::Edited )
	    status = sKeyEdited();
	else
	    status = sKeyOwn();

	auto* itm = new uiTreeViewItem( coltablistfld_,
		uiTreeViewItem::Setup().label(toUiString(seq->name()))
		.label(status) );
	if ( defaultct == seq->name() )
	    itm->setBold( -1, true );
    }

    coltablistfld_->setColumnWidthMode( uiTreeView::ResizeToContents );

    uiTreeViewItem* itm = coltablistfld_->findItem( selctnm, 0, true );
    if ( !itm ) return;

    coltablistfld_->setCurrentItem( itm );
    coltablistfld_->setSelected( itm, true );
    coltablistfld_->ensureItemVisible( itm );

    markercanvas_->reDrawNeeded.trigger();
}


void uiColorTableMan::updateTransparencyGraph()
{
    TypeSet<float> xvals;
    TypeSet<float> yvals;
    for ( int idx=0; idx<ctab_.transparencySize(); idx++ )
    {
	const Geom::Point2D<float> transp = ctab_.transparency( idx );
	if ( !transp.isDefined() )
	    continue;

	if ( !xvals.isEmpty() && mIsEqual(transp.x_,xvals.last(),1e-2) )
	    xvals += xvals.last();
	else
	    xvals += transp.x_;

        yvals += transp.y_;
    }

    cttranscanvas_->setVals( xvals.arr(), yvals.arr(), xvals.size() );
}


void uiColorTableMan::selChg( CallBacker* )
{
    const uiTreeViewItem* itm = coltablistfld_->selectedItem();
    if ( !itm || !ColTab::SM().get(itm->text(0),ctab_) )
	return;

    selstatus_ = itm->text( 1 );
    removebut_->setSensitive( selstatus_ != sKeyDefault().getFullString() );
    removebut_->setText( selstatus_==sKeyEdited().getFullString()
				? tr("Revert") : uiStrings::sRemove() );

    undefcolfld_->setColor( ctab_.undefColor() );
    markercolfld_->setColor( ctab_.markColor() );

    if ( enabletrans_ )
	updateTransparencyGraph();

    delete orgctab_;
    orgctab_ = new ColTab::Sequence;
    *orgctab_ = ctab_;
    issaved_ = true;
    updateSegmentFields();
    tableChanged.trigger();

    markercanvas_->reDrawNeeded.trigger();

    markercolfld_->display( false );
}


void uiColorTableMan::removeCB( CallBacker* )
{
    if ( selstatus_ == sKeyDefault().getFullString() )
    {
	uiMSG().error(
		tr("This is a default colortable and connot be deleted") );
	return;
    }

    const char* ctnm = ctab_.name();
    uiString msg(tr("%1 '%2' will be deleted.\n%3\nDo you wish to continue?")
	     .arg(selstatus_ == sKeyEdited().getFullString() ?
	     uiStrings::phrJoinStrings(sKeyEdited(),uiStrings::sColorTable()):
	     uiStrings::phrJoinStrings(tr("Own made"),uiStrings::sColorTable()))
	     .arg(ctnm).arg(selstatus_ == sKeyEdited().getFullString()
					 ? tr("and replaced by the default.\n")
					 : uiString::emptyString()));
    if ( !uiMSG().askDelete(msg) )
	return;

    BufferStringSet allctnms;
    ColTab::SM().getSequenceNames( allctnms );
    allctnms.sort();
    int newctabidx = allctnms.indexOf( ctnm );
    const BufferString selctnm = allctnms.get( newctabidx );
    const int toberemovedid = ColTab::SM().indexOf( ctnm );
    ColTab::SM().remove( toberemovedid );
    ColTab::SM().write();
    ColTab::SM().refresh();

    refreshColTabList( selctnm.buf() );
    selChg(0);
    tableAddRem.trigger();
}


void uiColorTableMan::flipCB( CallBacker* )
{
    ctab_.flipColor();
    ctab_.flipTransparency();
    tableChanged.trigger();

    ctabcanvas_->setRGB();
    updateTransparencyGraph();

    markercanvas_->setRange( *hp_ctabrange.getParam(this) );
    markercanvas_->reDrawNeeded.trigger();
}


void uiColorTableMan::saveCB( CallBacker* )
{
    if ( saveColTab(false) )
	tableAddRem.trigger();
}


void uiColorTableMan::saveAsCB( CallBacker* )
{
    if ( saveColTab( true ) )
	tableAddRem.trigger();
}


bool uiColorTableMan::saveColTab( bool saveas )
{
    BufferString newname = ctab_.name();
    if ( saveas )
    {
	uiGenInputDlg dlg( this,
			   uiStrings::phrJoinStrings(uiStrings::sColorTable(),
						     uiStrings::sName()),
			   uiStrings::sName(), new StringInpSpec(newname) );
	if ( !dlg.go() )
	    return false;

	newname = dlg.text();
    }

    ColTab::Sequence newctab;
    newctab = ctab_;
    newctab.setName( newname );

    const int newidx = ColTab::SM().indexOf( newname );

    uiString msg;
    if ( newidx<0 )
	newctab.setType( ColTab::Sequence::User );
    else
    {
	ColTab::Sequence::Type tp = ColTab::SM().get(newidx)->type();
	if ( tp == ColTab::Sequence::System )
	{
	    msg = tr("The default colortable will be replaced.\n"
		     "Do you wish to continue?\n"
		     "(Default colortable can be recovered by "
		     "removing the edited one)");
	    newctab.setType( ColTab::Sequence::Edited );
	}
	else if ( tp == ColTab::Sequence::User )
	{
	    msg = tr("Your own made colortable will be replaced\n"
		     "Do you wish to continue?");
	}
	else if ( tp == ColTab::Sequence::Edited )
	{
	    msg = tr("The Edited colortable will be replaced\n"
		     "Do you wish to continue?");
	}
    }

    if ( !msg.isEmpty() && !uiMSG().askContinue( msg ) )
	return false;

    newctab.setUndefColor( undefcolfld_->color() );
    newctab.setMarkColor( markercolfld_->color() );

    ColTab::SM().set( newctab );
    ColTab::SM().write();

    refreshColTabList( newctab.name() );
    selChg(0);
    return true;
}


bool uiColorTableMan::acceptOK( CallBacker* )
{
    ctab_.setUndefColor( undefcolfld_->color() );
    ctab_.setMarkColor( markercolfld_->color() );

    if ( !orgctab_ )
    {
	issaved_ = false;
	saveColTab( true );
    }
    else
    {
	if ( !(ctab_==*orgctab_) )
	    issaved_ = false;

	if ( !issaved_ )
	    saveColTab( false );
    }

    return issaved_;
}


bool uiColorTableMan::rejectOK( CallBacker* )
{
    if ( orgctab_ )
	ctab_ = *orgctab_;

    tableChanged.trigger();
    return true;
}


void uiColorTableMan::undefColSel( CallBacker* )
{
    ctab_.setUndefColor( undefcolfld_->color() );
    ctabcanvas_->setRGB();
    tableChanged.trigger();
}


void uiColorTableMan::rangeChangedCB( CallBacker* )
{
    auto* minfld = hp_minfld.getParam(this);
    auto* maxfld = hp_maxfld.getParam(this);
    auto* ctabrg = hp_ctabrange.getParam(this);
    float mininp = minfld->getFValue();
    float maxinp = hp_maxfld.getParam(this)->getFValue();

    if ( mininp==mUdf(float) || maxinp==mUdf(float) )
    {
	minfld->setValue(ctabrg->start_);
	maxfld->setValue(ctabrg->stop_);
	uiMSG().error(tr("Invalid Input entered, check that it is a number"));
	return;
    }

    if ( mininp >= ctabrg->stop_ )
    {
	minfld->setValue( ctabrg->start_ );
	maxfld->setValue( ctabrg->stop_ );
	uiMSG().error(tr("Invalid Input: Cannot have min range more than "
			 "max range."));
	return;
    }

    if ( maxinp <= ctabrg->start_ )
    {
	minfld->setValue( ctabrg->start_ );
	maxfld->setValue( ctabrg->stop_ );
	uiMSG().error(tr("Invalid Input: Cannot have max range less than "
			 "min range."));
	return;
    }

    ctabrg->start_ = mininp;

    ctabrg->stop_ = maxinp;

    markercanvas_->setRange(*ctabrg);
    rangeChanged().trigger();
    markercanvas_->reDrawNeeded.trigger();
}


void uiColorTableMan::markerColChgd( CallBacker* )
{
    ctab_.setMarkColor( markercolfld_->color() );
    tableChanged.trigger();
}


void uiColorTableMan::setHistogram( const TypeSet<float>& hist )
{
    setHistogram( hist, Interval<float>::udf() );
}


void uiColorTableMan::setHistogram( const TypeSet<float>& hist,
				    const Interval<float>& minmax )
{
    *hp_ctabrange.getParam(this) = minmax;
    TypeSet<float>& myhist = const_cast<TypeSet<float>&>(hist);

    TypeSet<float> x2vals;
    const float step = (float)1/(float)myhist.size();
    for ( int idx=0; idx<myhist.size(); idx++ )
	x2vals += idx*step;

    TypeSet<float> y2vals;
    for ( int idx=myhist.size()-1; idx>=0; idx-- )
    {
	y2vals += myhist.get(idx);
    }

    cttranscanvas_->setY2Vals( x2vals.arr(), y2vals.arr(), myhist.size() );
    markercanvas_->setRange(minmax);
    markercanvas_->reDrawNeeded.trigger();

    const auto* rg = hp_ctabrange.getParam(this);
    const bool validrg = rg && !rg->isUdf();
    if ( validrg )
    {
	hp_minfld.getParam(this)->setValue( rg->start_ );
	hp_maxfld.getParam(this)->setValue( rg->stop_ );
    }

    hp_minfld.getParam(this)->setSensitive( validrg );
    hp_maxfld.getParam(this)->setSensitive( validrg );
}


Interval<float>* uiColorTableMan::getRange()
{
    return hp_ctabrange.getParam(this);
}


void uiColorTableMan::updateSegmentFields()
{
    NotifyStopper ns( nrsegbox_->valueChanging );
    int val = 0;
    if ( ctab_.isSegmentized() )
	val = ctab_.nrSegments()>0 ? 1 : 2;

    segmentfld_->setValue( val );
    nrsegbox_->display( val==1 );
    nrsegbox_->setValue( val==1 ? ctab_.nrSegments() : 8 );

    markercanvas_->reDrawNeeded.trigger();
}


void uiColorTableMan::segmentSel( CallBacker* )
{
    const int segval = segmentfld_->getIntValue();
    const bool segmented = segval==1;

    nrsegbox_->display( segmented );
    markercolfld_->display ( false );
    doSegmentize();
    if ( segmented )
    {
	setPtsToAnchSegsCB(nullptr);
	const float nrsegs = ctab_.nrSegments();
	cttranscanvas_->setup().ptsnaptol( nrsegs<10 && nrsegs>1 ? 0.7f/nrsegs
								    : 0.08 );
    }
    else
    {
	cttranscanvas_->setup().ptsnaptol( 0.08 );
    }
}


void uiColorTableMan::insertSegmentCB( CallBacker * )
{
    nrsegbox_->setValue(nrsegbox_->getIntValue()+1);
    nrSegmentsCB( nullptr );
}


void uiColorTableMan::removeSegmentCB( CallBacker * )
{
    nrsegbox_->setValue(nrsegbox_->getIntValue()-1);
    nrSegmentsCB( nullptr );
}


void uiColorTableMan::nrSegmentsCB( CallBacker* )
{
    NotifyStopper( ctab_.colorChanged );
    doSegmentize();
    setPtsToAnchSegsCB(nullptr);
}

#define mAddColor(orgidx,newpos) { \
    const Color col = indextbl.colorForIndex( orgidx ); \
    ctab_.setColor( newpos, col.r(), col.g(), col.b() ); }

#define mEps 0.00001

void uiColorTableMan::doSegmentize()
{
    NotifyStopper ns( ctab_.colorChanged );
    const int segval = segmentfld_->getIntValue();
    const bool issegmented = (segval==1 || segval==3);
    if ( segval==0 )
	ctab_.setNrSegments( 0 );
    else if ( issegmented )
    {
	const int nrseg = nrsegbox_->getIntValue();
	if ( mIsUdf(nrseg) || nrseg < 2 )
	    return;

	ctab_.setNrSegments( nrseg );
    }
    else if ( issegmented )
    {
	const int nrseg = nrsegbox_->getIntValue();
	if ( mIsUdf(nrseg) || nrseg < 2 )
	    return;

	ctab_.setNrSegments( nrseg );
    }
    else
	ctab_.setNrSegments( -1 );

    markercanvas_->reDrawNeeded.trigger();

    ctabcanvas_->setRGB();
    tableChanged.trigger();
    const int nrsegsel = nrsegbox_->getIntValue();
    const bool compatible = ctab_.transparencySize()==nrsegsel*2+2;
}


void uiColorTableMan::setPtsToAnchSegsCB(CallBacker*)
{
    const int nrsegs = ctab_.nrSegments();
    const float nrseg = nrsegs;
    const float segsz = 1.0f/nrseg;
    TypeSet<float> tvals;
    if ( nrsegs<26 && nrsegs>1 )
    {
	const float quarter = segsz/4;
	for ( float i=segsz/2; i<1; i+=segsz )
	{
	    float high = 0;
	    float low = 255;
	    for ( float j=(i-quarter); j<(i+quarter); j+=0.002 )
	    {
		const float transp = ctab_.transparencyAt(j);
		if ( transp > high )
		    high = transp;

		if ( transp < low )
		    low = transp;
	    }

	    const float avg = (high+low)/2;
	    tvals.add( avg );
	}
    }

    TypeSet<float> transpvals;
    for ( int cidx=0; cidx<ctab_.size(); cidx++)
    {
	const float position = ctab_.position( cidx );
	const float transp = ctab_.transparencyAt(position);
	transpvals.add( transp );
    }

    while ( ctab_.transparencySize() > 2 )
	ctab_.removeTransparencyAt( 1 );

    if ( nrsegs==0 || nrsegs==-1 )
    {
	for ( int cidx=0; cidx<ctab_.size(); cidx++ )
	{
	    const float position = ctab_.position( cidx );
	    const float transp = transpvals.get(cidx);
	    const Geom::Point2D<float> newval = { position, transp };
	    ctab_.setTransparency( newval );
	}
    }
    else if ( nrsegs < 26 )
    {
	ctab_.setTransparency( {0,0} );
	ctab_.setTransparency( {.001,tvals.first()} );

	int rgidx = 0;

	for ( float cidx=segsz; cidx<1; cidx+=segsz )
	{
	    const Geom::Point2D<float> segval = { cidx-.001f,
						  tvals.get(rgidx++)};
	    ctab_.setTransparency( segval );

	    if ( rgidx > tvals.size()-1 )
		rgidx = tvals.size()-1;

	    if ( ctab_.transparencySize() < nrsegs*2+2 )
	    {
		const Geom::Point2D<float> segval2 = { cidx,
						       tvals.get(rgidx) };
		ctab_.setTransparency( segval2 );
	    }
	}

	if ( ctab_.transparencySize() < nrsegs*2+2 )
	{
	    ctab_.setTransparency( {.999,tvals.last()} );
	    ctab_.setTransparency( {1,0} );
	}
    }
    else
	uiMSG().error(tr("Number of Segments to replace the Points exceeds "
			 "the max threshold."));

    ctabcanvas_->setRGB();
    transpTableChgd( nullptr );
    //TODO: Figure out the optimal ratio for this
    cttranscanvas_->setup().ptsnaptol( nrsegs<10 && nrsegs>1 ? 0.7f/nrsegs
							     : 0.08 );
}


void uiColorTableMan::rightClick( CallBacker* )
{
    if ( !segmentfld_->isChecked() )
	return;
    if ( ctabcanvas_->getMouseEventHandler().isHandled() )
	return;

    const MouseEvent& ev =
	ctabcanvas_->getMouseEventHandler().event();
    uiWorldPoint wpt = w2uictabcanvas_->transform( ev.pos() );

    selidx_ = -1;
    for ( int idx=0; idx<ctab_.size(); idx++ )
    {
        if ( ctab_.position(idx) > wpt.x_ )
	{
	    selidx_ = idx;
	    break;
	}
    }

    if ( selidx_<0 ) return;
    OD::Color col = ctab_.color( (float) wpt.x_ );
    if ( selectColor(col,this,tr("Color selection"),false) )
    {
	ctab_.changeColor( selidx_-1, col.r(), col.g(), col.b() );
	ctab_.changeColor( selidx_, col.r(), col.g(), col.b() );
	tableChanged.trigger();
    }

    ctabcanvas_->getMouseEventHandler().setHandled( true );
}


void uiColorTableMan::markerDialogCB(CallBacker*)
{

    if ( ctab_.hasEqualSegments() )
    {
	return;
    }

    ColTab::Sequence coltab = ctab_;
    uiColTabMarkerDlg dlg( parent_, ctab_ );
    dlg.markersChanged.notify( mCB(this,uiColorTableMan,markerChange) );
    if ( !dlg.go() )
    {
	ctab_ = coltab;
	markerChange( nullptr );
    }//TODO: after the dlg is closed, the background stops being gray
}


void uiColorTableMan::rightClickTranspCB(CallBacker*)
{
    const MouseEvent& ev = cttranscanvas_->getMouseEventHandler().event();
    if ( OD::RightButton != ev.buttonState() )
	return;

    uiMenu mnu( parent_, uiStrings::sAction() );
    mnu.insertAction( new uiAction(uiStrings::phrJoinStrings(uiStrings::sView(),
					      uiStrings::sValue(mPlural))), 0 );

    const int res = mnu.exec();

    if ( res == 0 )
    {
	ColTab::Sequence coltab = ctab_;
	uiTranspValuesDlgPlus dlg( parent_, ctab_,
				   *hp_ctabrange.getParam(this) );
	dlg.valuesChanged.notify( mCB(this,uiColorTableMan,transpTableChgd) );

	if ( ctab_.nrSegments() > 1 )
	{
	    dlg.segmentInserted()
	       .notify( mCB(this,uiColorTableMan,insertSegmentCB) );
	    dlg.segmentRemoved()
	       .notify( mCB(this,uiColorTableMan,removeSegmentCB) );
	}

	if ( !dlg.go() && !(ctab_==coltab) )
	{
	    const uiString msg = tr("Save or Discard changes made to the "
				    "Color Positions?");
	    const bool save = uiMSG().askGoOn( msg, uiStrings::sSaveChanges(),
						    uiStrings::sDiscard());
	    if ( !save )
	    {
		ctab_ = coltab;
		transpTableChgd( nullptr );
		nrsegbox_->setValue( ctab_.nrSegments() );
	    }
	}
    }

    doSegmentize();
}


void uiColorTableMan::mouseMoveCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const Geom::PointF&,mousepos,cb);
    const Geom::PointF pos = cttranscanvas_->mapToValue( mousepos, false );
    const Geom::PointF pos2 = cttranscanvas_->mapToValue( mousepos, true );
    float xpos = pos.x_;
    float ypos = pos.y_;

    const bool inyrange = cttranscanvas_->yAxis(false)->range()
							.includes(ypos,true);
    const bool inxrange = cttranscanvas_->xAxis()->range().includes(xpos,true);

    float transperc = 0;
    const float ymax = cttranscanvas_->yAxis(false)->range().stop_;
    if ( inyrange )
    {
	transperc = float(ypos/ymax);
    }
    else
    {
	const float ymin = cttranscanvas_->yAxis(false)->range().start_;
	transperc = std::clamp( ypos, ymin, ymax );
    }
    transperc = std::clamp( transperc, 0.0f, 1.0f )*100;

    const float xval = std::clamp( xpos, 0.0f, 1.0f );

    float dataval = mUdf(float);
    uiString datavalstr = tr("Value: ");
    const bool udfrange = hp_ctabrange.getParam(this)->isUdf();
    if ( !udfrange )
    {
	const float min = hp_ctabrange.getParam(this)->start_;
	const float max = hp_ctabrange.getParam(this)->stop_;
	dataval = xval*(max-min)+min;
	datavalstr.append( toUiString( dataval, 0, 'g', 5 ) )
		  .append("  ").append("/").append("  ");
    }
    else
    {
	datavalstr = uiStrings::sEmptyString();
    }

    const uiString colorposval = toUiString( xval, 0, 'f', 2 );
    const uiString valperc = toUiString( xval*100, 0, 'f', 0 );
    const uiString transpvalstr = toUiString( transperc, 0, 'f', 0 );
    const uiString colorposstr = toUiString("Pos: %1 (0-1)  /  ")
				    .arg(colorposval);
    const uiString valpercstr = toUiString("% Value: %1%  /  ")
				    .arg(valperc);

    const uiString posstr = toUiString("%1%2Transp: %3%")
				.arg( udfrange ? colorposstr
					       : uiStrings::sEmptyString() )
				.arg( datavalstr )
				.arg( transpvalstr );
    cttranscanvas_->setTitle( posstr );
    const MouseEvent& ev = cttranscanvas_->getMouseEventHandler().event();
    cttranscanvas_->setTitleAlignment(Alignment(Alignment::Left,
						Alignment::Top));
    if ( ( !inyrange || !inxrange ) && OD::LeftButton!=ev.buttonState() )
	cttranscanvas_->setTitle( uiStrings::sEmptyString() );
}


void uiColorTableMan::markerChange( CallBacker* )
{
    updateSegmentFields();
    ctabcanvas_->setRGB();
    sequenceChange( 0 );
    tableChanged.trigger();
}


void uiColorTableMan::transpTableChgd( CallBacker* cb )
{
    sequenceChange( nullptr );
    updateTransparencyGraph();
    tableChanged.trigger();
}


void uiColorTableMan::reDrawCB( CallBacker* )
{
    ctabcanvas_->setRGB();
    markercanvas_->reDrawNeeded.trigger();
    w2uictabcanvas_->set( uiWorldRect(0,0,0,255),
			  uiSize(mTransWidth/5, mTransWidth) );
}


void uiColorTableMan::sequenceChange( CallBacker* )
{
    ctabcanvas_->setRGB();
}


void uiColorTableMan::transptChg( CallBacker* )
{
    if ( ctab_.nrSegments() > 1 )
    {
	cttranscanvas_->allowAddingPoints(false);
    }
    else
	cttranscanvas_->allowAddingPoints(true);

    const int ptidx = cttranscanvas_->selPt();
    const int nrpts = cttranscanvas_->xVals().size();
    const bool equalseg = ctab_.nrSegments() > 1;
    const bool compatible = ctab_.transparencySize()==ctab_.nrSegments()*2+2;

    if ( ptidx < 0 && equalseg )
    {
	return;
    }
    else if ( ptidx < 0 )
    {
	ctab_.removeTransparencies();
	if ( cttranscanvas_->xVals().isEmpty() )
	    return;

	for ( int idx=0; idx<nrpts; idx++ )
	{
	    Geom::Point2D<float> pt( cttranscanvas_->xVals()[idx],
				     cttranscanvas_->yVals()[idx] );
	    if ( !pt.isDefined() )
		continue;

            if ( idx==0 && pt.x_>0 )
                pt.x_ = 0;
            else if ( idx==nrpts-1 && pt.x_<1 )
                pt.x_ = 1;

	    ctab_.setTransparency( pt );
	}
    }
    else
    {
	Geom::Point2D<float> pt( cttranscanvas_->xVals()[ptidx],
				 cttranscanvas_->yVals()[ptidx] );

	Geom::Point2D<float> pt2;
	int ptidx2 = ptidx;

	if ( ptidx!=0 && ptidx!=nrpts-1 && equalseg && compatible )
	{
	    if ( ptidx%2!=0 )
	    {
		ptidx2 = ptidx+1;
		pt2 = { cttranscanvas_->xVals()[ptidx2],
			cttranscanvas_->yVals()[ptidx] };
	    }
	    else
	    {
		ptidx2 = ptidx-1;
		pt2 = { cttranscanvas_->xVals()[ptidx2],
			cttranscanvas_->yVals()[ptidx] };
	    }
	}

	if ( !pt.isDefined() )
	    return;

	bool reset = false;
        if ( ptidx==0 && !mIsZero(pt.x_,mEps) )
	{
            pt.x_ = 0;
	    reset = true;
	}
        else if ( ptidx==nrpts-1 && !mIsZero(pt.x_-1,mEps) )
	{
            pt.x_ = 1;
	    reset = true;
	}

	if ( equalseg && compatible )
	{
	    updateTransparencyGraph();
	    ctab_.changeTransparency( ptidx2, pt2 );
	    pt.x_ = ctab_.transparency(ptidx).x_;
	}

	if ( reset )
	    updateTransparencyGraph();

	ctab_.changeTransparency( ptidx, pt );
    }

    tableChanged.trigger();
}


void uiColorTableMan::transptSel( CallBacker* )
{
    const int ptidx = cttranscanvas_->selPt();
    if ( ctab_.nrSegments() > 1 )
    {
	cttranscanvas_->allowAddingPoints(false);
	return;
    }
    else
	cttranscanvas_->allowAddingPoints(true);


    const int nrpts = cttranscanvas_->xVals().size();
    if ( ptidx<0 || nrpts == ctab_.transparencySize() )
	return;

    Geom::Point2D<float> pt( cttranscanvas_->xVals()[ptidx],
			     cttranscanvas_->yVals()[ptidx] );
    if ( !pt.isDefined() )
	return;

    ctab_.setTransparency( pt );
    tableChanged.trigger();
}


void uiColorTableMan::importColTab( CallBacker* )
{
    NotifyStopper notifstop( importbut_->activated );
    uiColTabImport dlg( this );
    if ( dlg.go() )
    {
	refreshColTabList( dlg.getCurrentSelColTab() );
	tableAddRem.trigger();
	selChg(0);
    }
}


void uiColorTableMan::exportColTab( CallBacker* )
{
    uiColTabExport dlg( this );
    dlg.go();
}


class uiColTabRenameDlg : public uiGenInputDlg
{ mODTextTranslationClass(uiColTabRenameDlg)
public:
uiColTabRenameDlg( uiParent* p, const char* oldnm )
    : uiGenInputDlg(p,tr("Rename Color Table"),tr("New name"),
		    new StringInpSpec(oldnm))
    , oldnm_(oldnm)
{
    setTitleText( toUiString("%1 '%2'").arg(uiStrings::sRename()).arg(oldnm));
}


bool acceptOK( CallBacker* ) override
{
    const BufferString newnm = text();
    if ( newnm==oldnm_ )
	return true;

    if ( ColTab::SM().indexOf(newnm.buf()) < 0 )
    {
	newnm_ = newnm.buf();
    }
    else
    {
	BufferString altnm;
	for ( int idx=2; idx<=10; idx++ )
	{
	    altnm.set( newnm ).add( " (").add(idx).add(")");
	    if ( ColTab::SM().indexOf(altnm.buf()) >= 0 )
		continue;

	    break;
	}

	const bool res = uiMSG().askGoOn( tr("Rename color table to '%1'?")
					  .arg(altnm) );
	if ( !res )
	    return false;

	newnm_ = altnm.buf();
    }

    return true;
}


BufferString oldnm_;
BufferString newnm_;


};

void uiColorTableMan::renameColTab( CallBacker* )
{
    auto* item = coltablistfld_->currentItem();
    if ( !item )
	return;

    if ( ctab_.type() != ColTab::Sequence::User )
    {
	uiMSG().error( tr("You can only rename user defined color tables") );
	return;
    }

    const BufferString oldnm = ctab_.name();
    uiColTabRenameDlg dlg( this, oldnm );
    if ( !dlg.go() )
	return;

    ColTab::SM().rename( oldnm, dlg.newnm_ );
    ColTab::SM().write();
    refreshColTabList( dlg.newnm_ );
}
