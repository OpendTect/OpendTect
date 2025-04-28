/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodcontourtreeitem.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "axislayout.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "executor.h"
#include "isocontourtracer.h"
#include "math.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "od_ostream.h"
#include "polygon.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vishorizondisplay.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uidialog.h"
#include "uiempartserv.h"
#include "uifiledlg.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uiprogressbar.h"
#include "uisellinest.h"
#include "uispinbox.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitreeview.h"
#include "uivispartserv.h"


static const int cMinNrNodesForLbl = 25;
static const int cMaxNrDiplayedLabels = 1000;

const char* uiContourTreeItem::sKeyContourDefString(){return "Contour Display";}
const char* uiContourTreeItem::sKeyZValue()	     { return "Z Values"; }

static int getInitialDec( const float val )
{
    const float logval = Math::Log10( val>0 ? val : -val );
    if ( mIsUdf(logval) )
	return 0;

    const int nrdigits = mNINT32( Math::Ceil(logval) );
    if ( nrdigits > 6 )
	return 0;

    return 6-nrdigits;
}

static float getNiceNumber( float val, int& nrdec )
{
    nrdec = 0;
    const float logval = Math::Log10( val>0 ? val : -val );
    if ( mIsUdf(logval) )
	return val;

    const int nrdigits = mNINT32( Math::Ceil(logval) );
    if ( nrdigits > 6 )
	return mCast(float, mNINT32(val/1e6) * 1e6);

    const float multiplier = Math::PowerOf( 10.f,
					    (float)(6-nrdigits) );
    const int rounded = mNINT32( val * multiplier );
    nrdec = 6-nrdigits;
    int divider = 10;
    while ( rounded % divider == 0 )
    {
	nrdec--;
	divider *= 10;
    }

    if ( nrdec < 0 )
	nrdec = 0;

    return mCast(float, rounded / multiplier);
}


class uiContourTreeItemContourData
{ mODTextTranslationClass(uiContourTreeItemContourData);
public:

    TypeSet<Coord3>				contourcoords_;
    TypeSet< Interval<int> >			contourcoordrgs_;
    RefObjectSet<Geometry::RangePrimitiveSet>	contourprimitivesets_;
    TypeSet<Coord3>				labelpositions_;
    TypeSet<int>				labelcontourlen_;
    TypeSet<BufferString>			labels_;
    TypeSet<Interval<int> >			labelranges_;
};


class uiContourTreeItemContourGenerator : public ParallelTask
{ mODTextTranslationClass(uiContourTreeItemContourGenerator);
public:

				uiContourTreeItemContourGenerator(
					uiContourTreeItem*,
					const Array2D<float>* field);
				~uiContourTreeItemContourGenerator()
				{}

    visBase::Text2*		getLabels() { return labels_.ptr(); }
    const TypeSet<double>&	getAreas() const { return areas_; }
    uiString			uiNrDoneText() const override;

private:

    bool	doPrepare(int) override;
    bool	doWork(od_int64 start, od_int64 stop, int) override;
    bool	doFinish(bool success) override;
    od_int64	nrIterations() const override;

private:

    bool prepForContourGenerator();
    bool setRowColRgs(visSurvey::HorizonDisplay*);

    bool generateContours(int contouridx,const IsoContourTracer*,
			     uiContourTreeItemContourData&,double& area) const;
    bool addDisplayCoord(const ODPolygon<float>& inputcontour, int vrtxidx,
			 uiContourTreeItemContourData&,int& lastvrtxidx) const;
    void makeContourClose(uiContourTreeItemContourData&,
			  Interval<int>& coordsrg) const;
    void addContourData(uiContourTreeItemContourData&);
    void addContourLabel(const Coord3& pos, const char* lbl);
    float getHorizonZValue(int rowidx,int colidx) const;

    int						nrcontours_;
    Threads::Atomic<od_int64>			totalnrshapes_;
    uiContourTreeItem*				uicitem_;
    Threads::Mutex				mutex_;
    uiContourTreeItemContourData		contourdata_;
				// from construction source
    const Array2D<float>*	field_;

    RefMan<visBase::Text2>	labels_;

    StepInterval<int>		rowrg_;
    StepInterval<int>		colrg_;
    float			zfactor_ = 0.f;
    ConstRefMan<EM::Horizon3D>	hor3d_;
    ConstRefMan<ZAxisTransform> ztransform_;
    ConstRefMan<mVisTrans>	displaytrans_;
    TypeSet<double>		areas_;

    bool			isfinishing_ = false;
};


uiContourTreeItemContourGenerator::uiContourTreeItemContourGenerator(
			uiContourTreeItem* p, const Array2D<float>* field )
    : nrcontours_(p->getNrContours())
    , uicitem_(p)
    , field_(field)
{
    totalnrshapes_ = nrcontours_;
    areas_.setSize( nrcontours_ );
    setName( "Generating contours" );
}


uiString uiContourTreeItemContourGenerator::uiNrDoneText() const
{
    return isfinishing_ ? tr("Contour elements added")
			: tr("Contours computed");
}


od_int64 uiContourTreeItemContourGenerator::nrIterations() const
{
    return !isfinishing_ ? nrcontours_
			 : contourdata_.contourprimitivesets_.size() +
					contourdata_.labelpositions_.size();
}


bool uiContourTreeItemContourGenerator::doPrepare(int)
{
    if( !setRowColRgs( uicitem_->getHorDisp() ) ||
	!prepForContourGenerator() )
	return false;

    return true;
}


bool uiContourTreeItemContourGenerator::setRowColRgs(
			    visSurvey::HorizonDisplay* hordisp )
{
    rowrg_.set( 0, 0, 0 );
    colrg_.set( 0, 0, 0 );
    rowrg_ = hordisp->geometryRowRange();
    colrg_ = hordisp->geometryColRange();
    return ( !rowrg_.width() || !colrg_.width() ) ? false : true;
}


bool uiContourTreeItemContourGenerator::prepForContourGenerator()
{
    uiVisPartServer* visserv = uicitem_->applMgr()->visServer();
    EM::ObjectID emid = uicitem_->getHorDisp()->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::EMM().getObject(emid));
    if ( !hor )
	return false;

    hor3d_ = hor;
    ConstRefMan<visSurvey::Scene> scene =
				  visserv->getScene( uicitem_->sceneID() );
    ztransform_ = scene ? scene->getZAxisTransform() : nullptr;
    zfactor_ = mCast( float, scene->zDomainInfo().userFactor() );
    displaytrans_ =
	uicitem_->lines_->getCoordinates()->getDisplayTransformation();

    return true;
}


bool uiContourTreeItemContourGenerator::doWork( od_int64 start, od_int64 stop,
									int )
{
    PtrMan<IsoContourTracer> ictracer =  new IsoContourTracer( *field_ );
    if( !ictracer )
	return false;

    ictracer->setSampling( rowrg_, colrg_ );

    uiContourTreeItemContourData newcontourdata;
    for ( int contouridx=mCast(int,start); contouridx<=stop; contouridx++ )
    {
	const int nrshapesbefore = newcontourdata.contourcoordrgs_.size() +
				   newcontourdata.labels_.size();
	double area;
	generateContours( contouridx, ictracer.ptr(), newcontourdata, area);
	const int nraddedshapes = newcontourdata.contourcoordrgs_.size() +
	    newcontourdata.labels_.size() - nrshapesbefore;
	totalnrshapes_ += nraddedshapes;
	addToNrDone( 1 );

	areas_[contouridx] = area;
	if ( !shouldContinue() )
	    return false;
    }

    Threads::MutexLocker datacollectlock( mutex_ );
    addContourData( newcontourdata );

    return true;
}


void uiContourTreeItemContourGenerator::addContourData(
	 uiContourTreeItemContourData& newcontourdata )
{
    const int contourcoordsz = contourdata_.contourcoords_.size();
    const Interval<int> lastcoordidxrg ( contourcoordsz, contourcoordsz-1 );
    contourdata_.contourcoords_.append( newcontourdata.contourcoords_);
    contourdata_.labelpositions_.append( newcontourdata.labelpositions_ );
    contourdata_.labelcontourlen_.append( newcontourdata.labelcontourlen_ );
    contourdata_.labels_.append( newcontourdata.labels_ );

    for ( int idx=0; idx<newcontourdata.contourcoordrgs_.size(); idx++ )
    {
	RefMan<Geometry::RangePrimitiveSet> ps =
				Geometry::RangePrimitiveSet::create();
	if ( !ps )
	    continue;

	ps->setRange( newcontourdata.contourcoordrgs_[idx] + lastcoordidxrg );
	contourdata_.contourprimitivesets_.push( ps.ptr() );
    }

    if ( contourdata_.labelranges_.size() )
    {
	const int lastlblidx =
	    contourdata_.labelranges_[contourdata_.labelranges_.size()-1].stop_;
	for ( int idx=0; idx<newcontourdata.labelranges_.size(); idx++ )
	    newcontourdata.labelranges_[idx] +=
	    Interval<int>(lastlblidx,lastlblidx);
    }

    contourdata_.labelranges_.append( newcontourdata.labelranges_);

}


bool uiContourTreeItemContourGenerator::generateContours( int contouridx,
				const IsoContourTracer* ictracer,
				uiContourTreeItemContourData& contourdata,
				double& area)const
{
    area = 0;
    const float contourval = uicitem_->contourintv_.start_ +
                             contouridx* uicitem_->contourintv_.step_;

    ManagedObjectSet<ODPolygon<float> > isocontours;
    ictracer->getContours( isocontours, contourval, false );

    const int lblpositionrgsz = contourdata.labelpositions_.size();
    Interval<int> lblpositionrg( lblpositionrgsz, lblpositionrgsz );

    for ( int cidx=0; cidx<isocontours.size(); cidx++ )
    {
	const ODPolygon<float>& curcontour = *isocontours[cidx];
	if ( !mIsUdf(area) && curcontour.isClosed() )
	    area += curcontour.area();
	else
	    area = mUdf(double);

	const int coordsrgsz = contourdata.contourcoords_.size();
	Interval<int> contourcoordsrg( coordsrgsz, coordsrgsz );
	for ( int vidx=0; vidx<curcontour.size(); vidx++ )
	{
	    const int vertexidx = vidx;
	    if( !addDisplayCoord( curcontour,vertexidx,contourdata,
                                  contourcoordsrg.stop_) )
		continue;
	    // if meet the condition adding contour label position
	    if ( curcontour.size()>cMinNrNodesForLbl &&
		 vertexidx==curcontour.size()/2 )
	    {
		const int lastvrtxidx = contourdata.contourcoords_.size() - 1;
		const Coord3 lblpos = contourdata.contourcoords_[lastvrtxidx];
		contourdata.labelpositions_.add( lblpos );
		contourdata.labelcontourlen_.add( curcontour.size() );
                lblpositionrg.stop_++;
	    }
	}

	if ( curcontour.isClosed() )
	    makeContourClose( contourdata,contourcoordsrg );

	contourdata.contourcoordrgs_.add( contourcoordsrg );
    }
    // if having label, add label into contour data
    if ( lblpositionrg.stop_ > lblpositionrg.start_ )
    {
	float labelval =
	    uicitem_->attrnm_==uiContourTreeItem::sKeyZValue()
	    ? (contourval+uicitem_->zshift_) * zfactor_ : contourval;
	contourdata.labelranges_.add( lblpositionrg );
	int nrdec = 0;
	labelval = getNiceNumber( labelval, nrdec );
	contourdata.labels_.add( toString(labelval,0,'f',nrdec) );
    }

    return true;
}


void uiContourTreeItemContourGenerator::makeContourClose(
    uiContourTreeItemContourData& contourdata,
    Interval<int>& coordsrg ) const
{
    if ( contourdata.contourcoords_.size() <=0 )
	return;

    contourdata.contourcoords_.add(
                contourdata.contourcoords_[coordsrg.start_] );
    coordsrg.stop_++;
}


float uiContourTreeItemContourGenerator::getHorizonZValue( int rowidx,
							   int colidx ) const
{
    const BinID bid( rowrg_.atIndex(rowidx), colrg_.atIndex(colidx) );
    float zval = hor3d_ ? hor3d_->getZ(bid) : mUdf(float);

    if ( ztransform_ && !mIsUdf(zval) )
	ztransform_->transform( bid, SamplingData<float>(zval,1), 1, &zval );

    return zval;
}


bool uiContourTreeItemContourGenerator::addDisplayCoord(
    const ODPolygon<float>& inputcontour, int vrtxidx,
    uiContourTreeItemContourData& contourdata,
    int& lastvrtxidx ) const
{
    const Geom::Point2D<float> vrtx = inputcontour.getVertex( vrtxidx );

    const float rowfidx = rowrg_.getfIndex( vrtx.x_ );
    const float colfidx = colrg_.getfIndex( vrtx.y_ );
    int rowidx = mMAX( 0, (int) rowfidx );
    int colidx = mMAX( 0, (int) colfidx );
    const float rowfrac = rowfidx - rowidx;
    const float colfrac = colfidx - colidx;

    const float z0 = getHorizonZValue( rowidx, colidx );

    // Contour algorithms is known to produce vertices on grid-lines only.
    float frac = colfrac;
    if ( fabs(0.5-rowfrac) < fabs(0.5-colfrac) )
    {
	frac = rowfrac;
	rowidx++;
    }
    else
	colidx++;

    const float z1 = getHorizonZValue( rowidx, colidx );

    Coord3 vrtxcoord;
    vrtxcoord.coord() = SI().binID2Coord().transform( Coord(vrtx.x_,vrtx.y_) );

    vrtxcoord.z_ = mIsUdf(z0) ? z1 : (mIsUdf(z1) ? z0 : (1.0-frac)*z0+frac*z1);
    if ( mIsUdf(vrtxcoord.z_) )
	return false;

    vrtxcoord.z_ += uicitem_->zshift_;

    visBase::Transformation::transform( displaytrans_.ptr(),
					vrtxcoord, vrtxcoord );
    contourdata.contourcoords_.add( vrtxcoord );
    lastvrtxidx++;
    return true;
}


bool uiContourTreeItemContourGenerator::doFinish( bool success )
{
    resetNrDone();
    isfinishing_ = true;

    if ( !success )
	return false;

    labels_ = visBase::Text2::create();
    labels_->setDisplayTransformation( displaytrans_.ptr() );
    labels_->setPickable( false, false );

    uicitem_->lines_->getCoordinates()->setPositions(
		      contourdata_.contourcoords_.arr(),
		      contourdata_.contourcoords_.size(), 0, true );

    for ( int idx=0; idx<contourdata_.contourprimitivesets_.size(); idx++ )
    {
	uicitem_->lines_->addPrimitiveSet(
			  contourdata_.contourprimitivesets_[idx] );
	addToNrDone( 1 );
	if ( !shouldContinue() )
	    return false;
    }

    float contourlenthreshold = 0.f;
    const int nrlabels = contourdata_.labelcontourlen_.size();
    if ( nrlabels >= cMaxNrDiplayedLabels )
    {
	// Approximation assuming uniform distribution yields order N algorithm
	float totalcontourlen = 0.f;
	for ( int idx=0; idx<nrlabels; idx++ )
	    totalcontourlen += contourdata_.labelcontourlen_[idx];

	const float mean = totalcontourlen / nrlabels;
	const float frac = (float) cMaxNrDiplayedLabels / (float) nrlabels;
	const int offset = cMinNrNodesForLbl;
	contourlenthreshold = offset + 2.f*(mean-offset)*(1.f-frac);
    }

    for ( int lbrgidx=0; lbrgidx<contourdata_.labelranges_.size(); lbrgidx++ )
    {
	const Interval<int> lbldata( contourdata_.labelranges_[lbrgidx] );
        for ( int ipos=lbldata.start_; ipos<lbldata.stop_; ipos++ )
	{
	    if ( contourdata_.labelcontourlen_[ipos] > contourlenthreshold )
	    {
		addContourLabel( contourdata_.labelpositions_[ipos],
				 contourdata_.labels_[lbrgidx] );
	    }
	}

	addToNrDone( 1 );
	if ( !shouldContinue() )
	    return false;
    }

    return true;
}


void uiContourTreeItemContourGenerator::addContourLabel(
    const Coord3& pos, const char* lbl)
{
    if ( !labels_ )
	return;

    const int idx = labels_->addText();
    visBase::Text* label = labels_->text( idx );
    if ( label )
    {
	BufferString labelonpole( lbl );
	labelonpole += "\n|";
	label->setText( toUiString(labelonpole) );
	label->setJustification( visBase::Text::BottomLeft );
	label->setPosition( pos, true );
	label->setFontData( FontData(18), labels_->getPixelDensity() );
    }
}


class uiContourParsDlg : public uiDialog
{ mODTextTranslationClass(uiContourParsDlg);

static const int defelevval = 0;
public:
uiContourParsDlg( uiParent* p, const char* attrnm, const Interval<float>& rg,
		  const StepInterval<float>& intv, const OD::LineStyle& ls,
		  const SceneID& sceneid )
    : uiDialog(p,Setup(tr("Contour Display Options"),mNoDlgTitle,
			mODHelpKey(mContourParsDlgHelpID) )
		.modal(false).nrstatusflds(1))
    , propertyChanged(this)
    , intervalChanged(this)
    , rg_(rg)
    , contourintv_(intv)
    , iszval_(StringView(uiContourTreeItem::sKeyZValue()) == attrnm)
{
    setOkCancelText( uiStrings::sApply(), uiStrings::sClose() );

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    ConstRefMan<visSurvey::Scene> scene = visserv->getScene( sceneid );
    if ( iszval_ )
    {
	zfac_ = mCast( float, scene->zDomainInfo().userFactor() );
	rg_.scale( zfac_ );
	contourintv_.scale( zfac_ );
    }

#define mAddZUnitStr(str) \
    if ( iszval_ ) \
        str.withUnit( scene->zDomainInfo().unitStr() )

    uiString lbltxt( tr("Total %1 range").arg(attrnm) );
    mAddZUnitStr(lbltxt);
    lbltxt.appendPhrase( toUiString(":"), uiString::Space,
							uiString::OnSameLine);
    BufferString lbltxtapnd;
    lbltxtapnd.add( rg_.start_ ).add( " - " ).add( rg_.stop_ );

    auto* lbl = new uiLabel( this,
		lbltxt.appendPhrase(toUiString(lbltxtapnd),
				    uiString::Space,uiString::OnSameLine) );

    lbltxt = tr("Contour Range").addSpace();
    mAddZUnitStr(lbltxt);
    intvfld_ = new uiGenInput(this,lbltxt,FloatInpIntervalSpec(contourintv_));
    mAttachCB( intvfld_->valueChanged, uiContourParsDlg::intvChanged );
    intvfld_->attach( leftAlignedBelow, lbl );

    uiSelLineStyle::Setup lssu; lssu.drawstyle(false);
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, intvfld_ );
    mAttachCB( lsfld_->changed, uiContourParsDlg::dispChanged );

    showlblsfld_ = new uiCheckBox( this, tr("Show labels") );
    showlblsfld_->activated.notify( mCB(this,uiContourParsDlg,uiDisplayCB) );
    showlblsfld_->attach( alignedBelow, lsfld_ );

    fontfld_ = new uiPushButton( this, tr("Font"),
			    mCB(this,uiContourParsDlg,selectFontCB), false );
    fontfld_->attach( rightOf, showlblsfld_ );

    alignbutsfld_ = new uiButtonGroup( this, "Alignment buttons",
				       OD::Horizontal );
    alignbutsfld_->attach( alignedBelow, showlblsfld_ );

    alignlblfld_ = new uiLabel( this, tr("Label alignment") );
    alignlblfld_->attach( leftOf, alignbutsfld_ );

    auto* leftbut = new uiRadioButton( alignbutsfld_, uiStrings::sLeft() );
    mAttachCB( leftbut->activated, uiContourParsDlg::dispChanged );

    auto* centerbut = new uiRadioButton( alignbutsfld_, uiStrings::sCenter() );
    mAttachCB( centerbut->activated, uiContourParsDlg::dispChanged );

    auto* rightbut = new uiRadioButton( alignbutsfld_, uiStrings::sRight() );
    mAttachCB( rightbut->activated, uiContourParsDlg::dispChanged );
    alignbutsfld_->selectButton( 0 );

    elevationfld_ = new uiLabeledSpinBox( this, tr("Label elevation") );
    elevationfld_->attach( alignedBelow, alignbutsfld_ );

    degreeslblfld_ = new uiLabel( this, toUiString("(deg)") );
    degreeslblfld_->attach( rightOf, elevationfld_ );

    elevationfld_->box()->setInterval( -180, 165, 15 );
    elevationfld_->box()->setValue( defelevval );
    elevationfld_->box()->doSnap( true );
    mAttachCB( elevationfld_->box()->valueChanging,
					    uiContourParsDlg::elevationChg );
    mAttachCB( elevationfld_->box()->valueChanged,
					    uiContourParsDlg::uiDisplayCB );
    mAttachCB( postFinalize(), uiContourParsDlg::finalizeCB );
}


~uiContourParsDlg()
{
    detachAllNotifiers();
}


const OD::LineStyle& getLineStyle() const
{
    return lsfld_->getStyle();
}


StepInterval<float> getContourInterval() const
{
    StepInterval<float> res = intvfld_->getFStepInterval();
    if ( iszval_ )
	res.scale( 1.0f/zfac_ );

    return res;
}


void setShowLabels( bool yn )
{
    showlblsfld_->setChecked( yn );
    uiDisplayCB( nullptr );
}


bool showLabels() const
{
    return showlblsfld_->isChecked();
}


void setFontData( const FontData& fontdata )
{
    fontdata_ = fontdata;
}


const FontData& getFontData() const
{
    return fontdata_;
}


float getRotation() const
{
    return elevationfld_->box()->getFValue();
}


void setLabelAlignment( visBase::Text::Justification alignment )
{
    if ( alignment == visBase::Text::BottomRight )
	alignbutsfld_->selectButton( 2 );
    else if ( alignment == visBase::Text::Bottom )
	alignbutsfld_->selectButton( 1 );
    else
	alignbutsfld_->selectButton( 0 );
}


int getLabelAlignment() const
{
    if ( alignbutsfld_->selectedId() == 1 )
	return visBase::Text::Bottom;
    if ( alignbutsfld_->selectedId() > 1 )
	return visBase::Text::BottomRight;

    return visBase::Text::BottomLeft;
}


void disableLabelElevation()
{
    elevationfld_->box()->setValue( defelevval );
}


bool isLabelElevationDisabled() const
{
    return elevationfld_->box()->getIntValue() ==
					elevationfld_->box()->minValue();
}


void setLabelElevation( float angle )
{
    elevationfld_->box()->setValue( mNINT32(angle*180.0/M_PI) );
    elevationChg( nullptr );
}


float getLabelElevation() const
{
    return mCast( float, elevationfld_->box()->getIntValue()*M_PI/180.0 );
}


    Notifier<uiContourParsDlg>	propertyChanged;
    Notifier<uiContourParsDlg>	intervalChanged;

protected:

bool acceptOK( CallBacker* ) override
{
    intervalChanged.trigger();
    propertyChanged.trigger();
    return false;
}


void dispChanged( CallBacker* )
{
    propertyChanged.trigger();
}


void uiDisplayCB( CallBacker* cb )
{
    const bool yn = showLabels();

    fontfld_->display( yn );
    alignbutsfld_->display( yn );
    alignlblfld_->display( yn );
    elevationfld_->display( yn );
    degreeslblfld_->display( yn && !isLabelElevationDisabled() );

    if ( cb )
	dispChanged( cb );
}


void selectFontCB( CallBacker* cb )
{
    selectFont( fontdata_, this );
    dispChanged( cb );
}


void elevationChg( CallBacker* )
{
    elevationfld_->box()->valueChanged.trigger();     // to call snapToStep(.)
}


void setInitialDecNr()
{
    StepInterval<float> intv = intvfld_->getFStepInterval();
    const int nrdecstart = getInitialDec( intv.start_ );
    const int nrdecstop = getInitialDec( intv.start_ );
    const int initnrdec = nrdecstart>nrdecstop ? nrdecstart : nrdecstop;
    intvfld_->setNrDecimals( initnrdec, 0 );
}


void finalizeCB( CallBacker* cb )
{
    setInitialDecNr();
    int nrdec=0;
    const float startval = getNiceNumber( contourintv_.start_, nrdec );
    intvfld_->setText( toString(startval,0,'f',nrdec), 0 );
    const float stopval = getNiceNumber( contourintv_.stop_, nrdec );
    intvfld_->setText( toString(stopval,0,'f',nrdec), 1 );
    intvChanged( nullptr );
}


void intvChanged( CallBacker* cb )
{
    StepInterval<float> intv = intvfld_->getFStepInterval();
    if ( intv.start_ < rg_.start_ || intv.start_ > rg_.stop_ )
    {
        intvfld_->setValue( Math::Ceil(rg_.start_), 0 );
        intv.start_ = Math::Ceil(rg_.start_);
    }
    if ( intv.stop_ < rg_.start_ || intv.stop_ > rg_.stop_ )
    {
        intvfld_->setValue( Math::Floor(rg_.stop_), 1 );
        intv.stop_ =  Math::Floor(rg_.stop_);
    }

    bool invalidstep = ( intv.step_ <= 0 || intv.step_ > rg_.width() ) ?
			true : false;
    if( invalidstep )
    {
        intvfld_->setValue( contourintv_.step_, 2 );
	if (cb)
	    return uiMSG().error(tr("Invalid step value"));
    }

    intv = intvfld_->getFStepInterval();
    if ( iszval_ )
	intv.scale( 1.0f/zfac_ );

    contourintv_.step_ = intv.step_;
    const float steps = intv.nrSteps();
    float nrsteps = intv.nrfSteps();
    const float eps = 1.0e-02;

    if ( mIsEqual(nrsteps,steps,eps) )
	nrsteps = steps;
    else
	nrsteps = Math::Floor( nrsteps );

    uiString txt( tr("Number of contours: %1").arg(nrsteps+1) );
    toStatusBar( txt );
}

    Interval<float>	rg_;
    StepInterval<float>	contourintv_;
    uiGenInput*		intvfld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		showlblsfld_;
    uiPushButton*	fontfld_;
    uiButtonGroup*	alignbutsfld_;
    uiLabel*		alignlblfld_;
    uiLabeledSpinBox*	elevationfld_;
    uiLabel*		degreeslblfld_;
    FontData		fontdata_;
    float		zfac_;
    bool		iszval_;
};


void uiContourTreeItem::initClass()
{
    uiODDataTreeItem::factory().addCreator( create, nullptr );
}

uiContourTreeItem::uiContourTreeItem( const char* parenttype )
    : uiODDataTreeItem(parenttype)
    , color_(0,0,0)
    , zshift_(mUdf(float))
    , showlabels_(true)
    , linewidth_(1)
    , lines_(nullptr)
    , drawstyle_(nullptr)
    , material_(nullptr)
    , labels_(nullptr)
    , contoursteprange_(mUdf(float),-mUdf(float))
    , optionsmenuitem_(m3Dots(uiStrings::sProperties()))
    , areamenuitm_(tr("Contour areas"))
    , propdlg_(nullptr)
{
    optionsmenuitem_.iconfnm = "disppars";
    areamenuitm_.iconfnm = "contourarea";
}


uiContourTreeItem::~uiContourTreeItem()
{
    detachAllNotifiers();
    delete propdlg_;

    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if ( hordisp )
	hordisp->getMovementNotifier()->remove(
		 mCB(this,uiContourTreeItem,checkCB));

    if ( lines_ || drawstyle_ )
    {
	pErrMsg("prepareForShutdown not run");
    }
}


int uiContourTreeItem::getNrContours() const
{
    return contourintv_.nrSteps() + 1;
}


void uiContourTreeItem::prepareForShutdown()
{
    visClosingCB( nullptr );
}


void uiContourTreeItem::visClosingCB( CallBacker* )
{
    removeAll();
}


uiString uiContourTreeItem::createDisplayName() const
{
    return tr( "Contours (%1)" ).arg( attrnm_ );
}


bool uiContourTreeItem::init()
{
    if ( !uiODDataTreeItem::init() )
	return false;

    uitreeviewitem_->setChecked( true );
    zshift_ = (float)applMgr()->visServer()->getTranslation( displayID() ).z_;
    mAttachCB( parent_->checkStatusChange(), uiContourTreeItem::checkCB );
    mAttachCB( ODMainWin()->applMgr().visServer()->removeAllNotifier(),
					    uiContourTreeItem::visClosingCB );

    return true;
}


uiODDataTreeItem* uiContourTreeItem::create( const Attrib::SelSpec& as,
					     const char* parenttype )
{
    BufferString defstr = as.defString();
    if ( defstr != sKeyContourDefString() )
	return nullptr;

    BufferString zkeystr = as.zDomainKey();
    // for old session file
    if ( zkeystr.isEmpty() )
	zkeystr = sKeyZValue();

    uiContourTreeItem* ctitem = new uiContourTreeItem( parenttype );
    if ( ctitem )
	ctitem->setAttribName( zkeystr );

    return ctitem ? ctitem : nullptr;
}


void uiContourTreeItem::checkCB(CallBacker*)
{
    bool newstatus = uitreeviewitem_->isChecked();
    if ( newstatus && parent_ )
	newstatus = parent_->isChecked();

    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if ( !hordisp )
	return;

    const bool display = newstatus && hordisp &&
				!hordisp->displayedOnlyAtSections();

    if ( lines_ )
	lines_->turnOn( display );

    if ( labels_ )
	labels_->turnOn( display && showlabels_ );

    updateZShift();
}


bool uiContourTreeItem::doubleClick( uiTreeViewItem* item )
{
    if ( item != uitreeviewitem_ )
	return uiTreeItem::doubleClick( item );

    if ( !select() )
	return false;

    showPropertyDlg();
    return true;
}


void uiContourTreeItem::removeAll()
{
    if ( lines_ )
    {
	applMgr()->visServer()->removeObject( lines_.ptr(), sceneID());
	lines_ = nullptr;
    }

    drawstyle_ = nullptr;
    material_ = nullptr;
    removeLabels();
}


void uiContourTreeItem::removeLabels()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( labels_ )
    {
	visserv->removeObject( labels_.ptr(), sceneID());
	labels_ = nullptr;
    }
}


void uiContourTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDataTreeItem::createMenu( menu, istb );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_,
		      &optionsmenuitem_, lines_, false );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_,
		     &areamenuitm_, lines_ && areas_.size(), false );
}

const uiString areaString()
{
    return od_static_tr("areaString",
		    SI().xyInFeet() ? "Area (sqft)" : "Area (m^2)");
}


void uiContourTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() || !lines_ )
	return;

    if ( mnuid==optionsmenuitem_.id )
    {
	menu->setIsHandled( true );
	showPropertyDlg();
    }
    if ( mnuid==areamenuitm_.id )
    {
	menu->setIsHandled( true );

	TypeSet<float> zvals, areas;
	getZVSAreaValues( zvals, areas );

	uiDialog dlg( ODMainWin(), uiDialog::Setup(tr("Contour areas"),
						   mNoDlgTitle,mNoHelpKey ) );
	dlg.setCancelText( uiString::emptyString() );

	RefMan<visSurvey::Scene> scene =
			applMgr()->visServer()->getScene( sceneID() );
	if ( !scene )
	{
	    pErrMsg("No scene");
	    return;
	}

	const ZDomain::Info& zinfo = scene->zDomainInfo();

	uiTable* table = new uiTable( &dlg, uiTable::Setup(areas.size(),2),
				      "Area table");

	table->setColumnLabel( 0, zinfo.getLabel() );
	table->setColumnLabel( 1, areaString() );

	for ( int idx=0; idx<areas.size(); idx++ )
	{
	    table->setText( RowCol(idx,0),
			    toString( zvals[idx] * zinfo.userFactor() ) );
	    table->setText( RowCol(idx,1), toString( areas[idx] ) );
	}

	auto* button = uiButton::getStd( &dlg, OD::SaveAs,
			     mCB(this,uiContourTreeItem,saveAreasAsCB), true );
	button->attach( leftAlignedBelow, table );

	dlg.go();
    }
}


void uiContourTreeItem::getZVSAreaValues( TypeSet<float>& zvals,
					  TypeSet<float>& areas ) const
{
    for ( int idx=0; idx<areas_.size(); idx++ )
    {
	if ( !mIsUdf(areas_[idx]))
	{
	    zvals += contourintv_.atIndex(idx);
	    areas += (float) areas_[idx];
	}
    }
}


void uiContourTreeItem::saveAreasAsCB(CallBacker*)
{
    uiFileDialog dlg( ODMainWin(), 0, "*.txt",
		      "Select file to store contour areas" );
    if ( !dlg.go() )
	return;

    od_ostream stream( dlg.fileName() );
    RefMan<visSurvey::Scene> scene =
			    applMgr()->visServer()->getScene( sceneID() );
    if ( !scene )
    {
	pErrMsg("No scene");
	return;
    }

    const ZDomain::Info& zinfo = scene->zDomainInfo();

    stream << zinfo.getLabel().getFullString()
	   << od_tab << areaString() << od_newline;

    TypeSet<float> zvals, areas;
    getZVSAreaValues( zvals, areas );

    for ( int idx=0; idx<areas.size(); idx++ )
    {
	stream << zvals[idx] * zinfo.userFactor() << od_tab
	       << areas[idx] << od_newline;
    }

    if ( stream.isBad() )
    {
	uiString errmsg = tr("Could not save file %1");

	if ( !stream.errMsg().getFullString().isEmpty() )
	    stream.addErrMsgTo( errmsg );
	else
	    errmsg.arg(".");

	uiMSG().error( errmsg );
    }
    else
    {
	uiMSG().message(tr("Area table saved as %1.").arg(dlg.fileName()));
    }
}


void uiContourTreeItem::showPropertyDlg()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    zshift_ = (float)visserv->getTranslation( displayID() ).z_;

    Interval<float> range( contoursteprange_.start_ + zshift_,
                           contoursteprange_.stop_ + zshift_ );

    StepInterval<float> oldintv( contourintv_ );
    oldintv += Interval<float>( zshift_, zshift_ );

    OD::LineStyle ls( OD::LineStyle::Solid, linewidth_, color_ );
    delete propdlg_;
    propdlg_ = new uiContourParsDlg( ODMainWin(), attrnm_, range,
				     oldintv, ls, sceneID() );
    if ( labels_ )
    {
	propdlg_->setShowLabels( labels_->isOn() );

	if ( labels_->nrTexts() )
	{
	    propdlg_->setFontData( labels_->text(0)->getFontData() );
	    propdlg_->setLabelAlignment( (visBase::Text::Justification)
				   labels_->text(0)->getJustification() );
	}
    }

    mAttachCB( propdlg_->propertyChanged, uiContourTreeItem::propChangeCB );
    mAttachCB( propdlg_->intervalChanged, uiContourTreeItem::intvChangeCB );
    propdlg_->go();
}


void uiContourTreeItem::updateUICContours( const StepInterval<float>& newintv )
{
    StepInterval<float> oldintv = contourintv_;
    const int nrsignificant = getInitialDec( newintv.start_ );
    const float eps = Math::PowerOf( 10.f, -mCast(float,nrsignificant) );
    oldintv += Interval<float>( zshift_, zshift_ );
    const bool intvchgd = !mIsEqual(newintv.start_,oldintv.start_,eps) ||
                          !mIsEqual(newintv.stop_,oldintv.stop_,eps) ||
                          !mIsEqual(newintv.step_,oldintv.step_,eps);

    if ( intvchgd )
    {
	contourintv_ = newintv;
	contourintv_ += Interval<float>( -zshift_, -zshift_ );
	startCreateUICContours();
    }
}


void uiContourTreeItem::intvChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg )
	return;

    StepInterval<float> newintv = dlg->getContourInterval();
    updateUICContours( newintv );
}


void uiContourTreeItem::propChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg || !lines_ )
	return;

    OD::LineStyle ls( dlg->getLineStyle() );
    drawstyle_->setLineStyle( ls );
    material_->setColor( ls.color_ );
    color_ = ls.color_;
    linewidth_ = ls.width_;

    if ( labels_ && lines_ )
    {
	showlabels_ = dlg->showLabels();
	labels_->turnOn( lines_->isOn() && showlabels_ );
	labels_->setFontData( dlg->getFontData() );
	const Coord3 axis ( 0, 0, 1 );
	const float rotangrad = Math::toRadians( dlg->getRotation() );
	for ( int idx=0; idx<labels_->nrTexts(); idx++ )
	{
	    labels_->text(idx)->setRotation( rotangrad, axis );
	    labels_->text(idx)->setJustification(
		    (visBase::Text::Justification) dlg->getLabelAlignment() );
	    labels_->text(idx)->setColor( color_ );
	}
    }
}


void uiContourTreeItem::startCreateUICContours()
{
    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if( !hordisp )
	return;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    const Array2D<float>*  field = getDataSet( hordisp );

    if ( !field || !createPolyLines() || !computeUICContourSteps( *field ) )
	return;

    bool showcontours = lines_->turnOn( false );
    bool showlabels = labels_ && labels_->turnOn( false );

    uiContourTreeItemContourGenerator ctrgtr ( this, field );
    uiTaskRunner taskrunner( ODMainWin() );
    if ( taskrunner.execute( ctrgtr ) )
    {
	setLabels( ctrgtr.getLabels() );
	lines_->turnOn( showcontours );
	if ( labels_ )
	    labels_->turnOn( showlabels );

	areas_ = ctrgtr.getAreas();

	bool validfound = false;
	for ( int idx=0; idx<areas_.size(); idx++ )
	{
	    if ( !mIsUdf(areas_[idx]))
	    {
		validfound = true;
		break;
	    }
	}

	if ( !validfound )
	    areas_.erase();
   }
    else
    {
	areas_.erase();
	removeLabels();
	removeOldUICContoursFromScene();
    }

    if ( hordisp->getZAxisTransform() )
	delete field;

    hordisp->getMovementNotifier()->notifyIfNotNotified(
	mCB(this,uiContourTreeItem,checkCB) );
}


Array2D<float>* uiContourTreeItem::getDataSet(
					visSurvey::HorizonDisplay* hordisp )
{
    EM::ObjectID emid = hordisp->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::EMM().getObject(emid));
    if ( !hor )
	return nullptr;

    if ( attrnm_ == uiContourTreeItem::sKeyZValue() )
    {
	Array2D<float>* arr=hor->geometry().geometryElement()->getArray();
	if ( hordisp->getZAxisTransform() )
	    arr = hor->createArray2D( hordisp->getZAxisTransform() );

	return arr;
    }

    const int dataid=applMgr()->EMServer()->loadAuxData( hor->id(),attrnm_);
    Array2D<float>* arr = hor->auxdata.createArray2D( dataid );
    return arr;
}


bool uiContourTreeItem::createPolyLines()
{
    if ( lines_ )
    {
	removeOldUICContoursFromScene();
	return true;
    }

    lines_ = visBase::PolyLine::create();
    if ( !lines_ )
	return false;

    lines_->setPickable( false, false );
    applMgr()->visServer()->addObject( lines_.ptr(), sceneID(), false );

    if ( !drawstyle_ )
    {
	drawstyle_ = visBase::DrawStyle::create();
	lines_->addNodeState( drawstyle_.ptr() );
    }

    if ( !material_ )
    {
	material_ = visBase::Material::create();
	material_->setColor( color_ );
	lines_->setMaterial( material_.ptr() );
    }

    return true;
}


bool uiContourTreeItem::setLabels( visBase::Text2* newlabels )
{
    if ( !material_ )
	return false;

    removeLabels();
    labels_ = newlabels;
    if ( !labels_ )
	return false;

    applMgr()->visServer()->addObject( labels_.ptr(), sceneID(), false );
    labels_->setMaterial( material_.ptr() );

    for ( int idx=0; idx<labels_->nrTexts(); idx++ )
	labels_->text(idx)->setColor( color_ );

    return true;
}


bool uiContourTreeItem::computeUICContourSteps( const Array2D<float>& field )
{
    if ( mIsUdf(contoursteprange_.start_) )
    {
	for ( int idx=0; idx<field.info().getSize(0); idx++ )
	{
	    for ( int idy=0; idy<field.info().getSize(1); idy++ )
	    {
		const float val = field.get( idx, idy );
		if ( !mIsUdf(val) ) contoursteprange_.include( val, false );
	    }
	}

        if ( mIsUdf(contoursteprange_.start_) )
	    return false;

	AxisLayout<float> al( contoursteprange_ );
	SamplingData<float> sd = al.sd_;
        sd.step_ /= 5;
	const float offset = (sd.start_-contoursteprange_.start_) / sd.step_;
	if ( offset < 0 || offset > 1 )
	{
	    const int nrsteps = mNINT32( Math::Floor(offset) );
            sd.start_ -= nrsteps * sd.step_;
	}

        contourintv_.start_ = sd.start_;
        contourintv_.stop_ = contoursteprange_.stop_;
        contourintv_.step_ = sd.step_;
	const int nrsteps = contourintv_.nrSteps();
        contourintv_.stop_ = sd.start_ + nrsteps*sd.step_;
    }

    if ( contourintv_.step_ <= 0 )
	return false;

    return true;
}


void uiContourTreeItem::removeOldUICContoursFromScene()
{
   if ( lines_ )
       lines_->removeAllPrimitiveSets();
}


visSurvey::HorizonDisplay* uiContourTreeItem::getHorDisp()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hordisp,
		    visserv->getObject(displayID()))
    return hordisp;
}


void uiContourTreeItem::updateColumnText( int col )
{
    uiODDataTreeItem::updateColumnText( col );
    if ( !col && !lines_ )
	startCreateUICContours();

    uiVisPartServer* visserv = applMgr()->visServer();
    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if ( !hordisp || !lines_ || !labels_ )
	return;

    const bool solomode = visserv->isSoloMode();
    const bool turnon = !hordisp->displayedOnlyAtSections() &&
			( (solomode && hordisp->isOn())
			|| (!solomode && hordisp->isOn() && isChecked()) );
    lines_->turnOn( turnon );
    labels_->turnOn( turnon && showlabels_ );
}


void uiContourTreeItem::updateZShift()
{
    if ( !lines_ || mIsUdf(zshift_) )
	return;

    Coord3 trans = applMgr()->visServer()->getTranslation( displayID() );

    const float deltaz = (float) (trans.z_ - zshift_);
    if ( !deltaz )
	return;

    for ( int idx=0; idx<lines_->getCoordinates()->size(true); idx++ )
    {
	if ( lines_->getCoordinates()->isDefined(idx) )
	{
	    Coord3 pos = lines_->getCoordinates()->getPos( idx );
            pos.z_ += deltaz;
	    lines_->getCoordinates()->setPos( idx, pos );
	}
    }

    lines_->dirtyCoordinates();

    if ( labels_ )
    {
	for ( int idx=0; idx<labels_->nrTexts(); idx++ )
	{
	    Coord3 pos = labels_->text(idx)->getPosition();
            pos.z_ += deltaz;
	    labels_->text(idx)->setPosition( pos );
	    BufferString txt = labels_->text(idx)->getText().getFullString();
	    float labelval = txt.toFloat();
	    labelval += deltaz * SI().zDomain().userFactor();
	    labels_->text(idx)->setText( toUiString( (int)labelval ) );
	}
    }

    zshift_ = (float) trans.z_;
}


BufferString uiContourTreeItem::selectAttribute( uiParent* p,
						 const MultiID& mid)
{
    const EM::IOObjInfo eminfo( mid );
    BufferStringSet attrnms;
    attrnms.add( sKeyZValue() );
    eminfo.getAttribNames( attrnms );
    if ( attrnms.size() == 1 )
	return sKeyZValue();

    uiDialog dlg( p, uiDialog::Setup(tr("Select Attribute to contour"),
				     mNoDlgTitle,mNoHelpKey) );
    uiListBox::Setup su( OD::ChooseOnlyOne, toUiString(eminfo.name()),
			 uiListBox::AboveMid );
    auto* attrlb = new uiListBox( &dlg, su, "horizondata" );
    attrlb->setHSzPol( uiObject::Wide );
    attrlb->addItems( attrnms );
    return dlg.go() ? attrlb->getText() : nullptr;
}
