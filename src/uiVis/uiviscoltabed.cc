/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiviscoltabed.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "visscenecoltab.h"
#include "visdataman.h"
#include "visdata.h"
#include "vissurvobj.h"
#include "uicolortable.h"


const char* uiVisColTabEd::sKeyColorSeq()	{ return "ColorSeq Name"; }
const char* uiVisColTabEd::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* uiVisColTabEd::sKeyClipRate()	{ return "Cliprate"; }
const char* uiVisColTabEd::sKeyAutoScale()	{ return "Auto scale"; }
const char* uiVisColTabEd::sKeySymmetry()	{ return "Symmetry"; }
const char* uiVisColTabEd::sKeySymMidval()	{ return "Symmetry Midvalue"; }

uiVisColTabEd::uiVisColTabEd( uiColorTable& ct )
    : survobj_( 0 )
    , uicoltab_(ct)
{
    visBase::DM().removeallnotify.notify(
	    mCB(this,uiVisColTabEd,removeAllVisCB) );
}


#define mImplNotification( refop, notify) \
    if ( survobj_ ) \
    { \
	if ( survobj_->getColTabMapperSetup(channel_,version_) ) \
	    survobj_->getColTabMapperSetup( channel_, version_ )-> \
		rangeChange.notify( mCB(this,uiVisColTabEd,mapperChangeCB) ); \
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


void uiVisColTabEd::setColTab( const ColTab::Sequence* seq, bool editseq,
			       const ColTab::MapperSetup* setup,
       			       bool enabletrans )
{
    uicoltab_.setSequence( seq, editseq, true );
    uicoltab_.setMapperSetup( setup, true );
    uicoltab_.enableTransparencyEdit( enabletrans );
}


void uiVisColTabEd::setColTab( visSurvey::SurveyObject* so, int channel,
			       int version )
{
    mImplNotification( unRef, remove );

    survobj_ = so;
    channel_ = channel;
    version_ = version;

    mImplNotification( ref, notify );

    uicoltab_.setSequence( so ? so->getColTabSequence(channel_) : 0,
			    so && so->canSetColTabSequence(), true );
    uicoltab_.setMapperSetup(
	    so ? so->getColTabMapperSetup(channel_,version_) : 0, true );
    uicoltab_.enableTransparencyEdit(
	    so && so->canHandleColTabSeqTrans(channel_) );
}


void uiVisColTabEd::mapperChangeCB( CallBacker* )
{
    const ColTab::MapperSetup* ms  = 
			survobj_->getColTabMapperSetup( channel_, version_ );

    uicoltab_.setMapperSetup( ms, true );
    if ( survobj_->getScene() )
	survobj_->getScene()->getSceneColTab()->setColTabMapperSetup( *ms );
}


void uiVisColTabEd::removeAllVisCB( CallBacker* )
{
    mImplNotification( unRef, remove);
    survobj_ = 0;
}


const ColTab::Sequence& uiVisColTabEd::getColTabSequence() const
{ return uicoltab_.colTabSeq(); }

const ColTab::MapperSetup& uiVisColTabEd::getColTabMapperSetup() const
{ return uicoltab_.colTabMapperSetup(); }

NotifierAccess& uiVisColTabEd::seqChange()
{ return uicoltab_.seqChanged; }

NotifierAccess& uiVisColTabEd::mapperChange()
{ return uicoltab_.scaleChanged; }

void uiVisColTabEd::setHistogram( const TypeSet<float>* hist )
{ uicoltab_.setHistogram( hist ); }

bool uiVisColTabEd::usePar( const IOPar& par )
{ return true; }

void uiVisColTabEd::fillPar( IOPar& par )
{}



// ----- uiColorBarDialog -----
uiColorBarDialog::uiColorBarDialog( uiParent* p, const char* title )
    : uiDialog(p, uiDialog::Setup(title,0,mNoHelpID).modal(false)
	       .oktext("Exit").dlgtitle("").canceltext(""))
    , winClosing( this )
{
    ColTab::Sequence ctseq( "" );
    uiColorTableGroup* grp = new uiColorTableGroup( this, ctseq, true, false );
    coltabed_ = new uiVisColTabEd( *grp );
    setDeleteOnClose( false );
}


bool uiColorBarDialog::closeOK()
{
    winClosing.trigger( this );
    return true;
}
