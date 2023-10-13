/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviscoltabed.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "visscenecoltab.h"
#include "visdataman.h"
#include "visdata.h"
#include "vissurvobj.h"
#include "uicolortable.h"
#include "od_helpids.h"


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
    mAttachCB( visBase::DM().removeallnotify, uiVisColTabEd::removeAllVisCB );
}


uiVisColTabEd::~uiVisColTabEd()
{
    detachAllNotifiers();
    mDynamicCastGet(visBase::DataObject*,dataobj,survobj_)
    if ( dataobj )
	dataobj->unRef();
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
    mDynamicCastGet(visBase::DataObject*,dataobj,survobj_)
    if ( dataobj )
    {
	dataobj->unRef();
	const ColTab::MapperSetup* ctmsu =
		survobj_->getColTabMapperSetup( channel_, version_ );
	if ( ctmsu )
	    mDetachCB( ctmsu->rangeChange, uiVisColTabEd::mapperChangeCB );
    }

    survobj_ = so;
    channel_ = channel;
    version_ = version;

    mDynamicCast(visBase::DataObject*,dataobj,survobj_)
    if ( dataobj )
    {
	dataobj->ref();
	const ColTab::MapperSetup* ctmsu =
		survobj_->getColTabMapperSetup( channel_, version_ );
	if ( ctmsu )
	    mAttachCB( ctmsu->rangeChange, uiVisColTabEd::mapperChangeCB );
    }

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
    if ( survobj_->getScene() && ms )
	survobj_->getScene()->getSceneColTab()->setColTabMapperSetup( *ms );
}


void uiVisColTabEd::removeAllVisCB( CallBacker* )
{
    mDynamicCastGet(visBase::DataObject*,dataobj,survobj_)
    if ( dataobj )
    {
	dataobj->unRef();
	const ColTab::MapperSetup* ctmsu =
		survobj_->getColTabMapperSetup( channel_, version_ );
	if ( ctmsu )
	    mDetachCB( ctmsu->rangeChange, uiVisColTabEd::mapperChangeCB );
    }

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
uiColorBarDialog::uiColorBarDialog( uiParent* p, const uiString& title )
    : uiDialog(p, uiDialog::Setup(title,mNoDlgTitle,
                                  mODHelpKey(mColorBarDialog) ).modal(false)
	       .oktext(uiStrings::sExit()).dlgtitle(uiString::emptyString())
	       .canceltext(uiString::emptyString()))
    , winClosing( this )
{
    ColTab::Sequence ctseq( "" );
    uiColorTableGroup* grp =
	new uiColorTableGroup( this, ctseq, OD::Vertical, false );
    coltabed_ = new uiVisColTabEd( *grp );
    setDeleteOnClose( false );
}


uiColorBarDialog::~uiColorBarDialog()
{}


bool uiColorBarDialog::closeOK()
{
    winClosing.trigger( this );
    return true;
}
