/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
________________________________________________________________________

-*/

#include "uiwelllogdisplay.h"

#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "coltabsequence.h"
#include "welllog.h"


uiWellLogDisplay::LogData::LogData( uiGraphicsScene& scn, bool isfirst,
				    const uiWellLogDisplay::Setup& s )
    : uiWellDahDisplay::DahObjData( scn, isfirst, s )
    , logSet(this)
{
    disp_.setColor( OD::Color::stdDrawColor( isfirst ? 0 : 1 ) );
}


uiWellLogDisplay::~uiWellLogDisplay()
{
    delete ld1_; ld1_ = 0;
    delete ld2_; ld2_ = 0;
}


void uiWellLogDisplay::gatherDataInfo( bool first )
{
    LogData& ld = logData( first );
    ld.xax_.setup().islog( ld.disp_.islogarithmic_ );
    ld.cliprate_ = ld.disp_.cliprate_;
    ld.valrg_ = ld.disp_.range_;

    uiWellDahDisplay::gatherDataInfo( first );
}


const Well::Log* uiWellLogDisplay::LogData::log() const
{
    mDynamicCastGet(const Well::Log*,dahlog,dahobj_)
    return dahlog;
}


void uiWellLogDisplay::LogData::setLog( const Well::Log* l )
{
    if ( dahobj_ != l )
    {
	dahobj_ = l;
	logSet.trigger();
    }
}


void uiWellLogDisplay::LogData::getInfoForDah( float dah,
						BufferString& msg ) const
{
    if ( !log() ) return;
    msg += log()->name();
    msg += ":";
    msg += toString( log()->getValue( dah ) );
    msg += log()->unitMeasLabel();
}


uiWellLogDisplay::uiWellLogDisplay( uiParent* p, const Setup& su )
    : uiWellDahDisplay(p,su)
    , setup_(su)
{
    delete ld1_; delete ld2_;
    ld1_ = new LogData( scene(), true, su );
    ld2_ = new LogData( scene(), false, su );
    postFinalise().notify( mCB(this,uiWellLogDisplay,init) );
}


void uiWellLogDisplay::draw()
{
    uiWellDahDisplay::draw();

    int style = logData(true).disp_.style_;
    if ( style==0 || style==2 )
	drawFilledCurve( true );
    else
	drawSeismicCurve( true );

    style = logData(false).disp_.style_;
    if ( style==0 || style==2 )
	drawFilledCurve( false );
    else
	drawSeismicCurve( false );
}


static const int cMaxNrLogSamples = 2000;
#define mGetLoopSize(nrsamp,step)\
    {\
	if ( nrsamp > cMaxNrLogSamples )\
	{\
	    step = (float)nrsamp/cMaxNrLogSamples;\
	    nrsamp = cMaxNrLogSamples;\
	}\
    }

void uiWellLogDisplay::drawCurve( bool first )
{
    uiWellDahDisplay::drawCurve( first );
    LogData& ld = logData( first );

    if ( !ld.curvepolyitm_ ) return;
    OD::LineStyle ls(OD::LineStyle::Solid);
    ls.width_ = ld.disp_.getSize();
    ls.color_ = ld.disp_.getColor();
    ld.curvepolyitm_->setPenStyle( ls );
    ld.curvepolyitm_->setVisible( ls.width_ > 0 );
}


void uiWellLogDisplay::drawSeismicCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = logData(first);
    deepErase( ld.curvepolyitms_ );

    const float rgstop = ld.xax_.range().stop;
    const float rgstart = ld.xax_.range().start;

    int sz = ld.log() ? ld.log()->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    ObjectSet< TypeSet<uiPoint> > pts;
    uiPoint closept;

    float zfirst = ld.log()->dah(0);
    mDefZPos( zfirst )
    const int pixstart = ld.xax_.getPix( rgstart );
    const int pixstop = ld.xax_.getPix( rgstop );
    const int midpt = (int)( (pixstop-pixstart)/2 );
    closept = uiPoint( midpt, ld.yax_.getPix( zfirst ) );

    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    *curpts += closept;
    uiPoint pt;
    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT32(idx*step);
	float dah = ld.log()->dah( index );
	if ( index && index < sz-1 )
	{
	    if ( dah>=ld.log()->dah(index+1) || dah<=ld.log()->dah(index-1) )
		continue;
	}
	mDefZPosInLoop( dah )

	float val = ld.log()->value( index );

	pt.x = ld.xax_.getPix(val);
	pt.y = closept.y = ld.yax_.getPix(zpos);

	if ( mIsUdf(val) || pt.x < closept.x )
	{
	    if ( !curpts->isEmpty() )
	    {
		pts += curpts;
		curpts = new TypeSet<uiPoint>;
		*curpts += closept;
	    }
	    continue;
	}
	*curpts += closept;
	*curpts += pt;
	*curpts += closept;
    }
    if ( pts.isEmpty() ) return;
    *pts[pts.size()-1] += uiPoint( closept.x, pt.y );

    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	OD::Color color = ld.disp_.seiscolor_;
	pli->setFillColor( color );
	pli->setPenStyle( OD::LineStyle(OD::LineStyle::Solid,1,color) );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


void uiWellLogDisplay::drawFilledCurve( bool first )
{
    uiWellLogDisplay::LogData& ld = logData(first);
    deepErase( ld.curvepolyitms_ );

    if ( !ld.disp_.isleftfill_ && !ld.disp_.isrightfill_ ) return;

    const float rgstop = ld.xax_.range().stop;
    const float rgstart = ld.xax_.range().start;
    const bool isrev = rgstop < rgstart;

    const float colrgstop = ld.disp_.fillrange_.stop;
    const float colrgstart = ld.disp_.fillrange_.start;
    const float colrgwidth = ld.disp_.fillrange_.width();
    const bool iscolrev = colrgstop < colrgstart;

    int sz = ld.log() ? ld.log()->size() : 0;
    if ( sz < 2 ) return;
    float step = 1;
    mGetLoopSize( sz, step );

    const bool fullpanelfill = ld.disp_.isleftfill_ && ld.disp_.isrightfill_;
    const bool isfillrev = !fullpanelfill &&
		 ( ( first && ld.disp_.isleftfill_ && !isrev )
		|| ( first && ld.disp_.isrightfill_ && isrev )
		|| ( !first && ld.disp_.isrightfill_ && !isrev )
		|| ( !first && ld.disp_.isleftfill_ && isrev ) );

    float zfirst = ld.log()->dah(0);
    mDefZPos( zfirst )
    const int pixstart = ld.xax_.getPix( rgstart );
    const int pixstop = ld.xax_.getPix( rgstop );
    uiPoint closept;
    closept.x = ( first ) ? isfillrev ? pixstart : pixstop
			  : isfillrev ? pixstop  : pixstart;
    closept.y = ld.yax_.getPix( zfirst );
    float prevcolpos = mUdf(float);
    TypeSet<float> colorposset;
    ObjectSet<TypeSet<uiPoint> > pts;
    TypeSet<uiPoint>* curpts = new TypeSet<uiPoint>;
    uiPoint pt;

    for ( int idx=0; idx<sz; idx++ )
    {
	const int index = mNINT32(idx*step);
	float dah = ld.log()->dah( index );
	if ( index && index < sz-1 )
	{
	    if ( dah >= ld.log()->dah(index+1) || dah <= ld.log()->dah(index-1))
		continue;
	}
	mDefZPosInLoop( dah )

	float val = ld.log()->value( index );
	bool isvalrev = iscolrev;
	if ( ld.disp_.iscoltabflipped_ )
	    isvalrev = !isvalrev;
	const float valdiff = isvalrev ? colrgstop-val : val-colrgstart;
	const bool isvaludf = mIsUdf(val);
	float colpos = isvaludf ? mUdf(float) : (!mIsZero(colrgwidth,mDefEpsF)
			? valdiff/colrgwidth : (val<=colrgstart ? 0.0f:1.0f));
	if ( !isvaludf && colpos<0.0f ) colpos = 0.0f;
	if ( !isvaludf && colpos>1.0f ) colpos = 1.0f;
	if ( mIsUdf(prevcolpos) )
	    prevcolpos = colpos;
	if ( fullpanelfill )
	    val = first ? rgstart : rgstop;

	if ( !mIsUdf(val) )
	{
	    pt.x = ld.xax_.getPix(val);
	    pt.y = ld.yax_.getPix(zpos);
	    if ( curpts->isEmpty() )
		*curpts += uiPoint( closept.x, pt.y );
	    *curpts += pt;
	}

	if ( !mIsEqual(colpos,prevcolpos,mDefEps) )
	{
	    *curpts += uiPoint( closept.x, pt.y );
	    pts += curpts;
	    colorposset += prevcolpos;
	    prevcolpos = colpos;

	    curpts = new TypeSet<uiPoint>;
	    if ( mIsUdf(colpos) ) continue;
	    *curpts += uiPoint( closept.x, pt.y );
	    *curpts += pt;
	}
    }

    if ( !pts.isEmpty() )
    {
	*curpts += uiPoint( closept.x, pt.y );
	pts += curpts;
	colorposset += prevcolpos;
    }

    const int tabidx = ColTab::SM().indexOf( ld.disp_.seqname_ );
    const ColTab::Sequence* seq = ColTab::SM().get( tabidx<0 ? 0 : tabidx );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	uiPolygonItem* pli = scene().addPolygon( *pts[idx], true );
	ld.curvepolyitms_ += pli;
	OD::Color color = ld.disp_.issinglecol_ ? ld.disp_.seiscolor_
					    : seq->color(colorposset[idx]);
	pli->setFillColor( color );
	pli->setPenStyle( OD::LineStyle(OD::LineStyle::None) );
	pli->setZValue( 1 );
    }
    deepErase( pts );
}


uiWellLogDisplay::LogData& uiWellLogDisplay::logData( bool first )
{
    return *static_cast<LogData*>( first ? ld1_ : ld2_ );
}


uiWellLogDispDlg::uiWellLogDispDlg( uiParent* p,
				    const uiWellLogDisplay::Setup& wldsu,
				    bool mkcopy )
    : uiDialog(p,uiDialog::Setup(uiStrings::sEmptyString(),mNoDlgTitle,
						       mNoHelpKey).modal(false))
    , logSet(this)
    , logsmine_(mkcopy)
    , log1_(0)
    , log2_(0)
{
    setCtrlStyle( CloseOnly );
    dispfld_ = new uiWellLogDisplay( this, wldsu );
    const CallBack cb( mCB(this,uiWellLogDispDlg,logSetCB) );
    dispfld_->logData(true).logSet.notify( cb );
    dispfld_->logData(false).logSet.notify( cb );
    dispfld_->dahObjData(true).col_ = dispfld_->logData(true).disp_.getColor();
    dispfld_->dahObjData(false).col_= dispfld_->logData(false).disp_.getColor();

    dispfld_->setPrefWidth( 300 );
    dispfld_->setPrefHeight( 500 );
    postFinalise().notify( cb );
    setDeleteOnClose( true );
}


uiWellLogDispDlg::~uiWellLogDispDlg()
{
    if ( logsmine_ )
    {
	delete const_cast<Well::Log*>(log1_);
	delete const_cast<Well::Log*>(log2_);
    }
}


void uiWellLogDispDlg::setLog( const Well::Log* wl, bool first,
			       const char* wellnm )
{
    if ( wl && logsmine_ )
    {
	Well::Log*& mywl = const_cast<Well::Log*&>(first ? log1_ : log2_);
	delete mywl; mywl = new Well::Log( *wl );
	wl = mywl;
    }

    if ( wellnm )
	first ? wellnm1_ = wellnm : wellnm2_ = wellnm;

    dispfld_->logData( first ).setLog( wl );
}


const Well::Log* uiWellLogDispDlg::getLog( bool first ) const
{
    return dispfld_->logData( first ).log();
}


void uiWellLogDispDlg::logSetCB( CallBacker* )
{
    const Well::Log* l1 = getLog( true );
    const Well::Log* l2 = getLog( false );
    uiString capt = tr("Log viewer");
    if ( l1 || l2 )
	capt = toUiString("%1 %2").arg(capt).arg(toUiString(": "));

    if ( l1 )
	capt = toUiString("%1 %2").arg(capt).arg(toUiString(l1->name()));
    if ( l2 )
    {
	if ( !l1->name().isEqual(l2->name()) )
	    capt = toUiString("%1 & %2").arg(capt).arg(toUiString(l2->name()));
    }

    uiString str = uiStrings::sEmptyString();
    if ( !wellnm1_.isEmpty() )
	str= toUiString(wellnm1_);
    if ( !wellnm2_.isEmpty() )
	str = tr( "%1 and %2" ).arg(str).arg(toUiString(wellnm2_.buf()));
    if ( !str.isEmpty() )
	capt = tr("%1 of %2").arg(capt).arg(str);

    setCaption( capt );
    if ( l1 && !wellnm1_.isEmpty() )
    {
	capt = tr("%1 of %2").arg(toUiString(l1->name())).arg(
						    toUiString(wellnm1_));
	dispfld_->dahObjData( true ).xax_.setCaption( capt );
    }

    if ( l2 && !wellnm2_.isEmpty() )
    {
	capt = tr("%1 of %2").arg(toUiString(l2->name())).arg(
						    toUiString(wellnm2_));
	dispfld_->dahObjData( false ).xax_.setCaption( capt );
    }

    logSet.trigger();
}


uiWellLogDispDlg* uiWellLogDispDlg::popupNonModal( uiParent* p,
				const Well::Log* wl1, const Well::Log* wl2,
				const char* wellnm1,
				const char* wellnm2)
{
    uiWellLogDisplay::Setup wldsu;
    wldsu.annotinside( false ).drawcurvenames( false );
    uiWellLogDispDlg* dlg = new uiWellLogDispDlg( p, wldsu, true );
    uiWellDahDisplay::Data data(dlg->logDisplay().zData());
    data.dispzinft_ = true;
    dlg->logDisplay().setData(data);
    dlg->setLog( wl1, true, wellnm1 );
    if ( wl2 )
	dlg->setLog( wl2, false, wellnm2 );

    dlg->setDeleteOnClose( true );
    dlg->show();
    return dlg;
}
