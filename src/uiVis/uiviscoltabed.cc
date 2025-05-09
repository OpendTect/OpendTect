/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviscoltabed.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "od_helpids.h"
#include "uicolortable.h"
#include "visdataman.h"
#include "visscenecoltab.h"
#include "vissurvobj.h"

const char* uiVisColTabEd::sKeyColorSeq()	{ return "ColorSeq Name"; }
const char* uiVisColTabEd::sKeyScaleFactor()	{ return "Scale Factor"; }
const char* uiVisColTabEd::sKeyClipRate()	{ return "Cliprate"; }
const char* uiVisColTabEd::sKeyAutoScale()	{ return "Auto scale"; }
const char* uiVisColTabEd::sKeySymmetry()	{ return "Symmetry"; }
const char* uiVisColTabEd::sKeySymMidval()	{ return "Symmetry Midvalue"; }

uiVisColTabEd::uiVisColTabEd( uiColorTable& ct )
    : uicoltab_(ct)
{
    mAttachCB( visBase::DM().removeallnotify, uiVisColTabEd::removeAllVisCB );
}


uiVisColTabEd::~uiVisColTabEd()
{
    detachAllNotifiers();
}


void uiVisColTabEd::setColTab( const ColTab::Sequence* seq, bool editseq,
			       const ColTab::MapperSetup* setup,
			       bool enabletrans )
{
    uicoltab_.setSequence( seq, editseq, true );
    uicoltab_.setMapperSetup( setup, true );
    uicoltab_.enableTransparencyEdit( enabletrans );
}


ConstRefMan<visBase::DataObject> uiVisColTabEd::getDataObj() const
{
    return getNonConst(*this).getDataObj();
}


RefMan<visBase::DataObject> uiVisColTabEd::getDataObj()
{
    return dataobj_.get();
}


void uiVisColTabEd::setColTab( visSurvey::SurveyObject* so, int channel,
			       int version )
{
    if ( dataobj_ )
	removeAllVisCB( nullptr );

    dataobj_ = dCast(visBase::DataObject*,so);
    channel_ = channel;
    version_ = version;

    if ( so )
    {
	const ColTab::MapperSetup* ctmsu =
			    so->getColTabMapperSetup( channel_, version_ );
	if ( ctmsu )
	    mAttachCB( ctmsu->rangeChange, uiVisColTabEd::mapperChangeCB );
    }

    uicoltab_.setSequence( so ? so->getColTabSequence(channel_) : nullptr,
			   so && so->canSetColTabSequence(), true );
    uicoltab_.setMapperSetup(
	    so ? so->getColTabMapperSetup(channel_,version_) : nullptr, true );
    uicoltab_.enableTransparencyEdit(
	    so && so->canHandleColTabSeqTrans(channel_) );
}


void uiVisColTabEd::mapperChangeCB( CallBacker* )
{
    if ( !dataobj_ )
	return;

    RefMan<visBase::DataObject> dataobj = dataobj_.get();
    mDynamicCastGet(visSurvey::SurveyObject*,survobj,dataobj.ptr());
    if ( !survobj )
	return;

    const ColTab::MapperSetup* ms  =
			survobj->getColTabMapperSetup( channel_, version_ );

    uicoltab_.setMapperSetup( ms, true );
    if ( survobj->getScene() && ms )
	survobj->getScene()->getSceneColTab()->setColTabMapperSetup( *ms );
}


void uiVisColTabEd::removeAllVisCB( CallBacker* )
{
    if ( dataobj_ )
    {
	ConstRefMan<visBase::DataObject> dataobj = dataobj_.get();
	mDynamicCastGet(const visSurvey::SurveyObject*,survobj,dataobj.ptr());
	if ( survobj )
	{
	    const ColTab::MapperSetup* ctmsu =
		    survobj->getColTabMapperSetup( channel_, version_ );
	    if ( ctmsu )
		mDetachCB( ctmsu->rangeChange, uiVisColTabEd::mapperChangeCB );
	}
    }

    dataobj_ = nullptr;
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
    : uiDialog(p,Setup(title,mODHelpKey(mColorBarDialog)).modal(false)
		   .oktext(uiStrings::sExit()).dlgtitle(uiString::emptyString())
		   .canceltext(uiString::emptyString()))
    , winClosing( this )
{
    ColTab::Sequence ctseq( "" );
    auto* grp = new uiColorTableGroup( this, ctseq, OD::Vertical, false );
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
