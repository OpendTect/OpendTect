/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdatapointsetdisplaymgr.cc,v 1.1 2009-04-04 10:20:48 cvskris Exp $";

#include "uivisdatapointsetdisplaymgr.h"

#include "uivispartserv.h"
#include "visdata.h"
#include "vissurvscene.h"
#include "vispointsetdisplay.h"

uiVisDataPointSetDisplayMgr::uiVisDataPointSetDisplayMgr(uiVisPartServer& serv )
    : visserv_( serv )
{}


uiVisDataPointSetDisplayMgr::~uiVisDataPointSetDisplayMgr()
{
    deepErase( displayinfos_ );
}


void uiVisDataPointSetDisplayMgr::lock()
{
    lock_.lock();
    visserv_.getChildIds( -1, allsceneids_ );
}


void uiVisDataPointSetDisplayMgr::unLock()
{ lock_.unLock(); }


int uiVisDataPointSetDisplayMgr::getNrParents() const
{
    return allsceneids_.size();
}


const char* uiVisDataPointSetDisplayMgr::getParentName( int parentidx ) const
{
    RefMan<visBase::DataObject> scene =
	visserv_.getObject( allsceneids_[parentidx] );
    return scene ? scene->name() : 0;
}


int uiVisDataPointSetDisplayMgr::addDisplay(const TypeSet<int>& parents,
					    const DataPointSet& dps )
{
    if ( !parents.size() )
	return -1;

    DisplayInfo* displayinfo = new DisplayInfo;
    if ( !displayinfo )
	return -1;

    int id = 0;
    while ( ids_.indexOf(id)!=-1 ) id++;

    for ( int idx=0; idx<parents.size(); idx++ )
    {
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( allsceneids_[idx] );
	if ( !sceneptr )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
	    visSurvey::PointSetDisplay::create();
	if ( !display )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	scene->addObject( display );
	display->setDataPack( dps );

	displayinfo->sceneids_ += allsceneids_[idx];
	displayinfo->visids_ += display->id();
    }

    if ( !displayinfo->sceneids_.size() )
    {
	delete displayinfo;
	return -1;
    }

    displayinfos_ += displayinfo;
    ids_ += id;

    return id;
}


void uiVisDataPointSetDisplayMgr::updateDisplay( int id,
    const TypeSet<int>& parents, const DataPointSet& dps )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    DisplayInfo& displayinfo = *displayinfos_[idx];
    TypeSet<int> wantedscenes;
    for ( int idy=0; idy<parents.size(); idy++ )
	wantedscenes += allsceneids_[parents[idy]];

    TypeSet<int> scenestoremove = displayinfo.sceneids_;
    scenestoremove.createDifference( wantedscenes );

    TypeSet<int> scenestoadd = wantedscenes;
    scenestoadd.createDifference( displayinfo.sceneids_ );

    for ( int idy=0; idy<scenestoremove.size(); idy++ )
    {
	const int sceneid = scenestoremove[idx];
	const int index = displayinfo.sceneids_.indexOf( sceneid );
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( allsceneids_[idx] );
	if ( !sceneptr )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	scene->removeObject( scene->getFirstIdx(displayinfo.visids_[index]) );

	displayinfo.sceneids_.remove( index );
	displayinfo.visids_.remove( index );
    }

    for ( int idy=0; idy<scenestoadd.size(); idy++ )
    {
	const int sceneid = scenestoadd[idy];
	RefMan<visBase::DataObject> sceneptr =
		visserv_.getObject( sceneid );
	if ( !sceneptr )
	    continue;

	RefMan<visSurvey::PointSetDisplay> display =
	    visSurvey::PointSetDisplay::create();
	if ( !display )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	scene->addObject( display );

	displayinfo.sceneids_ += sceneid;
	displayinfo.visids_ += display->id();
    }

    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const int displayid = displayinfo.visids_[idy];
	RefMan<visBase::DataObject> displayptr = visserv_.getObject(displayid);
	if ( !displayptr )
	    continue;

	mDynamicCastGet( visSurvey::PointSetDisplay*, display,
			 displayptr.ptr() );
	if ( !display )
	    continue;

	display->setDataPack( dps );
    }
}


void uiVisDataPointSetDisplayMgr::removeDisplay( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return;

    DisplayInfo& displayinfo = *displayinfos_[idx];
    for ( int idy=0; idy<displayinfo.visids_.size(); idy++ )
    {
	const int sceneid = displayinfo.sceneids_[idy];
	RefMan<visBase::DataObject> sceneptr = visserv_.getObject( sceneid );
	if ( !sceneptr )
	    continue;

	mDynamicCastGet( visSurvey::Scene*, scene, sceneptr.ptr() );
	if ( !scene )
	    continue;

	scene->removeObject( scene->getFirstIdx(displayinfo.visids_[idy]) );
    }

    delete displayinfos_.remove( idx );
    ids_.remove( idx );
}
