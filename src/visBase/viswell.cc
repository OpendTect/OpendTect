/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "viswell.h"

#include "visdrawstyle.h"
#include "viscoord.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"

#include "coltabsequence.h"
#include "trckeyzsampling.h"
#include "iopar.h"
#include "indexedshape.h"
#include "ranges.h"
#include "scaler.h"
#include "survinfo.h"
#include "uistrings.h"
#include "zaxistransform.h"

#include <osg/Switch>
#include <osg/Node>
#include <osgGeo/WellLog>
#include <osgGeo/PlaneWellLog>
#include <osgGeo/TubeWellLog>
#include <osgText/Text>


mCreateFactoryEntry( visBase::Well );

namespace visBase
{

const char* Well::linestylestr()	{ return "Line style"; }
const char* Well::showwelltopnmstr()	{ return "Show top name"; }
const char* Well::showwellbotnmstr()	{ return "Show bottom name"; }
const char* Well::showmarkerstr()	{ return "Show markers"; }
const char* Well::showmarknmstr()	{ return "Show markername"; }
const char* Well::markerszstr()		{ return "Marker size"; }
const char* Well::showlogsstr()		{ return "Show logs"; }
const char* Well::showlognmstr()	{ return "Show logname"; }
const char* Well::logwidthstr()		{ return "Screen width"; }

Well::Well()
    : VisualObjectImpl( false )
    , showmarkers_(true)
    , showlogs_(true)
    , transformation_(0)
    , zaxistransform_(0)
    , voiidx_(-1)
    , leftlogdisplay_( new osgGeo::PlaneWellLog )
    , rightlogdisplay_( new osgGeo::PlaneWellLog )
    , centerlogdisplay_( new osgGeo::PlaneWellLog )
    , pixeldensity_( getDefaultPixelDensity() )
{
    markerset_ = MarkerSet::create();
    markerset_->ref();
    addChild( markerset_->osgNode() );
    markerset_->setMaterial( new Material );

    track_ = PolyLine::create();
    track_->setColorBindType( VertexShape::BIND_OVERALL );
    track_->ref();

    track_->addPrimitiveSet( Geometry::RangePrimitiveSet::create() );
    addChild( track_->osgNode() );

    track_->setMaterial( new Material );

    welltoptxt_ =  Text2::create();
    wellbottxt_ =  Text2::create();
    welltoptxt_->ref();
    wellbottxt_->ref();
    welltoptxt_->setMaterial( track_->getMaterial() );
    wellbottxt_->setMaterial( track_->getMaterial() );

    addChild( welltoptxt_->osgNode() );
    addChild( wellbottxt_->osgNode() );

    markernames_ = Text2::create();
    markernames_->ref();
    addChild( markernames_->osgNode() );

    leftlogdisplay_->ref();
    addChild( leftlogdisplay_ );

    rightlogdisplay_->ref();
    addChild( rightlogdisplay_ );

    centerlogdisplay_->ref();
    addChild( centerlogdisplay_ );

    for ( int idx=0; idx<3; idx++ )
    {
	displaytube_[idx]= false;
	displaylog_[idx] = false;
	lognames_.add( "" );
    }
    markerset_->addPolygonOffsetNodeState();
}


Well::~Well()
{
    if ( transformation_ ) transformation_->unRef();

    removeChild( track_->osgNode() );
    track_->unRef();

    markerset_->unRef();

    welltoptxt_->unRef();
    wellbottxt_->unRef();
    markernames_->unRef();

    removeLogs();
    leftlogdisplay_->unref();
    rightlogdisplay_->unref();
    centerlogdisplay_->unref();
}


#define mRefDisplay( obj )\
{\
    if ( obj )\
    {\
	obj->ref();\
	addChild( obj );\
    }\
}\


osgGeo::WellLog*& Well::getLogDisplay( Side side )
{
    return (side==Left) ? leftlogdisplay_ :
	   (side==Right) ? rightlogdisplay_ : centerlogdisplay_;
}


const osgGeo::WellLog* Well::getLogDisplay( Side side ) const
{
    return (side==Left) ? leftlogdisplay_ :
    (side==Right) ? rightlogdisplay_ : centerlogdisplay_;
}


void Well::setLogTubeDisplay( Side side, bool yn )
{
    if ( displaytube_[(int)side] == yn ) return;

    displaytube_[(int)side] = yn;

    osgGeo::WellLog*& log = getLogDisplay( side );

    if ( log )
    {
	removeChild( log );
	log->unref();
    }

    if ( yn )
    {
	osgGeo::WellLog*& logdisplay = getLogDisplay( side );
	logdisplay = new osgGeo::TubeWellLog;
    }
    else
    {
	osgGeo::WellLog*& logdisplay = getLogDisplay( side );
	logdisplay = new osgGeo::PlaneWellLog;
    }

    mRefDisplay( getLogDisplay( side ) );
}


void Well::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zaxistransform_==zat )
	return;

    if ( zaxistransform_ )
    {
	zaxistransform_->unRef();
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	zaxistransform_->ref();
    }
}


void Well::transformZIfNeeded( Coord3& crd ) const
{
    if ( !zaxistransform_ ) return;

    const StringView ztransformkey( zaxistransform_->toZDomainKey() );
    if ( ztransformkey == ZDomain::sKeyDepth() )
    {
	if ( SI().depthsInFeet() )
	    crd.z *= mToFeetFactorD;

	return;
    }

    crd.z = zaxistransform_->transform( crd );
}


void Well::setPixelDensity(float dpi)
{
    DataObject::setPixelDensity( dpi );
    pixeldensity_ = dpi;

    track_->setPixelDensity( dpi );
    markerset_->setPixelDensity( dpi );
}


void Well::setTrack( const TypeSet<Coord3>& pts )
{
    TrcKeyZSampling cs( false );
    for ( int idx=0; idx<pts.size(); idx++ )
	cs.include( SI().transform(pts[idx]), (float) pts[idx].z );

    if ( zaxistransform_ && zaxistransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_ < 0 )
	    voiidx_ = zaxistransform_->addVolumeOfInterest( cs, true );
	else
	    zaxistransform_->setVolumeOfInterest( voiidx_, cs, true );
	zaxistransform_->loadDataIfMissing( voiidx_ );
    }

    track_->getCoordinates()->setEmpty();
    int ptidx = 0;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	if ( !pts[idx].isDefined() )
	    continue;

	Coord3 crd = pts[idx];
	transformZIfNeeded( crd );

	if ( !crd.isDefined() )
	    continue;

	track_->getCoordinates()->setPos( ptidx, crd );
	ptidx++;
    }

    RefMan<Geometry::RangePrimitiveSet> rps = 0;
    if ( !track_->nrPrimitiveSets() )
    {
	rps = Geometry::RangePrimitiveSet::create();
	track_->addPrimitiveSet( rps );

    }
    else
    {
	rps = (Geometry::RangePrimitiveSet*) track_->getPrimitiveSet( 0 );
    }

    rps->setRange( Interval<int>( 0, ptidx-1 ) );
}


void Well::setTrackProperties( OD::Color& col, int width)
{
    OD::LineStyle lst;
    lst.color_ = col;
    lst.width_ = width;
    setLineStyle( lst );
}


void Well::setLineStyle( const OD::LineStyle& lst )
{
    track_->setLineStyle( lst );
}


const OD::LineStyle& Well::lineStyle() const
{
    return track_->lineStyle();
}


void Well::updateText( Text* vistxt, const char* txt, const Coord3* pos,
		       const FontData& fnt, bool sizedynamic )
{
    vistxt->setText( toUiString(txt) );
    vistxt->setFontData( fnt, getPixelDensity() );
    vistxt->setPosition( *pos );
    vistxt->setCharacterSizeMode( sizedynamic ? Text::Object : Text::Screen );
    vistxt->setAxisAlignment( Text::OnScreen );
}


void Well::setWellName( const TrackParams& tp )
{
    track_->setName( tp.name_ );

    if ( welltoptxt_->nrTexts()<1 )
	 welltoptxt_->addText();

    if ( wellbottxt_->nrTexts()<1 )
	 wellbottxt_->addText();

    Coord3 crdtop = *tp.toppos_;
    Coord3 crdbot = *tp.botpos_;

    transformZIfNeeded( crdtop );
    transformZIfNeeded( crdbot );

    const int nrpos = track_->getCoordinates()->size();
    if ( nrpos>1 && mIsUdf(crdtop.z) )
	crdtop.z = track_->getCoordinates()->getPos( 0 ).z;
    if ( nrpos>1 && mIsUdf(crdbot.z) )
	crdbot.z = track_->getCoordinates()->getPos( nrpos-1 ).z;

    welltoptxt_->text(0)->setJustification( Text::Bottom );
    updateText( welltoptxt_->text(0), tp.isdispabove_ ? tp.name_.str() : 0,
		&crdtop, tp.font_, tp.nmsizedynamic_ );
    welltoptxt_->text(0)->setColor( tp.col_ );

    wellbottxt_->text(0)->setJustification( Text::Top );
    updateText( wellbottxt_->text(0), tp.isdispbelow_ ? tp.name_.str() : 0,
		&crdbot, tp.font_, tp.nmsizedynamic_ );
    wellbottxt_->text(0)->setColor( tp.col_ );
}


void Well::showWellTopName( bool yn )
{
    welltoptxt_->turnOn( yn );
}


void Well::showWellBotName( bool yn )
{
    wellbottxt_->turnOn( yn );
}


bool Well::wellTopNameShown() const
{
    return welltoptxt_->isOn();
}


bool Well::wellBotNameShown() const
{
   return wellbottxt_->isOn();
}


void Well::setMarkerSetParams( const MarkerParams& mp )
{
   MarkerStyle3D markerstyle;
   markerset_->setMarkerHeightRatio( 1.0f );

    switch ( mp.shapeint_ )
    {
    case 0:
	markerstyle = MarkerStyle3D::Cylinder;
	markerset_->setMarkerHeightRatio(
		( float )mp.cylinderheight_/mp.size_ );
	break;
    case 1:
	markerstyle = MarkerStyle3D::Cube;
	break;
    case 2:
	markerstyle = MarkerStyle3D::Sphere;
	break;
    case 3:
	markerstyle = MarkerStyle3D::Cone;
	break;
    default:
	pErrMsg( "Shape not implemented" );
	return;
    }

    markerstyle.size_ = mp.size_;
    markersize_ = mp.size_;
    markerset_->setMarkerStyle( markerstyle );
    markerset_->setMinimumScale( 1.0f );
    markerset_->setMaximumScale( 25.5f );

    markerset_->setAutoRotateMode( visBase::MarkerSet::NO_ROTATION );
}


void Well::addMarker( const MarkerParams& mp )
{
    Coord3 markerpos = *mp.pos_;
    transformZIfNeeded( markerpos );
    if ( mIsUdf(markerpos.z) )
	  return;

    const int markerid = markerset_->addPos( markerpos );
    markerset_->getMaterial()->setColor( mp.col_, markerid ) ;

    const int textidx = markernames_->addText();
    Text* txt = markernames_->text( textidx );
    txt->setColor( mp.namecol_ );
    txt->setJustification( Text::BottomLeft );

    updateText( txt, mp.name_, &markerpos, mp.font_, mp.nmsizedynamic_ );

    return;
}


void Well::updateMakerSize(float sizefactor)
{
    markerset_->setScreenSize( markersize_ +
					(markersize_/sizefactor)*markersize_ );
}


void Well::updateMakerNamePosition(Side side,float sizefactor)
{
    const float ratio = displaytube_[side] ? 2 : 1;
    for ( int idx=0; idx<markernames_->nrTexts(); idx++ )
    {
	const Coord3 pos = markernames_->text(idx)->getPosition();
	const osg::Vec3 delta(0.0f,-sizefactor*ratio,0.0f);
	const osg::Vec3 osgpos = Conv::to<osg::Vec3>(pos);
	const osg::Vec3 newpos = osgpos + delta;
	const Coord3 txtpos = Conv::to<Coord3>( newpos );
	markernames_->text(idx)->setPosition( txtpos );
    }
}


void Well::removeAllMarkers()
{
    markerset_->clearMarkers();
    markernames_->removeAll();
}


void Well::setMarkerScreenSize( int size )
{
    markersize_ = size;
    markerset_->setScreenSize( size );
}


int Well::markerScreenSize() const
{
    //TODO: Should we simply cast to int?
    return mNINT32(markerset_->getScreenSize());
}


bool Well::canShowMarkers() const
{
    return markerset_->getCoordinates()->size();
}


void Well::showMarkers( bool yn )
{
   markerset_->turnOn( yn );
}


bool Well::markersShown() const
{
    return markerset_->isOn();
}

void Well::showMarkerName( bool yn )
{
    markernames_->turnOn(yn);
}


bool Well::markerNameShown() const
{
    return markernames_->isOn();
}


#define mSclogval(val)\
{\
    val += 1;\
    val = ::log( val );\
}


#define FULLFILLVALUE 100

void Well::setLogData(const TypeSet<Coord3Value>& crdvals,
		    const TypeSet<Coord3Value>& crdvalsF,
		    const LogParams& lp, bool isFilled )
{
    float rgStartF(.0);
    float rgStopF(.0);
    LinScaler scalerF;
    LinScaler scaler;

    float rgStart(.0);
    float rgStop(.0);

    getLinScale(lp,scaler,false);
    Interval<float> selrg = lp.range_;
    getLinScaleRange( scaler,selrg,rgStart,rgStop,lp.islogarithmic_ );

    if(isFilled)
    {
	getLinScale(lp,scalerF);
	Interval<float> selrgF = lp.fillrange_;
	getLinScaleRange( scalerF,selrgF,rgStartF,rgStopF,lp.islogarithmic_ );
    }

    const bool rev = lp.range_.start > lp.range_.stop;
    const bool isfullfilled = lp.isleftfilled_ && lp.isrightfilled_ &&
			      lp.style_ != Seismic ;
    const bool fillrev = !isfullfilled &&
	(  ( lp.side_ == Left  && lp.isleftfilled_  && !rev )
	|| ( lp.side_ == Left  && lp.isrightfilled_ &&  rev )
	|| ( lp.side_ == Right && lp.isrightfilled_ && !rev )
	|| ( lp.side_ == Right && lp.isleftfilled_  &&	rev )
	|| ( lp.side_ == Center && lp.isrightfilled_ && !rev )
	|| ( lp.side_ == Center && lp.isleftfilled_  &&  rev )
	);

    auto logdisplay = getLogDisplay( lp.side_ );
    logdisplay->setRevScale( rev );
    logdisplay->setFillRevScale( fillrev );
    logdisplay->setFullFilled( isfullfilled );

    float maxval( 0 );
    for ( int idx=0; idx<crdvals.size(); idx++ )
    {
	float val = isfullfilled ? FULLFILLVALUE :
		    getValue( crdvals, idx, lp.islogarithmic_, scaler );

	const Coord3& pos = getPos( crdvals, idx );
	if ( mIsUdf(pos.z) || mIsUdf(val) )
	    continue;

	osg::Vec3Array* logPath = logdisplay->getPath();
	osg::FloatArray* shapeLog = logdisplay->getShapeLog();

	if(!logPath || !shapeLog)
	    continue;

	logPath->push_back(
	    osg::Vec3d((float) pos.x,(float) pos.y,(float) pos.z) );
	shapeLog->push_back(val);
	if ( val > maxval )
	    maxval = val;

	if ( isFilled )
	{
	    val = getValue( crdvalsF, idx, lp.islogarithmic_, scalerF );
	    if ( mIsUdf(val) )
		continue;

	    osg::FloatArray* fillLog = logdisplay->getFillLogValues();
	    if( !fillLog )
		continue;
	    fillLog->push_back( val );
	    osg::FloatArray* fillLogDepths = logdisplay->getFillLogDepths();
	    fillLogDepths->push_back(pos.z);
	}
    }

    if ( maxval )
    {
	updateMakerSize( maxval );
	updateMakerNamePosition( lp.side_, maxval );
    }

    logdisplay->setMaxFillValue( rgStopF );
    logdisplay->setMinFillValue( rgStartF );
    logdisplay->setMaxShapeValue( rgStop );
    logdisplay->setMinShapeValue( rgStart );

    showLog( showlogs_, lp.side_ );

    displaylog_[(int)lp.side_] = true;
    lognames_.get( (int)lp.side_ ) = lp.name_;
}


void Well::getLinScaleRange( const LinScaler& scaler,Interval<float>& selrg,
    float& rgstart, float& rgstop, bool islogarithmic_ )
{
    selrg.sort();
    rgstop = scaler.scale( selrg.stop );
    rgstart = scaler.scale( selrg.start );
    if ( islogarithmic_ )
    {
	mSclogval( rgstop );
	mSclogval( rgstart );
    }
}


void Well::getLinScale( const LogParams& lp,LinScaler& scaler, bool isFill )
{
    Interval<float> rg;
    if( isFill )
	rg = lp.valfillrange_;
    else
	rg = lp.valrange_;
    rg.sort();
    float minval = rg.start;
    float maxval = rg.stop;
    scaler.set( minval, 0, maxval, 100 );
}


#define mGetCoordVal(varnm,crdvals,idx) \
    if ( idx >= crdvals.size() ) \
	{ pErrMsg("Req idx out of range"); idx = crdvals.size() - 1; } \
    const Coord3Value& varnm = crdvals[idx]


Coord3 Well::getPos( const TypeSet<Coord3Value>& crdvals, int idx ) const
{
    mGetCoordVal(cv,crdvals,idx);

    Coord3 crd = cv.first;
    transformZIfNeeded( crd );
    if ( mIsUdf(crd.z) )
	return crd;

    Coord3 pos( crd );
    if ( transformation_ )
	transformation_->transform( crd, pos );
    return pos;
}


float Well::getValue( const TypeSet<Coord3Value>& crdvals, int idx,
		      bool sclog, const LinScaler& scaler ) const
{
    mGetCoordVal(cv,crdvals,idx);

    float val = (float) scaler.scale( cv.second );
    if ( mIsUdf(val) ) return mUdf(float);

    if ( val < 0 ) val = 0;
    if ( val > 100 ) val = 100;
    if ( sclog ) mSclogval(val);

    return val;
}


void Well::clearLog( Side side )
{
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->clearLog();
    displaylog_[(int)side] = false;
}


void Well::removeLogs()
{
    leftlogdisplay_->clearLog();
    rightlogdisplay_->clearLog();
    centerlogdisplay_->clearLog();
    for ( int idx=0; idx<3; idx++ )
	displaylog_[idx] = false;
}


void Well::setRepeat( int rpt, Side side )
{
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setRepeatNumber( rpt );
}


unsigned int Well::getRepeat( Side side ) const
{
    const osgGeo::WellLog* logdisplay = getLogDisplay( side );
    return logdisplay->getRepeatNumber();
}


float Well::getRepeatStep( Side side ) const
{
    const osgGeo::WellLog* logdisplay = getLogDisplay( side );
    return logdisplay->getRepeatStep();
}


void Well::setOverlapp( float ovlap, Side side )
{
    ovlap = 100 - ovlap;
    if ( ovlap < 0.0 || mIsUdf(ovlap)  ) ovlap = 0.0;
    if ( ovlap > 100.0 ) ovlap = 100.0;

    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setRepeatGap( ovlap );
}


void Well::setLogFill( bool fill, Side side )
{
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setLogFill( fill );
}


void Well::setLogStyle( int style, Side side )
{
    setLogTubeDisplay( side, style == 2 ? true : false );
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setSeisLogStyle( style == 1 ? true : false );
}


void Well::getLogStyle( Side side, int& style ) const
{
    const osgGeo::WellLog* logdisplay = getLogDisplay( side );

    if ( logdisplay->getSeisLogStyle() )
    {
	style = (int)Seismic;
	return;
    }

    if ( displaytube_[(int)side] )
	style = (int)Logtube;
    else
	style = (int)Welllog;
}


void Well::setLogColor( const OD::Color& col, Side side )
{
#define col2f(rgb) float(col.rgb())/255

    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    osg::Vec4 osgCol = Conv::to<osg::Vec4>(col);
    logdisplay->setLineColor( osgCol );
}


const OD::Color& Well::logColor( Side side ) const
{
    const osgGeo::WellLog* logdisplay = getLogDisplay( side );
    static OD::Color color;
    const osg::Vec4d& col = logdisplay->getLineColor();
    const int r = mNINT32(col[0]*255);
    const int g = mNINT32(col[1]*255);
    const int b = mNINT32(col[2]*255);
    color.set( (unsigned char)r, (unsigned char)g, (unsigned char)b );
    return color;
}


#define scolors2f(rgb) float(lp.seiscolor_.rgb())/255
#define colors2f(rgb) float(col.rgb())/255
void Well::setLogFillColorTab( const LogParams& lp, Side side  )
{
    int seqidx = ColTab::SM().indexOf( lp.seqname_ );
    if ( seqidx<0 || mIsUdf(seqidx) ) seqidx = 0;
    const ColTab::Sequence* seq = ColTab::SM().get( seqidx );

    osg::ref_ptr<osg::Vec4Array> clrTable = new osg::Vec4Array;
    for ( int idx=0; idx<256; idx++ )
    {
	const bool issinglecol = ( lp.style_ == Seismic ||
			(lp.style_ != Seismic && lp.issinglcol_ ) );
	float colstep = lp.iscoltabflipped_ ? 1-(float)idx/255 : (float)idx/255;
	OD::Color col = seq->color( colstep );
	float r = issinglecol ? scolors2f(r) : colors2f(r);
	float g = issinglecol ? scolors2f(g) : colors2f(g);
	float b = issinglecol ? scolors2f(b) : colors2f(b);
	clrTable->push_back(osg::Vec4(r,g,b,1.0));
    }

    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setFillLogColorTab( clrTable );
}


void Well::setLogLineDisplayed( bool isdisp, Side side )
{
    osgGeo::PlaneWellLog::DisplaySide dispside =
	(side==Left) ? osgGeo::PlaneWellLog::Left :
	(side==Right) ? osgGeo::PlaneWellLog::Right :
			osgGeo::PlaneWellLog::Center;

    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setDisplaySide( dispside );
}


bool Well::logLineDisplayed( Side side ) const
{
    osgGeo::WellLog* logdisplay =
			    const_cast<osgGeo::WellLog*>(getLogDisplay( side ));
    return logdisplay->getDisplayStatus();
}


void Well::setLogWidth( float width, Side side )
{
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay ->setLogWidth( width );
}


float Well::getLogWidth( Side side ) const
{
    const osgGeo::WellLog* logdisplay = getLogDisplay( side );
    return logdisplay->getLogWidth();
}


void Well::setLogLineWidth( int width, Side side )
{
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setLineWidth( width );
}


int Well::getLogLineWidth() const
{
    return mNINT32( leftlogdisplay_->getLineWidth() );
}


void Well::showLogs( bool yn )
{
    if( yn == logsShown() )
	return;

    if( yn )
    {
	addChild( leftlogdisplay_ );
	addChild( rightlogdisplay_ );
	addChild( centerlogdisplay_ );
    }
    else
    {
	removeChild( leftlogdisplay_ );
	removeChild( rightlogdisplay_ );
	removeChild( centerlogdisplay_ );
    }
}


void Well::showLog( bool yn, Side side )
{
    osgGeo::WellLog*& logdisplay = getLogDisplay( side );
    logdisplay->setShowLog( yn );
}


bool Well::logsShown() const
{
   return ( childIndex( leftlogdisplay_ )!= -1
       || childIndex( rightlogdisplay_ ) != -1
       || childIndex( centerlogdisplay_ ) != -1 ) ?
       true : false ;
}


bool Well::hasLog( Side side ) const
{
    return displaylog_[(int)side];
}


BufferString Well::getLogName( Side side ) const
{
    if ( displaylog_[(int)side] )
	return lognames_.get( (int)side );

    return BufferString::empty();
}

void Well::showLogName( bool yn )
{
}


bool Well::logNameShown() const
{ return false; }


void Well::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    track_->setDisplayTransformation( transformation_ );

    wellbottxt_->setDisplayTransformation( transformation_ );
    welltoptxt_->setDisplayTransformation( transformation_ );
    markernames_->setDisplayTransformation( transformation_ );
    markerset_->setDisplayTransformation( transformation_ );
}


const mVisTrans* Well::getDisplayTransformation() const
{ return transformation_; }


void Well::fillPar( IOPar& par ) const
{
    BufferString linestyle;
    lineStyle().toString( linestyle );
    par.set( linestylestr(), linestyle );

    par.setYN( showwelltopnmstr(), welltoptxt_->isOn() );
    par.setYN( showwellbotnmstr(), wellbottxt_->isOn() );
    par.setYN( showmarkerstr(), markersShown() );
    par.setYN( showmarknmstr(), markerNameShown() );
    par.setYN( showlogsstr(), logsShown() );
    par.setYN( showlognmstr(), logNameShown() );
    par.set( markerszstr(), markersize_ );
    par.set( logwidthstr(), getLogLineWidth() );
}


bool Well::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    BufferString linestyle;
    if ( par.get(linestylestr(),linestyle) )
    {
	OD::LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

#define mParGetYN(str,func) \
    doshow = true; \
    par.getYN(str,doshow); \
    func( doshow );

    bool doshow;
    mParGetYN(showwelltopnmstr(),showWellTopName);
    mParGetYN(showwellbotnmstr(),showWellBotName);
    mParGetYN(showmarkerstr(),showMarkers);	showmarkers_ = doshow;
    mParGetYN(showmarknmstr(),showMarkerName);  showlogs_ = doshow;
    mParGetYN(showlogsstr(),showLogs);
    mParGetYN(showlognmstr(),showLogName);

    par.get( markerszstr(), markersize_ );
    setMarkerScreenSize( markersize_ );

    return 1;
}


bool Well::getLogOsgData( LogStyle style, Side side, TypeSet<Coord3>& coords,
	TypeSet<OD::Color>& colors, TypeSet<TypeSet<int> >& pss,
	TypeSet<Coord3>& normals, bool path ) const
{
    if ( style==Logtube && !displaytube_[(int)side] )
	return false;

    if ( style==Welllog && displaytube_[(int)side] )
	return false;

    const osgGeo::WellLog* logdisplay = getLogDisplay( side );

    if ( !logdisplay )
	return false;

    osg::ref_ptr<osg::Geometry>  geom = 0;

    if ( style == Welllog || style== Seismic)
    {
	geom = path ?
	    logdisplay->getLogPathGeometry() : logdisplay->getLogGeometry();
    }
    else
    {
	geom = logdisplay->getTubeGeometry();
    }

    if ( !geom )
	return false;

    if ( geom->getNumPrimitiveSets()==0 ||
	 geom->getVertexArray()->getNumElements()==0 )
	return false;

    coords.erase();
    normals.erase();
    colors.erase();
    for ( int idx=0; idx<pss.size(); idx++ )
	pss[idx].erase();
    pss.erase();

    for ( int idx=0; idx<geom->getNumPrimitiveSets(); idx++ )
    {
	const osg::PrimitiveSet* osgps = geom->getPrimitiveSet( idx );
	if ( !osgps || osgps->getNumIndices()== 0 )
	    continue;

	TypeSet<int> ps;
	for ( int idy = 0; idy<osgps->getNumIndices(); idy++ )
	    ps += osgps->index( idy );

	pss += ps;
    }

    const osg::Vec3Array* vertices = mGetOsgVec3Arr( geom->getVertexArray() );
    const osg::Vec3Array* osgnormals = mGetOsgVec3Arr( geom->getNormalArray() );
    const osg::Vec4Array* clrarr = mGetOsgVec4Arr( geom->getColorArray() );

    for ( int idx=0; idx<vertices->size(); idx++ )
	coords += Conv::to<Coord3>( (*vertices)[idx] );

    for ( int idx=0; idx<osgnormals->size(); idx++ )
	normals += Conv::to<Coord3>( (*osgnormals)[idx] );

    for ( int idx=0; idx<clrarr->size(); idx++ )
	colors += Conv::to<::OD::Color>( (*clrarr)[idx] );

    return true;
}

} // namespace visBase
