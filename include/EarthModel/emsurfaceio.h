#ifndef emsurfaceio_h
#define emsurfaceio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfaceio.h,v 1.41 2012-08-02 09:20:43 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "bufstringset.h"
#include "emposid.h"
#include "executor.h"
#include "position.h"
#include "ranges.h"
#include "rowcol.h"

class StreamConn;
class IOObj;
template <class T> class DataInterpreter;
template <class T> class Array3D;

namespace EM
{
class Surface;
class dgbSurfDataReader;
class RowColSurfaceGeometry;


/*!
Surface Reader.

1. Construct (no changes are made to the surface in mem)
2. Select what you want to read
3. Do NextStep

*/

mClass dgbSurfaceReader : public ExecutorGroup
{
public:
			dgbSurfaceReader(const IOObj& ioobj,
					 const char* filetype);
			dgbSurfaceReader(const char* fullexp, const char* name,
					 const char* filetype);
			~dgbSurfaceReader();

    void		setOutput(EM::Surface&);
    void		setOutput(Array3D<float>&);
    			/*!<\note only z-values will be put in array
			    \note sizes of 1st and second dim must fit
			   	  row/col selection.
			    \note size in third dim must fit number of
			          sections given by selSections. */

    int			version() const		{ return version_; }

    bool		isOK() const;
    void		setGeometry();

    int			nrSections() const;
    EM::SectionID	sectionID(int) const;
    BufferString	sectionName(int) const;
    void		selSections(const TypeSet<EM::SectionID>&);
    			/*!< The given sectionIDs will be loaded. If
			     this function is not called, all avaliable
			     sections will be loaded. */

    const char*		dbInfo() const;
    int			nrAuxVals() const;
    const char*		auxDataName(int) const;
    float		auxDataShift(int) const;
    void		selAuxData(const TypeSet<int>&);
    			/*!< The specified data will be loaded. If this
			     function is not called, all avaliable
			     auxdata will be loaded. */

    const StepInterval<int>&	rowInterval() const;
    const StepInterval<int>&	colInterval() const;
    const Interval<float>&	zInterval() const;

    void		setRowInterval(const StepInterval<int>&);
    void		setColInterval(const StepInterval<int>&);
    void		setReadOnlyZ(bool yn=true);
    void 		setLineNames(const BufferStringSet&);
    void		setLinesTrcRngs(const TypeSet<StepInterval<int> >&);

    int			nrLines() const;
    BufferString	lineName(int) const;
    BufferString	lineSet(int) const;
    StepInterval<int>	lineTrcRanges( int idx ) const;
    int			stratLevelID() const;
    const IOPar*	pars() const;

    virtual od_int64	nrDone() const;
    virtual const char*	nrDoneText() const;
    virtual od_int64	totalNr() const;

    virtual int		nextStep();

    virtual const char*	message() const;

    static const char*	sKeyNrSections();
    static const char*	sKeyNrSectionsV1();
    static BufferString	sSectionIDKey(int idx);
    static BufferString	sSectionNameKey(int idx);
    static BufferString	sColStepKey(int rowidx);
    static const char*	sKeyDepthOnly();
    static const char*	sKeyRowRange();
    static const char*	sKeyColRange();
    static const char*	sKeyZRange();
    static const char*	sKeyInt16DataChar();
    static const char*	sKeyInt32DataChar();
    static const char*	sKeyInt64DataChar();
    static const char*	sKeyFloatDataChar();
    static const char*	sKeyDBInfo();
    static const char*	sKeyVersion();
    static const char*	sKeyLineSets();
    static const char*	sKeyLineIDs();
    static const char*	sKeyTraceRange();

    static const char*	sMsgParseError();
    static const char*	sMsgReadError();
    static const char*	sKeyUndefLineSet();
    static const char*	sKeyUndefLine();

protected:

    bool		isBinary() const;

    double		readDouble(std::istream&) const;
    int			readInt16(std::istream&) const;
    int			readInt32(std::istream&) const;
    od_int64		readInt64(std::istream&) const;
    int			int64Size() const;
    void		createAuxDataReader();
    bool		readParData(std::istream&,const IOPar&,const char*);
    void		mergeExternalPar(const char*);
    int			scanFor2DGeom(TypeSet< StepInterval<int> >&);
    bool		readHeaders(const char*);
    bool		readRowOffsets(std::istream&);
    RowCol		getFileStep() const;
    int			prepareNewSection(std::istream&);
    bool		shouldSkipRow(int row) const;
    int			skipRow(std::istream&);
    bool		prepareRowRead(std::istream&);
    int			currentRow() const;
    void		goToNextRow();
    void		createSection( const SectionID& );

    StreamConn*		conn_;

    BufferStringSet	sectionnames_;
    BufferStringSet	linenames_;
    BufferStringSet	linesets_;
    TypeSet<EM::SectionID> sectionids_;
    TypeSet<EM::SectionID> sectionsel_;
    bool		fullyread_;

    BufferStringSet	auxdatanames_;
    TypeSet<float>	auxdatashifts_;
    ObjectSet<EM::dgbSurfDataReader> auxdataexecs_;
    TypeSet<int>	auxdatasel_;

    const IOPar*	par_;

    BufferString	msg_;
    bool		error_;
    int			nrdone_;

    bool		isinited_;
    bool		setsurfacepar_;

    int			sectionsread_;
    int			sectionindex_;
    int			oldsectionindex_;
    int			firstrow_;
    int			nrrows_;
    int			rowindex_;

    DataInterpreter<int>* int32interpreter_;
    DataInterpreter<double>* floatinterpreter_;
    EM::Surface*	surface_;
    Array3D<float>*	cube_;

    StepInterval<int>	rowrange_;
    StepInterval<int>	colrange_;
    Interval<float>	zrange_;

    StepInterval<int>*	readrowrange_;
    StepInterval<int>*	readcolrange_;

    bool		readonlyz_;
    BufferString	dbinfo_;
    int			version_;

    bool		readVersion2Row(std::istream&,int,int);

//Version 3 stuff 
    bool		readVersion3Row(std::istream&,int,int,int,
	    					int noofcoltoskip=0);
    DataInterpreter<int>* int16interpreter_;
    DataInterpreter<od_int64>* int64interpreter_;
    TypeSet<od_int64>	rowoffsets_;
    TypeSet<od_int64>	sectionoffsets_;

//Version 1 stuff
    bool		readVersion1Row(std::istream&,int,int);
    RowCol		convertRowCol(int,int) const;
    bool		parseVersion1(const IOPar&);
    static const char*	sKeyTransformX();
    static const char*	sKeyTransformY();

    double		conv11, conv12, conv13, conv21, conv22, conv23;

// for loading horizon based on Lines trace range
   const BufferStringSet* readlinenames_;
   const TypeSet< StepInterval<int> >* linestrcrgs_; 
   static const char*	linenamesstr_;  
   void			init(const char* fulluserexp,const char* name);
};


/*!
Surface Writer.

1. Construct (no changes are made to filesystem) 
2. Select what you want to write
3. Do NextStep

*/

mClass dgbSurfaceWriter : public ExecutorGroup
{
public:
			dgbSurfaceWriter( const IOObj* ioobj,
					  const char* filetype,
					  const EM::Surface& surface,
			       		  bool binary );
			dgbSurfaceWriter( const char* fulluserexpr,
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

    int			nrSections() const;
    EM::SectionID	sectionID( int ) const;
    const char*		sectionName( int ) const;
    void		selSections(const TypeSet<EM::SectionID>&,
	    			    bool add=false);
    			/*!< The given sectionIDs will be written. If this
			     function is not called, all avaliable sections
			     will be written.
			*/
    void		setShift(float);
    			//!<Shift is added to z values before writing

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

    virtual od_int64		nrDone() const;
    virtual const char*		nrDoneText() const;
    virtual od_int64		totalNr() const;

    virtual int			nextStep();

    virtual const char*		message() const;

protected:
    bool			writeNewSection(std::ostream&);
    bool			writeRow(std::ostream&);

    bool			writeDouble(std::ostream&,double,
	    				   const char*) const;
    bool                 	writeInt16(std::ostream&,unsigned short,
	    				   const char*) const;
    bool                 	writeInt32(std::ostream&,od_int32,
	    				   const char*) const;
    bool                 	writeInt64(std::ostream&,od_int64,
	    				   const char*) const;

    void			finishWriting();
    bool			writingfinished_;

    StreamConn*			conn_;

    TypeSet<EM::SectionID>	sectionsel_;
    TypeSet<od_int64>		sectionoffsets_;
    od_int64			nrsectionsoffsetoffset_;
    TypeSet<int>		auxdatasel_;
    BufferString		dbinfo_;

    IOPar*			par_;

    BufferString		msg_;
    BufferString		fulluserexpr_;
    MultiID			objectmid_;
    int				nrdone_;

    int				sectionindex_;
    int				oldsectionindex_;
    od_int64			rowoffsettableoffset_;
    TypeSet<od_int64>		rowoffsettable_;
    int				firstrow_;
    int				nrrows_;
    int				rowindex_;

    const EM::Surface&		surface_;
    const EM::RowColSurfaceGeometry* geometry_;

    StepInterval<int>		rowrange_;
    StepInterval<int>		colrange_;
    Interval<float>		zrange_;

    StepInterval<int>*		writerowrange_;
    StepInterval<int>*		writecolrange_;
    Interval<int>		writtenrowrange_;
    Interval<int>		writtencolrange_;
    bool			writeonlyz_;
    bool			binary_;
    float			shift_;
    BufferString		filetype_;

    static const char*		sTab()		{ return "\t"; }
    static const char*		sEOL()		{ return "\n"; }
    static const char*		sEOLTab()	{ return "\n\t\t"; }
    static const char*		sMsgWriteError(){return "Cannot write surface";}

   void 			init(const char* fulluserexp);
};

};

#endif

