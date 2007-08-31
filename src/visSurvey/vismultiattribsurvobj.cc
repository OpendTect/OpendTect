/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vismultiattribsurvobj.cc,v 1.9 2007-08-31 12:48:58 cvskris Exp $";

#include "vismultiattribsurvobj.h"

#include "attribsel.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "viscolortab.h"
#include "vismultitexture2.h"
#include "iopar.h"


namespace visSurvey {

const char* MultiTextureSurveyObject::sKeyResolution()	{ return "Resolution"; }
const char* MultiTextureSurveyObject::sKeyTextTrans()	{ return "Trans"; }

MultiTextureSurveyObject::MultiTextureSurveyObject()
    : VisualObjectImpl(true)
    , texture_( visBase::MultiTexture2::create() )
    , onoffstatus_( true )
    , resolution_( 0 )
{
    texture_->ref();
    addChild( texture_->getInventorNode() );
    texture_->setTextureRenderQuality(1);

    material_->setColor( Color::White );
    material_->setAmbience( 0.8 );
    material_->setDiffIntensity( 0.8 );
}


MultiTextureSurveyObject::~MultiTextureSurveyObject()
{
    deepErase( as_ );
    setDataTransform( 0 );
    texture_->unRef();
}


bool MultiTextureSurveyObject::_init()
{
    return visBase::DataObject::_init() && addAttrib();
}


void MultiTextureSurveyObject::allowShading( bool yn )
{
    if ( texture_ )
	texture_->allowShading( yn );
}


void MultiTextureSurveyObject::turnOn( bool yn )
{
    onoffstatus_ = yn;
    updateMainSwitch();
}


bool MultiTextureSurveyObject::isOn() const
{ return onoffstatus_; }


int MultiTextureSurveyObject::getResolution() const
{ return resolution_; }


void MultiTextureSurveyObject::updateMainSwitch()
{
    bool newstatus = onoffstatus_;
    if ( newstatus )
    {
	newstatus = false;
	for ( int idx=nrAttribs()-1; idx>=0; idx-- )
	{
	    if ( isAttribEnabled(idx) )
	    {
		newstatus = true;
		break;
	    }
	}
    }

    VisualObjectImpl::turnOn( newstatus );
}


bool MultiTextureSurveyObject::canHaveMultipleAttribs() const
{ return true; }


int MultiTextureSurveyObject::nrAttribs() const
{ return as_.size(); }


bool MultiTextureSurveyObject::canAddAttrib() const
{
    const int maxnr = texture_->maxNrTextures();
    if ( !maxnr ) return true;

    return nrAttribs()<maxnr;
}


bool MultiTextureSurveyObject::addAttrib()
{
    as_ += new Attrib::SelSpec;
    addCache();

    while ( texture_->nrTextures()<as_.size() )
    {
	texture_->addTexture("");
	texture_->setOperation( texture_->nrTextures()-1,
				visBase::MultiTexture::BLEND );
    }

    updateMainSwitch();
    return true;
}


bool MultiTextureSurveyObject::removeAttrib( int attrib )
{
    if ( as_.size()<2 || attrib<0 || attrib>=as_.size() )
	return false;

    delete as_[attrib];
    as_.remove( attrib );

    removeCache( attrib );
    texture_->removeTexture( attrib );

    updateMainSwitch();
    return true;
}


bool MultiTextureSurveyObject::swapAttribs( int a0, int a1 )
{
    if ( a0<0 || a1<0 || a0>=as_.size() || a1>=as_.size() )
	return false;

    texture_->swapTextures( a0, a1 );
    as_.swap( a0, a1 );
    swapCache( a0, a1 );

    return true;
}


void MultiTextureSurveyObject::clearTextures()
{
    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	Attrib::SelSpec as; setSelSpec( idx, as );

	for ( int idy=nrTextures(idx)-1; idy>=0; idy-- )
	{
	    texture_->setData( idx, idy, 0, false );
	}
    }
}


void MultiTextureSurveyObject::setAttribTransparency( int attrib,
						      unsigned char nt )
{ texture_->setTextureTransparency( attrib, nt ); }


unsigned char
MultiTextureSurveyObject::getAttribTransparency( int attrib ) const
{ return texture_->getTextureTransparency( attrib ); }


const Attrib::SelSpec* MultiTextureSurveyObject::getSelSpec( int attrib ) const
{ return attrib>=0 && attrib<as_.size() ? as_[attrib] : 0; }


void MultiTextureSurveyObject::setSelSpec( int attrib,
					   const Attrib::SelSpec& as )
{
    if ( attrib>=0 && attrib<as_.size() )
	*as_[attrib] = as;

    emptyCache( attrib );

    const char* usrref = as.userRef();
    if ( !usrref || !*usrref )
	texture_->enableTexture( attrib, false );
}


bool MultiTextureSurveyObject::isClassification( int attrib ) const
{
    return attrib>=0 && attrib<isclassification_.size()
	? isclassification_[attrib] : false;
}


void MultiTextureSurveyObject::setClassification( int attrib, bool yn )
{
    if ( attrib<0 || attrib>=as_.size() )
	return;

    if ( yn )
    {
	while ( attrib>=isclassification_.size() )
	    isclassification_ += false;
    }
    else if ( attrib>=isclassification_.size() )
	return;

    isclassification_[attrib] = yn;
}


bool MultiTextureSurveyObject::isAngle( int attrib ) const
{ return texture_->isAngle( attrib ); }


void MultiTextureSurveyObject::setAngleFlag( int attrib, bool yn )
{ texture_->setAngleFlag( attrib, yn ); }


bool MultiTextureSurveyObject::isAttribEnabled( int attrib ) const 
{
    return texture_->isTextureEnabled( attrib );
}


void MultiTextureSurveyObject::enableAttrib( int attrib, bool yn )
{
    texture_->enableTexture( attrib, yn );
    updateMainSwitch();
}


const TypeSet<float>*
MultiTextureSurveyObject::getHistogram( int attrib ) const
{
    return texture_->getHistogram( attrib, texture_->currentVersion( attrib ) );
}


int MultiTextureSurveyObject::getColTabID( int attrib ) const
{
    return texture_->getColorTab( attrib ).id();
}


int MultiTextureSurveyObject::nrTextures( int attrib ) const
{
    return texture_->nrVersions( attrib );
}


void MultiTextureSurveyObject::selectTexture( int attrib, int idx )
{
    if ( attrib<0 || attrib>=nrAttribs() ||
	 idx<0 || idx>=texture_->nrVersions(attrib) ) return;

    texture_->setCurrentVersion( attrib, idx );
}


int MultiTextureSurveyObject::selectedTexture( int attrib ) const
{ 
    if ( attrib<0 || attrib>=nrAttribs() ) return 0;

    return texture_->currentVersion( attrib );
}


void MultiTextureSurveyObject::fillPar( IOPar& par,
					TypeSet<int>& saveids ) const
{
    visBase::VisualObject::fillPar( par, saveids );

    par.set( sKeyResolution(), resolution_ );
    par.setYN( visBase::VisualObjectImpl::sKeyIsOn(), isOn() );
    for ( int attrib=as_.size()-1; attrib>=0; attrib-- )
    {
	IOPar attribpar;
	as_[attrib]->fillPar( attribpar );
	const int coltabid = getColTabID(attrib);
	attribpar.set( sKeyColTabID(), coltabid );
	if ( saveids.indexOf( coltabid )==-1 ) saveids += coltabid;

	attribpar.setYN( visBase::VisualObjectImpl::sKeyIsOn(),
			 texture_->isTextureEnabled(attrib) );
	attribpar.set( sKeyTextTrans(),
			 texture_->getTextureTransparency(attrib) );

	BufferString key = sKeyAttribs();
	key += attrib;
	par.mergeComp( attribpar, key );
    }

    par.set( sKeyNrAttribs(), as_.size() );
}


int MultiTextureSurveyObject::usePar( const IOPar& par )
{
    const int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    par.get( sKeyResolution(), resolution_ );

    bool ison = true;
    par.getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
    turnOn( ison );

    int nrattribs;
    if ( par.get(sKeyNrAttribs(),nrattribs) ) //current format
    {
	TypeSet<int> coltabids( nrattribs, -1 );
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( attribpar->get(sKeyColTabID(),coltabids[attrib]) )
	    {
		visBase::DataObject* dataobj =
		    visBase::DM().getObject( coltabids[attrib] );
		if ( !dataobj ) return 0;
		mDynamicCastGet(const visBase::VisColorTab*,coltab,dataobj);
		if ( !coltab ) coltabids[attrib] = -1;
	    }
	}

	bool firstattrib = true;
	for ( int attrib=0; attrib<nrattribs; attrib++ )
	{
	    BufferString key = sKeyAttribs();
	    key += attrib;
	    PtrMan<const IOPar> attribpar = par.subselect( key );
	    if ( !attribpar )
		continue;

	    if ( !firstattrib )
		addAttrib();
	    else
		firstattrib = false;

	    const int attribnr = as_.size()-1;

	    as_[attribnr]->usePar( *attribpar );
	    const int coltabid = coltabids[attribnr];
	    if ( coltabid!=-1 )
	    {
		mDynamicCastGet(visBase::VisColorTab*,coltab, 
		       		visBase::DM().getObject(coltabid) );
		texture_->setColorTab( attribnr, *coltab );
	    }

	    ison = true;
	    attribpar->getYN( visBase::VisualObjectImpl::sKeyIsOn(), ison );
	    texture_->enableTexture( attribnr, ison );

	    unsigned int trans = 0;
	    attribpar->get( sKeyTextTrans(), trans );
	    texture_->setTextureTransparency( attribnr, trans );
	}
    }

    return 1;
}


void MultiTextureSurveyObject::getValueString( const Coord3& pos,
						BufferString& val ) const
{
    val = "undef";
    BufferString valname;

    for ( int idx=nrAttribs()-1; idx>=0; idx-- )
    {
	if ( !isAttribEnabled(idx) ||
		texture_->getTextureTransparency(idx)==255 )
	    continue;

	const int version = texture_->currentVersion(idx);
	float fval;
	if ( !getCacheValue(idx, version, pos, fval ) )
	    continue;

	bool islowest = true;
	for ( int idy=idx-1; idy>=0; idy-- )
	{
	    if ( !hasCache(idy) ||
		 !isAttribEnabled(idy) ||
		 texture_->getTextureTransparency(idy)==255 )
		continue;

	    islowest = false;
	    break;
	}    

	if ( !islowest )
	{
	    const Color col = texture_->getColorTab(idx).color(fval);
	    if ( col.t()==255 )
		continue;
	}

	if ( !mIsUdf(fval) )
	    val = fval;

	if ( nrAttribs()>1 )
	{
	    BufferString attribstr = "(";
	    attribstr += as_[idx]->userRef();
	    attribstr += ")";
	    val.insertAt( cValNameOffset(), (const char*)attribstr);
	}

	return;
    }
}


}; // namespace visSurvey
