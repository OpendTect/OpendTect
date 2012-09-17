#ifndef uiseispartserv_h
#define uiseispartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiseispartserv.h,v 1.47 2011/03/21 16:16:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
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

namespace PosInfo { class Line2DData; }
namespace Geometry { class RandomLine; }

/*! \brief Seismic User Interface Part Server */

mClass uiSeisPartServer : public uiApplPartServer
{
public:
			uiSeisPartServer(uiApplService&);
			//~uiSeisPartServer();
    const char*		name() const			{ return "Seismics"; }

    bool		importSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType or 3 == CBVS
    bool		exportSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType

    bool		select2DSeis(MultiID&,bool with_attr=false);
    bool		select2DLines(const MultiID&,BufferStringSet&);
    static bool		get2DLineGeometry(const MultiID& mid,const char* linenm,
	    				  PosInfo::Line2DData&);
    static void		get2DLineSetName(const MultiID&,BufferString&);
    static void		get2DLineInfo(BufferStringSet&,TypeSet<MultiID>&,
	    			      TypeSet<BufferStringSet>&);
    static void		get2DStoredAttribs(const MultiID&,
				    const char* linenm,BufferStringSet& attribs,
				    int steerpol=2);
    void		get2DZdomainAttribs(const MultiID&, const char* linenm,
	    				    const char* zdomainstr,
					    BufferStringSet& attribs);
    bool		create2DOutput(const MultiID&,const char* linekey,
				       CubeSampling&,SeisTrcBuf&);
    void 		getStoredGathersList(bool for3d,BufferStringSet&) const;
    void		storeRlnAs2DLine(const Geometry::RandomLine&) const;

    void		processTime2Depth() const;
    void		resortSEGY() const;
    void		processVelConv() const;

    void		manageSeismics(bool);
    void		managePreLoad();
    void		importWavelets();
    void		exportWavelets();
    void		manageWavelets();

protected:

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
