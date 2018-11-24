/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Feb 2009
________________________________________________________________________

-*/


#include "uiodcontourtreeitem.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "isocontourtracer.h"
#include "axislayout.h"
#include "math.h"
#include "polygon.h"
#include "survinfo.h"
#include "od_ostream.h"
#include "uistrings.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uiempartserv.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uiodapplmgr.h"
#include "uioddisplaytreeitem.h"
#include "uiodscenemgr.h"
#include "uisellinest.h"
#include "uistatusbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uitaskrunner.h"
#include "uitable.h"
#include "uifileselector.h"
#include "uifont.h"
#include "uibuttongroup.h"
#include "uispinbox.h"

#include "viscoord.h"
#include "visdrawstyle.h"
#include "vishorizondisplay.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"
#include "viscoord.h"
#include "zaxistransform.h"
#include "executor.h"
#include "od_helpids.h"

static const int cMinNrNodesForLbl = 25;
static const int cMaxNrDiplayedLabels = 1000;

const char* uiODContourTreeItem::sKeyContourDefString()
					      { return "Contour Display"; }

const char* uiODContourTreeItem::sKeyZValue()	     { return "Z Values"; }

static int getInitialDec( const float val )
{
    const float logval = Math::Log10( val>0 ? val : -val );
    if ( mIsUdf(logval) )
	return 0;

    const int nrdigits = mNINT32(Math::Ceil(logval));
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

    const int nrdigits = mNINT32(Math::Ceil(logval));
    if ( nrdigits > 6 )
	return mCast(float, mNINT32(val/1e6) * 1e6);

    const float multiplier = Math::PowerOf( 10.f,
					    (float)(6-nrdigits) );
    const int rounded = mNINT32(val * multiplier);
    nrdec = 6-nrdigits;
    int divider = 10;
    while ( rounded % divider == 0 )
    {
	nrdec--;
	divider *= 10;
    }

    if ( nrdec < 0 ) nrdec = 0;
    return mCast(float, rounded / multiplier);
}


class uiODContourTreeItemContourData
{ mODTextTranslationClass(uiODContourTreeItemContourData);
public:

    TypeSet<Coord3>			   contourcoords_;
    TypeSet< Interval<int> >		   contourcoordrgs_;
    ObjectSet<Geometry::RangePrimitiveSet> contourprimitivesets_;
    TypeSet<Coord3>			   labelpositions_;
    TypeSet<int>			   labelcontourlen_;
    TypeSet<BufferString>		   labels_;
    TypeSet<Interval<int> >		   labelranges_;
};


class uiODContourTreeItemContourGenerator : public ParallelTask
{ mODTextTranslationClass(uiODContourTreeItemContourGenerator);
public:

				uiODContourTreeItemContourGenerator(
					uiODContourTreeItem* p,
					const Array2D<float>* field);
				~uiODContourTreeItemContourGenerator()
				{ if ( labels_ ) labels_->unRef(); }

    visBase::Text*		getLabels() { return labels_; }
    const TypeSet<double>&	getAreas() const { return areas_; }
    uiString			nrDoneText() const;

protected:
    bool	doPrepare(int);
    bool	doWork( od_int64 start, od_int64 stop, int );
    bool	doFinish( bool success );
    od_int64	nrIterations() const;

private:

    bool prepForContourGenerator();
    bool setRowColRgs(visSurvey::HorizonDisplay*);

    bool generateContours(int contouridx,const IsoContourTracer*,
			   uiODContourTreeItemContourData&,double& area) const;
    bool addDisplayCoord(const ODPolygon<float>& inputcountour, int vrtxidx,
		       uiODContourTreeItemContourData&,int& lastvrtxidx) const;
    void makeContourClose(uiODContourTreeItemContourData&,
			  Interval<int>& coordsrg) const;
    void addContourData(uiODContourTreeItemContourData&);
    void addContourLabel(const Coord3& pos, const uiString& lbl);
    float getHorizonZValue(int rowidx,int colidx) const;

    int						nrcontours_;
    Threads::Atomic<od_int64>			totalnrshapes_;
    uiODContourTreeItem*				uicitem_;
    Threads::Mutex				mutex_;
    uiODContourTreeItemContourData		contourdata_;
				// from construction source
    const Array2D<float>*	field_;

    visBase::Text*		labels_;

    StepInterval<int>		rowrg_;
    StepInterval<int>		colrg_;
    float			zfactor_;
    const EM::Horizon3D*	hor3d_;
    const ZAxisTransform*	ztransform_;
    const mVisTrans*		displaytrans_;
    TypeSet<double>		areas_;

    bool			isfinishing_;
};


uiODContourTreeItemContourGenerator::uiODContourTreeItemContourGenerator(
			uiODContourTreeItem* p, const Array2D<float>* field )
    : nrcontours_( p->contourintv_.nrSteps() )
    , uicitem_( p )
    , field_( field )
    , zfactor_( 0 )
    , labels_( 0 )
    , isfinishing_( false )
{
    totalnrshapes_ = nrcontours_;
    areas_.setSize( nrcontours_ );
    setName( "Generating contours" );
}


uiString uiODContourTreeItemContourGenerator::nrDoneText() const
{
    return isfinishing_ ? tr("Contour elements added")
			: tr("Contours computed");
}


od_int64 uiODContourTreeItemContourGenerator::nrIterations() const
{
    return !isfinishing_ ? nrcontours_
			 : contourdata_.contourprimitivesets_.size() +
					contourdata_.labelpositions_.size();
}


bool uiODContourTreeItemContourGenerator::doPrepare( int )
{
    if( !setRowColRgs( uicitem_->getHorDisp() ) ||
	!prepForContourGenerator() )
	return false;
    return true;
}


bool uiODContourTreeItemContourGenerator::setRowColRgs(
			    visSurvey::HorizonDisplay* hordisp )
{
    rowrg_.set( 0, 0, 0 );
    colrg_.set( 0, 0, 0 );
    rowrg_ = hordisp->geometryRowRange();
    colrg_ = hordisp->geometryColRange();
    return ( !rowrg_.width() || !colrg_.width() ) ? false : true;
}


bool uiODContourTreeItemContourGenerator::prepForContourGenerator()
{
    uiVisPartServer* visserv = uicitem_->applMgr()->visServer();
    DBKey emid = uicitem_->getHorDisp()->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::MGR().getObject(emid));
    if( !hor ) return false;
    hor3d_ = hor;
    mDynamicCastGet(
	visSurvey::Scene*,scene, visserv->getObject( uicitem_->sceneID() ) );
    ztransform_ = scene ? scene->getZAxisTransform() : 0;
    zfactor_ = mCast( float, scene->zDomainInfo().userFactor() );
    displaytrans_ =
	uicitem_->lines_->getCoordinates()->getDisplayTransformation();

    return true;
}


bool uiODContourTreeItemContourGenerator::doWork( od_int64 start,od_int64 stop,
						 int )
{
    PtrMan<IsoContourTracer> ictracer =  new IsoContourTracer( *field_ );
    if( !ictracer ) return false;
    ictracer->setSampling( rowrg_, colrg_ );

    uiODContourTreeItemContourData newcontourdata;
    for ( int contouridx=mCast(int,start); contouridx<=stop; contouridx++ )
    {
	const int nrshapesbefore = newcontourdata.contourcoordrgs_.size() +
				   newcontourdata.labels_.size();
	double area;
	generateContours( contouridx, ictracer,newcontourdata, area );
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


void uiODContourTreeItemContourGenerator::addContourData(
	 uiODContourTreeItemContourData& newcontourdata )
{
    const int contourcoordsz = contourdata_.contourcoords_.size();
    const Interval<int> lastcoordidxrg ( contourcoordsz, contourcoordsz-1 );
    contourdata_.contourcoords_.append( newcontourdata.contourcoords_);
    contourdata_.labelpositions_.append( newcontourdata.labelpositions_ );
    contourdata_.labelcontourlen_.append( newcontourdata.labelcontourlen_ );
    contourdata_.labels_.append( newcontourdata.labels_ );

    for ( int idx=0; idx<newcontourdata.contourcoordrgs_.size(); idx++ )
    {
	Geometry::RangePrimitiveSet* ps = Geometry::RangePrimitiveSet::create();
	if ( !ps ) continue;

	ps->setRange( newcontourdata.contourcoordrgs_[idx] + lastcoordidxrg );
	contourdata_.contourprimitivesets_.push( ps );
    }

    if ( contourdata_.labelranges_.size() )
    {
	const int lastlblidx =
	    contourdata_.labelranges_[contourdata_.labelranges_.size()-1].stop;
	for ( int idx=0; idx<newcontourdata.labelranges_.size(); idx++ )
	    newcontourdata.labelranges_[idx] +=
	    Interval<int>(lastlblidx,lastlblidx);
    }

    contourdata_.labelranges_.append( newcontourdata.labelranges_);

}


bool uiODContourTreeItemContourGenerator::generateContours( int contouridx,
				const IsoContourTracer* ictracer,
				uiODContourTreeItemContourData& contourdata,
				double& area )const
{
    area = 0;
    const float contourval = uicitem_->contourintv_.start +
		       contouridx* uicitem_->contourintv_.step;

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
				  contourcoordsrg.stop) )
		continue;
	    // if meet the condition adding contour label position
	    if ( curcontour.size()>cMinNrNodesForLbl &&
		 vertexidx==curcontour.size()/2 )
	    {
		const int lastvrtxidx = contourdata.contourcoords_.size() - 1;
		const Coord3 lblpos = contourdata.contourcoords_[lastvrtxidx];
		contourdata.labelpositions_.add( lblpos );
		contourdata.labelcontourlen_.add( curcontour.size() );
		lblpositionrg.stop++;
	    }
	}
	if ( curcontour.isClosed() )
	    makeContourClose( contourdata,contourcoordsrg );
	contourdata.contourcoordrgs_.add( contourcoordsrg );
    }
    // if having label, add label into contour data
    if ( lblpositionrg.stop > lblpositionrg.start )
    {
	float labelval =
	    uicitem_->attrnm_==uiODContourTreeItem::sKeyZValue()
	    ? (contourval+uicitem_->zshift_) * zfactor_ : contourval;
	contourdata.labelranges_.add( lblpositionrg );
	int nrdec = 0;
	labelval = getNiceNumber( labelval, nrdec );

	{
	    BufferString str(labelval); str.addNewLine().add("|");
	    /*added line break and 'pipe' so as to cause the labels to be
	    'lifted' and now easily seen on countours for non-flat horizons*/

	    contourdata.labels_.add( str );
	}
    }

    return true;
}


void uiODContourTreeItemContourGenerator::makeContourClose(
    uiODContourTreeItemContourData& contourdata,
    Interval<int>& coordsrg ) const
{
    if ( contourdata.contourcoords_.size() <=0 ) return;
    contourdata.contourcoords_.add(
	contourdata.contourcoords_[coordsrg.start] );
    coordsrg.stop++;
}


float uiODContourTreeItemContourGenerator::getHorizonZValue( int rowidx,
							   int colidx ) const
{
    const BinID bid( rowrg_.atIndex(rowidx), colrg_.atIndex(colidx) );
    const float zval = hor3d_ ? hor3d_->getZ(bid) : mUdf(float);

    if ( ztransform_ && !mIsUdf(zval) )
	return ztransform_->transformTrc( TrcKey(bid), zval );

    return zval;
}


bool uiODContourTreeItemContourGenerator::addDisplayCoord(
    const ODPolygon<float>& inputcontour, int vrtxidx,
    uiODContourTreeItemContourData& contourdata,
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
    vrtxcoord.setXY( SI().binID2Coord().transform( Coord(vrtx.x_,vrtx.y_) ) );

    vrtxcoord.z_ = mIsUdf(z0) ? z1 : (mIsUdf(z1) ? z0 : (1.0-frac)*z0+frac*z1);
    if ( mIsUdf(vrtxcoord.z_) )
	return false;

    vrtxcoord.z_ += uicitem_->zshift_;

    visBase::Transformation::transform( displaytrans_, vrtxcoord, vrtxcoord );
    contourdata.contourcoords_.add( vrtxcoord );
    lastvrtxidx++;
    return true;
}


bool uiODContourTreeItemContourGenerator::doFinish( bool success )
{
    resetNrDone();
    isfinishing_ = true;

    if ( !success ) return false;

    if ( labels_ ) labels_->unRef();
    labels_ = visBase::Text::create();
    labels_->ref();
    labels_->setDisplayTransformation( displaytrans_ );
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
	for ( int ipos=lbldata.start; ipos<lbldata.stop; ipos++ )
	{
	    if ( contourdata_.labelcontourlen_[ipos] > contourlenthreshold )
	    {
		addContourLabel( contourdata_.labelpositions_[ipos],
				 toUiString(contourdata_.labels_[lbrgidx]) );
	    }
	}

	addToNrDone( 1 );
	if ( !shouldContinue() )
	    return false;
    }

    return true;
}


void uiODContourTreeItemContourGenerator::addContourLabel(
    const Coord3& pos, const uiString& lbl )
{
    if ( !labels_ ) return;

    const int idx = labels_->addText();
    visBase::TextDrawable* label = labels_->text( idx );
    if ( label )
    {
	uiString labelonpole( lbl );
	label->setText( labelonpole );
	label->setJustification( visBase::TextDrawable::BottomLeft );
	label->setPosition( pos, true );
	label->setFontData( FontData(18), labels_->getPixelDensity() );
    }
}


class uiContourParsDlg : public uiDialog
{ mODTextTranslationClass(uiContourParsDlg);
public:

uiContourParsDlg( uiParent* p, const char* attrnm, const Interval<float>& rg,
		  const StepInterval<float>& intv, const OD::LineStyle& ls,
		  int sceneid )
    : uiDialog(p,Setup(tr("Contour Display Options"),mNoDlgTitle,
		    mODHelpKey(mContourParsDlgHelpID) )
		    .nrstatusflds(1))
    , rg_(rg)
    , contourintv_(intv)
    , propertyChanged(this)
    , intervalChanged(this)
    , iszval_(FixedString(uiODContourTreeItem::sKeyZValue()) == attrnm)
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid))

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
    mAddZUnitStr(lbltxt); lbltxt.appendPlainText( ": " );
    BufferString lbltxtapnd;
    if ( iszval_ )
	lbltxtapnd.add( rg_.start ).add( " - " ).add( rg_.stop );
    else
	lbltxtapnd.add( rg_.start ).add( " - " ).add( rg_.stop );

    uiLabel* lbl = new uiLabel(this, lbltxt.appendPlainText(lbltxtapnd, true));

    lbltxt = tr("Contour Range").addSpace();
    mAddZUnitStr(lbltxt);
    intvfld_ = new uiGenInput(this,lbltxt,FloatInpIntervalSpec(contourintv_));
    intvfld_->valuechanged.notify( mCB(this,uiContourParsDlg,intvChanged) );
    intvfld_->attach( leftAlignedBelow, lbl );

    uiButton* applybut = uiButton::getStd( this, OD::Apply,
				mCB(this,uiContourParsDlg,applyCB), true);
    applybut->attach( rightTo, intvfld_ );

    uiSelLineStyle::Setup lssu; lssu.drawstyle(false);
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, intvfld_ );
    lsfld_->changed.notify( mCB(this,uiContourParsDlg,dispChanged) );

    showlblsfld_ = new uiCheckBox( this, tr("Show labels") );
    showlblsfld_->activated.notify( mCB(this,uiContourParsDlg,uiDisplayCB) );
    showlblsfld_->attach( alignedBelow, lsfld_ );

    fontfld_ = new uiPushButton( this, uiStrings::sFont(),
			    mCB(this,uiContourParsDlg,selectFontCB), false );
    fontfld_->attach( rightOf, showlblsfld_ );

    alignbutsfld_ = new uiButtonGroup( this, "OD::Alignment buttons",
				       OD::Horizontal );
    alignbutsfld_->attach( alignedBelow, showlblsfld_ );

    alignlblfld_ = new uiLabel( this, tr("Label alignment") );
    alignlblfld_->attach( leftOf, alignbutsfld_ );

    uiRadioButton* leftbut = new uiRadioButton( alignbutsfld_,
							   uiStrings::sLeft() );
    leftbut->activated.notify( mCB(this,uiContourParsDlg,dispChanged) );
    uiRadioButton* centerbut = new uiRadioButton( alignbutsfld_,
						  uiStrings::sCenter() );
    centerbut->activated.notify( mCB(this,uiContourParsDlg,dispChanged) );
    uiRadioButton* rightbut = new uiRadioButton( alignbutsfld_,
							  uiStrings::sRight() );
    rightbut->activated.notify( mCB(this,uiContourParsDlg,dispChanged) );
    alignbutsfld_->selectButton( 0 );

    elevationfld_ = new uiLabeledSpinBox( this, tr("Label elevation") );
    elevationfld_->attach( alignedBelow, alignbutsfld_ );

    degreeslblfld_ = new uiLabel( this, toUiString("(deg)") );
    degreeslblfld_->attach( rightOf, elevationfld_ );

    elevationfld_->box()->setSpecialValueText( "Off" );
    elevationfld_->box()->setInterval( -5, 75, 5 );
    elevationfld_->box()->doSnap( true );
    elevationfld_->box()->valueChanging.notify(
				mCB(this,uiContourParsDlg,elevationChg) );
    elevationfld_->box()->valueChanged.notify(
				mCB(this,uiContourParsDlg,uiDisplayCB) );
    disableLabelElevation();

    postFinalise().notify( mCB( this,uiContourParsDlg,finaliseCB) );
}


const OD::LineStyle& getLineStyle() const
{ return lsfld_->getStyle(); }


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
    uiDisplayCB( 0 );
}


bool showLabels() const
{ return showlblsfld_->isChecked(); }


void setFontData( const FontData& fontdata )
{ fontdata_ = fontdata; }


const FontData& getFontData() const
{ return fontdata_; }


void setLabelAlignment( visBase::TextDrawable::Justification alignment )
{
    if ( alignment == visBase::TextDrawable::BottomRight )
	alignbutsfld_->selectButton( 2 );
    else if ( alignment == visBase::TextDrawable::Bottom )
	alignbutsfld_->selectButton( 1 );
    else
	alignbutsfld_->selectButton( 0 );
}


int getLabelAlignment() const
{
    if ( alignbutsfld_->selectedId() == 1 )
	return visBase::TextDrawable::Bottom;
    if ( alignbutsfld_->selectedId() > 1 )
	return visBase::TextDrawable::BottomRight;

    return visBase::TextDrawable::BottomLeft;
}


void disableLabelElevation()
{ elevationfld_->box()->setValue( elevationfld_->box()->minValue() ); }


bool isLabelElevationDisabled() const
{
    return elevationfld_->box()->getIntValue() ==
					elevationfld_->box()->minValue();
}


void setLabelElevation( float angle )
{
    elevationfld_->box()->setValue( mNINT32(angle*180.0/M_PI) );
    elevationChg( 0 );
}


float getLabelElevation() const
{ return mCast( float, elevationfld_->box()->getIntValue()*M_PI/180.0 ); }


    Notifier<uiContourParsDlg>	propertyChanged;
    Notifier<uiContourParsDlg>	intervalChanged;

protected:

void applyCB( CallBacker* )
{ intervalChanged.trigger(); }


void dispChanged( CallBacker* )
{ propertyChanged.trigger(); }


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
    const int nrdecstart = getInitialDec( intv.start );
    const int nrdecstop = getInitialDec( intv.start );
    const int initnrdec = nrdecstart>nrdecstop ? nrdecstart : nrdecstop;
    intvfld_->setNrDecimals( initnrdec, 0 );
}

void finaliseCB( CallBacker* cb )
{
    setInitialDecNr();
    int nrdec=0;
    const float startval = getNiceNumber( contourintv_.start, nrdec );
    intvfld_->setText( toString(startval), 0 );
    const float stopval = getNiceNumber( contourintv_.stop, nrdec );
    intvfld_->setText( toString(stopval) , 1 );
}

void intvChanged( CallBacker* cb )
{
    StepInterval<float> intv = intvfld_->getFStepInterval();
    if ( intv.start < rg_.start || intv.start > rg_.stop )
	intvfld_->setValue( contourintv_.start, 0 );
    if ( intv.stop < rg_.start || intv.stop > rg_.stop )
	intvfld_->setValue( contourintv_.stop, 1 );

    bool invalidstep = ( intv.step <= 0 || intv.step > rg_.width() ) ?
			true : false;
    if( invalidstep )
    {
	intvfld_->setValue( contourintv_.step, 2 );
	if (cb)
	{
	    uiMSG().error(tr("Invalid step value"));
	    return;
	}
    }

    intv = intvfld_->getFStepInterval();
    contourintv_.step = intv.step;

    uiString txt( tr("Number of contours: %1").arg(intv.nrSteps()+1) );
    toStatusBar( txt );
}

    Interval<float>	rg_;
    StepInterval<float> contourintv_;
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


void uiODContourTreeItem::initClass()
{
    uiODDataTreeItem::factory().addCreator( create, "Contour" );
}


uiODContourTreeItem::uiODContourTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , optionsmenuitem_( m3Dots(uiStrings::sProperties()) )
    , areamenuitm_( tr("Contour areas") )
    , lines_( 0 )
    , drawstyle_( 0 )
    , material_(0)
    , linewidth_(1)
    , contoursteprange_(mUdf(float),-mUdf(float))
    , zshift_(mUdf(float))
    , color_(0,0,0)
    , showlabels_(true)
    , labels_( 0 )
{
    optionsmenuitem_.iconfnm = "disppars";
    areamenuitm_.iconfnm = "contourarea";
    ODMainWin()->applMgr().visServer()->removeAllNotifier().notify(
	    mCB(this,uiODContourTreeItem,visClosingCB) );
}


uiODContourTreeItem::~uiODContourTreeItem()
{
    visSurvey::HorizonDisplay* hordisp = getHorDisp();

    if ( hordisp )
	hordisp->getMovementNotifier()->remove(
		 mCB(this,uiODContourTreeItem,checkCB));

    applMgr()->visServer()->removeAllNotifier().remove(
		 mCB(this,uiODContourTreeItem,visClosingCB) );

    if ( lines_ || drawstyle_ )
    {
	pErrMsg("prepareForShutdown not run");
    }

    if ( !parent_ )
	return;

    parent_->checkStatusChange()->remove(mCB(this,uiODContourTreeItem,checkCB));

}


void uiODContourTreeItem::prepareForShutdown()
{ visClosingCB( 0 ); }


void uiODContourTreeItem::visClosingCB( CallBacker* )
{  removeAll(); }


uiString uiODContourTreeItem::createDisplayName() const
{ return toUiString("%1 (%2)").arg(uiStrings::sContour(mPlural)).arg(attrnm_); }


bool uiODContourTreeItem::init()
{
    if ( !uiODDataTreeItem::init() ) return false;
    uitreeviewitem_->setChecked( true );
    zshift_ = (float)applMgr()->visServer()->getTranslation( displayID() ).z_;
    parent_->checkStatusChange()->notify(mCB(this,uiODContourTreeItem,checkCB));
    return true;
}


uiODDataTreeItem* uiODContourTreeItem::create( const Attrib::SelSpec& as,
					     const char* parenttype )
{
    BufferString defstr = as.defString();
    if ( defstr != sKeyContourDefString() )
	return 0;

    BufferString zkeystr = as.zDomainKey();
    // for old session file
    if ( zkeystr.isEmpty() )
	zkeystr = sKeyZValue();

    uiODContourTreeItem* ctitem = new uiODContourTreeItem( parenttype );
    if ( ctitem )
	ctitem->setAttribName( zkeystr );

    return ctitem ? ctitem : 0;
}


void uiODContourTreeItem::checkCB(CallBacker*)
{
    bool newstatus = uitreeviewitem_->isChecked();
    if ( newstatus && parent_ )
	newstatus = parent_->isChecked();

    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if ( !hordisp ) return;

    const bool display = newstatus && hordisp &&
				!hordisp->displayedOnlyAtSections();

    if ( lines_ ) lines_->turnOn( display );
    if ( labels_ ) labels_->turnOn( display && showlabels_ );

    updateZShift();
}


#define mUnRefAndZero(var) if ( var ) { var->unRef(); var = 0; }

void uiODContourTreeItem::removeAll()
{
    if ( lines_ )
    {
	applMgr()->visServer()->removeObject( lines_, sceneID() );
	mUnRefAndZero(lines_);
    }
    mUnRefAndZero( drawstyle_ );
    mUnRefAndZero( material_ );
    removeLabels();
}


void uiODContourTreeItem::removeLabels()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( labels_ )
    {
	visserv->removeObject( labels_, sceneID() );
	mUnRefAndZero( labels_ );
    }
}


void uiODContourTreeItem::createMenu( MenuHandler* menu, bool istb )
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


void uiODContourTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);

    if ( mnuid==-1 || menu->isHandled() || !lines_ )
	return;

    if ( mnuid==optionsmenuitem_.id )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	zshift_ = (float)visserv->getTranslation( displayID() ).z_;

	menu->setIsHandled( true );
	Interval<float> range( contoursteprange_.start + zshift_,
			       contoursteprange_.stop + zshift_ );

	StepInterval<float> oldintv( contourintv_ );
	oldintv += Interval<float>( zshift_, zshift_ );

	uiContourParsDlg dlg( ODMainWin(), attrnm_, range, oldintv,
			      OD::LineStyle(OD::LineStyle::Solid,
			      linewidth_,color_), sceneID() );
	if ( labels_ )
	{
	    dlg.setShowLabels( labels_->isOn() );

	    if ( labels_->nrTexts() )
	    {
		dlg.setFontData( labels_->text(0)->getFontData() );
		dlg.setLabelAlignment( (visBase::TextDrawable::Justification)
				       labels_->text(0)->getJustification() );

		dlg.disableLabelElevation();
	    }
	}

	dlg.propertyChanged.notify( mCB(this,uiODContourTreeItem,
								propChangeCB) );
	dlg.intervalChanged.notify( mCB(this,uiODContourTreeItem,
								intvChangeCB) );

	const bool res = dlg.go();
	if ( !res ) return;

	StepInterval<float> newintv = dlg.getContourInterval();
	updateUICContours( newintv );
    }
    if ( mnuid==areamenuitm_.id )
    {
	menu->setIsHandled( true );

	TypeSet<float> zvals, areas;
	getZVSAreaValues( zvals, areas );

	uiDialog dlg( ODMainWin(), uiDialog::Setup(tr("Countour areas"),
				   uiString::empty(), mNoHelpKey ) );
	dlg.setCancelText( uiString::empty() );

	RefMan<visSurvey::Scene> mDynamicCast( visSurvey::Scene*, scene,
			applMgr()->visServer()->getObject(sceneID()) );

	if ( !scene ) { pErrMsg("No scene"); return; }

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

	uiButton* button = uiButton::getStd( &dlg, OD::SaveAs,
			   mCB(this,uiODContourTreeItem,saveAreasAsCB), true );
	button->attach( leftAlignedBelow, table );

	dlg.go();
    }
}


void uiODContourTreeItem::getZVSAreaValues( TypeSet<float>& zvals,
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


void uiODContourTreeItem::saveAreasAsCB(CallBacker*)
{
    uiFileSelector::Setup fssu;
    fssu.setForWrite().setFormat( File::Format::textFiles() );
    uiFileSelector uifs( ODMainWin(), fssu );
    if ( uifs.go() )
    {
	const BufferString fnm( uifs.fileName() );
	od_ostream stream( fnm );
	RefMan<visSurvey::Scene> mDynamicCast( visSurvey::Scene*, scene,
			    applMgr()->visServer()->getObject(sceneID()) );

	if ( !scene )
	    { pErrMsg("No scene"); return; }

	const ZDomain::Info& zinfo = scene->zDomainInfo();

	stream << toString(zinfo.getLabel())
	       << od_tab << toString(areaString()) << od_newline;

	TypeSet<float> zvals, areas;
	getZVSAreaValues( zvals, areas );

	for ( int idx=0; idx<areas.size(); idx++ )
	    stream << zvals[idx] * zinfo.userFactor() << od_tab
		   << areas[idx] << od_newline;

	if ( !stream.isBad() )
	    mTIUiMsg().message( tr("Area table saved as:\n%1.").arg(fnm) );
	else
	{
	    uiString errmsg = uiStrings::phrCannotSave(tr("file %1").arg(fnm));
	    stream.addErrMsgTo( errmsg );
	    mTIUiMsg().error( errmsg );
	}
    }
}


void uiODContourTreeItem::updateUICContours(const StepInterval<float>& newintv)
{
    StepInterval<float> oldintv = contourintv_;
    const int nrsignificant = getInitialDec( newintv.start );
    const float eps = Math::PowerOf( 10.f, -mCast(float,nrsignificant) );
    oldintv += Interval<float>( zshift_, zshift_ );
    const bool intvchgd = !mIsEqual(newintv.start,oldintv.start,eps) ||
			  !mIsEqual(newintv.stop,oldintv.stop,eps) ||
			  !mIsEqual(newintv.step,oldintv.step,eps);

    if ( intvchgd )
    {
	contourintv_ = newintv;
	contourintv_ += Interval<float>( -zshift_, -zshift_ );
	startCreateUICContours();
    }
}


void uiODContourTreeItem::intvChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg ) return;

    StepInterval<float> newintv = dlg->getContourInterval();
    updateUICContours( newintv );
}


void uiODContourTreeItem::propChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg || !lines_ ) return;

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
	for ( int idx=0; idx<labels_->nrTexts(); idx++ )
	{
	    labels_->text(idx)->setJustification(
	    (visBase::TextDrawable::Justification) dlg->getLabelAlignment() );
	}
    }
}


void uiODContourTreeItem::startCreateUICContours()
{
    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if( !hordisp ) return;

    uiUserShowWait usw( getUiParent(), tr("Generating contours") );
    const Array2D<float>*  field = getDataSet( hordisp );

    if ( !field ||
	 !createPolyLines() ||
	 !computeUICContourSteps( *field ) )
	return;

    bool showcontours = lines_->turnOn( false );
    bool showlabels = labels_ && labels_->turnOn( false );

    uiODContourTreeItemContourGenerator ctrgtr ( this, field );
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
	mCB(this,uiODContourTreeItem,checkCB) );
}


Array2D<float>* uiODContourTreeItem::getDataSet(
    visSurvey::HorizonDisplay* hordisp )
{
    DBKey emid = hordisp->getObjectID();
    mDynamicCastGet(EM::Horizon3D*,hor,EM::MGR().getObject(emid));
    if ( !hor ) return 0;

    if ( attrnm_ == uiODContourTreeItem::sKeyZValue() )
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


bool uiODContourTreeItem::createPolyLines()
{
    if ( lines_ )
    {
	removeOldUICContoursFromScene();
	return true;
    }

    if ( ( lines_ = visBase::PolyLine::create() ) == 0 ) return false;
    lines_->ref();
    lines_->setPickable( false, false );

    applMgr()->visServer()->addObject( lines_, sceneID(), false );

#define mCreateAndRef(var,postfix)\
    {\
    var = new visBase::postfix;\
    var->ref();\
    }

    if ( !drawstyle_ )
    {
	mCreateAndRef(drawstyle_,DrawStyle);
	lines_->addNodeState( drawstyle_ );
    }

    if ( !material_ )
    {
	mCreateAndRef(material_,Material);
	material_->setColor( color_ );
	lines_->setMaterial( material_ );
    }

    return true;
}


bool uiODContourTreeItem::setLabels( visBase::Text* newlabels )
{
    if( !material_ )
	return false;

    removeLabels();
    labels_ = newlabels;
    if( !labels_ ) return false;

    labels_->ref();
    applMgr()->visServer()->addObject( labels_, sceneID(), false );
    labels_->setMaterial( material_ );

    return true;
}


bool uiODContourTreeItem::computeUICContourSteps( const Array2D<float>& field )
{
    if ( mIsUdf(contoursteprange_.start) )
    {
	for ( int idx=0; idx<field.getSize(0); idx++ )
	{
	    for ( int idy=0; idy<field.getSize(1); idy++ )
	    {
		const float val = field.get( idx, idy );
		if ( !mIsUdf(val) ) contoursteprange_.include( val, false );
	    }
	}

	if ( mIsUdf(contoursteprange_.start) )
	    return false;

	AxisLayout<float> al( contoursteprange_ );
	SamplingData<float> sd = al.sd_;
	sd.step /= 5;
	const float offset = ( sd.start - contoursteprange_.start ) / sd.step;
	if ( offset < 0 || offset > 1 )
	{
	    const int nrsteps = mNINT32(Math::Floor(offset));
	    sd.start -= nrsteps * sd.step;
	}

	contourintv_.start = sd.start;
	contourintv_.stop = contoursteprange_.stop;
	contourintv_.step = sd.step;
	const int nrsteps = contourintv_.nrSteps();
	contourintv_.stop = sd.start + nrsteps*sd.step;
    }

    if ( contourintv_.step <= 0 )
	return false;

    return true;
}


void uiODContourTreeItem::removeOldUICContoursFromScene()
{
   if ( lines_ )
       lines_->removeAllPrimitiveSets();
}


visSurvey::HorizonDisplay* uiODContourTreeItem::getHorDisp()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::HorizonDisplay*,hordisp,
		    visserv->getObject(displayID()))
    return hordisp;
}


void uiODContourTreeItem::updateColumnText( int col )
{
    uiODDataTreeItem::updateColumnText( col );
    if ( !col && !lines_ )
	startCreateUICContours();

    uiVisPartServer* visserv = applMgr()->visServer();
    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if ( !hordisp || !lines_ || !labels_ ) return;

    const bool solomode = visserv->isSoloMode();
    const bool turnon = !hordisp->displayedOnlyAtSections() &&
			( (solomode && hordisp->isOn())
			|| (!solomode && hordisp->isOn() && isChecked()) );
    lines_->turnOn( turnon );
    labels_->turnOn( turnon && showlabels_ );
}


void uiODContourTreeItem::updateZShift()
{
    if ( !lines_ || mIsUdf(zshift_) ) return;

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
	    BufferString txt = toString( labels_->text(idx)->getText() );
	    float labelval = txt.toFloat();
	    labelval += deltaz * SI().zDomain().userFactor();
	    labels_->text(idx)->setText( toUiString( (int)labelval ) );
	}
    }

    zshift_ = (float) trans.z_;
}


int uiODContourTreeItem::getNumberOfLines() const
{
    if ( !lines_ ) return 0;

    return lines_->nrPrimitiveSets();
}


bool uiODContourTreeItem::getLineCoordinates( int iln,
				TypeSet<Coord3>& linecoords ) const
{
    if ( !lines_ || iln<0 || iln>=lines_->nrPrimitiveSets() )
	return false;

    linecoords.setEmpty();

    const Geometry::PrimitiveSet* ps = lines_->getPrimitiveSet( iln );
    if ( !ps ) return false;

    for ( int idx=0; idx<ps->size(); idx++ )
    {
	if ( lines_->getCoordinates()->isDefined(idx) )
	{
	    const Coord3 pos = lines_->getCoordinates()->getPos( idx );
	    linecoords += pos;
	}
    }

    return true;
}




