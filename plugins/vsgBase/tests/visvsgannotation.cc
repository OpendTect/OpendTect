/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgannotation.h"
#include "visvsgdata.h"
#include "visvsgdatagroup.h"
#include "visvsgdataman.h"
#include "visvsgfontcreator.h"
#include "visvsgtransform.h"
#include "math2.h"
#include "thread.h"
#include "vsggeotext.h"
#include "vsggeoupdater.h"
#include "vsgtesthelper.h"


namespace
{

struct LitDiff
{
    int		nrpix_ = 0;
    int		minx_ = INT_MAX;
    int		maxx_ = -1;
    int		miny_ = INT_MAX;
    int		maxy_ = -1;
};


bool isLit( const vsg::ubvec4& pix )
{
    return pix.r>128 || pix.g>128 || pix.b>128;
}


LitDiff newlyLit( const vsgTest::OffscreenViewer& ov,
		  const TypeSet<vsg::ubvec4>& before,
		  const TypeSet<vsg::ubvec4>& after,
		  int x0=0, int x1=INT_MAX, int y0=0, int y1=INT_MAX )
{
    LitDiff ret;
    for ( int y=y0; y<=mMIN(y1,int(ov.height())-1); y++ )
    {
	for ( int x=x0; x<=mMIN(x1,int(ov.width())-1); x++ )
	{
	    const size_t idx = size_t(y)*ov.width() + size_t(x);
	    if ( !isLit(after[idx]) || isLit(before[idx]) )
		continue;

	    ret.nrpix_++;
	    ret.minx_ = mMIN( ret.minx_, x );
	    ret.maxx_ = mMAX( ret.maxx_, x );
	    ret.miny_ = mMIN( ret.miny_, y );
	    ret.maxy_ = mMAX( ret.maxy_, y );
	}
    }

    return ret;
}


class AnnotationSetup
{
public:
    AnnotationSetup( vsgTest::OffscreenViewer& ov, bool viewerfirst=true,
		     bool clearfontsatend=true )
	: ov_(ov)
	, clearfontsatend_(clearfontsatend)
    {
	if ( viewerfirst )
	    connect();

	dGBVis::FontCreator::instance().clear();
	vsgGeo::Text::setViewportHeight( float(ov.height()) );
	root_ = dGBVis::DataObjectGroup::create();
	ov.scene().addChild( vsg::ref_ptr<vsg::Node>(root_->vsgNode()) );

	trans_ = dGBVis::Transformation::create();
	trans_->setA( 0.016, 0, 0, -150*0.016,
		      0, 0.016, 0, -350*0.016,
		      0, 0, -1.6, 0.8,
		      0, 0, 0, 1 );

	annot_ = dGBVis::Annotation::create();
	annot_->setDisplayTransformation( trans_.ptr() );
	setSampling( 200, 400 );
	annot_->setFont( font() );
	if ( viewerfirst )
	{
	    annot_->showText( false );
	    annot_->showScale( false );
	}

	root_->addObject( annot_.ptr() );
    }

    void connect()
    { dGBVis::DataObject::setCommonViewer( &ov_.viewer() ); }

    bool waitFontWithoutViewer()
    {
	dGBVis::FontCreator& creator =
		dGBVis::FontCreator::instance();
	for ( int idx=0; idx<1500; idx++ )
	{
	if ( creator.get(font()) )
		return true;

	Threads::sleep( 0.02 );
	}

	return false;
    }

    ~AnnotationSetup()
    {
	annot_ = nullptr;
	root_ = nullptr;
	ov_.scene().children.clear();
	ov_.viewer().deviceWaitIdle();
	dGBVis::DataObject::setCommonViewer( nullptr );
	if ( clearfontsatend_ )
	    dGBVis::FontCreator::instance().clear();
    }

    static FontData	font()
			{ return FontData( 12, "Arial", FontData::Normal,
					   false ); }

    void		setSampling( int inlstop, int crlstop )
			{
			    TrcKeyZSampling tkzs( false );
			    tkzs.hsamp_.start_ = BinID( 100, 300 );
			    tkzs.hsamp_.stop_ = BinID( inlstop, crlstop );
			    tkzs.hsamp_.step_ = BinID( 1, 1 );
			    tkzs.zsamp_.set( 0.f, 1.f, 0.004f );
			    annot_->setTrcKeyZSampling( tkzs );
			}

    bool		waitFont()
			{
			    dGBVis::FontCreator& creator =
					dGBVis::FontCreator::instance();
			    creator.request( font() );
			    for ( int idx=0; idx<1500; idx++ )
			    {
				ov_.frame( 1 );
				if ( creator.get(font()) )
				    break;

				Threads::sleep( 0.02 );
			    }

			    ov_.frame( 6 );
			    return creator.get( font() ).valid();
			}

    TypeSet<vsg::ubvec4> render( int nrframes=4 )
			{
			    TypeSet<vsg::ubvec4> pixels;
			    ov_.frame( nrframes );
			    ov_.readColor( pixels );
			    return pixels;
			}

    vsgTest::OffscreenViewer&		ov_;
    const bool				clearfontsatend_;
    RefMan<dGBVis::DataObjectGroup>	root_;
    RefMan<dGBVis::Transformation>	trans_;
    RefMan<dGBVis::Annotation>		annot_;
};

} // namespace


static bool testAnnotationText()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Annotation text: Off-screen viewer",
			       ov.errMsg().c_str() );

    AnnotationSetup setup( ov );
    dGBVis::Annotation& annot = *setup.annot_;
    annot.setColor( OD::Color::White() );
    annot.setText( 0, toUiString("Inline") );
    annot.setText( 1, toUiString("Crossline") );
    annot.setText( 2, toUiString("Z") );
    mRunStandardTest( ov.compile(), "Annotation text: Viewer compiled" );
    mRunStandardTest( setup.waitFont(), "Annotation text: Font is loaded" );

    const TypeSet<vsg::ubvec4> boxonly = setup.render();
    const int nrmsgsbefore = ov.nrValidationMessages();
    mRunStandardTest( !annot.getTicks().empty(),
		      "Annotation text: Ticks are computed" );

    annot.showScale( true );
    const TypeSet<vsg::ubvec4> withscale = setup.render();
    const LitDiff scale = newlyLit( ov, boxonly, withscale );
    BufferString msg( "Pixels added by the tick values: ", scale.nrpix_ );
    mRunStandardTestWithError( annot.isScaleShown() && scale.nrpix_>=50,
			       "Annotation text: Tick values are drawn", msg );

    annot.showScale( false );
    const TypeSet<vsg::ubvec4> scaleoff = setup.render();
    mRunStandardTest( newlyLit(ov,boxonly,scaleoff).nrpix_==0,
		      "Annotation text: Tick values go away when "
		      "switched off" );

    annot.showText( true );
    const TypeSet<vsg::ubvec4> withnames = setup.render();
    const LitDiff inlname = newlyLit( ov, boxonly, withnames, 60, 140, 0, 45 );
    msg.set( "Pixels of the inline name: " ).add( inlname.nrpix_ )
       .add( ", right edge x: " ).add( inlname.maxx_ );
    mRunStandardTestWithError( annot.isTextShown() && inlname.nrpix_>=20,
			       "Annotation text: Axis names are drawn", msg );
    mRunStandardTestWithError( inlname.maxx_<=132,
			       "Annotation text: An axis name is "
			       "right-justified at the middle of its axis",
			       msg );
    mRunStandardTest( annot.getText(0).getFullString().isEqual("Inline"),
		      "Annotation text: The axis name is kept" );

    const FontData bigfont( 14, "Arial", FontData::Bold, false );
    annot.setFont( bigfont );
    BufferString gotfont, wantedfont;
    annot.getFont().putTo( gotfont );
    bigfont.putTo( wantedfont );
    mRunStandardTestWithError( gotfont==wantedfont,
			       "Annotation text: The annotation font is kept",
			       BufferString("Got: ",gotfont) );
    annot.setFont( AnnotationSetup::font() );

    annot.showText( false );
    mRunStandardTest( newlyLit(ov,boxonly,setup.render()).nrpix_==0,
		      "Annotation text: Axis names go away when switched off" );

    annot.showText( true );
    annot.showScale( true );
    annot.setColor( OD::Color(255,0,0) );
    ov.frame( 4 );
    const vsgTest::Bounds red = vsgTest::brightBounds( ov );
    msg.set( "Red pixels: " ).add( red.nrred_ ).add( " of " ).add( red.nrpix_ );
    mRunStandardTestWithError( red.nrpix_>0 && red.nrred_ > red.nrpix_*9/10,
			       "Annotation text: The annotation colour is "
			       "used for the text", msg );
    annot.setColor( OD::Color::White() );

    const TypeSet<vsg::ubvec4> before = setup.render();
    setup.setSampling( 160, 400 );
    const TypeSet<vsg::ubvec4> resized = setup.render( 6 );
    int nrdiff = 0;
    for ( int idx=0; idx<before.size(); idx++ )
    {
	if ( isLit(before[idx]) != isLit(resized[idx]) )
	    nrdiff++;
    }

    mRunStandardTestWithError( nrdiff>100,
			       "Annotation text: The labels follow a change "
			       "of the sampling",
			       BufferString("Changed pixels: ",nrdiff) );

    msg.set( "New messages: " ).add( ov.nrValidationMessages()-nrmsgsbefore );
    mRunStandardTestWithError( ov.nrValidationMessages()==nrmsgsbefore,
			       "Annotation text: The labels cause no "
			       "validation messages", msg );
    return true;
}


static bool testCreatedBeforeViewer()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(),
			       "Annotation before viewer: Off-screen viewer",
			       ov.errMsg().c_str() );

    AnnotationSetup setup( ov, false );
    dGBVis::Annotation& annot = *setup.annot_;
    annot.setColor( OD::Color::White() );
    annot.setText( 0, toUiString("Inline") );
    mRunStandardTest( setup.waitFontWithoutViewer(),
		      "Annotation before viewer: The font is made before "
		      "the viewer exists" );

    setup.connect();
    mRunStandardTest( ov.compile(),
		      "Annotation before viewer: Viewer compiled" );
    ov.frame( 6 );
    const TypeSet<vsg::ubvec4> withtext = setup.render( 6 );

    annot.showText( false );
    annot.showScale( false );
    const TypeSet<vsg::ubvec4> boxonly = setup.render( 6 );
    const LitDiff text = newlyLit( ov, boxonly, withtext );
    mRunStandardTestWithError( text.nrpix_>=50,
			       "Annotation before viewer: Labels created "
			       "before the viewer get their font when it "
			       "connects",
			       BufferString("Pixels of text: ",text.nrpix_) );
    return true;
}


static bool testReopenViewer()
{
    {
	vsgTest::OffscreenViewer ov;
	mRunStandardTestWithError( ov.isOK(),
				   "Reopening the viewer: First off-screen "
				   "viewer", ov.errMsg().c_str() );

	AnnotationSetup setup( ov, true, false );
	setup.annot_->showText( true );
	setup.annot_->showScale( true );
	mRunStandardTest( ov.compile(),
			  "Reopening the viewer: Viewer compiled" );
	mRunStandardTest( setup.waitFont(),
			  "Reopening the viewer: Font is loaded" );
	setup.render( 6 );
    }

    vsgTest::OffscreenViewer secondov;
    mRunStandardTestWithError( secondov.isOK(),
			       "Reopening the viewer: A new viewer can be "
			       "created after the first one is closed",
			       secondov.errMsg().c_str() );
    return true;
}


static bool testFontChange()
{
    vsgTest::OffscreenViewer ov;
    mRunStandardTestWithError( ov.isOK(), "Font change: Off-screen viewer",
			       ov.errMsg().c_str() );

    AnnotationSetup setup( ov );
    dGBVis::Annotation& annot = *setup.annot_;
    annot.setColor( OD::Color::White() );
    mRunStandardTest( ov.compile(), "Font change: Viewer compiled" );
    mRunStandardTest( setup.waitFont(), "Font change: Font is loaded" );

    const TypeSet<vsg::ubvec4> boxonly = setup.render();
    annot.showScale( true );
    const LitDiff first = newlyLit( ov, boxonly, setup.render() );
    mRunStandardTest( first.nrpix_>=50, "Font change: Tick values are drawn" );

    const FontData newfont( 20, "Courier", FontData::Bold, false );
    annot.setFont( newfont );
    dGBVis::FontCreator& creator = dGBVis::FontCreator::instance();
    for ( int idx=0; idx<1500 && !creator.get(newfont); idx++ )
    {
	ov.frame( 1 );
	Threads::sleep( 0.02 );
    }

    const LitDiff second = newlyLit( ov, boxonly, setup.render(8) );
    BufferString msg( "Pixels of the tick values with the first font: " );
    msg.add( first.nrpix_ ).add( ", with the new one: " )
       .add( second.nrpix_ );
    mRunStandardTestWithError( second.nrpix_>=50 &&
			       abs(second.nrpix_-first.nrpix_)>=20,
			       "Font change: A new annotation font is used "
			       "by the open viewer", msg );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    dGBVis::DataObject::setVisualizationThread( Threads::currentThread() );
    dGBVis::initStdClasses();

    if ( !testAnnotationText() ||
	 !testCreatedBeforeViewer() ||
	 !testReopenViewer() ||
	 !testFontChange() )
	return 1;

    return 0;
}
