#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________

-*/

#include "wellcommon.h"
#include "sharedobject.h"
#include "dbkey.h"


namespace Well
{

class DisplayProperties;
class DisplayProperties3D;
class DisplayProperties2D;


/*!
\brief The holder of all data concerning a certain well.

  For Well::Data from database this object gets filled with either calls to
  Well::MGR().get to get everything or Well::Reader::get*() to only get some
  of it (only track, only logs, ...).

  Note that a well is not a POSC well in the sense that it describes the data
  for one well bore. Thus, a well has a single track. This may mean duplication
  when more well tracks share an upper part.
*/

mExpClass(Well) Data : public SharedObject
{
public:

				Data(const char* nm=0);
				~Data();
				mDeclMonitorableAssignment(Data);
				mDeclInstanceCreatedNotifierAccess(Data);

    const Info&			info() const		{ return info_; }
    Info&			info()			{ return info_; }

    const Track&		track() const		{ return track_; }
    Track&			track()			{ return track_; }

    const LogSet&		logs() const		{ return logs_; }
    LogSet&			logs()			{ return logs_; }

    const MarkerSet&		markers() const		{ return markers_; }
    MarkerSet&			markers()		{ return markers_; }

    const D2TModel&		d2TModel() const	{ return gtMdl(false); }
    D2TModel&			d2TModel()		{ return gtMdl(false); }
    const D2TModel&		checkShotModel() const	{ return gtMdl(true); }
    D2TModel&			checkShotModel()	{ return gtMdl(true); }

    DisplayProperties3D&	displayProperties3d()
				    { return disp3d_; }
    const DisplayProperties3D&	displayProperties3d() const
				    { return disp3d_; }
    DisplayProperties2D&	displayProperties2d()
				    { return disp2d_; }
    const DisplayProperties2D&	displayProperties2d() const
				    { return disp2d_; }
    DisplayProperties&	displayProperties( bool for2d=false );
    const DisplayProperties&	displayProperties( bool for2d=false ) const;


    void			setEmpty(); //!< removes everything

    void			levelToBeRemoved(CallBacker*);

    bool			haveTrack() const;
    bool			haveLogs() const;
    bool			haveMarkers() const;
    bool			haveD2TModel() const;
    bool			haveCheckShotModel() const;
    bool			displayPropertiesRead() const;

				// Following return null when mdl is empty:
    D2TModel*			d2TModelPtr()	    { return gtMdlPtr(false); }
    D2TModel*			checkShotModelPtr() { return gtMdlPtr(true); }
    const D2TModel*		d2TModelPtr() const { return gtMdlPtr(false); }
    const D2TModel*		checkShotModelPtr() const
						    { return gtMdlPtr(true); }

				// name comes from Info
    virtual BufferString	getName() const;
    virtual void		setName(const char*);
    virtual const OD::String&	name() const;

    virtual void		touch() const;
    virtual DirtyCountType	dirtyCount() const;

    DBKey			dbKey() const;
    mDeprecated DBKey		multiID() const		{ return dbKey(); }

    static bool			depthsInFeet();

protected:

    Info&		info_;
    Track&		track_;
    LogSet&		logs_;
    D2TModel&		d2tmodel_;
    D2TModel&		csmodel_;
    MarkerSet&		markers_;
    DisplayProperties2D&	disp2d_;
    DisplayProperties3D&	disp3d_;

    D2TModel&		gtMdl(bool) const;
    D2TModel*		gtMdlPtr(bool) const;
};


} // namespace Well
