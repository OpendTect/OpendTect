#ifndef emsurfaceio_h
#define emsurfaceio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceio.h,v 1.8 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "executor.h"
#include "ranges.h"
#include "rowcol.h"
#include "bufstringset.h"

class StreamConn;
class IOObj;
template <class T> class DataInterpreter;

namespace EM
{
class Surface;
class SurfPosCalc;
class RowColConverter;
class dgbSurfDataReader;


/*!
Surface Reader.

1. Construct (no changes are made to the surface in mem)
2. Select what you want to read
3. Do NextStep

*/

class dgbSurfaceReader : public ExecutorGroup
{
public:
			dgbSurfaceReader( const IOObj& ioobj,
					  const char* filetype,
					  EM::Surface* surface );
			/*!< Sets up object and reads header.
			\param ioobj	The IOObj with info about where to
					read data.
			\param filetype	The filetype that should be in
					the file header.
			\param surface	The surface that should be filled with
					data. Note that a null pointer can
					be given if the obj only is used
					to get data from the header
			*/
			~dgbSurfaceReader();
			/*!< Closes the stream */

    void		setSurface( EM::Surface* s )	{ surface = s; }
    bool		isOK() const;

    int			nrPatches() const;
    EM::PatchID		patchID( int ) const;
    const char*		patchName( int ) const;
    void		selPatches(const TypeSet<EM::PatchID>&);
    			/*!< The given patchIDs will be loaded. If this
			     function is not called, all avaliable patches
			     will be loaded.
			*/

    const char*		dbInfo() const;
    int			nrAuxVals() const;
    const char*		auxDataName(int) const;
    void		selAuxData(const TypeSet<int>&);
    			/*!< The specified data will be loaded. If this
			     function is not called, all avaliable auxdata
			     will be loaded.
			*/

    const StepInterval<int>&	rowInterval() const;
    const StepInterval<int>&	colInterval() const;
    void			setRowInterval( const StepInterval<int>& );
    void			setColInterval( const StepInterval<int>& );

    const IOPar*		pars() const;

    void			setSurfPosCalc( SurfPosCalc* );
    				/*!<
				    If given, the reader will only read
				    the z coordinate from the file anc compute
				    the x and y from the (nonconverted) rowcol
				    with the given object.
				    \note Passed object Becomes mine.
				 */

    void			setRowColConverter( RowColConverter* );
    				/*!<
				    If given, the reader will convert the file's
				    rowcol with the given object before adding
				    nodes to the surface.
				    in memory.
				    \note Passed object Becomes mine.
				 */

    void			setReadFillType(bool yn);


    virtual int			nrDone() const;
    virtual const char*		nrDoneText() const;
    virtual int			totalNr() const;

    virtual int			nextStep();

    virtual const char*		message() const;

    static const char*		nrpatchstr;
    static const char*		patchidstr;
    static const char*		patchnamestr;
    static const char*		rowrangestr;
    static const char*		colrangestr;
    static const char*		intdatacharstr;
    static const char*		floatdatacharstr;
    static const char*		dbinfostr;
    static const char*		versionstr;

    static const char*		badconnstr;
    static const char*		parseerrorstr;

protected:

    double		readFloat(istream&) const;
    int                 readInt(istream&) const;
    StreamConn*		conn;

    BufferStringSet		patchnames;
    TypeSet<EM::PatchID>	patchids;
    TypeSet<EM::PatchID>	patchsel;

    BufferStringSet		auxdatanames;
    ObjectSet<EM::dgbSurfDataReader> auxdataexecs;
    TypeSet<int>		auxdatasel;

    const IOPar*		par;

    BufferString		msg;
    bool			error;
    int				nrdone;

    int				patchindex;
    int				oldpatchindex;
    int				firstrow;
    int				nrrows;
    int				rowindex;

    DataInterpreter<int>*	intinterpreter;
    DataInterpreter<double>*	floatinterpreter;
    EM::Surface*		surface;

    StepInterval<int>		rowrange;
    StepInterval<int>		colrange;

    StepInterval<int>*		readrowrange;
    StepInterval<int>*		readcolrange;

    SurfPosCalc*		surfposcalc;
    RowColConverter*		rcconv;
    bool			readfilltype;

    BufferString		dbinfo;

};


/*!
Surface Writer.

1. Construct (no changes are made to filesystem) 
2. Select what you want to write
3. Do NextStep

*/

class dgbSurfaceWriter : public ExecutorGroup
{
public:
			dgbSurfaceWriter( const IOObj* ioobj,
					  const char* filetype,
					  const EM::Surface& surface,
			       		  bool binary );
			/*!< Sets up object, but does not touch file (that's 
			     done in nextStep() )
			\param ioobj	The IOObj with info about where to
					write data.
			\param filetype	The filetype that should be in
					the file header.
			\param surface	The surface that should be written with
					data. 
			*/
			~dgbSurfaceWriter();
			/*!< Closes the stream */

    int			nrPatches() const;
    EM::PatchID		patchID( int ) const;
    const char*		patchName( int ) const;
    void		selPatches(const TypeSet<EM::PatchID>&);
    			/*!< The given patchIDs will be written. If this
			     function is not called, all avaliable patches
			     will be written.
			*/

    int			nrAuxVals() const;
    const char*		auxDataName(int) const;
    const char*		auxDataInfo(int) const;
    void		selAuxData(const TypeSet<int>&);
    			/*!< The specified data will be written. If this
			     function is not called, all avaliable auxdata
			     will be written.
			*/

    const StepInterval<int>&	rowInterval() const;
    const StepInterval<int>&	colInterval() const;
    void			setRowInterval( const StepInterval<int>& );
    void			setColInterval( const StepInterval<int>& );

    bool			writeOnlyZ() const;
    void			setWriteOnlyZ(bool yn);
    IOPar*			pars();

    virtual int			nrDone() const;
    virtual const char*		nrDoneText() const;
    virtual int			totalNr() const;

    virtual int			nextStep();

    virtual const char*		message() const;

protected:

    bool			writeFloat(ostream&,float,
	    				   const char* eol) const;
    bool                 	writeInt(ostream&,int,const char* eol) const;
    StreamConn*			conn;
    const IOObj*		ioobj;

    TypeSet<EM::PatchID>	patchsel;
    TypeSet<int>		auxdatasel;
    BufferString		dbinfo;

    IOPar&			par;

    BufferString		msg;
    int				nrdone;

    int				patchindex;
    int				oldpatchindex;
    int				firstrow;
    int				nrrows;
    int				rowindex;

    const EM::Surface&		surface;

    StepInterval<int>		rowrange;
    StepInterval<int>		colrange;

    StepInterval<int>*		writerowrange;
    StepInterval<int>*		writecolrange;
    bool			writeonlyz;
    bool			binary;
    BufferString		filetype;
};


/*! Helper class for the reader that is handy when only z values are written
and the x & y should be computed from the RowCol */

class SurfPosCalc
{
public:
    virtual			~SurfPosCalc() {}
    virtual Coord		getPos(const RowCol& ) const = 0;
};


/* Helper class for the writer that is used with old file-formats. The class
should compute a RowCol that is used in EM from the RowCol stored on file.
*/


class RowColConverter
{
public:
    virtual			~RowColConverter() {}
    virtual RowCol		get(const RowCol& ) const = 0;
};



};

#endif

