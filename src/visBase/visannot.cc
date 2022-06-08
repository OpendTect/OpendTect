/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/


#include "visannot.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vismaterial.h"
#include "ranges.h"
#include "visosg.h"
#include "samplingdata.h"
#include "axislayout.h"
#include "iopar.h"
#include "survinfo.h"
#include "uistring.h"
#include "vistransform.h"
#include "threadwork.h"
#include "visscene.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/Shader>

#include <osgGeo/OneSideRender>
#include <osgGeo/VolumeTechniques>


mCreateFactoryEntry( visBase::Annotation )

namespace visBase
{

const char* Annotation::textprefixstr()	    { return "Text "; }
const char* Annotation::cornerprefixstr()   { return "Corner "; }
const char* Annotation::showtextstr()	    { return "Show Text"; }
const char* Annotation::showscalestr()	    { return "Show Scale"; }

static const int dim0psindexes[] = { 2, 5, 3, 4 };
static const int dim0coordindexes[] = { 0, 4, 7, 3 };

static const int dim1psindexes[] = { 0, 5, 1, 4 };
static const int dim1coordindexes[] = { 0, 4, 5, 1 };

static const int dim2psindexes[] = { 0, 3, 1, 2 };
static const int dim2coordindexes[] = { 0, 3, 2, 1 };

static const int* psindexarr[] = {dim0psindexes,dim1psindexes,dim2psindexes};
static const int* coordindexarr[] =
    { dim0coordindexes, dim1coordindexes, dim2coordindexes };



static TrcKeyZSampling getDefaultScale( const TrcKeyZSampling& cs )
{
    TrcKeyZSampling scale = cs;

    const AxisLayout<int> inlal( (Interval<int>)cs.hsamp_.inlRange() );
    scale.hsamp_.start_.inl() = inlal.sd_.start;
    scale.hsamp_.step_.inl() = inlal.sd_.step;

    const AxisLayout<int> crlal( (Interval<int>)cs.hsamp_.crlRange() );
    scale.hsamp_.start_.crl() = crlal.sd_.start;
    scale.hsamp_.step_.crl() = crlal.sd_.step;

    const AxisLayout<float> zal( (Interval<float>)cs.zsamp_ );
    scale.zsamp_.start = zal.sd_.start; scale.zsamp_.step = zal.sd_.step;

    return scale;
}


Annotation::Annotation()
    : VisualObjectImpl(false )
    , geode_(new osg::Geode)
    , axisnames_(Text2::create())
    , axisannot_(Text2::create())
    , gridlines_(new osgGeo::OneSideRender)
    , displaytrans_(0)
    , scale_(false)
    , tkzs_(true)
    , scene_( 0 )
    , allowshading_( true )
{
    tkzsdefaultscale_ = getDefaultScale( tkzs_ );

    getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode_->ref();
    addChild( geode_ );

    scalefactor_[0] = scalefactor_[1] = scalefactor_[2] = 1;

    annotcolor_ = OD::Color::White();

    setPickable( false, false );

    osg::Vec3f ptr[8];

    ptr[0] = osg::Vec3f( 0, 0, 0 );
    ptr[1] = osg::Vec3f( 0, 0, 1 );
    ptr[2] = osg::Vec3f( 0, 1, 1 );
    ptr[3] = osg::Vec3f( 0, 1, 0 );
    ptr[4] = osg::Vec3f( 1, 0, 0 );
    ptr[5] = osg::Vec3f( 1, 0, 1 );
    ptr[6] = osg::Vec3f( 1, 1, 1 );
    ptr[7] = osg::Vec3f( 1, 1, 0 );

    osg::ref_ptr<osg::Vec3Array> coords = new osg::Vec3Array( 8, ptr );
    box_ = new osg::Geometry;
    box_->ref();
    box_->setName( "Box" );

    box_->setVertexArray( coords );

    GLubyte indices[] = { 0, 1, 1, 2, 2, 3, 3, 0,
			   4, 5, 5, 6, 6, 7, 7, 4,
			   0, 4, 1, 5, 2, 6, 3, 7 };

    box_->addPrimitiveSet(
	    new osg::DrawElementsUByte( GL_LINES, 24, indices  ) );

    geode_->addDrawable( box_ );

    addChild( axisannot_->osgNode() );
    addChild( axisnames_->osgNode() );

#define mAddText \
    { \
	const int txtidx = axisnames_->addText(); \
	text = axisnames_->text( txtidx ); \
	text->setJustification( Text::Right ); \
    }

    Text* text = 0; mAddText mAddText mAddText

    gridlines_->ref();
    gridlinecoords_ = new osg::Vec3Array;
    gridlinecoords_->ref();
    updateTextPos();

    mAttachCB( getMaterial()->change, Annotation::updateTextColor );
    getMaterial()->setColor( annotcolor_ );
}


Annotation::~Annotation()
{
    detachAllNotifiers();
    box_->unref();
    gridlinecoords_->unref();
    geode_->unref();
    gridlines_->unref();

    if ( displaytrans_ ) displaytrans_->unRef();
}


void Annotation::setScene( visBase::Scene* scene )
{
    if ( !scene )
	return;

    scene_ = scene;

    mAttachCB( scene_->contextIsUp, Annotation::firstTraversal );
}


void Annotation::firstTraversal(CallBacker*)
{
    if ( allowshading_ && osgGeo::RayTracedTechnique::isShadingSupported() )
    {
	osg::ref_ptr<osg::Program> program = new osg::Program;

	BufferString code =
	    "void main(void)\n"
	    "{\n"
	    "	 gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
	    "	 gl_FrontColor = gl_FrontMaterial.diffuse;"
	    "}\n";

	osg::ref_ptr<osg::Shader> vertexShader =
		new osg::Shader( osg::Shader::VERTEX, code.str() );
	program->addShader( vertexShader.get() );

	const float factor = 1.1;
	code = "void main(void)\n"
	       "{\n"
	       "    gl_FragDepth = gl_FragCoord.z";

	code.add(" * ").add(toString(factor,1) );
	code.add(";\n");
	code.add(""
		 "    if ( gl_FragDepth>0.999999 ) gl_FragDepth = 0.999999; \n"
		 "    gl_FragColor.a = gl_FrontMaterial.diffuse.a;\n"
		 "    gl_FragColor.rgb = gl_Color.rgb;\n"
		 "}\n");

	osg::ref_ptr<osg::Shader> fragmentShader =
		new osg::Shader( osg::Shader::FRAGMENT, code.str() );
	program->addShader( fragmentShader.get() );

	gridlines_->getOrCreateStateSet()->setAttributeAndModes(program.get());
    }

    scene_->contextIsUp.remove( mCB(this,Annotation,firstTraversal) );
}


void Annotation::setDisplayTransformation(const visBase::Transformation* tr)
{
    if ( displaytrans_ ) displaytrans_->unRef();
    displaytrans_ = tr;
    if ( displaytrans_ ) displaytrans_->ref();

    setTrcKeyZSampling( tkzs_ );
}


#define mImplSwitches( str, node ) \
void Annotation::show##str( bool yn ) \
{ \
    if ( yn==is##str##Shown() ) \
	return; \
    if ( yn ) \
    { \
	addChild( node ); \
    } \
    else \
    { \
	removeChild( node ); \
    } \
} \
 \
 \
bool Annotation::is##str##Shown() const \
{ \
    return childIndex( node )!=-1; \
}


mImplSwitches( Text, axisnames_->osgNode() )
mImplSwitches( Scale, axisannot_->osgNode() )
mImplSwitches( GridLines, gridlines_ )


const FontData& Annotation::getFont() const
{
    return axisnames_->text()->getFontData();
}

void Annotation::setFont( const FontData& fd )
{
    axisnames_->setFontData( fd );
    axisannot_->setFontData( fd );
}


void Annotation::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    tkzs_ = cs;
    tkzsdefaultscale_ = getDefaultScale( tkzs_ );

    const Interval<int> inlrg = cs.hsamp_.inlRange();
    const Interval<int> crlrg = cs.hsamp_.crlRange();
    const Interval<float>& zrg = cs.zsamp_;

    setCorner( 0, inlrg.start, crlrg.start, zrg.start );
    setCorner( 1, inlrg.stop, crlrg.start, zrg.start );
    setCorner( 2, inlrg.stop, crlrg.stop, zrg.start );
    setCorner( 3, inlrg.start, crlrg.stop, zrg.start );
    setCorner( 4, inlrg.start, crlrg.start, zrg.stop );
    setCorner( 5, inlrg.stop, crlrg.start, zrg.stop );
    setCorner( 6, inlrg.stop, crlrg.stop, zrg.stop );
    setCorner( 7, inlrg.start, crlrg.stop, zrg.stop );

    box_->dirtyBound();
    box_->dirtyDisplayList();
    geode_->dirtyBound();

    updateTextPos();
    updateGridLines();
}


const TrcKeyZSampling& Annotation::getTrcKeyZSampling() const
{ return tkzs_; }


void Annotation::setScale( const TrcKeyZSampling& cs )
{
    scale_ = cs;
    updateTextPos();
    updateGridLines();
}


const TrcKeyZSampling& Annotation::getScale() const
{ return scale_.isEmpty() ? tkzsdefaultscale_ : scale_; }


void Annotation::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    axisnames_->setPixelDensity( dpi );
    axisannot_->setPixelDensity( dpi );
}


void Annotation::setCorner( int idx, float x, float y, float z )
{
    auto* cornercoords = sCast(const osg::Vec3f*,
			       box_->getVertexArray()->getDataPointer());
    osg::Vec3f& coord = cCast(osg::Vec3&,cornercoords[idx]);
    mVisTrans::transform( displaytrans_, osg::Vec3(x,y,z), coord );
}


void Annotation::setText( int dim, const uiString& string )
{
    axisnames_->text(dim)->setText( string );
}


static SamplingData<float> getAxisSD( const TrcKeyZSampling& cs, int dim,
				      int* stopidx=0 )
{
    SamplingData<float> sd;
    float stop;

    if ( dim==0 )
    {
	sd.set( AxisLayout<int>(cs.hsamp_.inlRange()).sd_ );
	stop = cs.hsamp_.inlRange().stop;
    }
    else if ( dim==1 )
    {
	sd.set( AxisLayout<int>(cs.hsamp_.crlRange()).sd_ );
	stop = cs.hsamp_.crlRange().stop;
    }
    else
    {
	sd.set( AxisLayout<float>(cs.zsamp_).sd_ );
	stop = cs.zsamp_.stop;
    }

    const float eps = 1e-6;
    if ( stopidx )
	*stopidx = (int) sd.getfIndex( stop + eps*sd.step );

    return sd;
}


void Annotation::updateGridLines()
{
    osg::Vec3Array* coords = mGetOsgVec3Arr( gridlinecoords_ );

    for ( int idx=gridlines_->getNumDrawables()-1; idx>=0; idx-- )
	gridlines_->removeDrawable( gridlines_->getDrawable( idx ) );

    for ( int idx=gridlines_->getNumDrawables(); idx<6; idx++ )
    {
	osg::Geometry* geometry = new osg::Geometry;
	gridlines_->addDrawable( geometry );
	geometry->setVertexArray( mGetOsgVec3Arr(gridlinecoords_) );
	geometry->addPrimitiveSet(
		    new osg::DrawElementsUByte( osg::PrimitiveSet::LINES) );
    }

#define mGetDrawElements( i ) \
    ((osg::DrawElementsUByte*) \
     ((osg::Geometry*)gridlines_->getDrawable(i))->getPrimitiveSet(0))

    for ( int idx=0; idx<6; idx++ )
    {
	mGetDrawElements(idx)->resize( 0 );
    }

    coords->resize( 0 );

    auto* cornercoords = sCast(const osg::Vec3f*,
			       box_->getVertexArray()->getDataPointer());
    for ( int dim=0; dim<3; dim++ )
    {
	osg::Vec3 p0;
	osg::Vec3 p1;
	getAxisCoords( dim, p0, p1 );

	osg::Vec3 dir(0,0,0);
	dir[dim] = 1;
	mVisTrans::transformDir( displaytrans_, dir );
	dir.normalize();

	osg::Vec3 lstart0, lstart1;

	mVisTrans::transform( displaytrans_, p0, lstart0 );
	mVisTrans::transform( displaytrans_, p1, lstart1 );
	gridlines_->setLine( dim*2, osgGeo::Line3(lstart0, dir) );
	gridlines_->setLine( dim*2+1, osgGeo::Line3(lstart1, -dir) );

	Interval<float> range( p0[dim], p1[dim] );

	int stopidx;
	const SamplingData<float> sd = getAxisSD( getScale(), dim, &stopidx );

	const int* psindexes = psindexarr[dim];
	const int* cornerarr = coordindexarr[dim];


	osg::Vec3f corners[] =
		{ cornercoords[cornerarr[0]],
		    cornercoords[cornerarr[1]],
		    cornercoords[cornerarr[2]],
		    cornercoords[cornerarr[3]] };

	if ( displaytrans_ )
	{
	    displaytrans_->transformBack( corners[0] );
	    displaytrans_->transformBack( corners[1] );
	    displaytrans_->transformBack( corners[2] );
	    displaytrans_->transformBack( corners[3] );
	}

	for ( int idx=0; idx<=stopidx; idx++ )
	{
	    const float val = sd.atIndex(idx);
	    if ( val <= range.start )		continue;
	    else if ( val > range.stop )	break;

	    corners[0][dim] = corners[1][dim] = corners[2][dim] =
			      corners[3][dim] = val;

	    const unsigned short firstcoordindex = coords->size();
	    for ( int idy=0; idy<4; idy++ )
	    {
		osg::Vec3 pos;
		mVisTrans::transform( displaytrans_, corners[idy], pos );
		coords->push_back( pos );

		mGetDrawElements(psindexes[idy])->push_back(
			firstcoordindex+idy);
		mGetDrawElements(psindexes[idy])->push_back(
			idy!=3 ? firstcoordindex+idy+1 : firstcoordindex );
	    }
	}
    }

    for ( int idx=gridlines_->getNumDrawables()-1; idx>=0; idx-- )
	gridlines_->getDrawable(idx)->dirtyDisplayList();

    gridlines_->dirtyBound();
}


void Annotation::getAxisCoords( int dim, osg::Vec3& p0, osg::Vec3& p1 ) const
{
    int pidx0;
    int pidx1;

    if ( dim==0)
    {
	pidx0 = 0;
	pidx1 = 1;
    }
    else if ( dim==1 )
    {
	pidx0 = 0;
	pidx1 = 3;
    }
    else
    {
	pidx0 = 0;
	pidx1 = 4;
    }

    auto* cornercoords = sCast(const osg::Vec3f*,
			       box_->getVertexArray()->getDataPointer());
    mVisTrans::transformBack( displaytrans_, cornercoords[pidx0], p0 );
    mVisTrans::transformBack( displaytrans_, cornercoords[pidx1], p1 );
}


void Annotation::updateTextPos()
{
    // Color and Font are not set to extra added text. Hence this hack.
    // Both are set again at the end of this function.
    const OD::Color col = getMaterial()->getColor();
    const FontData& fd = getFont();
    int curscale = 0;
    for ( int dim=0; dim<3; dim++ )
    {
	osg::Vec3 p0;
	osg::Vec3 p1;
	getAxisCoords( dim, p0, p1 );

	osg::Vec3 tp;
	tp[0] = (p0[0]+p1[0]) / 2;
	tp[1] = (p0[1]+p1[1]) / 2;
	tp[2] = (p0[2]+p1[2]) / 2;

	mVisTrans::transform( displaytrans_, tp );
	axisnames_->text(dim)->setPosition( tp );

	Interval<float> range( p0[dim], p1[dim] );

	int stopidx;
	const SamplingData<float> sd = getAxisSD( getScale(), dim, &stopidx );

	for ( int idx=0; idx<=stopidx; idx++ )
	{
	    float val = sd.atIndex(idx);
	    if ( val <= range.start )		continue;
	    else if ( val > range.stop )	break;

	    if ( curscale>=axisannot_->nrTexts() )
	    {
		axisannot_->addText();
	    }

	    Text* text = axisannot_->text(curscale++);

	    osg::Vec3 pos( p0 );
	    pos[dim] = val;
	    float displayval = val;
	    displayval *= scalefactor_[dim];

	    mVisTrans::transform( displaytrans_, pos );
	    text->setPosition( pos );
	    text->setText( toUiString(displayval) );
	    text->setColor( col );
	}
    }

    for ( int idx=axisannot_->nrTexts()-1; idx>=curscale; idx-- )
    {
	axisannot_->removeText( axisannot_->text(idx) );
    }

    setFont( fd );
    getMaterial()->setColor( col );
}


void Annotation::setScaleFactor( int dim, int nv )
{
    scalefactor_[dim] = nv;
    updateTextPos();
}


void Annotation::updateTextColor( CallBacker* )
{
    for ( int idx=0; idx<axisannot_->nrTexts(); idx++ )
	axisannot_->text(idx)->setColor( getMaterial()->getColor() );

    for ( int idx=0; idx<axisnames_->nrTexts(); idx++ )
	axisnames_->text(idx)->setColor( getMaterial()->getColor() );
}


void Annotation::fillPar( IOPar& par ) const
{
    BufferString key;

    auto* cornercoords = sCast(const osg::Vec3f*,
			       box_->getVertexArray()->getDataPointer());
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	const osg::Vec3f& pos = cornercoords[idx];
	par.set( key, pos[0], pos[1], pos[2] );
    }

    par.setYN( showtextstr(), isTextShown() );
    par.setYN( showscalestr(), isScaleShown() );
}


bool Annotation::usePar( const IOPar& par )
{
    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;

	double x, y, z;
	if ( !par.get( key, x, y, z ) )
	    return false;

	setCorner( idx, x, y, z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;

	const char* text = par.find( key );
	if ( !text ) return -1;

	setText( idx, toUiString(text) );
    }

    bool yn = true;
    par.getYN( showtextstr(), yn );
    showText( yn );

    yn = true;
    par.getYN( showscalestr(), yn );
    showScale( yn );

    return true;
}

} // namespace visBase
