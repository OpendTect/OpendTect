/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/


#include "uiviscoltabed.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "visscenecoltab.h"
#include "visdataman.h"
#include "visdata.h"
#include "vissurvobj.h"
#include "uicoltabsel.h"
#include "od_helpids.h"


const char* uiVisColTabEd::sKeyColorSeq()	{ return "ColorSeq Name"; }
const char* uiVisColTabEd::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* uiVisColTabEd::sKeyClipRate()	{ return "Cliprate"; }
const char* uiVisColTabEd::sKeyAutoScale()	{ return "Auto scale"; }
const char* uiVisColTabEd::sKeySymmetry()	{ return "Symmetry"; }
const char* uiVisColTabEd::sKeySymMidval()	{ return "Symmetry Midvalue"; }

uiVisColTabEd::uiVisColTabEd( uiColTabToolBar& cttb )
    : survobj_( 0 )
    , coltabsel_(cttb.selTool())
{
    visBase::DM().removeallnotify.notify(
	    mCB(this,uiVisColTabEd,removeAllVisCB) );
}


#define mImplNotification( refop, notifyop ) \
    if ( survobj_ ) \
    { \
	ConstRefMan<ColTab::MapperSetup> mapsu \
		= survobj_->getColTabMapperSetup(channel_,version_); \
	if ( mapsu ) \
	    { mapsu->objectChanged().notifyop( \
		    mCB(this,uiVisColTabEd,mapperChangeCB) ); } \
	const ColTab::Sequence* seq \
		= survobj_->getColTabSequence(channel_); \
	if ( seq ) \
	    { seq->objectChanged().notifyop( \
		    mCB(this,uiVisColTabEd,colSeqModifCB) ); } \
	mDynamicCastGet( visBase::DataObject*, dataobj, survobj_ ); \
	if ( dataobj ) \
	    dataobj->refop(); \
    }

uiVisColTabEd::~uiVisColTabEd()
{
    mImplNotification( unRef, remove );
    visBase::DM().removeallnotify.remove(
	    mCB(this,uiVisColTabEd,removeAllVisCB) );
}


void uiVisColTabEd::setColTab( const ColTab::Sequence* seq,
			       const ColTab::MapperSetup* setup )
{
    if ( seq )
	coltabsel_.setSeqName( seq->name() );
    if ( setup )
	coltabsel_.useMapperSetup( *setup );
}


void uiVisColTabEd::setColTab( visSurvey::SurveyObject* so, int channel,
			       int version )
{
    mImplNotification( unRef, remove );

    survobj_ = so;
    channel_ = channel;
    version_ = version;

    mImplNotification( ref, notifyIfNotNotified );

    if ( so )
	setColTab( so->getColTabSequence(channel_),
		    so->getColTabMapperSetup(channel_,version_) );
}


void uiVisColTabEd::colSeqModifCB( CallBacker* )
{
    ConstRefMan<ColTab::MapperSetup> ms  =
			survobj_->getColTabMapperSetup( channel_, version_ );
    if ( ms )
    {
	//TODO this is a terrible hack to force remap.
	// Long live the centralization of Probe power!
	ms->sendEntireObjectChangeNotification();
	survobj_->setColTabSequence( channel_, *coltabsel_.sequence() );
    }
}


void uiVisColTabEd::mapperChangeCB( CallBacker* )
{
    ConstRefMan<ColTab::MapperSetup> ms  =
			survobj_->getColTabMapperSetup( channel_, version_ );

    if ( ms )
	coltabsel_.useMapperSetup( *ms );
    if ( survobj_->getScene() && ms )
	survobj_->getScene()->getSceneColTab()->setColTabMapperSetup( *ms );
}


void uiVisColTabEd::removeAllVisCB( CallBacker* )
{
    mImplNotification( unRef, remove );
    survobj_ = 0;
}


const ColTab::Sequence& uiVisColTabEd::getColTabSequence() const
{ return *coltabsel_.sequence(); }

ConstRefMan<ColTab::MapperSetup> uiVisColTabEd::getColTabMapperSetup() const
{ return coltabsel_.mapperSetup(); }

NotifierAccess& uiVisColTabEd::seqChange()
{ return coltabsel_.seqChanged; }

NotifierAccess& uiVisColTabEd::mapperChange()
{ return coltabsel_.mapperSetup()->objectChanged(); }


void uiVisColTabEd::setDistribution( const DataDistribution<float>& distr )
{
    coltabsel_.useDistribution( distr );
}


bool uiVisColTabEd::usePar( const IOPar& par )
{
    return true;
}

void uiVisColTabEd::fillPar( IOPar& par )
{
}



// ----- uiColorBarDialog -----
uiColorBarDialog::uiColorBarDialog( uiParent* p, const uiString& title )
    : uiDialog(p, uiDialog::Setup(title,mNoDlgTitle,
                                  mODHelpKey(mColorBarDialog) ).modal(false)
	       .oktext(uiStrings::sExit()).dlgtitle(uiString::emptyString())
	       .canceltext(uiString::emptyString()))
    , winClosing( this )
{
    coltabed_ = new uiVisColTabEd( *new uiColTabToolBar(this) );
    setDeleteOnClose( false );
}


bool uiColorBarDialog::closeOK()
{
    winClosing.trigger( this );
    return true;
}
