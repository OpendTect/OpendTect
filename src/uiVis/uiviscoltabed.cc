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
    , isDisplayedChange(this)
{
    visBase::DM().removeallnotify.notify(
	    mCB(this,uiVisColTabEd,removeAllVisCB) );
}


#define mImplNotification( refop, notifyop ) \
    if ( survobj_ ) \
    { \
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


bool uiVisColTabEd::isDisplayed() const
{
    return coltabsel_.asParent()->isDisplayed();
}


void uiVisColTabEd::display( bool yn )
{
    if ( isDisplayed() == yn )
	return;
    coltabsel_.asParent()->display( yn );
    isDisplayedChange.trigger();
}


void uiVisColTabEd::setColTab( const ColTab::Sequence& seq,
			       ColTab::Mapper& mapper )
{
    coltabsel_.setSequence( seq );
    coltabsel_.setMapper( mapper );
    display( true );
}


//TODO the end result should be to get rid of this thing
void uiVisColTabEd::setColTab( visSurvey::SurveyObject* so, int channel )
{
    mImplNotification( unRef, remove );

    survobj_ = so;
    channel_ = channel;

    mImplNotification( ref, notifyIfNotNotified );

    if ( !so )
	display( false );
    else
	setColTab( so->getColTabSequence(channel_),
		const_cast<ColTab::Mapper&>(so->getColTabMapper(channel_)) );
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
