#ifndef uiseispartserv_h
#define uiseispartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiapplserv.h"
#include "multiid.h"

class BufferStringSet;
class CubeSampling;
class IOPar;
class SeisTrcBuf;
class uiFlatViewWin;

namespace PosInfo { class Line2DData; }
namespace Geometry { class RandomLine; }

/*!
\ingroup uiSeis
\brief Seismic User Interface Part Server
*/

mExpClass(uiSeis) uiSeisPartServer : public uiApplPartServer
{
public:
			uiSeisPartServer(uiApplService&);
			//~uiSeisPartServer();
    const char*		name() const			{ return "Seismics"; }

    bool		importSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType or 3 == CBVS
    bool		exportSeis(int opt);
    			//!< opt == (int)uiSeisSEGYEntry::DataType

    bool		select2DSeis(MultiID&);
    bool		select2DLines(TypeSet<Pos::GeomID>&,int& action);
    static void		get2DDataSetName(const MultiID&,BufferString&);
    static void		get2DLineInfo(TypeSet<Pos::GeomID>&,
	    			      BufferStringSet&);
    static void		get2DStoredAttribs(const char* linenm,
				       BufferStringSet& attribs,int steerpol=2);
    void		get2DZdomainAttribs(const char* linenm,
	    				    const char* zdomainstr,
					    BufferStringSet& attribs);
    bool		create2DOutput(const MultiID&,const char* linekey,
				       CubeSampling&,SeisTrcBuf&);
    void 		getStoredGathersList(bool for3d,BufferStringSet&) const;
    void		storeRlnAs2DLine(const Geometry::RandomLine&) const;

    void		processTime2Depth() const;
    void		resortSEGY() const;
    void		processVelConv() const;
    void		createMultiCubeDataStore() const;

    void		manageSeismics(int opt);
    void		managePreLoad();
    void		importWavelets();
    void		exportWavelets();
    void		manageWavelets();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		ioSeis(int,bool);
};

#endif

