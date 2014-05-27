/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uicontourtreeitem.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "isocontourtracer.h"
#include "axislayout.h"
#include "mousecursor.h"
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
#include "uifiledlg.h"

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
const char* uiContourTreeItem::sKeyContourDefString(){return "Contour Display";}
const char* uiContourTreeItem::sKeyZValue()	     { return "Z Values"; }


class uiContourTreeItemContourData
{
public:

    TypeSet<Coord3>			   contourcoords_;
    TypeSet< Interval<int> >		   contourcoordrgs_;
    ObjectSet<Geometry::RangePrimitiveSet> contourprimitivesets_;
    TypeSet<Coord3>			   labelpositions_;
    TypeSet<BufferString>		   labels_;
    TypeSet<Interval<int> >		   labelranges_;
};


class uiContourTreeItemContourGenerator : public ParallelTask
{
public:

				uiContourTreeItemContourGenerator(
					uiContourTreeItem* p,
				        const Array2D<float>* field);
				~uiContourTreeItemContourGenerator()
				{ if ( labels_ ) labels_->unRef(); }

    visBase::Text2*		getLabels() { return labels_; }
    const TypeSet<double>&	getAreas() const { return areas_; }

    uiString			uiNrDoneText() const
                                { return "Contours created"; }

protected:
    bool	doPrepare(int);
    bool	doWork(od_int64 start, od_int64 stop, int);
    bool	doFinish(bool success);
    od_int64	nrIterations() const { return nrcontours_; }

private:

    bool prepForContourGenerator();
    bool setRowColRgs(visSurvey::HorizonDisplay*);

    bool generateContours(int contouridx,const IsoContourTracer*,
			     uiContourTreeItemContourData&,double& area) const;
    bool addDisplayCoord(const ODPolygon<float>& inputcountour, int vrtxidx,
			 uiContourTreeItemContourData&,int& lastvrtxidx) const;
    void makeContourClose(uiContourTreeItemContourData&,
			  Interval<int>& coordsrg) const;
    void addContourData(uiContourTreeItemContourData&);
    void addContourLabel(const Coord3& pos, const char* lbl);

    int						nrcontours_;
    Threads::Atomic<od_int64>			totalnrshapes_;
    uiContourTreeItem*				uicitem_;
    Threads::Mutex				mutex_;
    uiContourTreeItemContourData		contourdata_;
				// from construction source
    const Array2D<float>*	field_;

    visBase::Text2*		labels_;

    StepInterval<int>		rowrg_;
    StepInterval<int>		colrg_;
    float			zfactor_;
    const EM::Horizon3D*	hor3d_;
    const ZAxisTransform*	ztransform_;
    const mVisTrans*		displaytrans_;
    TypeSet<double>		areas_;
};


uiContourTreeItemContourGenerator::uiContourTreeItemContourGenerator(
			uiContourTreeItem* p, const Array2D<float>* field )
    : nrcontours_( p->contourintv_.nrSteps() )
    , uicitem_( p )
    , field_( field )
    , zfactor_( 0 )
    , labels_( 0 )
{
    totalnrshapes_ = nrcontours_;
    areas_.setSize( nrcontours_ );
    setName( "Generating contours" );
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


bool uiContourTreeItemContourGenerator::doWork(od_int64 start,od_int64 stop,int)
{
    PtrMan<IsoContourTracer> ictracer =  new IsoContourTracer( *field_ );
    if( !ictracer ) return false;
    ictracer->setSampling( rowrg_, colrg_ );

    uiContourTreeItemContourData newcontourdata;
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


void uiContourTreeItemContourGenerator::addContourData(
	 uiContourTreeItemContourData& newcontourdata )
{
    const int contourcoordsz =  contourdata_.contourcoords_.size();
    const Interval<int> lastcoordidxrg ( contourcoordsz, contourcoordsz-1 );
    contourdata_.contourcoords_.append( newcontourdata.contourcoords_);
    contourdata_.labelpositions_.append( newcontourdata.labelpositions_ );
    contourdata_.labels_.append( newcontourdata.labels_ );

    for ( int idx=0; idx<newcontourdata.contourcoordrgs_.size(); idx++ )
    {
	Geometry::RangePrimitiveSet* ps =
	    Geometry::RangePrimitiveSet::create();
	ps->setRange( newcontourdata.contourcoordrgs_[idx] + lastcoordidxrg );
	contourdata_.contourprimitivesets_.push( ps );
    }

    if ( contourdata_.labelranges_.size() )
    {
	const int lastlblidx =
	    contourdata_.labelranges_[contourdata_.labelranges_.size()-1].stop;
	for ( int idx = 0; idx< newcontourdata.labelranges_.size(); idx++ )
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
	const float labelval =
	    uicitem_->attrnm_==uiContourTreeItem::sKeyZValue()
	    ? (contourval+uicitem_->zshift_) * zfactor_ : contourval;
	contourdata.labelranges_.add( lblpositionrg );
	contourdata.labels_.add( toString( (int)labelval ) );
    }

    return true;
}


void uiContourTreeItemContourGenerator::makeContourClose(
    uiContourTreeItemContourData& contourdata,
    Interval<int>& coordsrg ) const
{
    if ( contourdata.contourcoords_.size() <=0 ) return;
    contourdata.contourcoords_.add(
	contourdata.contourcoords_[coordsrg.start] );
    coordsrg.stop++;
}


bool uiContourTreeItemContourGenerator::addDisplayCoord(
    const ODPolygon<float>& inputcountour, int vrtxidx,
    uiContourTreeItemContourData& contourdata,
    int& lastvrtxidx ) const
{
    const Geom::Point2D<float> vrtx = inputcountour.getVertex( vrtxidx );
    const BinID bidnearestvrtx( rowrg_.snap(vrtx.x), colrg_.snap(vrtx.y) );
    float zval = hor3d_->getZ( bidnearestvrtx );
    if ( ztransform_ )
	ztransform_->transform( bidnearestvrtx,
				SamplingData<float>(zval,1), 1, &zval );
    if ( mIsUdf(zval) )
	return false;

    Coord3 vrtxcoord;
    vrtxcoord.coord() = SI().binID2Coord().transform( bidnearestvrtx );
    vrtxcoord.z = zval + uicitem_->zshift_;
    visBase::Transformation::transform( displaytrans_, vrtxcoord, vrtxcoord );
    contourdata.contourcoords_.add( vrtxcoord );
    lastvrtxidx++;
    return true;
}


bool uiContourTreeItemContourGenerator::doFinish( bool success )
{
    if ( !success ) return false;

    if ( labels_ ) labels_->unRef();
    labels_ = visBase::Text2::create();
    labels_->ref();
    labels_->setDisplayTransformation( displaytrans_ );

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

    for (int lbrgidx = 0 ; lbrgidx<contourdata_.labelranges_.size(); lbrgidx++)
    {
	const Interval<int> lbldata( contourdata_.labelranges_[lbrgidx] );
	const od_int64 stoplblidx = contourdata_.labelranges_[lbrgidx].stop;

	for ( int ipos=lbldata.start; ipos<stoplblidx; ipos++ )
	      addContourLabel(
           contourdata_.labelpositions_[ipos], contourdata_.labels_[lbrgidx] );

	addToNrDone( 1 );
	if ( !shouldContinue() )
	    return false;
    }

    return true;
}


void uiContourTreeItemContourGenerator::addContourLabel(
    const Coord3& pos, const char* lbl)
{
    if ( !labels_ ) return;

    const int idx = labels_->addText();
    visBase::Text* label = labels_->text( idx );
    if ( label )
    {
	label->setText( lbl );
	label->setPosition( pos, true );
	label->setFontData( FontData( 18 ), labels_->getPixelDensity() );
    }
}


class uiContourParsDlg : public uiDialog
{
public:

uiContourParsDlg( uiParent* p, const char* attrnm, const Interval<float>& rg,
		  const StepInterval<float>& intv, const LineStyle& ls,
		  int sceneid )
    : uiDialog(p,Setup("Contour Display Options",mNoDlgTitle,
                        mODHelpKey(mContourParsDlgHelpID) )
		 .nrstatusflds(1))
    , rg_(rg)
    , contourintv_(intv)
    , propertyChanged(this)
    , intervalChanged(this)
    , iszval_(FixedString(uiContourTreeItem::sKeyZValue()) == attrnm)
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
	str.add( " " ).add( scene->zDomainInfo().unitStr(true) )

    BufferString lbltxt( "Total ", attrnm, " range" ); mAddZUnitStr(lbltxt);
    lbltxt.add( ": " ).add( rg_.start ).add( " - " ).add( rg_.stop );
    uiLabel* lbl = new uiLabel( this, lbltxt );

    lbltxt = "Contour range "; mAddZUnitStr(lbltxt);
    intvfld_ = new uiGenInput(this,lbltxt,FloatInpIntervalSpec(contourintv_));
    intvfld_->valuechanged.notify( mCB(this,uiContourParsDlg,intvChanged) );
    intvfld_->attach( leftAlignedBelow, lbl );

    uiPushButton* applybut = new uiPushButton( this, "Apply", true );
    applybut->attach( rightTo, intvfld_ );
    applybut->activated.notify( mCB(this,uiContourParsDlg,applyCB) );

    uiSelLineStyle::Setup lssu; lssu.drawstyle(false);
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, intvfld_ );
    lsfld_->changed.notify( mCB(this,uiContourParsDlg,dispChanged) );

    showlblsfld_ = new uiCheckBox( this, "Show labels" );
    showlblsfld_->activated.notify( mCB(this,uiContourParsDlg,dispChanged) );
    showlblsfld_->attach( alignedBelow, lsfld_ );

    intvChanged( 0 );
}


const LineStyle& getLineStyle() const
{ return lsfld_->getStyle(); }


StepInterval<float> getContourInterval() const
{
    StepInterval<float> res = intvfld_->getFStepInterval();
    if ( iszval_ )
	res.scale( 1.0f/zfac_ );

    return res;
}


void setShowLabels( bool yn )
{ showlblsfld_->setChecked( yn ); }


bool showLabels() const
{ return showlblsfld_->isChecked(); }

    Notifier<uiContourParsDlg>	propertyChanged;
    Notifier<uiContourParsDlg>	intervalChanged;

protected:

void applyCB( CallBacker* )
{ intervalChanged.trigger(); }


void dispChanged( CallBacker* )
{ propertyChanged.trigger(); }


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
	if( cb ) return uiMSG().error( "Invalid step value" );
    }

    intv = intvfld_->getFStepInterval();
    contourintv_.step = intv.step;

    BufferString txt( "Number of contours: ", intv.nrSteps()+1 );
    toStatusBar( txt );
}

    Interval<float>	rg_;
    StepInterval<float>	contourintv_;
    uiGenInput*		intvfld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		showlblsfld_;
    float		zfac_;
    bool		iszval_;
};


void uiContourTreeItem::initClass()
{ uiODDataTreeItem::factory().addCreator( create, 0 ); }

uiContourTreeItem::uiContourTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , optionsmenuitem_( "Properties ..." )
    , areamenuitm_( "Contour areas" )
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
	    mCB(this,uiContourTreeItem,visClosingCB) );
}


uiContourTreeItem::~uiContourTreeItem()
{
    visSurvey::HorizonDisplay* hordisp = getHorDisp();

    if ( hordisp )
	hordisp->getMovementNotifier()->remove(
		 mCB(this,uiContourTreeItem,checkCB));

    applMgr()->visServer()->removeAllNotifier().remove(
		 mCB(this,uiContourTreeItem,visClosingCB) );

    if ( lines_ || drawstyle_ )
    {
	pErrMsg("prepareForShutdown not run");
    }

    if ( !parent_ )
	return;

    parent_->checkStatusChange()->remove(mCB(this,uiContourTreeItem,checkCB));

}


void uiContourTreeItem::prepareForShutdown()
{ visClosingCB( 0 ); }


void uiContourTreeItem::visClosingCB( CallBacker* )
{  removeAll(); }


BufferString uiContourTreeItem::createDisplayName() const
{ return BufferString ( "Contours (", attrnm_, ")" ); }


bool uiContourTreeItem::init()
{
    if ( !uiODDataTreeItem::init() ) return false;
    uitreeviewitem_->setChecked( true );
    zshift_ = (float)applMgr()->visServer()->getTranslation( displayID() ).z;
    parent_->checkStatusChange()->notify(mCB(this,uiContourTreeItem,checkCB));
    return true;
}


uiODDataTreeItem* uiContourTreeItem::create( const Attrib::SelSpec& as,
					     const char* parenttype )
{
    BufferString defstr = as.defString();
    return defstr == sKeyContourDefString() ? new uiContourTreeItem(parenttype)
					    : 0;
}


void uiContourTreeItem::checkCB(CallBacker*)
{
    bool newstatus = uitreeviewitem_->isChecked();
    if ( newstatus && parent_ )
	newstatus = parent_->isChecked();

    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if ( !hordisp ) return;

    const bool display = newstatus && hordisp &&
				!hordisp->getOnlyAtSectionsDisplay();

    if ( lines_ ) lines_->turnOn( display );
    if ( labels_ ) labels_->turnOn( display && showlabels_ );

    updateZShift();
}


#define mUnRefAndZero(var) if ( var ) { var->unRef(); var = 0; }

void uiContourTreeItem::removeAll()
{
    if ( lines_ )
    {
	applMgr()->visServer()->removeObject( lines_, sceneID() );
	mUnRefAndZero( lines_ );
    }
    mUnRefAndZero( drawstyle_ );
    mUnRefAndZero( material_ );
    removeLabels();
}


void uiContourTreeItem::removeLabels()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( labels_ )
    {
	visserv->removeObject( labels_, sceneID() );
	mUnRefAndZero( labels_ );
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

const char* areaString()
{
    return SI().xyInFeet() ? "Area (sqft)" : "Area (m^2)";
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

        uiVisPartServer* visserv = applMgr()->visServer();
        zshift_ = (float)visserv->getTranslation( displayID() ).z;

        menu->setIsHandled( true );
        Interval<float> range( contoursteprange_.start + zshift_,
                               contoursteprange_.stop + zshift_ );

        StepInterval<float> oldintv( contourintv_ );
        oldintv += Interval<float>( zshift_, zshift_ );

        uiContourParsDlg dlg( ODMainWin(), attrnm_, range, oldintv,
                              LineStyle(LineStyle::Solid,linewidth_,color_),
                              sceneID() );
        if ( labels_ )
            dlg.setShowLabels( labels_->isOn() );

        dlg.propertyChanged.notify( mCB(this,uiContourTreeItem,propChangeCB) );
        dlg.intervalChanged.notify( mCB(this,uiContourTreeItem,intvChangeCB) );
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

        uiDialog dlg( ODMainWin(), uiDialog::Setup("Countour areas", 0,
				   mNoHelpKey ) );
        dlg.setCancelText( 0 );

        RefMan<visSurvey::Scene> mDynamicCast( visSurvey::Scene*, scene,
                        applMgr()->visServer()->getObject(sceneID()) );

        if ( !scene ) { pErrMsg("No scene"); return; }

        const ZDomain::Info& zinfo = scene->zDomainInfo();

	uiTable* table = new uiTable( &dlg, uiTable::Setup(areas.size(),2),
                                      "Area table");

        const BufferString zdesc( zinfo.userName(), " ", zinfo.unitStr(true) );
        table->setColumnLabel( 0, zdesc.buf() );


        table->setColumnLabel( 1, areaString() );

        for ( int idx=0; idx<areas.size(); idx++ )
        {
            table->setText( RowCol(idx,0),
                            toString( zvals[idx] * zinfo.userFactor() ) );
            table->setText( RowCol(idx,1), toString( areas[idx] ) );
        }

        uiPushButton* button = new uiPushButton( &dlg, sSaveAs(),
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
    if ( dlg.go() )
    {
        od_ostream stream( dlg.fileName() );
        RefMan<visSurvey::Scene> mDynamicCast( visSurvey::Scene*, scene,
                            applMgr()->visServer()->getObject(sceneID()) );

        if ( !scene ) { pErrMsg("No scene"); return; }

        const ZDomain::Info& zinfo = scene->zDomainInfo();

        stream << BufferString(zinfo.userName(), " ", zinfo.unitStr(true))
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
            BufferString errmsg( "Could not save file");

	    if ( stream.errMsg().getFullString() )
                stream.addErrMsgTo( errmsg );
            else
                errmsg.add( "." );

            uiMSG().error( errmsg.buf() );
        }
        else
        {
            uiMSG().message( BufferString( "Area table saved as ",
                                          dlg.fileName(), ".").buf());
        }
    }
}


void uiContourTreeItem::updateUICContours( const StepInterval<float>& newintv )
{
    StepInterval<float> oldintv = contourintv_;
    oldintv += Interval<float>( zshift_, zshift_ );
    const bool intvchgd = !mIsEqual(newintv.start,oldintv.start,1e-4) ||
			  !mIsEqual(newintv.stop,oldintv.stop,1e-4) ||
			  !mIsEqual(newintv.step,oldintv.step,1e-4);

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
    if ( !dlg ) return;

    StepInterval<float> newintv = dlg->getContourInterval();
    updateUICContours( newintv );
}


void uiContourTreeItem::propChangeCB( CallBacker* cb )
{
    mDynamicCastGet(uiContourParsDlg*,dlg,cb);
    if ( !dlg || !lines_ ) return;

    LineStyle ls( dlg->getLineStyle() );
    drawstyle_->setLineStyle( ls );
    material_->setColor( ls.color_ );
    color_ = ls.color_;
    linewidth_ = ls.width_;

    if ( labels_ && lines_ )
    {
	showlabels_ = dlg->showLabels();
	labels_->turnOn( lines_->isOn() && showlabels_ );
    }
}


void uiContourTreeItem::startCreateUICContours()
{
    visSurvey::HorizonDisplay* hordisp = getHorDisp();
    if( !hordisp ) return;

    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    const Array2D<float>*  field = getDataSet( hordisp );

    if ( !field ||
	 !createPolyLines() ||
	 !computeUICContourSteps( *field ) )
	return;

    bool showcontours = lines_->turnOn( false );
    bool showlabels = labels_  && labels_->turnOn( false );

    uiContourTreeItemContourGenerator ctrgtr ( this, field );
    uiTaskRunner taskrunner( ODMainWin() );
    if ( taskrunner.execute( ctrgtr ) )
    {
	setLabels( ctrgtr.getLabels() );
	lines_->turnOn( showcontours );
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
    if ( !hor ) return 0;

    EM::SectionID sid = hor->sectionID( 0 );
    if ( attrnm_ == uiContourTreeItem::sKeyZValue() )
    {
	Array2D<float>* arr=hor->geometry().sectionGeometry(sid)->getArray();
	if ( hordisp->getZAxisTransform() )
	    arr = hor->createArray2D( sid, hordisp->getZAxisTransform() );
	return arr;
    }

    const int dataid=applMgr()->EMServer()->loadAuxData( hor->id(),attrnm_);
    Array2D<float>* arr = hor->auxdata.createArray2D( dataid, sid );
    return arr;
}


bool uiContourTreeItem::createPolyLines()
{
    if ( lines_ )
    {
	removeOldUICContoursFromScene();
	return true;
    }

    if ( ( lines_ = visBase::PolyLine::create() ) == 0 ) return false;
    lines_->ref();

    applMgr()->visServer()->addObject( lines_, sceneID(), false );

#define mCreateAndRef(var,postfix)\
    {\
    var = new visBase::postfix;\
    var->ref();\
    }

    if ( !drawstyle_ )
    {
	mCreateAndRef( drawstyle_,DrawStyle );
	lines_->addNodeState( drawstyle_ );
    }

    if ( !material_ )
    {
	mCreateAndRef( material_,Material );
	material_->setColor( color_ );
	lines_->setMaterial( material_ );
    }

    return true;
}


bool uiContourTreeItem::setLabels( visBase::Text2* newlabels )
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


bool uiContourTreeItem::computeUICContourSteps( const Array2D<float>& field )
{
    if ( mIsUdf(contoursteprange_.start) )
    {
	for ( int idx=0; idx<field.info().getSize(0); idx++ )
	{
	    for ( int idy=0; idy<field.info().getSize(1); idy++ )
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
	    const int nrsteps = mNINT32( floor(offset) );
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
    if ( !hordisp || !lines_ || !labels_ ) return;

    const bool solomode = visserv->isSoloMode();
    const bool turnon = !hordisp->getOnlyAtSectionsDisplay() &&
			( (solomode && hordisp->isOn())
			|| (!solomode && hordisp->isOn() && isChecked()) );
    lines_->turnOn( turnon );
    labels_->turnOn( turnon && showlabels_ );
}


void uiContourTreeItem::updateZShift()
{
    if ( !lines_ || mIsUdf(zshift_) ) return;

    Coord3 trans = applMgr()->visServer()->getTranslation( displayID() );

    const float deltaz = (float) (trans.z - zshift_);
    if ( !deltaz )
	return;

    for ( int idx=0; idx<lines_->getCoordinates()->size(true); idx++ )
    {
	if ( lines_->getCoordinates()->isDefined(idx) )
	{
	    Coord3 pos = lines_->getCoordinates()->getPos( idx );
	    pos.z += deltaz;
	    lines_->getCoordinates()->setPos( idx, pos );
	}
    }

    lines_->dirtyCoordinates();

    for ( int idx=0; idx<labels_->nrTexts(); idx++ )
    {
	Coord3 pos = labels_->text(idx)->getPosition();
	pos.z += deltaz;
	labels_->text(idx)->setPosition( pos );
	BufferString txt = labels_->text(idx)->getText().getFullString();
	float labelval = txt.toFloat();
	labelval += deltaz * SI().zDomain().userFactor();
	labels_->text(idx)->setText( toString( (int)labelval ) );
    }

    zshift_ = (float) trans.z;
}

