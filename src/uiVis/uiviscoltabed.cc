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
	const ColTab::Sequence& seq = survobj_->getColTabSequence(channel_); \
	seq.objectChanged().notifyop( mCB(this,uiVisColTabEd,colSeqModifCB) ); \
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


void uiVisColTabEd::setNoColTab()
{
    coltabsel_.asParent()->display( false );
}


void uiVisColTabEd::setColTab( const ColTab::Sequence& seq,
			       ColTab::Mapper& mapper )
{
    coltabsel_.asParent()->display( true );
    coltabsel_.setSequence( seq );
    coltabsel_.setMapper( mapper );
}


//TODO the end result should be to get rid of this thing
void uiVisColTabEd::setColTab( visSurvey::SurveyObject* so, int channel )
{
    mImplNotification( unRef, remove );

    survobj_ = so;
    channel_ = channel;

    mImplNotification( ref, notifyIfNotNotified );

    if ( !so )
	setNoColTab();
    else
	setColTab( so->getColTabSequence(channel_),
		const_cast<ColTab::Mapper&>(so->getColTabMapper(channel_)) );
}


void uiVisColTabEd::colSeqModifCB( CallBacker* )
{
    //TODO this is a terrible hack to force remap. Add watching to vis and
    // remove this monstrous function ...

    if ( !survobj_ )
	return;

    coltabsel_.mappingChanged.trigger();
    survobj_->setColTabSequence( channel_, coltabsel_.sequence() );
}


void uiVisColTabEd::removeAllVisCB( CallBacker* )
{
    mImplNotification( unRef, remove );
    survobj_ = 0;
}


const ColTab::Sequence& uiVisColTabEd::getColTabSequence() const
{
    return coltabsel_.sequence();
}


const ColTab::Mapper& uiVisColTabEd::getColTabMapper() const
{
    return coltabsel_.mapper();
}


NotifierAccess& uiVisColTabEd::seqChange()
{
    return coltabsel_.seqChanged;
}


bool uiVisColTabEd::usePar( const IOPar& par )
{
    return true;
}

void uiVisColTabEd::fillPar( IOPar& par )
{
}
