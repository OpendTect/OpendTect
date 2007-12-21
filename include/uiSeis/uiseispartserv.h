#ifndef uiseispartserv_h
#define uiseispartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiseispartserv.h,v 1.26 2007-12-21 12:37:35 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
#include "menuhandler.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"


class BufferString;
class BufferStringSet;
class CubeSampling;
class MultiID;
class SeisTrcBuf;
class uiPopupMenu;
class uiFlatViewWin;
class uiSeisTrcBufViewer;

namespace PosInfo { class Line2DData; }

/*! \brief Seismic User Interface Part Server */

class uiSeisPartServer : public uiApplPartServer
{
public:
			uiSeisPartServer(uiApplService&);
			//~uiSeisPartServer();
    const char*		name() const			{ return "Seismics"; }

    enum ExternalType	{ SegY, CBVS };
    bool		importSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType or 3 == CBVS
    bool		exportSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType

    bool		select2DSeis(MultiID&,bool with_attr=false);
    bool		select2DLines(const MultiID&,BufferStringSet&);
    bool		get2DLineGeometry(const MultiID& mid,const char* linenm,
	    				  PosInfo::Line2DData&) const;
    void		get2DLineSetName(const MultiID&,BufferString&) const;
    void		get2DLineInfo(BufferStringSet&,TypeSet<MultiID>&,
	    			      TypeSet<BufferStringSet>&);
    void		get2DStoredAttribs(const MultiID&,const char* linenm,
	    				   BufferStringSet&) const;
    bool		create2DOutput(const MultiID&,const char* linekey,
				       CubeSampling&,SeisTrcBuf&);
    MenuItem*		storedGathersSubMenu(bool createnew);
    BufferStringSet 	getStoredGathersList() const;
    bool		handleGatherSubMenu(int mnuid,const BinID&);

    void		manageSeismics();
    void		importWavelets();
    void		manageWavelets();

protected:

    MultiID		segyid;
    MenuItem		storedgathermenuitem;

    bool		ioSeis(int,bool);
    void		setTrcBufViewTitle(const char*,const BinID&);

    uiSeisTrcBufViewer*	trcbufview_;
    void		viewerClosed(CallBacker*);

};


/*!\mainpage Seismics User Interface

  Most of the tools in this module enable import and export of seismic
  data into/from the OpendTect data store. The merge and manage tools manipulate
  seismic data already in the data store.

  A standalone 'application' is the uiSeisMMProc class, which handles the
  distribution of processing on multiple machines.

*/

#endif
