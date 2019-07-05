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

const char* Annotation::textprefixstr()		{ return "Text "; }
const char* Annotation::cornerprefixstr()	{ return "Corner "; }
const char* Annotation::showtextstr()		{ return "Show Text"; }
const char* Annotation::showscalestr()		{ return "Show Scale"; }

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


static int nrDims()		{ return 3; }
static int nrVertPerDim()	{ return 4; }

Annotation::Annotation()
    : VisualObjectImpl(false )
    , geode_(new osg::Geode)
    , gridlines_(new osgGeo::OneSideRender)
    , displaytrans_(nullptr)
    , scale_(false)
    , tkzs_(true)
    , scene_(nullptr)
    , annotcolor_(Color::White())
{
    tkzsdefaultscale_ = getDefaultScale( tkzs_ );

    getStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    geode_->ref();
    addChild( geode_ );

    scalefactor_[0] = scalefactor_[1] = scalefactor_[2] = 1;

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

    box_->addPrimitiveSet( new osg::DrawElementsUByte(GL_LINES,24,indices) );
    geode_->addDrawable( box_ );

    for ( int ivert=0; ivert<nrVertPerDim(); ivert++ )
    {
	for ( int dim=0; dim<nrDims(); dim++ )
	{
	    const Color& col = dim%3==0 ? Color::Red()
					: ( dim%3==1 ? Color::Green()
						     : Color::Blue() );

	    Text* axisnm = Text::create();
	    axisnames_.add( axisnm );
	    const int nidx = axisnm->addText();
	    axisnm->text(nidx)->setJustification(
					TextDrawable::BottomRight );
	    axisnm->text(nidx)->setColor( col );
	    addChild( axisnm->osgNode() );

	    Text* axisvals = Text::create();
	    axisvalues_.add( axisvals );
	    const int vidx = axisvals->addText();
	    axisvals->text(vidx)->setJustification(
					TextDrawable::BottomRight );
	    axisvals->text(vidx)->setColor( col );
	    addChild( axisvals->osgNode() );
	}
    }

    gridlines_->ref();
    gridlinecoords_ = new osg::Vec3Array;
    gridlinecoords_->ref();

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

    if ( displaytrans_ )
	displaytrans_->unRef();
}


void Annotation::setScene( visBase::Scene* scn )
{
    if ( !scn )
	return;

    if ( scene_ )
	{ mDetachCB( scene_->contextIsUp, Annotation::firstTraversal ); }

    scene_ = scn;
    mAttachCB( scene_->contextIsUp, Annotation::firstTraversal );
}


void Annotation::firstTraversal( CallBacker* )
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

	code =   "void main(void)\n"
		 "{\n"
		 "    gl_FragDepth = gl_FragCoord.z * 1.1;\n";
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


void Annotation::setDisplayTransformation( const visBase::Transformation* vtr )
{
    if ( displaytrans_ ) displaytrans_->unRef();
    displaytrans_ = vtr;
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

void Annotation::showText( bool yn )
{
    axisnmison_ = yn;
    for ( Text *p:axisnames_ )
	p->turnOn( yn );
}


void Annotation::showScale( bool yn )
{
    axisvalsison_ = yn;
    for ( Text *p:axisvalues_ )
	p->turnOn( yn );
}


mImplSwitches( GridLines, gridlines_ )


bool Annotation::isFaceGridShown( int dim, bool first ) const
{
    return gridlines_ ? gridlines_->isDrawn( dim, first ) : false;
}


const FontData& Annotation::getFont() const
{
    return axisnames_[0]->text()->getFontData();
}

void Annotation::setFont( const FontData& fd )
{
    for ( Text *p:axisnames_ )
	p->setFontData( fd );

    for ( Text *p:axisvalues_ )
	p->setFontData( fd );
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
    box_->dirtyGLObjects();
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
{
    return scale_.isEmpty() ? tkzsdefaultscale_ : scale_;
}


void Annotation::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    for( Text *p:axisnames_ )
	p->setPixelDensity( dpi );

    for( Text *p:axisvalues_ )
	p->setPixelDensity( dpi );
}


void Annotation::setCorner( int idx, float x, float y, float z )
{
    osg::Vec3& coord =
	 ((osg::Vec3*) box_->getVertexArray()->getDataPointer())[idx];

    mVisTrans::transform( displaytrans_, osg::Vec3(x,y,z), coord );
}


void Annotation::setText( int dim, const uiString& string )
{
    for ( int ivert=0; ivert<nrVertPerDim(); ivert++ )
    {
	const int textidx = ivert*nrDims() + dim;
	axisnames_[textidx]->text(0)->setText( string );
    }
}


static SamplingData<float> getAxisSD( const TrcKeyZSampling& cs, int dim,
				      int* stopidx=nullptr )
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

    const float eps = 1e-6f;
    if ( stopidx )
	*stopidx = (int) sd.getfIndex( stop + eps*sd.step );

    return sd;
}


static bool isValidVal( const TrcKeyZSampling& tkzs, int dim, float val )
{
    if ( dim==0 )
	return tkzs.hsamp_.inlRange().includes( val, false );

    if ( dim==1 )
	return tkzs.hsamp_.crlRange().includes( val, false );

    return tkzs.zsamp_.includes( val, false );
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

    const osg::Vec3f* cornercoords = (const osg::Vec3f*)
	box_->getVertexArray()->getDataPointer();

    for ( int dim=0; dim<nrDims(); dim++ )
    {
	osg::Vec3 p0, p1;
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

	const Interval<float> range( p0[dim], p1[dim] );

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
	gridlines_->getDrawable(idx)->dirtyGLObjects();

    gridlines_->dirtyBound();
}


void Annotation::getAxisCoords( int dim, osg::Vec3& p0, osg::Vec3& p1 ) const
{
    const int pidx0 = 0;
    const int pidx1 = dim == 0 ? 1 : ( dim == 1 ? 3 : 4 );

    const osg::Vec3f* cornercoords =
	(const osg::Vec3f*)box_->getVertexArray()->getDataPointer();

    mVisTrans::transformBack( displaytrans_, cornercoords[pidx0], p0 );
    mVisTrans::transformBack( displaytrans_, cornercoords[pidx1], p1 );
}


void Annotation::updateTextPos()
{
    // Color and Font are not set to extra added text. Hence this hack.
    // Both are set again at the end of this function.
    const Color col = getMaterial()->getColor();
    const FontData& fd = getFont();

    const osg::Vec3 inlrg( tkzs_.hsamp_.start_.inl(),
			   tkzs_.hsamp_.lineRange().center(),
			   tkzs_.hsamp_.stop_.inl() );
    const osg::Vec3 crlrg( tkzs_.hsamp_.start_.crl(),
			   tkzs_.hsamp_.trcRange().center(),
			   tkzs_.hsamp_.stop_.crl() );
    const osg::Vec3 zrg( tkzs_.zsamp_.start, tkzs_.zsamp_.center(),
			 tkzs_.zsamp_.stop );
    TypeSet<osg::Vec3> vertcenterpos;
    vertcenterpos += osg::Vec3( inlrg[1], crlrg[0], zrg[0] );
    vertcenterpos += osg::Vec3( inlrg[0], crlrg[1], zrg[0] );
    vertcenterpos += osg::Vec3( inlrg[0], crlrg[0], zrg[1] );
    vertcenterpos += osg::Vec3( inlrg[1], crlrg[2], zrg[0] );
    vertcenterpos += osg::Vec3( inlrg[2], crlrg[1], zrg[0] );
    vertcenterpos += osg::Vec3( inlrg[2], crlrg[0], zrg[1] );
    vertcenterpos += osg::Vec3( inlrg[1], crlrg[0], zrg[2] );
    vertcenterpos += osg::Vec3( inlrg[0], crlrg[1], zrg[2] );
    vertcenterpos += osg::Vec3( inlrg[0], crlrg[2], zrg[1] );
    vertcenterpos += osg::Vec3( inlrg[1], crlrg[2], zrg[2] );
    vertcenterpos += osg::Vec3( inlrg[2], crlrg[1], zrg[2] );
    vertcenterpos += osg::Vec3( inlrg[2], crlrg[2], zrg[1] );
    for ( int idx=0; idx<vertcenterpos.size(); idx++ )
    {
	osg::Vec3 newpos;
	mVisTrans::transform( displaytrans_, vertcenterpos[idx], newpos );
	axisnames_[idx]->text(0)->setPosition( newpos );
    }

    int axisidx = 0;
    for ( int vert=0; vert<nrVertPerDim(); vert++ )
    {
	for ( int dim=0; dim<nrDims(); dim++ )
	{
	    int stopidx;
	    const SamplingData<float> sd =
				getAxisSD( getScale(), dim, &stopidx );

	    Text* axisvals = axisvalues_[axisidx];
	    axisvals->removeAll();

	    for ( int idx=0; idx<=stopidx; idx++ )
	    {
		const float val = sd.atIndex(idx);
		if ( !isValidVal(tkzs_,dim,val) )
		    continue;

		const int txtidx = axisvals->addText();
		TextDrawable* text = axisvals->text( txtidx );

		osg::Vec3 pos = vertcenterpos[axisidx];
		pos[dim] = val;
		float displayval = val;
		displayval *= scalefactor_[dim];

		mVisTrans::transform( displaytrans_, pos );
		text->setPosition( pos );
		text->setText( toUiString(displayval) );
		text->setColor( col );
	    }

	    axisidx++;
	}
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
    const Color col = getMaterial()->getColor();
    for ( int idx=0; idx<axisvalues_.size(); idx++ )
	for ( int idt=0; idt<axisvalues_[idx]->nrTexts(); idt++ )
	    axisvalues_[idx]->text(idt)->setColor( col );

    for ( int idx=0; idx<axisnames_.size(); idx++ )
	axisnames_[idx]->text( 0 )->setColor( col );
}


void Annotation::fillPar( IOPar& par ) const
{
    BufferString key;

    const osg::Vec3f* cornercoords =
	(const osg::Vec3f*) box_->getVertexArray()->getDataPointer();

    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	const osg::Vec3 pos = cornercoords[idx];
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
    //showText( yn );

    yn = true;
    par.getYN( showscalestr(), yn );
    showScale( yn );

    return true;
}

} // namespace visBase
