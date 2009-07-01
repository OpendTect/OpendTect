/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: viswell.cc,v 1.47 2009-07-01 07:49:50 cvsbruno Exp $";

#include "viswell.h"
#include "vispolyline.h"
#include "visdrawstyle.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vismaterial.h"
#include "vistransform.h"
#include "vismarker.h"
#include "survinfo.h"
#include "iopar.h"
#include "ranges.h"
#include "undefval.h"
#include "scaler.h"

#include "coltabsequence.h"


#include "SoPlaneWellLog.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>

mCreateFactoryEntry( visBase::Well );

namespace visBase
{

static const int sMaxNrLogSamples = 2000;

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
    , showmarkers(true)
    , transformation(0)
				       
{
    SoSeparator* sep = new SoSeparator;
    addChild( sep );
    drawstyle = DrawStyle::create();
    drawstyle->ref();
    sep->addChild( drawstyle->getInventorNode() );

    track = PolyLine::create();
    track->ref();
    track->setMaterial( Material::create() );
    sep->addChild( track->getInventorNode() );
    welltoptxt = Text2::create();
    wellbottxt = Text2::create();
    welltoptxt->ref();
    wellbottxt->ref();
    welltoptxt->setMaterial( track->getMaterial() );
    wellbottxt->setMaterial( track->getMaterial() );
    sep->addChild( welltoptxt->getInventorNode() );
    sep->addChild( wellbottxt->getInventorNode() );

    markergroup = DataObjectGroup::create();
    markergroup->ref();
    addChild( markergroup->getInventorNode() );

    markernmswitch = new SoSwitch;
    addChild( markernmswitch );
    markernames = DataObjectGroup::create();
    markernames->setSeparate( false );
    markernames->ref();
   markernmswitch->addChild( markernames->getInventorNode() );
    markernmswitch->whichChild = 0;

    lognmswitch = new SoSwitch;
    lognmleft = Text2::create();
    lognmswitch->addChild( lognmleft->getInventorNode() );
    lognmright = Text2::create();
    lognmswitch->addChild( lognmright->getInventorNode() );
    lognmswitch->whichChild = 0;

    setRepeat(0);
}


Well::~Well()
{
    if ( transformation ) transformation->unRef();

    removeChild( welltoptxt->getInventorNode() );
    welltoptxt->unRef();
    removeChild( wellbottxt->getInventorNode() );
    wellbottxt->unRef();

    removeChild( track->getInventorNode() );
    track->unRef();

    removeChild( drawstyle->getInventorNode() );
    drawstyle->unRef();

    markergroup->removeAll();
    removeChild( markergroup->getInventorNode() );
    markergroup->unRef();

    markernames->removeAll();
    removeChild( markernames->getInventorNode() );
    markernames->unRef();
}


void Well::setTrack( const TypeSet<Coord3>& pts )
{
    while ( track->size()>pts.size() )
	track->removePoint( 0 );

    track->setDisplayTransformation( transformation );
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	const Coord3& crd = pts[idx];
	if ( idx>=track->size() )
	    track->addPoint( crd );
	else
	    track->setPoint( idx, crd );
    }
}


void Well::setTrackProperties( Color& col, int width )
{
    track->getMaterial()->setColor( col );
    drawstyle->setLineWidth( width );
}


void Well::setLineStyle( const LineStyle& lst )
{
    track->getMaterial()->setColor( lst.color_ );
    drawstyle->setLineStyle( lst ); 
}


const LineStyle& Well::lineStyle() const
{
    static LineStyle ls;
    ls.type_ = drawstyle->lineStyle().type_;
    ls.width_ = drawstyle->lineStyle().width_;
    ls.color_ = track->getMaterial()->getColor();
    return ls;
}

#define msetWellName( nm, pos, post, sz ) \
    well##post##txt->setDisplayTransformation( transformation ); \
    well##post##txt->setText( nm ); \
    well##post##txt->setSize( sz ); \
    if ( !SI().zRange(true).includes(pos.z) ) \
	pos.z = SI().zRange(true).limitValue( pos.z ); \
    well##post##txt->setPosition( pos ); \
    well##post##txt->setJustification( Text::Center ); 


void Well::setWellName( const char* nm, Coord3 toppos, Coord3 botpos,
       				bool isnameabove, bool isnamebelow, int sz )
{
    if ( isnameabove && !isnamebelow )
    {
	msetWellName( nm, toppos, top, sz );
	msetWellName( "", botpos, bot, sz );
    }

    else if ( isnamebelow && !isnameabove )
    {
	msetWellName( nm, botpos, bot, sz );
	msetWellName( "", toppos, top, sz );
    }

    else if ( isnameabove && isnamebelow )
    {
	msetWellName( nm, toppos, top, sz );
	msetWellName( nm, botpos, bot, sz );
    }
    else
    {
	msetWellName( "", toppos, top, sz );
	msetWellName( "", botpos, bot, sz );
    }
}


void Well::showWellTopName( bool yn )
{ welltoptxt->turnOn( yn ); }


void Well::showWellBotName( bool yn )
{ wellbottxt->turnOn( yn ); }


bool Well::wellTopNameShown() const
{ return welltoptxt->isOn(); }


bool Well::wellBotNameShown() const
{ return wellbottxt->isOn(); }


void Well::addMarker( const Coord3& pos, const Color& color,
		    const char* nm, bool iscircular, int nmsize)
{
    Marker* marker = Marker::create();

    SoSeparator* markershapesep = new SoSeparator;
    markershapesep->ref();
    SoCoordinate3* markercoords= new SoCoordinate3;
    markershapesep->addChild(markercoords);

    if ( !iscircular )
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
    else
    {
	marker->setType( MarkerStyle3D::Cylinder );
	SoCylinder* markershape = new SoCylinder;
	markershape->height.setValue(0);
	markershape->radius.setValue(1);
	markershape->parts.setValue(SoCylinder::BOTTOM);
	markershapesep->addChild(markershape);
    }	


    marker->setMarkerShape(markershapesep);
    markershapesep->unref();

    marker->doRestoreProportions(false);
    markergroup->addObject( marker );
    marker->setDisplayTransformation( transformation );
    marker->setCenterPos( pos );
    marker->getMaterial()->setColor( color );
    marker->setScreenSize( markersize );
    marker->turnOn( showmarkers );

    Text2* markernm = Text2::create();
    markernm->setDisplayTransformation( transformation );
    markernm->setText( nm );
    markernm->setSize( nmsize );
    markernm->setPosition( pos );
    markernm->setJustification( Text::Left );
    markernames->addObject( markernm );
}


void Well::removeAllMarkers() 
{
    markergroup->removeAll();
    markernames->removeAll();
}


void Well::setMarkerScreenSize( int size )
{
    markersize = size;
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(Marker*,marker,markergroup->getObject(idx))
	marker->setScreenSize( size );
    }
}


int Well::markerScreenSize() const
{
    mDynamicCastGet(Marker*,marker,markergroup->getObject(0))
    return marker ? mNINT(marker->getScreenSize()) : markersize;
}


bool Well::canShowMarkers() const
{ return markergroup->size(); }


void Well::showMarkers( bool yn )
{
    showmarkers = yn;
    for ( int idx=0; idx<markergroup->size(); idx++ )
    {
	mDynamicCastGet(Marker*,marker,markergroup->getObject(idx))
	marker->turnOn( yn );
    }
}


bool Well::markersShown() const
{
    mDynamicCastGet(Marker*,marker,markergroup->getObject(0))
    return marker && marker->isOn();
}


void Well::showMarkerName( bool yn )
{ markernmswitch->whichChild = yn ? 0 : SO_SWITCH_NONE; }


bool Well::markerNameShown() const
{ return markernmswitch->whichChild.getValue()==0; }



void Well::initializeData( int lognr, const Interval<float>& range,
			   float& step, int& nrsamp )
{
    step = 1;
    if ( nrsamp > sMaxNrLogSamples )
    {
	step = (float)nrsamp / sMaxNrLogSamples;
	nrsamp = sMaxNrLogSamples;
    }
}


void Well::setSampleData( const TypeSet<Coord3Value>& crdvals,int idx,
			  int nrsamp, float step, Coord3& pos,
			  bool sclog, float prevval, int lognr
			  ,const LinScaler& scaler, float& val)
{
    bool isvalundef = false;
    int index = mNINT(idx*step);
    const Coord3Value& cv = crdvals[index];
    pos = cv.coord ;
    if ( transformation )
	pos = transformation->transform( pos );
    if ( mIsUdf(pos.z) ) return;
	val = scaler.scale( cv.value );
    if ( mIsUdf(val) )
    {
	val = 105; //for undef value (>100), to be set transparent
	isvalundef = true;
    }
    else if ( val < 0 )
	val = 0;
    else if ( val > 100 )
	val = 100;

    if ( sclog && !isvalundef )
    {
	val += 1;
	val = ::log( val );
    }
}


void Well::setLogData( const TypeSet<Coord3Value>& crdvals, const char* lognm,
		       const Interval<float>& range, bool sclog, int lognr )
{
    int nrsamp = crdvals.size();
    float step;
    initializeData( lognr, range, step, nrsamp );
    const bool rev = range.start > range.stop;
    for ( int idx=0; idx<log.size(); idx++ )
	log[idx]->setRevScale( rev, lognr );
    Interval<float> rg = range; rg.sort();
    LinScaler scaler( rg.start, 0, rg.stop, 100 );
    Coord3 pos;
    float val;
    float prevval = 0;

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	setSampleData( crdvals, idx, nrsamp, step, pos, sclog,
			prevval, lognr, scaler, val );

	for ( int i=0; i<log.size(); i++ )
	    log[i]->setLogValue( idx, SbVec3f(pos.x,pos.y,pos.z), val, lognr );
	prevval = val;
    }
    showLog( true, lognr );
}


void Well::setFillLogData( const TypeSet<Coord3Value>& crdvals,
       			const char* lognm, const Interval<float>& range,
       			bool sclog, int lognr )
{
    int nrsamp = crdvals.size();
    float step;
    initializeData( lognr, range, step, nrsamp );
    Interval<float> rg = range; rg.sort();
    LinScaler scaler( rg.start, 0, rg.stop, 100 );
    Coord3 pos;
    float val;
    float prevval = 0;

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	setSampleData( crdvals, idx, nrsamp, step, pos, sclog,
			prevval, lognr, scaler, val );

	for ( int i=0; i<log.size(); i++ )
	    log[i]->setFillLogValue( idx, SbVec3f(pos.x,pos.y,pos.z), val, lognr );
	prevval = val;
    }
}


void Well::clearLog( int lognr )
{
    for ( int i=0; i<log.size(); i++ )
        log[i]->clearLog( lognr );
}


void Well::hideUnwantedLogs( int lognr, int rpt )
{ 
    for ( int i=rpt; i<log.size(); i++)
	showOneLog(false, lognr, i);	
}


void Well::removeLog( const int rpt, int lognr)
{
    int lz=log.size();

    for ( int k=0; k<lz; k++)
	log[k]->resetLogData( lognr );

    for ( int i=1; i<=lz-rpt; i++ )
    {
	log[lz-i]->clearLog(1);
	log[lz-i]->clearLog(2);
	removeChild( log[lz-i] );
	log.remove( lz-i );
    }
}


void Well::setRepeat( int rpt )
{
    if ( rpt < 0 || mIsUdf(rpt) ) rpt = 0; 
    int lz=log.size();

    if (rpt > lz)   
    {
	for ( int i=lz; i<rpt; i++ )
	{
	    log += new SoPlaneWellLog;
	    addChild( log[i] );
	}
    }
}


void Well::setOverlapp( float ovlap, int lognr )
{
    ovlap = 100 - ovlap;
    if ( ovlap < 0.0 || mIsUdf(ovlap)  ) ovlap = 0.0;
    if ( ovlap > 100.0 || mIsUdf(ovlap)  ) ovlap = 100.0;
    for ( int i=0; i<log.size(); i++ )
        log[i]->setShift( i*ovlap, lognr );
}


void Well::setLogFill( bool fill, int lognr )
{
    if ( log.size() > 0 )
	log[0]->setLogFill( fill, lognr );
}


void Well::setLogStyle( bool style, int lognr )
{
    for ( int i=0; i<log.size(); i++ )
	log[i]->setLogStyle( (1-style), lognr );
}


void Well::setLogColor( const Color& col, int lognr )
{
#define col2f(rgb) float(col.rgb())/255
    for ( int i=0; i<log.size(); i++ )
        log[i]->setLineColor( SbVec3f(col2f(r),col2f(g),col2f(b)), lognr );
}


const Color& Well::logColor( int lognr ) const
{
    static Color color;
    const SbVec3f& col = log[0]->lineColor( lognr );
    const int r = mNINT(col[0]*255);
    const int g = mNINT(col[1]*255);
    const int b = mNINT(col[2]*255);
    color.set( (unsigned char)r, (unsigned char)g, (unsigned char)b );
    return color;
}


void Well::setLogFillColorTab( const char* seqname, int lognr,
			       const Color& color, const bool iswelllog,
       				const bool issinglecol	)
{
#define scolors2f(rgb) float(color.rgb())/255
#define colors2f(rgb) float(Col.rgb())/255
	
    float colors[257][3];
    int idx= ColTab::SM().indexOf(seqname);
    if (idx<0 || mIsUdf(idx))
	idx=0;
    const ColTab::Sequence* seq = ColTab::SM().get( idx);
    Color Col;

    for (int idx=0; idx<256; idx++ )
    {
	if ( (!iswelllog || ( iswelllog && issinglecol )) )
	{
	    colors[idx][0] = scolors2f(r);
	    colors[idx][1] = scolors2f(g);
	    colors[idx][2] = scolors2f(b);
	}
	else
	{
	    Col = seq->color((float)idx/255);
	    colors[idx][0] = colors2f(r);
	    colors[idx][1] = colors2f(g);
	    colors[idx][2] = colors2f(b);
	}
    }
	    colors[256][0] = 0;
	    colors[256][1] = 0;
	    colors[256][2] = 0;

    for ( int idx=0; idx<log.size(); idx++ )
	log[idx]->setLogFillColorTab( colors, lognr );
}


void Well::setLogLineWidth( float width, int lognr )
{
    for ( int i=0; i<log.size(); i++ )
        log[i]->setLineWidth( width, lognr );
}


float Well::logLineWidth( int lognr ) const
{
    return log.size() ? log[0]->lineWidth( lognr ) : 0 ;
}


void Well::setLogWidth( int width, int lognr )
{
    if (lognr == 1)
    {
	for ( int i=0; i<log.size(); i++ )
	    log[i]->screenWidth1.setValue( (float)width );
    }
    else
    {
	for ( int i=0; i<log.size(); i++ )
	    log[i]->screenWidth2.setValue( (float)width );
    }
}


int Well::logWidth() const
{
    return log.size() ? mNINT(log[0]->screenWidth1.getValue()) 
	|| mNINT(log[0]->screenWidth2.getValue()) : false;
}


void Well::showLogs( bool yn )
{
    for ( int i=0; i<log.size(); i++ )
    {
        log[i]->showLog( yn, 1 );
        log[i]->showLog( yn, 2 );
    }
}

void Well::showLog( bool yn, int lognr )
{
    for ( int i=0; i<log.size(); i++ )
        log[i]->showLog( yn, lognr );
}


void Well::showOneLog( bool yn, int lognr, int i )
{
    log[i]->showLog( yn, lognr );
}


bool Well::logsShown() const
{
    return log.size() ? log[0]->logShown(1) || log[0]->logShown(2) : false;
}


void Well::showLogName( bool yn )
{} 

bool Well::logNameShown() const
{ return false; }


void Well::setDisplayTransformation( Transformation* nt )
{
    if ( transformation )
	transformation->unRef();
    transformation = nt;
    if ( transformation )
	transformation->ref();
}


Transformation* Well::getDisplayTransformation()
{ return transformation; }


void Well::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    BufferString linestyle;
    lineStyle().toString( linestyle );
    par.set( linestylestr(), linestyle );

    par.setYN( showwelltopnmstr(), welltoptxt->isOn() );
    par.setYN( showwellbotnmstr(), wellbottxt->isOn() );
    par.setYN( showmarkerstr(), markersShown() );
    par.setYN( showmarknmstr(), markerNameShown() );
    par.setYN( showlogsstr(), logsShown() );
    par.setYN( showlognmstr(), logNameShown() );
    par.set( markerszstr(), markersize );
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
    mParGetYN(showmarkerstr(),showMarkers);	showmarkers = doshow;
    mParGetYN(showmarknmstr(),showMarkerName);
    mParGetYN(showlogsstr(),showLogs);
    mParGetYN(showlognmstr(),showLogName);

    par.get( markerszstr(), markersize );
    setMarkerScreenSize( markersize );

    return 1;
}

}; // namespace visBase
