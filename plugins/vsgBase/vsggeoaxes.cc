/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vsggeoaxes.h"

#include "vsggeotriangles.h"

#include <vsg/app/RecordTraversal.h>
#include <vsg/app/ViewMatrix.h>
#include <vsg/app/Window.h>
#include <vsg/maths/transform.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace vsgGeo
{

namespace
{

constexpr float cPi = 3.14159265358979f;
constexpr int cNrSides = 10;
constexpr int cNrSphereSegments = 12;
constexpr int cNrSphereRings = 8;
constexpr int cNrDiscSides = 24;
constexpr float cPopScale = 1.08f;
constexpr float cPopTime = 0.12f;


class AxesUpdate : public vsg::Inherit<Updatable,AxesUpdate>
{
public:
		AxesUpdate( AxesNode& node )
		    : node_(&node)
		{}

    void	applyUpdates( vsg::Viewer& viewer, Compiler& compiler ) override
    {
	vsg::ref_ptr<AxesNode> node = node_.ref_ptr();
	if ( node )
	    node->applyUpdates( viewer, compiler );
    }

private:

    vsg::observer_ptr<AxesNode>	node_;
};


vsg::vec4 moderncolor( int dim )
{
    static const vsg::vec4 colors[3] =
    {
	vsg::vec4( 0.94f, 0.08f, 0.12f, 1.f ),
	vsg::vec4( 0.06f, 0.76f, 0.14f, 1.f ),
	vsg::vec4( 0.06f, 0.38f, 1.00f, 1.f )
    };

    return colors[dim];
}


vsg::vec3 arrowDirection( int dim )
{
    if ( dim == 0 )
	return vsg::vec3( 0.f, 1.f, 0.f );

    if ( dim == 1 )
	return vsg::vec3( 1.f, 0.f, 0.f );

    return vsg::vec3( 0.f, 0.f, -1.f );
}


vsg::vec4 shade( const vsg::vec4& color, const vsg::vec3& normal )
{
    static const vsg::vec3 light =
			vsg::normalize( vsg::vec3(0.4f,-0.5f,0.75f) );
    const float factor = 0.45f + 0.55f*std::max( 0.f, vsg::dot(normal,light) );
    return vsg::vec4( color.r*factor, color.g*factor, color.b*factor, color.a );
}


struct Mesh
{
    void addTriangle( const vsg::vec3& pa, const vsg::vec3& pb,
		      const vsg::vec3& pc, const vsg::vec3& na,
		      const vsg::vec3& nb, const vsg::vec3& nc,
		      const vsg::vec4& color )
    {
	pos_.push_back( pa );
	pos_.push_back( pb );
	pos_.push_back( pc );
	col_.push_back( shaded_ ? shade(color,na) : color );
	col_.push_back( shaded_ ? shade(color,nb) : color );
	col_.push_back( shaded_ ? shade(color,nc) : color );
    }

    void addTube( const vsg::vec3& dir, float radius, float start, float stop,
		  const vsg::vec4& color )
    {
	const vsg::vec3 axis = vsg::normalize( dir );
	const vsg::vec3 anyvec = vsg::normalize( vsg::vec3(1.f,1.f,1.f) );
	const vsg::vec3 curu = vsg::normalize( vsg::cross(axis,anyvec) );
	const vsg::vec3 curv = vsg::normalize( vsg::cross(axis,curu) );
	const vsg::vec3 begin = axis * start;
	const vsg::vec3 end = axis * stop;
	for ( int idx=0; idx<cNrSides; idx++ )
	{
	    const float angle0 = 2.f * cPi * idx / cNrSides;
	    const float angle1 = 2.f * cPi * (idx+1) / cNrSides;
	    const vsg::vec3 vec0 =
			curu*std::cos(angle0) + curv*std::sin(angle0);
	    const vsg::vec3 vec1 =
			curu*std::cos(angle1) + curv*std::sin(angle1);
	    addTriangle( begin+vec0*radius, begin+vec1*radius, end+vec1*radius,
			 vec0, vec1, vec1, color );
	    addTriangle( begin+vec0*radius, end+vec1*radius, end+vec0*radius,
			 vec0, vec1, vec0, color );
	}
    }

    void addDisc( float radius, const vsg::vec4& color )
    {
	const vsg::vec3 normal( 0.f, 0.f, 1.f );
	const vsg::vec3 center( 0.f, 0.f, 0.f );
	for ( int idx=0; idx<cNrDiscSides; idx++ )
	{
	    const float angle0 = 2.f * cPi * idx / cNrDiscSides;
	    const float angle1 = 2.f * cPi * (idx+1) / cNrDiscSides;
	    addTriangle( center,
			 vsg::vec3(radius*std::cos(angle0),
				   radius*std::sin(angle0),0.f),
			 vsg::vec3(radius*std::cos(angle1),
				   radius*std::sin(angle1),0.f),
			 normal, normal, normal, color );
	}
    }

    bool empty() const
    {
	return pos_.empty();
    }

    void addSphere( float radius, const vsg::vec4& color )
    {
	for ( int ring=0; ring<cNrSphereRings; ring++ )
	{
	    const float theta0 = cPi * ring / cNrSphereRings;
	    const float theta1 = cPi * (ring+1) / cNrSphereRings;
	    for ( int seg=0; seg<cNrSphereSegments; seg++ )
	    {
		const float phi0 = 2.f * cPi * seg / cNrSphereSegments;
		const float phi1 = 2.f * cPi * (seg+1) / cNrSphereSegments;
		const vsg::vec3 n00 = onSphere( theta0, phi0 );
		const vsg::vec3 n01 = onSphere( theta0, phi1 );
		const vsg::vec3 n10 = onSphere( theta1, phi0 );
		const vsg::vec3 n11 = onSphere( theta1, phi1 );
		addTriangle( n00*radius, n10*radius, n11*radius, n00, n10, n11,
			     color );
		addTriangle( n00*radius, n11*radius, n01*radius, n00, n11, n01,
			     color );
	    }
	}
    }

    void addArrow( const vsg::vec3& dir, float radius, float length,
		   const vsg::vec4& color )
    {
	const vsg::vec3 axis = vsg::normalize( dir );
	const vsg::vec3 anyvec = vsg::normalize( vsg::vec3(1.f,1.f,1.f) );
	const vsg::vec3 curu = vsg::normalize( vsg::cross(axis,anyvec) );
	const vsg::vec3 curv = vsg::normalize( vsg::cross(axis,curu) );
	const float conelength = 0.33f * length;
	const vsg::vec3 tip = axis * length;
	const vsg::vec3 conebase = axis * ( length - conelength );
	for ( int idx=0; idx<cNrSides; idx++ )
	{
	    const float angle0 = 2.f * cPi * idx / cNrSides;
	    const float angle1 = 2.f * cPi * (idx+1) / cNrSides;
	    const vsg::vec3 vec0 =
			curu*std::cos(angle0) + curv*std::sin(angle0);
	    const vsg::vec3 vec1 =
			curu*std::cos(angle1) + curv*std::sin(angle1);
	    const vsg::vec3 norm0 = vsg::normalize( vec0 + axis );
	    const vsg::vec3 norm1 = vsg::normalize( vec1 + axis );
	    addTriangle( tip, conebase+vec0*radius, conebase+vec1*radius,
			 vsg::normalize(norm0+norm1), norm0, norm1, color );
	    addTriangle( conebase, conebase+vec1*radius, conebase+vec0*radius,
			 -axis, -axis, -axis, color );

	    const vsg::vec3 low0 = vec0 * ( radius / 2.f );
	    const vsg::vec3 low1 = vec1 * ( radius / 2.f );
	    addTriangle( low0, low1, conebase+low1, vec0, vec1, vec1, color );
	    addTriangle( low0, conebase+low1, conebase+low0, vec0, vec1, vec0,
			 color );
	}
    }

    void addCone( const vsg::vec3& dir, float radius, float start, float stop,
		  const vsg::vec4& color )
    {
	const vsg::vec3 axis = vsg::normalize( dir );
	const vsg::vec3 anyvec = vsg::normalize( vsg::vec3(1.f,1.f,1.f) );
	const vsg::vec3 curu = vsg::normalize( vsg::cross(axis,anyvec) );
	const vsg::vec3 curv = vsg::normalize( vsg::cross(axis,curu) );
	const vsg::vec3 base = axis * start;
	const vsg::vec3 tip = axis * stop;
	for ( int idx=0; idx<cNrSides; idx++ )
	{
	    const float angle0 = 2.f * cPi * idx / cNrSides;
	    const float angle1 = 2.f * cPi * (idx+1) / cNrSides;
	    const vsg::vec3 vec0 =
			curu*std::cos(angle0) + curv*std::sin(angle0);
	    const vsg::vec3 vec1 =
			curu*std::cos(angle1) + curv*std::sin(angle1);
	    addTriangle( tip, base+vec0*radius, base+vec1*radius, axis, vec0,
			 vec1, color );
	    addTriangle( base, base+vec1*radius, base+vec0*radius, -axis, -axis,
			 -axis, color );
	}
    }

    vsg::ref_ptr<vsg::Node> createNode(
		const vsg::ref_ptr<const vsg::Options>& options ) const
    {
	vsg::ref_ptr<vsg::vec3Array> vertices =
			vsg::vec3Array::create( pos_.size() );
	std::copy( pos_.begin(), pos_.end(), vertices->begin() );
	vsg::ref_ptr<vsg::vec4Array> colors =
			vsg::vec4Array::create( col_.size() );
	std::copy( col_.begin(), col_.end(), colors->begin() );
	return createTriangleList( vertices, colors, options );
    }

    static Mesh getModernMesh( float length, float radius, float dotradius )
    {
	Mesh mesh;
	mesh.shaded_ = false;
	const float headlength = std::min( 11.f, 0.25f*length );
	const float shaftstop = std::max( dotradius, length-headlength );
	for ( int dim=0; dim<3; dim++ )
	{
	    mesh.addTube( arrowDirection(dim), std::max(0.6f,radius*0.26f),
			  dotradius, shaftstop, moderncolor(dim) );
	    mesh.addCone( arrowDirection(dim), std::max(2.8f,radius*0.8f),
			  shaftstop, length, moderncolor(dim) );
	}

	return mesh;
    }

    static Mesh getDotMesh( float radius )
    {
	Mesh mesh;
	mesh.shaded_ = false;
	mesh.addDisc( radius, vsg::vec4(1.f,0.82f,0.05f,1.f) );
	return mesh;
    }

    static Mesh getClassicMesh( float length, float radius )
    {
	static const vsg::vec4 colors[3] =
	{
	    vsg::vec4( 1.f, 0.f, 0.f, 1.f ),
	    vsg::vec4( 0.f, 1.f, 0.f, 1.f ),
	    vsg::vec4( 0.f, 0.55f, 1.f, 1.f )
	};

	Mesh mesh;
	mesh.addSphere( radius*0.9f, vsg::vec4(1.f,1.f,0.f,1.f) );
	for ( int dim=0; dim<3; dim++ )
	    mesh.addArrow( arrowDirection(dim), radius, length, colors[dim] );

	return mesh;
    }

    static vsg::vec3 onSphere( float theta, float phi )
    {
	return vsg::vec3( std::sin(theta)*std::cos(phi),
			  std::sin(theta)*std::sin(phi), std::cos(theta) );
    }

    bool			shaded_ = true;
    std::vector<vsg::vec3>	pos_;
    std::vector<vsg::vec4>	col_;
};

} // anonymous namespace


AxesNode::AxesNode()
    : transform_(vsg::MatrixTransform::create())
    , geometrygroup_(vsg::Group::create())
{
    transform_->addChild( geometrygroup_ );
    addChild( transform_ );
    const char32_t* names[] = { U"N", U"E", U"Z" };
    for ( int dim=0; dim<3; dim++ )
    {
	texts_[dim] = Text::create();
	texts_[dim]->setAxisAlignment( Text::AxisAlignment::Screen );
	texts_[dim]->setScreenSized( true );
	texts_[dim]->setCharacterHeight( textsize_ );
	texts_[dim]->setText( names[dim] );
	labelxforms_[dim] = vsg::MatrixTransform::create();
	labelxforms_[dim]->addChild( texts_[dim] );
	addChild( labelxforms_[dim] );
    }

    centergroup_ = vsg::Group::create();
    centerxform_ = vsg::MatrixTransform::create();
    centerxform_->addChild( centergroup_ );
    addChild( centerxform_ );

    applyStyleToTexts();
    requestUpdate();
}


AxesNode::~AxesNode()
{}


void AxesNode::setUpdater( Updater* updater )
{
    if ( updater_.ref_ptr().get() == updater )
	return;

    updater_ = updater;
    for ( auto& text : texts_ )
	text->setUpdater( updater );

    requestUpdate();
}


void AxesNode::setPrimaryCamera( vsg::ref_ptr<vsg::Camera> camera )
{
    primarycamera_ = camera;
}


void AxesNode::setSize( vsg::vec2 size )
{
    if ( size.x==radius_ && size.y==length_ )
	return;

    radius_ = size.x;
    length_ = size.y;
    requestUpdate();
}


void AxesNode::setRadius( const float& radius )
{
    setSize( vsg::vec2(radius,length_) );
}


void AxesNode::setLength( const float& length )
{
    setSize( vsg::vec2(radius_,length) );
}


void AxesNode::setPosition( vsg::vec2 pos )
{
    pos_ = pos;
}


void AxesNode::setAnnotationColor( vsg::vec4 col )
{
    annotcolor_ = col;
    applyStyleToTexts();
}


void AxesNode::setAnnotationText( int dim, const std::u32string& text )
{
    if ( dim>=0 && dim<3 )
	texts_[dim]->setText( text );
}


const std::u32string& AxesNode::getAnnotationText( int dim ) const
{
    return texts_[dim]->text();
}


void AxesNode::setAnnotationTextSize( float size )
{
    textsize_ = size;
    for ( auto& text : texts_ )
	text->setCharacterHeight( size );
}


void AxesNode::setAnnotationFont( vsg::ref_ptr<vsg::Font> font )
{
    for ( auto& text : texts_ )
	text->setFont( font );
}


void AxesNode::setStyle( Style style )
{
    if ( style == style_ )
	return;

    style_ = style;
    applyStyleToTexts();
    requestUpdate();
}


float AxesNode::dotRadius() const
{
    return std::max( 2.75f, radius_*0.825f );
}


float AxesNode::labelDistance() const
{
    return 0.55f*textsize_ + 3.f;
}


float AxesNode::getMargin() const
{
    return style_==Style::Modern ? 1.05f*textsize_ + 7.f : 10.f;
}


void AxesNode::applyStyleToTexts()
{
    const bool modern = style_ == Style::Modern;
    for ( int dim=0; dim<3; dim++ )
    {
	texts_[dim]->setAlignment(
		modern ? Text::HAlign::Center : Text::HAlign::Left,
		modern ? Text::VAlign::Center : Text::VAlign::Bottom );
	texts_[dim]->setColor( annotcolor_ );
    }
}


void AxesNode::requestUpdate()
{
    vsg::ref_ptr<Updater> updater = updater_.ref_ptr();
    if ( !updater )
    {
	rebuild( nullptr );
	return;
    }

    if ( !updatehelper_ )
	updatehelper_ = AxesUpdate::create( *this );

    updater->requestUpdate( updatehelper_ );
}


void AxesNode::applyUpdates( vsg::Viewer&, Compiler& compiler )
{
    rebuild( &compiler );
}


void AxesNode::rebuild( Compiler* compiler )
{
    const bool modern = style_ == Style::Modern;
    const Mesh mesh = modern
		    ? Mesh::getModernMesh( length_, radius_, dotRadius() )
		    : Mesh::getClassicMesh( length_, radius_ );

    vsg::ref_ptr<Updater> updater = updater_.ref_ptr();
    vsg::ref_ptr<const vsg::Options> options;
    if ( updater )
	options = updater->sharedOptions();

    std::vector<vsg::ref_ptr<vsg::Node>> oldnodes;
    oldnodes.push_back( geometry_ );
    geometry_ = mesh.createNode( options );
    geometrygroup_->children.clear();
    geometrygroup_->addChild( geometry_ );
    std::vector<vsg::ref_ptr<vsg::Node>> newnodes;
    newnodes.push_back( geometry_ );

    const Mesh dot = modern ? Mesh::getDotMesh( dotRadius() ) : Mesh();

    oldnodes.push_back( centergeometry_ );
    centergeometry_ = dot.empty() ? nullptr : dot.createNode( options );
    centergroup_->children.clear();
    if ( centergeometry_ )
    {
	centergroup_->addChild( centergeometry_ );
	newnodes.push_back( centergeometry_ );
    }

    if ( !compiler )
	return;

    for ( auto& node : newnodes )
	compiler->add( node );

    for ( auto& node : oldnodes )
    {
	if ( node )
	    compiler->release( node );
    }
}


void AxesNode::setPointer( float x, float y )
{
    const float dx = x - pos_.x;
    const float dy = y - pos_.y;
    const float reach = length_ * cPopScale;
    over_ = dx*dx + dy*dy <= reach*reach;
}


void AxesNode::clearPointer()
{
    over_ = false;
}


void AxesNode::advance( std::chrono::steady_clock::time_point now )
{
    if ( !hasadvanced_ )
    {
	lastadvance_ = now;
	hasadvanced_ = true;
    }

    const std::chrono::duration<float> elapsed = now - lastadvance_;
    const float passed = elapsed.count();
    lastadvance_ = now;
    const float target = over_ ? 1.f : 0.f;
    if ( pop_ == target )
	return;

    const float step = passed / cPopTime;
    pop_ = target>pop_ ? std::min( target, pop_+step )
		       : std::max( target, pop_-step );
}


void AxesNode::accept( vsg::RecordTraversal& rt ) const
{
    AxesNode* axes = const_cast<AxesNode*>( this );
    axes->advance( std::chrono::steady_clock::now() );
    axes->updateTransform();
    vsg::Group::accept( rt );
}


void AxesNode::updateTransform()
{
    vsg::dmat4 rotation;
    if ( primarycamera_ && primarycamera_->viewMatrix )
	rotation = primarycamera_->viewMatrix->transform();

    rotation[3] = vsg::dvec4( 0., 0., 0., 1. );
    const double scale = 1. + double(cPopScale-1.f) * double(pop_);
    transform_->matrix = vsg::translate( double(pos_.x), double(pos_.y), 0. ) *
			 rotation * vsg::scale( scale, scale, scale );
    centerxform_->matrix =
		vsg::translate( double(pos_.x), double(pos_.y), 2. ) *
		vsg::scale( scale, scale, 1. );
    const bool modern = style_ == Style::Modern;
    const double reach = double(length_) * scale;
    for ( int dim=0; dim<3; dim++ )
    {
	const vsg::vec3 dir = arrowDirection( dim );
	const vsg::dvec4 tip = rotation * vsg::dvec4( dir.x*reach, dir.y*reach,
						      dir.z*reach, 0. );
	vsg::dvec2 labelpos( pos_.x+tip.x, pos_.y+tip.y );
	if ( modern )
	{
	    vsg::dvec2 screendir( tip.x, tip.y );
	    const double screenlen = vsg::length( screendir );
	    screendir = screenlen>1e-3 ? screendir/screenlen
				       : vsg::dvec2( 0.7071, 0.7071 );
	    labelpos += screendir * double( labelDistance() );
	}

	labelxforms_[dim]->matrix = vsg::translate( labelpos.x, labelpos.y,
						     tip.z );
    }
}


void AxesHandler::setAxes( AxesNode* axes )
{
    axes_ = axes;
}


void AxesHandler::apply( vsg::MoveEvent& ev )
{
    vsg::ref_ptr<AxesNode> axes = axes_.ref_ptr();
    vsg::ref_ptr<vsg::Window> window = ev.window.ref_ptr();
    if ( !axes || !window )
	return;

    const float height = float( window->extent2D().height );
    axes->setPointer( float(ev.x), height-float(ev.y) );
}

} // namespace vsgGeo
