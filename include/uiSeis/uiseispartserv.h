#ifndef uiseisman_h
#define uiseisman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiseispartserv.h,v 1.10 2004-09-17 15:51:47 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"

class BufferString;
class BufferStringSet;
class CubeSampling;
class MultiID;
class SeisTrcBuf;


/*! \brief Seismic User Interface Part Server */

class uiSeisPartServer : public uiApplPartServer
{
public:
			uiSeisPartServer(uiApplService&);
    const char*		name() const			{ return "Seismics"; }

    enum ExternalType	{ SegY, CBVS };
    bool		importSeis(ExternalType);
    bool		exportSeis();

    bool		select2DSeis(MultiID&);
    bool		select2DLines(const MultiID&,BufferStringSet&);
    void		get2DLineSetName(const MultiID&,BufferString&);
    void		get2DStoredAttribs(const MultiID&,const char*,
	    				   BufferStringSet&);
    bool		create2DOutput(const MultiID&,const char*,const char*,
				       CubeSampling&,SeisTrcBuf&);

    bool		mergeSeis();
    void		manageSeismics();

protected:

    MultiID		segyid;

    bool		ioSeis(ExternalType,bool);

};


/*!\mainpage Seismics User Interface

  Most of the tools in this module enable import and export of seismic
  data into/from the OpendTect data store. The merge and manage tools manipulate
  seismic data already in the data store.

  A standalone 'application' is the uiSeisMMProc class, which handles the
  distribution of processing on multiple machines.

*/

#endif
