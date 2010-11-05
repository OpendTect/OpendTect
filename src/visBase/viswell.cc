/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: viswell.cc,v 1.61 2010-11-05 12:46:28 cvsbruno Exp $";

#include "viswell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "vismarker.h"
#include "survinfo.h"
#include "coltabsequence.h"
#include "iopar.h"
#include "ranges.h"
#include "scaler.h"

#include "SoPlaneWellLog.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoSwitch.h>

mCreateFactoryEntry( visBase::Well );

namespace visBase
{

static const int cMaxNrLogSamples = 2000;

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
    , transformation_(0)
				       
{
    SoSeparator* sep = new SoSeparator;
    addChild( sep );
    drawstyle_ = DrawStyle::create();
    drawstyle_->ref();
    sep->addChild( drawstyle_->getInventorNode() );

    track_ = PolyLine::create();
    track_->ref();
    track_->setMaterial( Material::create() );
    sep->addChild( track_->getInventorNode() );
    welltoptxt_= Text2::create();
    wellbottxt_ = Text2::create();
    welltoptxt_->ref();
    wellbottxt_->ref();
    welltoptxt_->setMaterial( track_->getMaterial() );
    wellbottxt_->setMaterial( track_->getMaterial() );
    sep->addChild( welltoptxt_->getInventorNode() );
    sep->addChild( wellbottxt_->getInventorNode() );

    markergroup_ = DataObjectGroup::create();
    markergroup_->ref();
    addChild( markergroup_->getInventorNode() );

    markernmswitch_ = new SoSwitch;
    addChild( markernmswitch_);
    markernames_ = DataObjectGroup::create();
    markernames_->setSeparate( false );
    markernames_->ref();
    markernmswitch_->addChild( markernames_->getInventorNode() );
    markernmswitch_->whichChild = 0;

    lognmswitch_ = new SoSwitch;
    lognmleft_ = Text2::create();
    lognmswitch_->addChild( lognmleft_->getInventorNode() );
    lognmright_ = Text2::create();
    lognmswitch_->addChild( lognmright_->getInventorNode() );
    lognmswitch_->whichChild = 0;

    setRepeat(0);
}


Well::~Well()
{
    if ( transformation_ ) transformation_->unRef();

    removeChild( welltoptxt_->getInventorNode() );
    welltoptxt_->unRef();
    removeChild( wellbottxt_->getInventorNode() );
    wellbottxt_->unRef();

    removeChild( track_->getInventorNode() );
    track_->unRef();

    removeChild( drawstyle_->getInventorNode() );
    drawstyle_->unRef();

    markergroup_->removeAll();
    removeChild( markergroup_->getInventorNode() );
    markergroup_->unRef();

    markernames_->removeAll();
    removeChild( markernames_->getInventorNode() );
    markernames_->unRef();

    removeLogs();
}


void Well::setTrack( const TypeSet<Coord3>& pts )
{
    while ( track_->size()>pts.size() )
	track_->removePoint( 0 );

    track_->setDisplayTransformation( transformation_ );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	const Coord3& crd = pts[idx];
	if ( idx>=track_->size() )
	    track_->addPoint( crd );
	else
	    track_->setPoint( idx, crd );
    }
}


void Well::setTrackProperties( Color& col, int width)
{
    track_->getMaterial()->setColor( col );
    drawstyle_->setLineWidth( width );
}


void Well::setLineStyle( const LineStyle& lst )
{
    track_->getMaterial()->setColor( lst.color_ );
    drawstyle_->setLineStyle( lst ); 
}


const LineStyle& Well::lineStyle() const
{
    static LineStyle ls;
    ls.type_ = drawstyle_->lineStyle().type_;
    ls.width_ = drawstyle_->lineStyle().width_;
    ls.color_ = track_->getMaterial()->getColor();
    return ls;
}

#define mSetWellName( nm, pos, post, sz ) \
    well##post##txt_->setDisplayTransformation( transformation_ ); \
    well##post##txt_->setText( nm ); \
    well##post##txt_->setSize( sz ); \
    if ( !SI().zRange(true).includes(pos->z) ) \
	pos->z = SI().zRange(true).limitValue( pos->z ); \
    well##post##txt_->setPosition( *pos ); \
    well##post##txt_->setJustification( Text::Center );

void Well::setWellName( const TrackParams& tp )
{
    mSetWellName( tp.isdispabove_ ? tp.name_ : "", tp.toppos_, top, tp.namesz_);
    mSetWellName( tp.isdispbelow_ ? tp.name_ : "", tp.botpos_, bot, tp.namesz_);
}


void Well::showWellTopName( bool yn )
{ welltoptxt_->turnOn( yn ); }


void Well::showWellBotName( bool yn )
{ wellbottxt_->turnOn( yn ); }


bool Well::wellTopNameShown() const
{ return welltoptxt_->isOn(); }


bool Well::wellBotNameShown() const
{ return wellbottxt_->isOn(); }


void Well::addMarker( const MarkerParams& mp )
{
    Marker* marker = Marker::create();
    SoSeparator* markershapesep = new SoSeparator;
    markershapesep->ref();
    SoCoordinate3* markercoords= new SoCoordinate3;
    markershapesep->addChild(markercoords);

    if ( mp.shapeint_ == 0 )
    {
	marker->setType( MarkerStyle3D::Cylinder );
	SoCylinder* markershape = new SoCylinder;
	markershape->radius.setValue( 1 );
	const int height = mp.cylinderheight_;
	markershape->height.setValue( (float)height/10 );
	SoCylinder::Part part = height ? SoCylinder::ALL : SoCylinder::BOTTOM;
	markershape->parts.setValue( part );
	markershapesep->addChild( markershape );
    }
    else if ( mp.shapeint_ == 2 )
    {
	marker->setType( MarkerStyle3D::Sphere );
	SoSphere* markershape = new SoSphere;
	markershape->radius.setValue( 0.5 );
	markershapesep->addChild(markershape);
    }
    else
    {
	marker->setType( MarkerStyle3D::Cube );
	SoFaceSet* markershape = new SoFaceSet;
	markercoords->point.set1Value(0,-1,-1,0);
	markercoords->point.set1Value(1,-1, 1,0);
	markercoords->point.set1Value(2, 1, 1,0);
	markercoords->point.set1Value(3, 1,-1,0);
	markershape->numVertices.setValue(4);
	markershapesep->addChild(markershape);
    }	

    marker->setMarkerShape(markershapesep);
    markershapesep->unref();

    //marker->doRestoreProportions(false);
    markergroup_->addObject( marker );
    marker->setDisplayTransformation( transformation_ );
    marker->setCenterPos( *mp.pos_ );
    marker->getMaterial()->setColor( mp.col_ );
    marker->setScreenSize( mp.size_ );
    marker->turnOn( showmarkers_ );

    Text2* markernm = Text2::create();
    markernm->setDisplayTransformation( transformation_ );
    markernm->setText( mp.name_ );
    markernm->setSize( mp.namesize_ );
    markernm->setPosition( *mp.pos_ );
    markernm->setJustification( Text::Left );
    markernm->getMaterial()->setColor( mp.namecol_ );
    markernames_->addObject( markernm );
}


void Well::removeAllMarkers() 
{
    markergroup_->removeAll();
    markernames_->removeAll();
}


void Well::setMarkerScreenSize( int size )
{
    markersize_ = size;
    for ( int idx=0; idx<markergroup_->size(); idx++ )
    {
	mDynamicCastGet(Marker*,marker,markergroup_->getObject(idx))
	marker->setScreenSize( size );
    }
}


int Well::markerScreenSize() const
{
    mDynamicCastGet(Marker*,marker,markergroup_->getObject(0))
    return marker ? mNINT(marker->getScreenSize()) : markersize_;
}


bool Well::canShowMarkers() const
{ return markergroup_->size(); }


void Well::showMarkers( bool yn )
{
    showmarkers_ = yn;
    for ( int idx=0; idx<markergroup_->size(); idx++ )
    {
	mDynamicCastGet(Marker*,marker,markergroup_->getObject(idx))
	marker->turnOn( yn );
    }
}


bool Well::markersShown() const
{
    mDynamicCastGet(Marker*,marker,markergroup_->getObject(0))
    return marker && marker->isOn();
}


void Well::showMarkerName( bool yn )
{ markernmswitch_->whichChild = yn ? 0 : SO_SWITCH_NONE; }


bool Well::markerNameShown() const
{ return markernmswitch_->whichChild.getValue()==0; }


#define mGetLoopSize(nrsamp,step)\
{\
    if ( nrsamp > cMaxNrLogSamples )\
    {\
	step = (float)nrsamp/cMaxNrLogSamples;\
	nrsamp = cMaxNrLogSamples;\
    }\
}
void Well::setLogData( const TypeSet<Coord3Value>& crdvals, 
		       const LogParams& lp )
{
    Interval<float> rg = lp.range_; 
    const bool rev = rg.start > rg.stop;
    const bool isfullfilled = lp.isleftfilled_ && lp.isrightfilled_; 
    const bool fillrev = !isfullfilled &&  
		      ( ( lp.lognr_ == 1 && lp.isleftfilled_ && !rev )
		     || ( lp.lognr_ == 1 && lp.isrightfilled_ && rev )
		     || ( lp.lognr_ == 2 && lp.isrightfilled_ && !rev )
		     || ( lp.lognr_ == 2 && lp.isleftfilled_ && rev ) );

    for ( int idx=0; idx<log_.size(); idx++ )
    {
	log_[idx]->setRevScale( rev, lp.lognr_ );
	log_[idx]->setFillRevScale( fillrev, lp.lognr_ );
    }
    rg.sort();
    LinScaler scaler( rg.start, 0, rg.stop, 100 );
    
    int nrsamp = crdvals.size();
    float step = 1;
    mGetLoopSize( nrsamp, step );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int index = mNINT(idx*step);
	const float val = isfullfilled ? 100 : 
	    		getValue( crdvals, index, lp.islogarithmic_, scaler );
	const Coord3& pos = getPos( crdvals, index );
	if ( mIsUdf( pos.z ) || mIsUdf( val ) )
	    continue;   
	
	for ( int lidx=0; lidx<log_.size(); lidx++ )
	    log_[lidx]->setLogValue( idx, SbVec3f(pos.x,pos.y,pos.z), 
		    		     val, lp.lognr_ );
    }
    showLog( true, lp.lognr_ );
}


#define mSclogval(val)\
{\
    val += 1;\
    val = ::log( val );\
}
void Well::setFilledLogData( const TypeSet<Coord3Value>& crdvals, 
			     const LogParams& lp )
{
    Interval<float> rg = lp.valfillrange_;
    rg.sort();
    float minval = rg.start;
    float maxval = rg.stop;
    float newminval = crdvals[0].value;
    float newmaxval = newminval;

    int nrsamp = crdvals.size();
    float step = 1;
    mGetLoopSize( nrsamp, step );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int index = mNINT(idx*step);
	const Coord3Value& cv = crdvals[index];
	const float val = cv.value;
	if ( !mIsUdf(val) )
	{
	    if ( newminval>val ) newminval = val;
	    if ( newmaxval<val ) newmaxval = val;
	}
    }

    LinScaler scaler( newminval, 0, newmaxval, 100 );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int index = mNINT(idx*step);
	const float val = getValue( crdvals, index, lp.islogarithmic_, scaler );
	const Coord3& pos = getPos( crdvals, index );
	if ( mIsUdf( pos.z ) || mIsUdf( val ) )
	    continue;   
	for ( int lidx=0; lidx<log_.size(); lidx++ )
	    log_[lidx]->setFillLogValue( idx, val, lp.lognr_ );
    }

    const float diffminval = minval - newminval;
    const float diffmaxval = maxval - newmaxval;
    Interval<float> selrg = lp.fillrange_;
    selrg.sort();
    float rgstop = scaler.scale(selrg.stop-diffmaxval);
    float rgstart = scaler.scale(selrg.start-diffminval);
    if ( lp.islogarithmic_ )
    {
	mSclogval( rgstop ); 
	mSclogval( rgstart ); 
    }
    
    for ( int logidx=0; logidx<log_.size(); logidx++ )
	log_[logidx]->setFillExtrValue( rgstop, rgstart, lp.lognr_ );
    showLog( true, lp.lognr_ );
}


const Coord3 Well::getPos( const TypeSet<Coord3Value>& crdv, int idx ) const 
{
    const Coord3Value& cv = crdv[idx];
    Coord3 pos( 0,0,0 );
    if ( transformation_ )
	pos = transformation_->transform( cv.coord );
    return pos;
}


const float Well::getValue( const TypeSet<Coord3Value>& crdvals, int idx, 
			    bool sclog, const LinScaler& scaler ) const
{
    const Coord3Value& cv = crdvals[idx];
    float val = scaler.scale( cv.value );
    if ( val < 0 || mIsUdf(val) ) val = 0;
    if ( val > 100 ) val = 100;
    if ( sclog ) mSclogval(val);

    return val;
}


void Well::clearLog( int lognr )
{
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->clearLog( lognr );
}


void Well::hideUnwantedLogs( int lognr, int rpt )
{ 
    for ( int idx=rpt; idx<log_.size(); idx++)
	showOneLog(false, lognr, idx);	
}


void Well::removeLogs()
{
    for ( int idx=log_.size()-1; idx>=0; idx-- )
    {
	log_[idx]->unrefNoDelete();
	removeChild( log_[idx]  );
	log_.remove( idx );
    }
}


void Well::setRepeat( int rpt )
{
    if ( rpt < 0 || mIsUdf(rpt) ) rpt = 0; 
    const int lz=log_.size();

    for ( int idx=lz; idx<rpt; idx++ )
    {
	log_ += new SoPlaneWellLog;
	log_[idx]->setLogConstantSize( log_[0]->logConstantSize() );
	addChild( log_[idx] );
    }
}


void Well::setOverlapp( float ovlap, int lognr )
{
    ovlap = 100 - ovlap;
    if ( ovlap < 0.0 || mIsUdf(ovlap)  ) ovlap = 0.0;
    if ( ovlap > 100.0 ) ovlap = 100.0;
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setShift( idx*ovlap, lognr );
}


void Well::setLogFill( bool fill, int lognr )
{
    if ( log_.size() > 0 )
	log_[0]->setLogFill( fill, lognr );
}


void Well::setLogStyle( bool style, int lognr )
{
    for ( int idx=0; idx<log_.size(); idx++ )
	log_[idx]->setLogStyle( (1-style), lognr );
}


void Well::setLogColor( const Color& col, int lognr )
{
#define col2f(rgb) float(col.rgb())/255
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setLineColor( SbVec3f(col2f(r),col2f(g),col2f(b)), lognr );
}


const Color& Well::logColor( int lognr ) const
{
    static Color color;
    const SbVec3f& col = log_[0]->lineColor( lognr );
    const int r = mNINT(col[0]*255);
    const int g = mNINT(col[1]*255);
    const int b = mNINT(col[2]*255);
    color.set( (unsigned char)r, (unsigned char)g, (unsigned char)b );
    return color;
}


#define scolors2f(rgb) float(lp.seiscolor_.rgb())/255
#define colors2f(rgb) float(col.rgb())/255
void Well::setLogFillColorTab( const LogParams& lp, int lognr )
{
    int idx = ColTab::SM().indexOf( lp.seqname_ );
    if ( idx<0 || mIsUdf(idx) ) idx = 0;
    const ColTab::Sequence* seq = ColTab::SM().get( idx );

    float colors[256][3];
    for (int idx=0; idx<256; idx++ )
    {
	const bool issinglecol = ( !lp.iswelllog_ || 
	    		(lp.iswelllog_ && lp.issinglcol_ ) );
	float colstep = lp.iscoltabflipped_ ? 1-(float)idx/255 : (float)idx/255;
	Color col = seq->color( colstep );
	colors[idx][0] = issinglecol ? scolors2f(r) : colors2f(r);
	colors[idx][1] = issinglecol ? scolors2f(g) : colors2f(g);
	colors[idx][2] = issinglecol ? scolors2f(b) : colors2f(b);
    }

    for ( int idx=0; idx<log_.size(); idx++ )
	log_[idx]->setFilledLogColorTab( colors, lognr );
}


void Well::setLogLineDisplayed( bool isdisp, int lognr )
{
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setLineDisplayed( isdisp, lognr );
}


bool Well::logLineDisplayed( int lognr ) const 
{
    return log_.size() ? log_[0]->lineWidth( lognr ) : 0 ;
}


void Well::setLogLineWidth( float width, int lognr )
{
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setLineWidth( width, lognr );
}


float Well::logLineWidth( int lognr ) const
{
    return log_.size() ? log_[0]->lineWidth( lognr ) : 0 ;
}


void Well::setLogWidth( int width, int lognr )
{
    if (lognr == 1)
    {
	for ( int i=0; i<log_.size(); i++ )
	    log_[i]->screenWidth1.setValue( (float)width );
    }
    else
    {
	for ( int i=0; i<log_.size(); i++ )
	    log_[i]->screenWidth2.setValue( (float)width );
    }
}


int Well::logWidth() const
{
    return log_.size() ? mNINT(log_[0]->screenWidth1.getValue()) 
		      || mNINT(log_[0]->screenWidth2.getValue()) : false;
}


void Well::showLogs( bool yn )
{
    for ( int idx=0; idx<log_.size(); idx++ )
    {
        log_[idx]->showLog( yn, 1 );
        log_[idx]->showLog( yn, 2 );
    }
}


void Well::showLog( bool yn, int lognr )
{
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->showLog( yn, lognr );
}


void Well::showOneLog( bool yn, int lognr, int idx )
{
    log_[idx]->showLog( yn, lognr );
}


bool Well::logsShown() const
{
    return log_.size() ? log_[0]->logShown(1) || log_[0]->logShown(2) : false;
}


void Well::setLogConstantSize( bool yn )
{
    for ( int idx=0; idx<log_.size(); idx++ )
	log_[idx]->setLogConstantSize( yn );
}


bool Well::logConstantSize() const
{
    return log_.size() ? log_[0]->logConstantSize() : true;
}


void Well::showLogName( bool yn )
{} 


bool Well::logNameShown() const
{ return false; }


void Well::setDisplayTransformation( Transformation* nt )
{
    if ( transformation_ )
	transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();
}


Transformation* Well::getDisplayTransformation()
{ return transformation_; }


void Well::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

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
    par.set( logwidthstr(), logWidth() );
}


int Well::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    BufferString linestyle;
    if ( par.get(linestylestr(),linestyle) )
    {
	LineStyle lst;
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
    mParGetYN(showmarknmstr(),showMarkerName);
    mParGetYN(showlogsstr(),showLogs);
    mParGetYN(showlognmstr(),showLogName);

    par.get( markerszstr(), markersize_ );
    setMarkerScreenSize( markersize_ );

    return 1;
}

}; // namespace visBase
