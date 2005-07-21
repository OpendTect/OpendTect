#ifndef uiseispartserv_h
#define uiseispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiseispartserv.h,v 1.16 2005-07-21 11:30:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
#include "position.h"
#include "menuhandler.h"

class BufferString;
class BufferStringSet;
class CubeSampling;
class Line2DGeometry;
class MultiID;
class SeisTrcBuf;
class uiPopupMenu;


/*! \brief Seismic User Interface Part Server */

class uiSeisPartServer : public uiApplPartServer
{
public:
			uiSeisPartServer(uiApplService&);
    const char*		name() const			{ return "Seismics"; }

    enum ExternalType	{ SegY, CBVS };
    bool		importSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType or 3 == CBVS
    bool		exportSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType

    bool		select2DSeis(MultiID&,bool with_attr=false);
    bool		select2DLines(const MultiID&,BufferStringSet&);
    bool		get2DLineGeometry(const MultiID& mid,const char* linenm,
					  Line2DGeometry&) const;
    void		get2DLineSetName(const MultiID&,BufferString&) const;
    void		get2DStoredAttribs(const MultiID&,const char* linenm,
	    				   BufferStringSet&) const;
    bool		create2DOutput(const MultiID&,const char* linekey,
				       CubeSampling&,SeisTrcBuf&);
    MenuItem*		storedGathersSubMenu();
    BufferStringSet 	getStoredGathersList ();
    bool		handleGatherSubMenu (int mnuid, BinID bid);

    void		manageSeismics();

protected:

    MultiID		segyid;
    MenuItem		storedgathermenuitem;

    bool		ioSeis(int,bool);

};


/*!\mainpage Seismics User Interface

  Most of the tools in this module enable import and export of seismic
  data into/from the OpendTect data store. The merge and manage tools manipulate
  seismic data already in the data store.

  A standalone 'application' is the uiSeisMMProc class, which handles the
  distribution of processing on multiple machines.

*/

#endif
