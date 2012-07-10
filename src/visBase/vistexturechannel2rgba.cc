/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jun 2008
___________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: vistexturechannel2rgba.cc,v 1.66 2012-07-10 13:26:36 cvsjaap Exp $";

#include "vistexturechannel2rgba.h"

#include "vistexturechannels.h"
#include "color.h"
#include "coltabsequence.h"
#include "coltab.h"
#include "simpnumer.h"
#include "task.h"
#include "thread.h"

#include "SoTextureChannelSet.h"
#include "SoColTabTextureChannel2RGBA.h"
#include "SoTextureComposer.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoFragmentShader.h"
#include "Inventor/nodes/SoShaderProgram.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoTextureUnit.h"
#include "Inventor/nodes/SoVertexShader.h"
#include "Inventor/nodes/SoShaderParameter.h"
#include "Inventor/nodes/SoTexture2.h"
#include "SoOD.h"

#include <osgGeo/LayeredTexture>

#define mNrColors	255
#define mLayersPerUnit	4

mCreateFactoryEntry( visBase::ColTabTextureChannel2RGBA );

namespace visBase
{

class MappedTextureDataSetImpl : public MappedTextureDataSet
{
public:

static MappedTextureDataSetImpl* create()
mCreateDataObj( MappedTextureDataSetImpl );

int nrChannels() const
{ return tc_->channels.getNum(); }



bool addChannel()
{ return true; }


bool enableNotify( bool yn )
{ return tc_->channels.enableNotify( yn ); }


void setNrChannels( int nr )
{ tc_->channels.setNum( nr ); }


void touch()
{ tc_->touch(); }


void setChannelData( int channel,const SbImagei32& image )
{ tc_->channels.set1Value( channel, image ); }


const SbImagei32* getChannelData() const
{ return tc_->channels.getValues( 0 ); }


SoNode* gtInvntrNode()
{ return tc_; }


protected:

~MappedTextureDataSetImpl()
{ tc_->unref(); }


    SoTextureChannelSet*	tc_;
};


MappedTextureDataSetImpl::MappedTextureDataSetImpl()
    : tc_( new SoTextureChannelSet )
{ tc_->ref(); }


mCreateFactoryEntry( MappedTextureDataSetImpl );

class ColTabSequenceTransparencyCheck : public ParallelTask
{
public:

ColTabSequenceTransparencyCheck( const unsigned char* cols,
				 const unsigned char* vals, od_int64 sz,
				 bool findintermediate )
    : finished_( false )
    , vals_( vals )
    , cols_( cols )
    , totalnr_( sz )
    , findintermediate_( findintermediate )
    , result_( SoTextureComposerInfo::cHasNoTransparency() )
{}



char getTransparency() const	{ return result_; }


od_int64 nrIterations() const	{ return totalnr_; }

private:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    char localresult( SoTextureComposerInfo::cHasNoTransparency() );

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	unsigned char ucval = vals_[idx];
	unsigned char opacity = cols_[ucval*4+3];

	if ( opacity!=255 )
	{
	    if ( opacity==0 )
	    {
		if ( localresult==SoTextureComposerInfo::cHasNoTransparency() )
		{
		    localresult = 
		      SoTextureComposerInfo::cHasNoIntermediateTransparency();
		    if ( !findintermediate_ )
		    {
			// enough if we detect that the sequence is not opaque
			localresult = 
				SoTextureComposerInfo::cHasTransparency();
		    	break;
		    }
		}
	    }
	    else
	    {
		localresult = SoTextureComposerInfo::cHasTransparency();
		break;
	    }
	}

	if ( idx%10000 )
	    continue;

	addToNrDone( 10000 );

	if ( !shouldContinue() )
	    break;

	Threads::MutexLocker lock( resultlock_ );
	if ( finished_ )
	    break;
    }

    Threads::MutexLocker lock( resultlock_ );

    if ( !findintermediate_ )
	finished_ = true;

    // SoTextureComposerInfo values and their relation to transparency:
    // cHasNoTransparency() - 0 (opaque)
    // cHasNoIntermediateTransparency() - 0 or 255
    // cHasTransparency() - 1 to 255

    // CASE	LOCAL RESULT		RESULT			NEW RESULT
    //	a	hasTransparency		hasTransparency		hasTransparency
    //  b	hasTransparency		cHasNoInter.		hasTransparency
    //  c	hasTransparency		Opaque			hasTransparency
    //  d	cHasNoInter.		hasTransparency		hasTransparency
    //  e	cHasNoInter.		cHasNoInter.		cHasNoInter.
    //  f	cHasNoInter.		Opaque			cHasNoInter.
    //  g	Opaque			hasTransparency		hasTransparency
    //  h	Opaque			cHasNoInter.		cHasNoInter.
    //  i	Opaque			Opaque			Opaque

    if ( localresult==SoTextureComposerInfo::cHasTransparency() )
	result_ = localresult;  // cases a, b, c
    else if ( result_==SoTextureComposerInfo::cHasNoTransparency() )
	result_ = localresult;  // cases f, i
	// cases d, e, g, h (& i) - no change to result_

    return true;
}

    od_int64			totalnr_;
    const unsigned char*	vals_;
    const unsigned char*	cols_;

    Threads::Mutex		resultlock_;
    bool			finished_;
    bool			findintermediate_;
    char			result_;
};


TextureChannel2RGBA::TextureChannel2RGBA()
    : channels_( 0 )
    , shadingallowed_( true )
    , enableinterpolation_( true )
{}


MappedTextureDataSet* TextureChannel2RGBA::createMappedDataSet() const
{ return MappedTextureDataSetImpl::create(); }


void TextureChannel2RGBA::setChannels( TextureChannels* ch )
{
    channels_ = ch;
}


void TextureChannel2RGBA::allowShading( bool yn )
{
    shadingallowed_ = yn;
}


bool TextureChannel2RGBA::interpolationEnabled() const
{
    return enableinterpolation_;
}


void TextureChannel2RGBA::enableInterpolation( bool yn )
{
    enableinterpolation_ = yn;
}


bool TextureChannel2RGBA::canUseShading() const
{
    return shadingallowed_ && SoOD::supportsFragShading()==1 &&
	   SoOD::supportsVertexShading()==1;
}


bool ColTabTextureChannel2RGBA::canUseShading() const
{
    return TextureChannel2RGBA::canUseShading() &&
	   SoOD::maxNrTextureUnits()>=3;
}



ColTabTextureChannel2RGBA::ColTabTextureChannel2RGBA()
    : shaderswitch_( new SoSwitch )
    , noneshadinggroup_( new SoGroup )
    , converter_( 0 )
    , shadinggroup_( 0 )
    , shaderctab_( 0 )
    , fragmentshader_( 0 )
    , numlayers_( 0 )
    , startlayer_( 0 )
    , layeropacity_( 0 )
    , shadingcomplexity_( 0 )
    , nonshadingcomplexity_( 0 )
{
    shaderswitch_->ref();
    shaderswitch_->addChild( noneshadinggroup_ );
}


int ColTabTextureChannel2RGBA::maxNrChannels() const
{ return 8; } //TODO


ColTabTextureChannel2RGBA::~ColTabTextureChannel2RGBA()
{
    shaderswitch_->unref();
    deepErase( coltabs_ );
    deepErase( osgcolsequences_ );
    deepErase( osgcolseqarrays_ );
}


void ColTabTextureChannel2RGBA::setChannels( TextureChannels* ch )
{
    TextureChannel2RGBA::setChannels( ch );
    adjustNrChannels();
}


void ColTabTextureChannel2RGBA::updateOsgTexture() const
{
    if ( channels_ && channels_->getOsgTexture() )
    {
	TypeSet<int> ids;
	osgGeo::LayeredTexture& laytex = *channels_->getOsgTexture();
	for ( int procidx=laytex.nrProcesses()-1; procidx>=0; procidx-- )
	{
	    mDynamicCastGet( osgGeo::ColTabLayerProcess*, proc,
			     laytex.getProcess(procidx) );

	    const int id = proc->getDataLayerID();
	    if ( laytex.getDataLayerIndex(id)<0 )
	    {
		const osgGeo::ColorSequence* colseq = proc->getColorSequence();
		const int colseqidx = osgcolsequences_.indexOf( colseq );
		delete osgcolsequences_.remove( colseqidx );
		delete osgcolseqarrays_.remove( colseqidx );
		laytex.removeProcess( proc );
	    }
	    else
		ids.insert( 0, id );
	}

	for ( int channel=0; channel<channels_->nrChannels(); channel++ )
	{
	    const int id = (*channels_->getOsgIDs(channel))[0];
	    int procidx = ids.indexOf(id);
	    if ( procidx != channel )
	    {
		mDynamicCastGet( osgGeo::ColTabLayerProcess*, proc,
				 laytex.getProcess(procidx) );
		if ( !proc )
		{
		    procidx = laytex.nrProcesses();
		    proc = new osgGeo::ColTabLayerProcess( laytex );
		    laytex.addProcess( proc );
		    proc->setDataLayerID( id );

		    TypeSet<unsigned char>* ts = new TypeSet<unsigned char>();
		    getColors( channel, *ts );
		    osgcolseqarrays_ += ts;
		    osgcolsequences_ += new osgGeo::ColorSequence( ts->arr() );

		    proc->setColorSequence( osgcolsequences_[procidx] );

		    const Color& col = getSequence(channel)->undefColor();
		    const osg::Vec4f newudfcol( col.r(), col.g(), col.b(),
			    			255-col.t() );
		    proc->setNewUndefColor( newudfcol/255.0 );
		}
		else
		    ids.remove( procidx );

		for ( int idx=procidx; idx>channel; idx-- )
		{
		    laytex.moveProcessEarlier( proc );
		    osgcolsequences_.swap( idx, idx-1 );
		    osgcolseqarrays_.swap( idx, idx-1 );
		}

		ids.insert( channel, id );
	    }
	}
    }
}


void ColTabTextureChannel2RGBA::adjustNrChannels() const
{
    const int nr = channels_ ? channels_->nrChannels() : 0;

    while ( coltabs_.size()<nr )
    {
	coltabs_ += new ColTab::Sequence( ColTab::defSeqName() );
	enabled_ += true;
	opacity_ += 255;
    }

    while ( coltabs_.size()>nr )
    {
	delete coltabs_.remove( nr );
	enabled_.remove( nr );
	opacity_.remove( nr );
    }

    updateOsgTexture();
}


void ColTabTextureChannel2RGBA::swapChannels( int ch0, int ch1 )
{
    coltabs_.swap( ch0, ch1 );
    enabled_.swap( ch0, ch1 );
    opacity_.swap( ch0, ch1 );

    update();
}


void ColTabTextureChannel2RGBA::notifyChannelInsert( int ch )
{
    if ( ch<0 && ch>coltabs_.size() )
	return;

    coltabs_.insertAt( new ColTab::Sequence(ColTab::defSeqName()), ch );
    enabled_.insert( ch, true );
    opacity_.insert( ch, 255 );

    update();
}


void ColTabTextureChannel2RGBA::notifyChannelRemove( int ch )
{
    if ( ch<0 && ch>=coltabs_.size() )
	return;

    delete coltabs_.remove( ch );
    enabled_.remove( ch );
    opacity_.remove( ch );

    update();
}


void ColTabTextureChannel2RGBA::setSequence( int channel, 
		const ColTab::Sequence& seq )
{
    if ( channel>=coltabs_.size() )
	adjustNrChannels();

    if ( *coltabs_[channel] == seq )
	return;
    
    *coltabs_[channel] = seq;
    update();

    if ( channels_ && channels_->getOsgTexture() )
    {
	getColors( channel, *osgcolseqarrays_[channel] );
	osgcolsequences_[channel]->touch();

	osgGeo::LayeredTexture& laytex = *channels_->getOsgTexture();
	if ( laytex.getProcess(channel) )
	{
	    const Color& col = getSequence(channel)->undefColor();
	    const osg::Vec4f newudfcol( col.r(), col.g(), col.b(),
					255-col.t() );
	    laytex.getProcess(channel)->setNewUndefColor( newudfcol/255.0 );
	}
    }
}


const ColTab::Sequence*
ColTabTextureChannel2RGBA::getSequence( int channel ) const
{
    if ( channel>=coltabs_.size() )
	adjustNrChannels();

    return coltabs_[channel];
}


void ColTabTextureChannel2RGBA::setEnabled( int ch, bool yn )
{
    if ( ch>=coltabs_.size() )
	adjustNrChannels();

    if ( enabled_[ch]==yn )
	return;

    enabled_[ch] = yn;
    update();

    if ( channels_ && channels_->getOsgTexture() )
    {
	const float opac = yn ? opacity_[ch]/255.0 : 0.0;
	channels_->getOsgTexture()->getProcess(ch)->setOpacity( opac );
    }
}


void ColTabTextureChannel2RGBA::enableInterpolation( bool yn )
{
    if ( enableinterpolation_==yn )
	return;

    TextureChannel2RGBA::enableInterpolation( yn );

    if ( shadingcomplexity_ ) 
	shadingcomplexity_->textureQuality.setValue( yn ? 0.9 : 0.1 );

    if ( nonshadingcomplexity_ ) 
    {
	nonshadingcomplexity_->textureQuality.setValue( yn ? 0.9 : 0.1 );

	//Crappy stuff!
	if ( !yn )
	    converter_->touch();
	//end of crap
    }
}

bool ColTabTextureChannel2RGBA::isEnabled( int ch ) const
{
    if ( ch>=enabled_.size() )
	adjustNrChannels();

    return enabled_[ch];
}


void ColTabTextureChannel2RGBA::setTransparency( int ch, unsigned char t )
{
    if ( ch>=opacity_.size() )
	adjustNrChannels();

    if ( opacity_[ch] == 255-t )
	return;

    opacity_[ch] = 255-t;
    update();

    if ( channels_ && channels_->getOsgTexture() )
    {
	const float opac = enabled_[ch] ? opacity_[ch]/255.0 : 0.0;
	channels_->getOsgTexture()->getProcess(ch)->setOpacity( opac );
    }
}


unsigned char ColTabTextureChannel2RGBA::getTransparency( int ch ) const
{
    if ( ch>=opacity_.size() )
	adjustNrChannels();

    return 255-opacity_[ch];
}


void ColTabTextureChannel2RGBA::allowShading( bool yn )
{
    if ( shadingallowed_==yn )
	return;

    TextureChannel2RGBA::allowShading( yn );
    update();

    if ( channels_ && channels_->getOsgTexture() )
	channels_->getOsgTexture()->useShaders( yn );
}


bool ColTabTextureChannel2RGBA::usesShading() const
{
    return shaderswitch_->whichChild.getValue()==1;
}


bool ColTabTextureChannel2RGBA::createRGBA( SbImagei32& res ) const
{
    if ( !channels_ )
	return false;

    channels_->ref();
    
    SoColTabTextureChannel2RGBA* conv = new SoColTabTextureChannel2RGBA;
    conv->ref();
    doFill( conv );

    conv->processChannels( channels_->getChannels(), channels_->nrChannels() );

    od_int64 nrpixels;    
    for ( int rgba=0; rgba<4; rgba++ )
    {
	const SbImagei32& image = conv->getRGBA( rgba );
	if ( !rgba )
	{
	    const SbVec3i32 size = image.getSize();
	    nrpixels = size[0];
	    nrpixels *= size[1];
	    nrpixels *= size[2];

	    res.setValue( image.getSize(), 4, 0 );
	}

	SbVec3i32 dummy;
	int dummy2;

	const unsigned char* src = image.getValue( dummy, dummy2 );
	unsigned char* dst = res.getValue( dummy, dummy2 )+rgba;

	for ( int idx=0; idx<nrpixels; idx++, dst+=4, src++ )
	    *dst = *src;
    }

    conv->unref();
    channels_->unRef();
    return true;
}


void ColTabTextureChannel2RGBA::update()
{
    adjustNrChannels();

    const bool doshading = shadingallowed_ && canUseShading();
    if ( doshading )
	setShadingVars();
    else
    {
    	if ( !converter_ )
    	{
	    nonshadingcomplexity_ = new SoComplexity;
    	    nonshadingcomplexity_->textureQuality.setValue(
    		    enableinterpolation_ ? 0.9 : 0.1 );
	    noneshadinggroup_->addChild( nonshadingcomplexity_ );

    	    converter_ = new SoColTabTextureChannel2RGBA;
	    noneshadinggroup_->addChild( converter_ );
    	}
    
	doFill( converter_ );
    }

    shaderswitch_->whichChild = doshading ? 1 : 0;
}


void ColTabTextureChannel2RGBA::setShadingVars()
{
    const bool douseshading = shadingallowed_ && canUseShading();

    if ( douseshading && !shadinggroup_ )
    {
	shadinggroup_ = new SoGroup;
	shaderswitch_->addChild( shadinggroup_ );

	tci_ = new SoTextureComposerInfo;
	shadinggroup_->addChild( tci_ );
	tci_->units.set1Value( 0, 0 );
	tci_->units.set1Value( 1, 1 );

	const int ctabunitnr = SoOD::maxNrTextureUnits()-1;
	SoTextureUnit* ctabunit = new SoTextureUnit;
	shadinggroup_->addChild( ctabunit );
	ctabunit->unit = ctabunitnr;

	SoComplexity* complexity = new SoComplexity;
	complexity->textureQuality.setValue(0.1);
	shadinggroup_->addChild( complexity );

	shaderctab_ = new SoTexture2;
	shadinggroup_->addChild( shaderctab_ );

	SoShaderProgram* shaderprogram = new SoShaderProgram;

	SoVertexShader* vertexshader = new SoVertexShader;
	vertexshader->sourceType = SoShaderObject::GLSL_PROGRAM;
	vertexshader->sourceProgram.setValue( sVertexShaderProgram() );

	shaderprogram->shaderObject.addNode( vertexshader );

	SoFragmentShader* fragmentshader = new SoFragmentShader;
	fragmentshader->sourceType = SoShaderObject::GLSL_PROGRAM;
	BufferString shadingprog;
	createFragShadingProgram( maxNrChannels(), shadingprog );
	fragmentshader->sourceProgram.setValue( shadingprog.buf() );

	numlayers_ = new SoShaderParameter1i;
	numlayers_->name.setValue("numlayers");
	fragmentshader->parameter.addNode( numlayers_ );

	SoShaderParameter1i* ctabunitvar = new SoShaderParameter1i;
	ctabunitvar->name.setValue("ctabunit");
	ctabunitvar->value.setValue( ctabunitnr );
	fragmentshader->parameter.addNode( ctabunitvar );

	SoShaderParameter1i* dataunit0 = new SoShaderParameter1i;
	dataunit0->name.setValue("dataunit0");
	dataunit0->value.setValue( 0 );
	fragmentshader->parameter.addNode( dataunit0 );

	SoShaderParameter1i* dataunit1 = new SoShaderParameter1i;
	dataunit1->name.setValue("dataunit1");
	dataunit1->value.setValue( 1 );
	fragmentshader->parameter.addNode( dataunit1 );

	startlayer_ = new SoShaderParameter1i;
	startlayer_->name.setValue("startlayer");
	fragmentshader->parameter.addNode( startlayer_ );
	startlayer_->value.setValue( 0 );

	layeropacity_ = new SoShaderParameterArray1f;
	layeropacity_->name.setValue("layeropacities");
	fragmentshader->parameter.addNode( layeropacity_ );

	shaderprogram->shaderObject.addNode( fragmentshader );
	shadinggroup_->addChild( shaderprogram );

	//Reset texture unit.
	ctabunit = new SoTextureUnit;
	shadinggroup_->addChild( ctabunit );
	ctabunit->unit = 0;

	shadingcomplexity_ = new SoComplexity;
	shadingcomplexity_->textureQuality.setValue(
		enableinterpolation_ ? 0.9 : 0.1 );
	shadinggroup_->addChild( shadingcomplexity_ );
    }

    if ( !coltabs_.size() )
	return;

    const int nrchannels = coltabs_.size()>1
	? getPow2Sz( coltabs_.size() ) : 1;
    shaderctab_->image.setValue( SbVec2s( 256, nrchannels ), 4, 0 );
    
    SbVec2s dummy; int dummy2;
    unsigned char* textureptr = shaderctab_->image.startEditing(dummy,dummy2);
    
    TypeSet<unsigned char> cols;
    for ( int channelidx=0; channelidx<nrchannels; channelidx++ )
    {
	getColors( channelidx, cols );
	memcpy( textureptr+channelidx*256*4, cols.arr(), 256*4 );
    }

    shaderctab_->image.finishEditing();

    if ( layeropacity_->value.getNum()>nrchannels )
	layeropacity_->value.deleteValues( nrchannels, -1 );

    int firstlayer = 0;
    char layertrans = SoTextureComposerInfo::cHasNoIntermediateTransparency();
    numlayers_->value.setValue( nrchannels );

    for ( int idx=nrchannels-1; idx>=0; idx-- )
    {
	if ( enabled_.size()<=idx || !enabled_[idx] )
	    continue;
	const SbImagei32& channel = channels_->getChannels()[idx];
	SbVec3i32 size;
	const unsigned char* vals = channel.getValue( size, dummy2 );
	
	// Starting from the front layer, find the (foremost) layer which is 
	// fully opaque (if any). That will be the first layer to be rendered 
	// (rendering starts at this layer and proceeds forward).
	// If there is no opaque layer, all layers will be rendered.
	// layertrans will be cHasNoTransparency if an opaque layer is found.
	// Otherwise, cHasTransparency if a partially transparent layer is 
	// found. Or else, it is cHasNoIntermediateTransparency.	

	if ( !vals )
	    continue;

	if ( opacity_[idx] == 0 )
	    continue;

	firstlayer = idx;
	char thislayertrans = getTextureTransparency( idx );
	if ( thislayertrans == SoTextureComposerInfo::cHasNoTransparency() )
	{
	    layertrans = thislayertrans;
	    break;
	}
	else if ( thislayertrans == SoTextureComposerInfo::cHasTransparency() )
	    layertrans = thislayertrans;
    }

    for ( int idx=0; idx<nrchannels; idx++ )
    {
	const SbImagei32& channel = channels_->getChannels()[idx];
	SbVec3i32 size;
	const unsigned char* vals = channel.getValue( size, dummy2 );
	layeropacity_->value.set1Value( idx,
	    vals && idx<enabled_.size() && enabled_[idx]
	    ? (float) opacity_[idx]/255. : 0.0 );
    }

    tci_->transparencyInfo = layertrans;
    startlayer_->value.setValue( firstlayer );
}


void ColTabTextureChannel2RGBA::createFragShadingProgram(int nrchannels,
						BufferString& res ) const
{
    const char* variables =
	"uniform sampler2D   ctabunit;				\n"
	"uniform int	    startlayer;				\n"
	"varying vec3	    ecPosition3;			\n"
	"varying vec3	    fragmentNormal;			\n"
	"uniform int	    numlayers;\n";

    const char* functions =
	"void processLayer( in float val, in float layeropacity, in float ctab, in bool first )\n"
	"{								\n"
	"    float ctabval = 0.001953125+0.996093750*val;		\n"
	// ctabval = 1./512 + 255/256*val
	// Layers ordered from back to front. First = backmost
	"    vec4 col = texture2D( ctabunit, vec2( ctabval, ctab ) );	\n"
	"    float blend = col.a * layeropacity;			\n"
	// Process only if pixel is not transparent.
	"    if ( (blend==0.0) )					\n"
	"        return;  						\n"
	"    								\n"
	"    if ( first )						\n"
	"    {								\n"
	"        gl_FragColor.rgb = col.rgb;				\n"
	"	 gl_FragColor.a = blend;				\n"
	"    }								\n"
	"    else							\n"
	"    {								\n"
	// Color values are premultiplied with their alpha values.
	"        gl_FragColor.rgb = col.rgb*blend + gl_FragColor.rgb * (1.0 - blend); \n"
	"    }								\n"
	"}								\n\n";

    BufferString mainprogstart =
	"void main()							\n"
	"{								\n"
	"    if ( gl_FrontMaterial.diffuse.a<=0.0 )			\n"
	"	discard;						\n"
	"    float fnumlayers = float( numlayers );			\n"
	"    if ( startlayer<0 )					\n"
	"	gl_FragColor = vec4(1.0,1.0,1.0,1.0);				\n"
	"    else							\n"
	"    {								\n"
	"	vec2 tcoord = gl_TexCoord[0].st;			\n"
	"	vec4 data;						\n"
	"	bool first = true;					\n";


    res = variables;

    res += "uniform float  layeropacities["; res += nrchannels; res += "];\n";

    const int nrunits = nrchannels ? (nrchannels-1)/mLayersPerUnit+1 : 0;
    for ( int idx=0; idx<nrunits; idx++ )
    {
	res += "uniform sampler2D     dataunit";
	res += idx; res += ";\n";
    }

    res += functions;
    res += mainprogstart;

    int layer = 0;
    for ( int unit=0; unit<nrunits; unit++ )
    {
	if ( !unit )
	    res += "\tif ( startlayer<4 )\n\t{\n";
	else
	{
	    res += "\tif ( startlayer<"; res += (unit+1)*4;
	    res += " && numlayers>"; res += unit*4; res += ")\n\t{\n";
	}

	res += "\t    data = texture2D( dataunit";
	res += unit;
	res += ", tcoord );\n";

	for ( int idx=0; idx<mLayersPerUnit && layer<nrchannels; idx++,layer++ )
	{
	    res += "\t    if ( startlayer<="; res += layer;
	    res += " && numlayers>"; res += layer; res += " )\n";
	    res += "\t    {\n\t\tfloat ctab= (";
	    res += layer;
	    res += ".5)/fnumlayers;\n";
	    res += "\t\tprocessLayer( data["; res += idx; res += "], layeropacities[";
	    res += layer; res += "], ctab, first );\n";
	    res += "\t\tfirst = false;\n";
	    res += "\t    }\n";
	}

	res += "\t}\n";
    }

    res += "    }\n";
    res += "    gl_FragColor.a *= gl_FrontMaterial.diffuse.a;\n";
    res += "    gl_FragColor.rgb *= gl_Color.rgb;\n";

    res += "}";
}


void ColTabTextureChannel2RGBA::doFill(
	SoColTabTextureChannel2RGBA* conv ) const
{
    const int nrchannels = coltabs_.size();

    TypeSet<unsigned char> cols;
    for ( int channelidx=0; channelidx<nrchannels; channelidx++ )
    {
	getColors( channelidx, cols );
    
	SbImagei32 image;
	image.setValue( SbVec3i32(1,1,256), 4, cols.arr() );
	conv->colorsequences.set1Value( channelidx, image );

	conv->enabled.set1Value( channelidx,enabled_[channelidx] &&
				 opacity_[channelidx]>0 );
	conv->opacity.set1Value( channelidx, opacity_[channelidx] );
    }

    if ( conv->enabled.getNum()>nrchannels )
	conv->enabled.deleteValues( nrchannels, -1 );

    if ( conv->opacity.getNum()>nrchannels )
	conv->opacity.deleteValues( nrchannels, -1 );

}


void ColTabTextureChannel2RGBA::getColors( int channelidx,
	TypeSet<unsigned char>& cols ) const
{
    if ( channelidx>=coltabs_.size() )
    {
	cols.setSize( 256*4, 255 );
	return;
    }

    const ColTab::Sequence& seq = *coltabs_[channelidx];
    if ( cols.size()!=((mNrColors+1)*4) )
	cols.setSize( ((mNrColors+1)*4), 0 );

    unsigned char* arr = cols.arr();
    for ( int idx=0; idx<mNrColors; idx++ )
    {
	const float val = ((float) idx)/(mNrColors-1);
	const Color col = seq.color( val );

	(*arr++) = col.r();
	(*arr++) = col.g();
	(*arr++) = col.b();
	(*arr++) = 255-col.t();
    }

    const Color col = seq.undefColor();
    (*arr++) = col.r();
    (*arr++) = col.g();
    (*arr++) = col.b();
    (*arr++) = 255-col.t();
}


char ColTabTextureChannel2RGBA::getTextureTransparency( int channelidx ) const
{
    if ( !enabled_[channelidx] )
	return SoTextureComposerInfo::cHasNoIntermediateTransparency();
	// disabled - so transparent

    // Texture transparency is a combination of opacity of the layer
    // and the transparency of the channels.

    // Layer: 0 - transparent. 1-254 - translucent. 255 - opaque.
    // CASE	LAYER		SEQUENCE		COMBINED
    //	a	  0		   X			cHasNoInter.
    //  b	1-254		cHasTransparency	cHasTransparency
    //  c	1-254		cHasNoInter.		cHasTransparency
    //  d	1-254		Opaque			cHasTransparency
    //  e	 255		cHasTransparency	cHasTransparency
    //  f	 255		cHasNoInter.		cHasNoInter.
    //  g	 255		Opaque			Opaque

    // If the sequence has transparency, all the individual pixels must be 
    // checked to find the type of transparency. This will be done by 
    // ColTabSequenceTransparencyCheck.
	
    bool hastrans = false;  // assume layer is fully opaque
    if ( opacity_[channelidx]!=255 )
    {
	if ( opacity_[channelidx]==0 )
	    return SoTextureComposerInfo::cHasNoIntermediateTransparency();
	    // transparent layer; need not check sequence (case a)
	else
	    hastrans = true;  // layer opacity indicates translucency (b/c/d)
    }

    if ( hastrans )
	return SoTextureComposerInfo::cHasTransparency();  // cases b, c, d

    // check the per-pixel transparency
    channels_->ref();
    const SbImagei32& channel = channels_->getChannels()[channelidx];
    
    SbVec3i32 size;
    int dummy;
    const unsigned char* vals = channel.getValue( size, dummy );
    od_int64 nrpixels = size[0];
    nrpixels *= size[1];
    nrpixels *= size[2];
    channels_->unRef();

    TypeSet<unsigned char> cols;
    getColors( channelidx, cols );

    // check the entire sequence for transparency type
    ColTabSequenceTransparencyCheck trspcheck( cols.arr(), vals, nrpixels,true);
    trspcheck.execute();

    return trspcheck.getTransparency();  // cases e, f, g
}


SoNode* ColTabTextureChannel2RGBA::gtInvntrNode()
{ return shaderswitch_; }


const char* ColTabTextureChannel2RGBA::sVertexShaderProgram()
{
    return
"varying vec3 ecPosition3;\n"
"varying vec3 fragmentNormal;\n"
"void main(void)\n"
"{\n"
"    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;\n"
"    ecPosition3 = ecPosition.xyz / ecPosition.w;\n"
"    fragmentNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
"\n"
"    vec4 diffuse = vec4(0.0,0.0,0.0,0.0);\n"
"    vec4 ambient = vec4(0.0,0.0,0.0,0.0);\n"
"    vec4 specular = vec4(0.0,0.0,0.0,0.0);\n"
"\n"
"    for ( int light = 0; light<2; light++ )\n"
"    {\n"
"        vec3 lightDir = normalize(vec3(gl_LightSource[light].position));\n"
"        float NdotL = abs( dot(fragmentNormal, lightDir) );\n"
"\n"
"        diffuse += gl_FrontMaterial.diffuse * \n"
"                       gl_LightSource[light].diffuse * NdotL;\n"
"        ambient += gl_FrontMaterial.ambient*gl_LightSource[light].ambient;\n"
"        float pf = 0.0;\n"
"        if (NdotL != 0.0)\n"
"        {\n"
"           float nDotHV = abs( \n"
"	          dot(fragmentNormal, vec3(gl_LightSource[light].halfVector)));\n"
"           pf = pow(nDotHV, gl_FrontMaterial.shininess);\n"
"        }\n"
"        specular += gl_FrontMaterial.specular * \n"
"            gl_LightSource[light].specular * pf;\n"
"    }\n"
"\n"
"    vec3 lightning =\n"
"       gl_FrontLightModelProduct.sceneColor.rgb +\n"
"       ambient.rgb * gl_FrontMaterial.ambient.rgb +\n"
"       diffuse.rgb * gl_Color.rgb +\n"
"       specular.rgb * gl_FrontMaterial.specular.rgb;\n"
"    gl_Position = ftransform();\n"
"    gl_FrontColor.rgb = lightning;\n"
"    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n"
"}\n";
}







}; // namespace visBase
