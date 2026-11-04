#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

/*!
 * \file uiod2dviewerbackend.h
 * \brief Common contract for the selected standalone 2D viewer backend.
 *
 * uiODViewer2DMgr forwards viewer requests through this interface. Application
 * policy selects the native or Advanced implementation independently of
 * backend registration.
 *
 * The interface supports viewer lookup, navigation, seismic updates,
 * interpretation objects and closure within a survey. It does not transfer
 * viewer state between backends or surveys.
 */

#include "uiodmainmod.h"
#include "attribsel.h"
#include "callback.h"
#include "emposid.h"
#include "flatview.h"
#include "multiid.h"
#include "typeset.h"
#include "trckeyzsampling.h"
#include "uistring.h"

class VolumeDataPack;
class Viewer2DPosDataSel;

/*! \brief Shared interface types and identity allocators for 2D viewers. */
namespace OD2DViewer
{

//! Outcome of a request to open a 2D viewer.
enum class OpenRequestStatus2DV
{
    Opened = 0,      //!< A viewer was opened or an existing viewer was reused.
    Unsupported = 1, //!< The backend does not support the requested operation.
    InvalidData = 2, //!< The input data is invalid or unavailable.
    NoBackend = 3,   //!< The backend is unavailable or dispatch is suspended.
    Cancelled = 4    //!< The operation was cancelled.
};

//! Type of a display object exposed by a 2D viewer backend.
enum class ObjectKind2DV
{
    Unknown = 0,   //!< Unclassified display object.
    Seismic = 1,   //!< Seismic data layer.
    Horizon3D = 2, //!< Three-dimensional horizon.
    Horizon2D = 3, //!< Two-dimensional horizon.
    Fault = 4,     //!< Fault surface.
    FaultSS2D = 5, //!< Two-dimensional fault stick set.
    FaultSS3D = 6, //!< Three-dimensional fault stick set.
    PickSet = 7,   //!< Point set or polygon.
    FaultSet = 8   //!< Collection of faults.
};

//! Outcome and viewer identity returned by an open operation.
mExpStruct(uiODMain) OpenResult2DV
{
    //! Outcome of the request.
    OpenRequestStatus2DV status_ {OpenRequestStatus2DV::InvalidData};
    //! Viewer ID on success.
    Viewer2DID id_ {Viewer2DID::udf()};
    //! User-facing explanation; successful operations leave this empty.
    uiString message_ {toUiString("Invalid data provided.")};

    //! Returns whether the operation succeeded with a valid viewer ID.
    bool isOK() const
    { return id_.isValid() && status_ == OpenRequestStatus2DV::Opened; }
};

//! Snapshot of a viewer's identity, position and synchronization data.
mExpStruct(uiODMain) Viewer2DInfo
{
    Viewer2DID id_ {Viewer2DID::udf()}; //!< Unique viewer ID.
    VisID sourceid_ {VisID::udf()}; //!< Associated source display, if any.
    SceneID sceneid_ {SceneID::udf()}; //!< Associated source scene, if any.
    TrcKeyZSampling sampling_ {false}; //!< Current spatial and Z sampling.
    Pos::GeomID geomid_ {Pos::GeomID::udf()}; //!< Survey geometry ID, if set.
    RandomLineID randomlineid_ {RandomLineID::udf()}; //!< Random line ID.
    Attrib::SelSpec selspec_; //!< Attribute selection used for synchronization.
    FlatView::DataDispPars::VD disppars_; //!< VD settings for that selection.
};

//! Identifies a displayed object and its underlying shared data.
mExpStruct(uiODMain) Object2DVInfo
{
    Vis2DID id_ {Vis2DID::udf()}; //!< Unique display object ID.
    Viewer2DID viewerid_ {Viewer2DID::udf()}; //!< Containing viewer ID.
    ObjectKind2DV kind_ {ObjectKind2DV::Unknown}; //!< Display object kind.
    EM::ObjectID emid_ {EM::ObjectID::udf()}; //!< EarthModel ID, if set.
    MultiID multiid_ {MultiID::udf()}; //!< Persistent data ID, when available.
};

//! Allocates a viewer ID shared by native and Advanced backends.
mGlobal(uiODMain) Viewer2DID allocateViewerID();

//! Allocates an object ID from the sequence shared with native View2D objects.
mGlobal(uiODMain) Vis2DID allocateObjectID();

} // namespace OD2DViewer

/*!
 * \brief Abstract interface for native and Advanced 2D viewer backends.
 *
 * uiODViewer2DMgr selects a backend and forwards viewer operations to it.
 * Registration supplies a backend; it does not change the selection policy.
 *
 * \par Ownership and lifetime
 * uiODViewer2DMgr owns the native adapter and native viewers. An Advanced
 * plugin owns its backend, which owns its windows. Registration does not
 * transfer ownership. The plugin must keep its backend alive until
 * uiODViewer2DMgr::unregisterBackend() completes.
 *
 * \par Implementations and notifications
 * OD supplies uiODNative2DViewerBackend. The Advanced plugin supplies its own
 * concrete implementation. Backends can publish changes through the notifier
 * members below. The manager forwards native viewer notifications directly
 * from the native implementation instead of through those members.
 */
mExpClass(uiODMain) uiOD2DViewerBackend : public CallBacker
{
public:
    //! Initializes the backend's notification sources.
    uiOD2DViewerBackend();
    //! Detaches the backend's notifier subscriptions.
    ~uiOD2DViewerBackend() override;

    //! Opens only the specified source-display attribute.
    virtual OD2DViewer::OpenResult2DV open(const VisID& source,int attribute,
					FlatView::Viewer::VwrDest dest) = 0;
    //! Opens the usable attribute stack of a whole source display.
    //! The default implementation returns an unsupported result.
    virtual OD2DViewer::OpenResult2DV openDisplayStack(const VisID& source,
					FlatView::Viewer::VwrDest dest);
    //! Opens seismic data with the supplied attribute and display settings.
    virtual OD2DViewer::OpenResult2DV open(const VolumeDataPack& data,
		const Attrib::SelSpec& selection,
		const FlatView::DataDispPars::VD& displaypars,
		FlatView::Viewer::VwrDest dest) = 0;
    /*! Opens a position/data selection with initial axis scales.
	\param selection Position and attribute to display.
	\param dest Requested seismic display mode.
	\param initialx1 First-axis positions per centimetre; undefined uses
	       the viewer's default scale.
	\param initialx2 Second-axis positions per centimetre; undefined uses
	       the viewer's default scale.
    */
    virtual OD2DViewer::OpenResult2DV open(Viewer2DPosDataSel& selection,
		FlatView::Viewer::VwrDest dest,float initialx1,
		float initialx2) = 0;

    //! Adds this backend's visible viewer IDs; excludes hidden extension hosts.
    virtual void getViewerIDs(TypeSet<Viewer2DID>& ids) const = 0;
    //! Retrieves viewer metadata; returns false if the viewer is unavailable.
    virtual bool getViewerInfo(Viewer2DID id,
				OD2DViewer::Viewer2DInfo& info) const = 0;
    //! Appends the display-object IDs belonging to the specified viewer.
    virtual void getViewerObjects(Viewer2DID id,
				  TypeSet<Vis2DID>& ids) const = 0;
    //! Retrieves display-object metadata; returns false if unavailable.
    virtual bool getObjectInfo(Vis2DID id,
				OD2DViewer::Object2DVInfo& info) const = 0;
    //! Changes the position and sampling of an existing viewer.
    virtual bool setPosition(Viewer2DID id,
			     const TrcKeyZSampling& sampling) = 0;
    //! Updates seismic data, attribute selection and display settings.
    virtual bool updateSeismic(Viewer2DID id,const VolumeDataPack& data,
		const Attrib::SelSpec& selection,
		const FlatView::DataDispPars::VD& displaypars,
		FlatView::Viewer::VwrDest dest) = 0;

    /*! Prepares viewers for closing, including save decisions where required.
	Preparation must not destroy controls or unregister windows.
	\param ids Viewers to prepare for closure.
	\param promptsAlreadyHandled Whether a global save phase has completed.
	\return false on cancellation or failed saving.
    */
    virtual bool prepareClose(const TypeSet<Viewer2DID>& ids,
			      bool promptsAlreadyHandled=false) = 0;
    //! Closes viewers that have been prepared for closing.
    virtual void commitClose(const TypeSet<Viewer2DID>& ids) = 0;
    //! Discards pending close preparation without closing viewers.
    virtual void cancelClosePreparation() = 0;

    //! Requests EarthModel displays, optionally adopting existing tracking.
    //! Returns false if the requested operation is unsupported.
    virtual bool addObjects(OD2DViewer::ObjectKind2DV kind,
		const TypeSet<EM::ObjectID>& ids,bool tracking=false) = 0;
    //! Removes EarthModel displays without deleting the shared object.
    virtual void removeObject(OD2DViewer::ObjectKind2DV kind,
			      const EM::ObjectID& id) = 0;
    //! Displays the point sets or polygons identified by persistent IDs.
    virtual void addPickSets(const TypeSet<MultiID>& ids) = 0;
    //! Removes a pick set's displays without deleting the shared pick set.
    virtual void removePickSet(const MultiID& id) = 0;

    //! Object-addition notification carrying the exposed display-object ID.
    CNotifier<uiOD2DViewerBackend,Vis2DID> objectAdded;
    //! Object-removal notification carrying the exposed display-object ID.
    CNotifier<uiOD2DViewerBackend,Vis2DID> objectToBeRemoved;
    //! Position-change notification carrying the viewer ID.
    CNotifier<uiOD2DViewerBackend,Viewer2DID> positionChanged;
    //! Viewer-closure notification carrying the viewer ID.
    CNotifier<uiOD2DViewerBackend,Viewer2DID> viewerClosed;
};
