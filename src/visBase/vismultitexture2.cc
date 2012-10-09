/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id$";


#include "vismultitexture2.h"

#include "arrayndimpl.h"
#include "array2dresample.h"
#include "interpol2d.h"
#include "errh.h"
#include "simpnumer.h"
#include "thread.h"
#include "viscolortab.h"
#include "SoOD.h"

#include "Inventor/nodes/SoShaderProgram.h"
#include "Inventor/nodes/SoFragmentShader.h"
#include "Inventor/nodes/SoVertexShader.h"
#include "Inventor/nodes/SoShaderParameter.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoComplexity.h"
#include "Inventor/nodes/SoTextureUnit.h"

#include "SoColTabMultiTexture2.h"
#include "SoShaderTexture2.h"
#include "SoSplitTexture2.h"

mCreateFactoryEntry( visBase::MultiTexture2 );

#define mLayersPerUnit		4
#define mMaxNrUnits		3 //Coresponds to 8 layers


namespace visBase
{

MultiTexture2::MultiTexture2()
    : switch_( new SoSwitch )
    , texture_( new SoColTabMultiTexture2 )
    , nonshadinggroup_( new SoGroup )
    , shadinggroup_( new SoGroup )
    , complexity_( new SoComplexity )
    , shadingcomplexity_( 0 )
    , size_( -1, -1 )
    , useshading_( false )
    , dosplittexture_( false )			  
    , allowshading_( true )
    , ctabtexture_( 0 )
    , datatexturegrp_( 0 )
    , shaderprogram_( 0 )
    , fragmentshader_( 0 )
    , numlayers_( 0 )
    , layeropacity_( 0 )
    , layersize0_( 0 )
    , layersize1_( 0 )
    , ctabunit_( 0 )
    , enableinterpolation_( true )
{
    switch_->ref();
    switch_->addChild( nonshadinggroup_ );
    switch_->addChild( shadinggroup_ );

    nonshadinggroup_->addChild( complexity_ );
    complexity_->type.setIgnored( true );
    complexity_->value.setIgnored( true );
    complexity_->textureQuality.setValue( 1 );

    texture_->setNrThreads( __iswin__ ? 1 : Threads::getNrProcessors() );
    nonshadinggroup_->addChild( texture_ );

    turnOn( true );
}


MultiTexture2::~MultiTexture2()
{ switch_->unref(); }


int MultiTexture2::maxNrTextures() const
{
    static const int nrunits = mMIN( SoOD::maxNrTextureUnits(), mMaxNrUnits );
    return (nrunits-1) * mLayersPerUnit;
}


SoNode* MultiTexture2::gtInvntrNode()
{ return switch_; }


bool MultiTexture2::turnOn( bool yn )
{
    const bool res = isOn();
    if ( !yn )
	switch_->whichChild = SO_SWITCH_NONE;
    else
    {
	reviewShading();
	switch_->whichChild = useshading_ ? 1 : 0;
    }

    return res;
}


bool MultiTexture2::isOn() const
{
    return switch_->whichChild.getValue()!=SO_SWITCH_NONE;
}


void MultiTexture2::clearAll()
{
    size_.row = -1; size_.col = -1;

    for ( int idx=0; idx<nrTextures(); idx++ )
    {
	for ( int idy=0; idy<nrVersions(idx); idy++ )
	{
	    setData( idx, idy, 0 );
	}
    }
}


bool MultiTexture2::canUseShading() const
{
    return allowshading_ && SoOD::supportsFragShading()==1 &&
	   SoOD::supportsVertexShading()==1;
}


bool MultiTexture2::splitsTexture() const
{
    return dosplittexture_ && useshading_;
}


void MultiTexture2::splitTexture( bool yn )
{
    if ( dosplittexture_ == yn )
	return;

    dosplittexture_ = yn;
    updateSoTextureInternal( 0 );
}


void MultiTexture2::setTextureTransparency( int texturenr, unsigned char trans )
{
    reviewShading();

    if ( useshading_ )
    {
	createShadingVars();

	while ( layeropacity_->value.getNum()<texturenr )
	    layeropacity_->value.set1Value( layeropacity_->value.getNum(),
		isTextureEnabled(layeropacity_->value.getNum()) &&
	    getCurrentTextureIndexData(layeropacity_->value.getNum()) ? 1 : 0 );

	const float opacity = 1.0 - (float) trans/255;
	layeropacity_->value.set1Value( texturenr,
		isTextureEnabled( texturenr ) &&
		getCurrentTextureIndexData( texturenr ) ? opacity : 0 );
	opacity_.setSize( nrTextures(), 1 );
	opacity_[texturenr] = opacity;
	updateShadingVars();
    }
    else
    {
	while ( texture_->opacity.getNum()<texturenr )
	    texture_->opacity.set1Value( texture_->opacity.getNum(), 255 );
	texture_->opacity.set1Value( texturenr, 255-trans );
    }
}


void MultiTexture2::swapTextures( int t0, int t1 )
{
    if ( useshading_ )
    {
	float tmp = layeropacity_->value[t1];
	layeropacity_->value.set1Value( t1, layeropacity_->value[t0] );
	layeropacity_->value.set1Value( t0, tmp );

	tmp = opacity_[t1];
	opacity_[t1] = opacity_[t0];
	opacity_[t0] = tmp;
    }
    else
    {
	const unsigned char tmp = texture_->opacity[t1];
	texture_->opacity.set1Value( t1, texture_->opacity[t0] );
	texture_->opacity.set1Value( t0, tmp );
    }

    MultiTexture::swapTextures( t0, t1 );
}



unsigned char MultiTexture2::getTextureTransparency( int texturenr ) const
{
    if ( useshading_ )
    {
	return texturenr<opacity_.size()
	    ? 255-mNINT32(opacity_[texturenr]*255) : 0;
    }
    else
    {
	if ( texturenr>=texture_->opacity.getNum() )
	    return 0;

	return 255-texture_->opacity[texturenr];
    }
}


void MultiTexture2::setOperation( int texturenr, MultiTexture::Operation op )
{
    reviewShading();

    if ( useshading_ )
    {
	if ( op!=MultiTexture::BLEND )
	    pErrMsg("Not implemented");
	return;
    }

    SoColTabMultiTexture2::Operator nop = SoColTabMultiTexture2::BLEND;
    if ( op==MultiTexture::REPLACE)
	nop = SoColTabMultiTexture2::REPLACE;
    else if ( op==MultiTexture::ADD )
	nop = SoColTabMultiTexture2::ADD;

    while ( texture_->operation.getNum()<texturenr )
	texture_->operation.set1Value( texture_->operation.getNum(),
				       SoColTabMultiTexture2::BLEND  );

    texture_->operation.set1Value( texturenr, nop );
}


MultiTexture::Operation MultiTexture2::getOperation( int texturenr ) const
{
    if ( useshading_ )
	return MultiTexture::BLEND;

    if ( texturenr>=texture_->operation.getNum() ||
	 texture_->operation[texturenr]==SoColTabMultiTexture2::BLEND )
	return MultiTexture::BLEND;
    else if ( texture_->operation[texturenr]==SoColTabMultiTexture2::REPLACE )
	return MultiTexture::REPLACE;

    return MultiTexture::ADD;
}


void MultiTexture2::setTextureRenderQuality( float quality )
{}


float MultiTexture2::getTextureRenderQuality() const
{ return 1; }


void MultiTexture2::enableInterpolation( bool yn )
{
    enableinterpolation_ = yn;

    if ( useshading_ && canUseShading() && shadingcomplexity_ )
	shadingcomplexity_->textureQuality.setValue( yn ? 0.9 : 0.1 );
    else if ( complexity_ )
    {
	complexity_->textureQuality.setValue( yn ? 0.9 : 0.1 );
    	//if ( !yn ) updateColorTables();
	//Crap, if you need it to work, ebale it.
    }
}


bool MultiTexture2::interpolationEnabled() const
{ return enableinterpolation_; }


int MultiTexture2::getMaxTextureSize()
{ return SoColTabMultiTexture2::getMaxSize(); }


bool MultiTexture2::setDataOversample( int texture, int version,
				       int resolution, bool interpol,
	                               const Array2D<float>* data, bool copy )
{
    if ( !data ) return setData( texture, version, data );

    allowshading_ = false;	//If user has oversampling, we cannot easily
    				//go back to non-oversamples
    const int datax0size = data->info().getSize(0);
    const int datax1size = data->info().getSize(1);
    if ( datax0size<2 || datax1size<2  )
	return setData( texture, version, data, copy );

    const static int minpix2d = 128;
    const int maxpix2d = getMaxTextureSize();

    const int newx0 = getPow2Sz( datax0size*resolution,true,minpix2d,maxpix2d );
    const int newx1 = getPow2Sz( datax1size*resolution,true,minpix2d,maxpix2d );

    if ( !setSize( newx0, newx1 ) )
	return false;

    Array2DImpl<float> interpoldata( newx0, newx1 );
    if ( !interpoldata.isOK() )
	return false;

    if ( interpol )
	polyInterp( *data, interpoldata );
    else
	nearestValInterp( *data, interpoldata );

    const int totalsz = interpoldata.info().getTotalSz();
    float* arr = new float[totalsz];
    if ( !arr )
	return false;
    memcpy( arr, interpoldata.getData(), totalsz*sizeof(float) );
    return setTextureData( texture, version, arr, totalsz, true );
}


static bool newsize_ = false;

bool MultiTexture2::setSize( int sz0, int sz1 )
{
    if ( size_.row==sz0 && size_.col==sz1 )
	return true;

    if ( size_.row>=0 && size_.col>=0 &&
		(nrTextures()>1 || (nrTextures() && nrVersions(0)>1)) )
    {
	pErrMsg("Invalid size" );
	return false;
    }

    size_.row = sz0;
    size_.col = sz1;
    if ( layersize0_ )
    {
	layersize0_->value.setValue( size_.col );
	layersize1_->value.setValue( size_.row );
    }

    newsize_ = true;
    reviewShading();
    newsize_ = false;

    return true;
}

void MultiTexture2::nearestValInterp( const Array2D<float>& inp,
				      Array2D<float>& out ) const
{   
    const int inpsize0 = inp.info().getSize( 0 );
    const int inpsize1 = inp.info().getSize( 1 );
    const int outsize0 = out.info().getSize( 0 );
    const int outsize1 = out.info().getSize( 1 );
    const float x0step = (inpsize0-1)/(float)(outsize0-1);
    const float x1step = (inpsize1-1)/(float)(outsize1-1);

    for ( int x0=0; x0<outsize0; x0++ )
    {
	const int x0sample = mNINT32( x0*x0step );
	for ( int x1=0; x1<outsize1; x1++ )
	{
	    const float x1pos = x1*x1step;
	    out.set( x0, x1, inp.get( x0sample, mNINT32(x1pos) ) );
	}
    }
}


void MultiTexture2::polyInterp( const Array2D<float>& inp,
				Array2D<float>& out ) const
{
    Array2DReSampler<float,float> resampler( inp, out, true );
    resampler.execute();
}


bool MultiTexture2::setData( int texture, int version,
			     const Array2D<float>* data, bool copy )
{
    if ( data && !setSize( data->info().getSize(0), data->info().getSize(1) ) )
	return false;

    const int totalsz = data ? data->info().getTotalSz() : 0;
    const float* dataarray = data ? data->getData() : 0;
    bool manage = false;
    if ( data && (!dataarray || copy ) )
    {
	float* arr = new float[totalsz];
	data->getAll( arr );
	manage = true;
	dataarray = arr;
    }

    return setTextureData( texture, version, dataarray, totalsz, manage );
}


bool MultiTexture2::setIndexData( int texture, int version,
				  const Array2D<unsigned char>* data )
{
    const int totalsz = data ? data->info().getTotalSz() : 0;
    const unsigned char* dataarray = data ? data->getData() : 0;
    float manage = false;
    if ( data && !dataarray )
    {
	unsigned char* arr = new unsigned char[totalsz];
	data->getAll( arr );

	manage = true;
	dataarray = arr;
    }

    return setTextureIndexData( texture, version, dataarray, totalsz, manage );
}


void MultiTexture2::updateSoTextureInternal( int texturenr )
{
    reviewShading();
    const unsigned char* texture0 = getCurrentTextureIndexData(texturenr);

    if ( size_.row<0 || size_.col<0 || !texture0 )
    {
	if ( useshading_ && layeropacity_ )
	    layeropacity_->value.set1Value( texturenr, 0 );
	else 
	    texture_->enabled.set1Value( texturenr, false );

	return;
    }

    if ( useshading_ )
    {
	const int nrelem = size_.col*size_.row;

	const int unit = texturenr/mLayersPerUnit;
	int texturenrbase = unit*mLayersPerUnit;
	int num = nrTextures()-texturenrbase;
	if ( num>4 ) num = 4;
	else if ( num==2 ) num=3;

	unsigned const char* t0 = getCurrentTextureIndexData(texturenrbase);
	unsigned const char* t1 = num>1
	    ? getCurrentTextureIndexData(texturenrbase+1) : 0;
	unsigned const char* t2 = num>2
	    ? getCurrentTextureIndexData(texturenrbase+2) : 0;
	unsigned const char* t3 = num>3
	    ? getCurrentTextureIndexData(texturenrbase+3) : 0;

	ArrPtrMan<unsigned char> ptr = new unsigned char[num*nrelem];
	if ( !ptr ) return;

	unsigned char* curptr = ptr;

	for ( int idx=0; idx<nrelem; idx++, curptr+= num )
	{
	    curptr[0] = t0 ? *t0++ : 255;
	    if ( num==1 ) continue;

	    curptr[1] = t1 ? *t1++ : 255;
	    curptr[2] = t2 ? *t2++ : 255;
	    if ( num!=4 ) continue;
	    curptr[3] = t3 ? *t3++ : 255;
	}

	createShadingVars();

	if ( dosplittexture_ )
	{
	    mDynamicCastGet( SoSplitTexture2*, texture,
			     datatexturegrp_->getChild( unit*2+1 ) );
	    if ( !texture )
		return;

	    texture->image.setValue( SbVec2s(size_.col,size_.row), num, ptr,
				     SoSFImage::COPY );
	}
	else
	{
	    mDynamicCastGet( SoShaderTexture2*, texture,
			     datatexturegrp_->getChild( unit*2+1 ) );
	    if ( !texture )
		return;
	    
	    texture->image.setValue( SbVec2s(size_.col,size_.row), num, ptr,
				     SoSFImage::COPY );
	}
    }
    else
    {
	const SbImagei32 image( texture0, SbVec2i32(size_.col,size_.row), 1 );
	texture_->image.set1Value( texturenr, image );
    }

    updateColorTables();
}


void MultiTexture2::updateColorTables()
{
    reviewShading();
    if ( useshading_ )
    {
	if ( !ctabtexture_ ) 
	    return;
	
	const int nrtextures = nrTextures();
	unsigned char* arrstart = 0;

	SbVec2s cursize;
	int curnc;
	bool finishedit = false;
	unsigned char* curarr = ctabtexture_->image.startEditing(cursize,curnc);

	if ( curnc==4 && cursize[1]==nrtextures && cursize[0]==256 )
	{
	    arrstart = curarr;
	    finishedit = true;
	}
	else
	    arrstart = new unsigned char[nrtextures*256*4];

	unsigned char* arr = arrstart;

	opacity_.setSize( nrtextures, 1 );

	for ( int idx=0; idx<nrtextures; idx++ )
	{
	    const VisColorTab& ctab = getColorTab( idx );
	    const int nrsteps = ctab.nrSteps();

	    for ( int idy=0; idy<256; idy++ )
	    {
		const Color col =
		    idy<=nrsteps ? ctab.tableColor( idy ) : Color::Black();

		*(arr++) = col.r();
		*(arr++) = col.g();
		*(arr++) = col.b();
		*(arr++) = 255-col.t();
	    }
	}

	if ( finishedit )
	    ctabtexture_->image.finishEditing();
	else
	{
	    ctabtexture_->image.setValue( SbVec2s(256,nrtextures), 4, 
		    arrstart, SoSFImage::NO_COPY_AND_DELETE );
	}

	updateShadingVars();
    }
    else
    {
	int totalnr = 0;
	const int nrtextures = nrTextures();
	for ( int idx=0; idx<nrtextures; idx++ )
	    totalnr += getColorTab( idx ).nrSteps() + 1;

	unsigned char* arrstart = 0;

	SbVec2s cursize;
	int curnc;
	bool finishedit = false;
	unsigned char* curarr = texture_->colors.startEditing( cursize, curnc );
	if ( curnc==4 && cursize[1]==totalnr )
	{
	    arrstart = curarr;
	    finishedit = true;
	}
	else
	    arrstart = new unsigned char[totalnr*4];

	unsigned char* arr = arrstart;

	if ( texture_->numcolor.getNum()>nrtextures )
	    texture_->numcolor.deleteValues( nrtextures, -1 );
	if ( texture_->component.getNum()>nrtextures )
	    texture_->component.deleteValues( nrtextures, -1 );
	if ( texture_->enabled.getNum()>nrtextures )
	    texture_->enabled.deleteValues( nrtextures, -1 );

	for ( int idx=0; idx<nrtextures; idx++ )
	{
	    if ( !isTextureEnabled(idx) || !getCurrentTextureIndexData(idx) )
	    {
		texture_->enabled.set1Value( idx, false );
		continue;
	    }

	    texture_->enabled.set1Value( idx, true );

	    const VisColorTab& ctab = getColorTab( idx );
	    const int nrsteps = ctab.nrSteps();

	    texture_->numcolor.set1Value( idx, nrsteps+1 ); //one extra for udf
	    for ( int idy=0; idy<=nrsteps; idy++ )
	    {
		const Color col = ctab.tableColor( idy );
		*(arr++) = col.r();
		*(arr++) = col.g();
		*(arr++) = col.b();
		*(arr++) = 255-col.t();
	    }

	    SoColTabMultiTexture2::Operator op = SoColTabMultiTexture2::BLEND;
	    if ( !idx || getOperation(idx)==MultiTexture::REPLACE)
		op = SoColTabMultiTexture2::REPLACE;
	    else if ( getOperation(idx)==MultiTexture::ADD )
		op = SoColTabMultiTexture2::ADD;

	    texture_->component.set1Value( idx, getComponents(idx) );
	}

	if ( finishedit )
	    texture_->colors.finishEditing();
	else
	    texture_->colors.setValue( SbVec2s(totalnr,1), 4, arrstart,
				      SoSFImage::NO_COPY_AND_DELETE );
    }
}


void MultiTexture2::updateShadingVars()
{
    const int nrtextures = nrTextures();

    if ( layeropacity_->value.getNum()>nrtextures )
	layeropacity_->value.deleteValues( nrtextures, -1 );


    numlayers_->value.setValue( nrtextures );

    int firstlayer = -1;
    bool firstlayerhastrans;

    for ( int idx=nrtextures-1; idx>=0; idx-- )
    {
	if ( isTextureEnabled(idx) && getCurrentTextureIndexData(idx) )
	{
	    firstlayer = idx;
	    firstlayerhastrans = hasTransparency( idx );
	    if ( !firstlayerhastrans )
		break;
	}
    }

    if ( firstlayer>=0 )
    {
	for ( int idx=0; idx<nrtextures; idx++ )
	{
	    if ( dosplittexture_ )
	    {
		const int unit = idx/mLayersPerUnit;
		mDynamicCastGet( SoSplitTexture2*, texture,
				datatexturegrp_->getChild( unit*2+1 ) );
		if ( texture )
		    texture->transparencyInfo = firstlayerhastrans
			? SoSplitTexture2::cHasTransparency()
			: SoSplitTexture2::cHasNoTransparency();
	    }

	    layeropacity_->value.set1Value( idx,
		    isTextureEnabled(idx) && getCurrentTextureIndexData(idx)
		    ? opacity_[idx] : 0 );
	}
    }

    startlayer_->value.setValue( firstlayer );
}
    

	
void MultiTexture2::insertTextureInternal( int texturenr )
{
    texture_->image.insertSpace( texturenr, 1 );
    updateSoTextureInternal( texturenr );
}


void MultiTexture2::removeTextureInternal( int texturenr )
{
    reviewShading();
    if ( useshading_ )
    {
	updateSoTextureInternal( texturenr );
    }
    else if ( texture_->image.getNum()>texturenr )
	texture_->image.deleteValues( texturenr, 1 );

    updateColorTables();
}


void MultiTexture2::createShadingVars()
{
    if ( !ctabtexture_ )
    {
	const int ctabunit = SoOD::maxNrTextureUnits()-1;

	SoTextureUnit* unit = new SoTextureUnit;
	unit->unit = ctabunit;
	shadinggroup_->addChild( unit );

	SoComplexity* complexity = new SoComplexity;
	complexity->textureQuality.setValue( 0.1 );
	shadinggroup_->addChild( complexity );

	ctabtexture_ = new SoShaderTexture2;
	shadinggroup_->addChild( ctabtexture_ );

	if ( !shadingcomplexity_ )
	    shadingcomplexity_ = new SoComplexity;
    	    
	shadingcomplexity_->textureQuality.setValue( 
		enableinterpolation_ ? 1 : 0.1 );
	shadinggroup_->addChild( shadingcomplexity_ );

	datatexturegrp_ = new SoGroup;
	shadinggroup_->addChild( datatexturegrp_ );

	shaderprogram_ = new SoShaderProgram();

	SoVertexShader* vertexshader = new SoVertexShader;
	vertexshader->sourceType = SoShaderObject::GLSL_PROGRAM;
	vertexshader->sourceProgram.setValue( sVertexShaderProgram() );

	shaderprogram_->shaderObject.addNode( vertexshader );

	fragmentshader_ = new SoFragmentShader;
	fragmentshader_->sourceType = SoShaderObject::GLSL_PROGRAM;
	BufferString shadingprog;
	createShadingProgram( maxNrTextures(), shadingprog );
	fragmentshader_->sourceProgram.setValue( shadingprog.buf() );
	numlayers_ = new SoShaderParameter1i;
	numlayers_->name.setValue("numlayers");
	fragmentshader_->parameter.addNode( numlayers_ );

	startlayer_ = new SoShaderParameter1i;
	startlayer_->name.setValue("startlayer");
	fragmentshader_->parameter.addNode( startlayer_ );
	startlayer_->value.setValue( 0 );

	layeropacity_ = new SoShaderParameterArray1f;
	layeropacity_->name.setValue("trans");
	fragmentshader_->parameter.addNode( layeropacity_ );

	ctabunit_ = new SoShaderParameter1i;
	ctabunit_->name.setValue("ctabunit");
	ctabunit_->value.setValue( ctabunit );
	fragmentshader_->parameter.addNode( ctabunit_ );

	layersize0_ = new SoShaderParameter1i;
	layersize0_->name.setValue("texturesize0");
	layersize0_->value.setValue(size_.col);
	fragmentshader_->parameter.addNode( layersize0_ );

	layersize1_ = new SoShaderParameter1i;
	layersize1_->name.setValue("texturesize1");
	layersize1_->value.setValue(size_.row);
	fragmentshader_->parameter.addNode( layersize1_ );

	const int maxnrunits = (MultiTexture2::maxNrTextures()-1)/
	    			mLayersPerUnit+1;
	
	usedtextureunits_.erase();
	for ( int idx=0; idx<maxnrunits; idx++ )
	{
	    SoShaderParameter1i* dataunit = new SoShaderParameter1i;
	    BufferString nm = "dataunit";
	    nm += idx;
	    dataunit->name.setValue( nm.buf() );
	    dataunit->value.setValue( idx );
	    usedtextureunits_ += idx;
	    fragmentshader_->parameter.addNode( dataunit );
	}

	shaderprogram_->shaderObject.addNode( fragmentshader_ );
	shadinggroup_->addChild( shaderprogram_ );

	//Reset the texture unit so the shape's
	//tcoords come in the right unit.
	unit = new SoTextureUnit;
	unit->unit = 0;
	shadinggroup_->addChild( unit );
    }

    const int nrunits = (nrTextures()-1)/mLayersPerUnit+1;
    for ( int idx=datatexturegrp_->getNumChildren()/2; idx<nrunits; idx++ )
    {
	SoTextureUnit* u1 = new SoTextureUnit;
	u1->unit = idx;
	datatexturegrp_->addChild( u1 );
	datatexturegrp_->addChild( dosplittexture_ 
		? (SoNode*) new SoSplitTexture2 
		: (SoNode*) new SoShaderTexture2 );
    }
}


const char* MultiTexture2::sVertexShaderProgram()
{
    return 
"varying vec3 ecPosition3;\n"
"varying vec3 fragmentNormal;\n"
"void main(void)\n"
"{\n"
"    vec4 ecPosition = gl_ModelViewMatrix * gl_Vertex;\n"
"    ecPosition3 = ecPosition.xyz / ecPosition.w;\n"
"    fragmentNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
"    vec3 lightDir = normalize(vec3(gl_LightSource[0].position));\n"
"    float NdotL = abs( dot(fragmentNormal, lightDir) );\n"
"\n"
"    vec4 diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse * NdotL;\n"
"    vec4 ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;\n"
"    float pf = 0.0;\n"
"    if (NdotL != 0.0)\n"
"    {\n"
"	float nDotHV = abs( dot(fragmentNormal, vec3(gl_LightSource[0].halfVector)));\n"
"	pf = pow(nDotHV, gl_FrontMaterial.shininess);\n"
"    }\n"
"    vec4 specular = gl_FrontMaterial.specular * gl_LightSource[0].specular * pf;\n"
"\n"
"    vec3 lightning =\n"
"	gl_FrontLightModelProduct.sceneColor.rgb +\n"
"	ambient.rgb * gl_FrontMaterial.ambient.rgb +\n"
"	diffuse.rgb * gl_Color.rgb +\n"
"	specular.rgb * gl_FrontMaterial.specular.rgb;\n"
"    gl_Position = ftransform();\n"
"    gl_FrontColor.rgb = lightning;\n"
"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"}\n";
}






void MultiTexture2::reviewShading()
{
    bool res = false;
    if ( canUseShading() )
    {
	const int maxshadingsize = SoShaderTexture2::getMaxSize();
	if ( size_.row>0 && size_.col>0 && (dosplittexture_ ||
	     (size_.row<=maxshadingsize && size_.col<=maxshadingsize)) )
	    res = true;
	else if ( newsize_ && size_.row>0 && size_.col>0 )
	{
	    BufferString msg( "Texture size is too large for using shading.\n"
			      "Current size is (" );
	    msg += size_.row; msg += ","; msg += size_.col; msg += "). ";
	    msg += "Maximum size is: ("; msg += maxshadingsize; msg += ",";
	    msg += maxshadingsize; msg += ").";
	    ErrMsg( msg );
	}
    }

    if ( useshading_==res )
	return;

    useshading_ = res;
    turnOn( isOn() ); //update switch
    if ( res ) createShadingVars();
}


void  MultiTexture2::createShadingProgram( int nrlayers,
					   BufferString& res ) const
{
    const char* variables = 
"#extension GL_ARB_texture_rectangle : enable				\n\
uniform sampler2DRect   ctabunit;					\n\
uniform int             startlayer;					\n\
varying vec3		ecPosition3;					\n\
varying vec3		fragmentNormal;					\n\
uniform int             numlayers;\n";

    const char* functions = 

"void processLayer( in float val, in float layeropacity, in int layer )	\n"
"{									\n"
    "if ( layer==startlayer )						\n"
    "{									\n"
	"gl_FragColor = texture2DRect( ctabunit,			\n"
				  "vec2(val*255.0, float(layer)+0.5) );	\n"
	"gl_FragColor.a *= layeropacity;				\n"
    "}									\n"
    "else if ( layeropacity>0.0 )					\n"
    "{									\n"
	"vec4 col = texture2DRect( ctabunit, 				\n"
				   "vec2(val*255.0, float(layer)+0.5) );\n"
	"layeropacity *= col.a;						\n"
	"gl_FragColor.rgb = mix(gl_FragColor.rgb,col.rgb,layeropacity);	\n"
	"if ( layeropacity>gl_FragColor.a )				\n"
	    "gl_FragColor.a = layeropacity;				\n"
    "} 									\n"
"}\n\n";

    BufferString mainprogstart =
"void main()								\n\
{									\n\
    if ( gl_FrontMaterial.diffuse.a<=0.0 )				\n\
	discard;							\n\
    if ( startlayer<0 )							\n\
	gl_FragColor = vec4(1.0,1.0,1.0,1.0);					\n\
    else								\n\
    {									\n\
	vec2 tcoord = gl_TexCoord[0].st;				\n\
	vec4 data;\n";


    res = variables;
    if ( !dosplittexture_ )
    {
	res += "uniform int             texturesize0;\n";
	res += "uniform int             texturesize1;\n";
    }

    res += "uniform float           trans["; res += nrlayers; res += "];\n";

    const int nrunits = nrlayers ? (nrlayers-1)/mLayersPerUnit+1 : 0;
    for ( int idx=0; idx<nrunits; idx++ )
    {
	res += dosplittexture_ ? "uniform sampler2D	dataunit" :
	    			 "uniform sampler2DRect dataunit";
	res += idx; res += ";\n";
    }

    res += functions;
    res += mainprogstart;
    if ( !dosplittexture_ )
    {
	res += "	tcoord.s *= texturesize0;\n";
	res += "	tcoord.t *= texturesize1;\n";
    }

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

	res += dosplittexture_ ? "\t    data = texture2D( dataunit" :
				 "\t    data = texture2DRect( dataunit";
	res += unit;
	res += ", tcoord );\n";

	for ( int idx=0; idx<mLayersPerUnit && layer<nrlayers; idx++,layer++ )
	{
	    res += "\t    if ( startlayer<="; res += layer; 
	    res += " && numlayers>"; res += layer; res += " )\n";
	    res += "\t\tprocessLayer( data["; res +=
	    idx; res += "], trans[";
	    res += layer; res += "], "; res += layer; res += ");\n";
	}

	res += "\t}\n";
    }

    res += "    }\n";
    res += "    gl_FragColor.a *= gl_FrontMaterial.diffuse.a;\n";
    res += "    gl_FragColor.rgb *= gl_Color.rgb;\n";

    res += "}";
}


}; //namespace
