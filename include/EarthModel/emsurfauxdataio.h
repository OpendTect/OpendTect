#ifndef emsurfauxdataio_h
#define emsurfauxdataio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emsurfauxdataio.h,v 1.12 2005-01-17 16:27:10 nanne Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "executor.h"


class BinIDSampler;
template <class T> class DataInterpreter;

namespace EM
{

class Surface;

/*!\brief  Writes auxdata to file */

class dgbSurfDataWriter : public Executor
{
public:
    				dgbSurfDataWriter(const EM::Surface& surf,
						  int dataidx,
						  const BinIDSampler* sel,
						  bool binary,
						  const char* filename);
			/*!<\param surf		The surface with the values
			    \param dataidx	The index of the data to be
			    			written
			    \param sel		A selection of which data
			    			that should be written.
						Can be null, i.e. no selection
			    \param binary	Specify whether the data should
			    			be written in binary format
			*/

				~dgbSurfDataWriter();

    virtual int			nextStep();
    virtual int			nrDone() const;
    virtual int			totalNr() const;
    virtual const char*		message() const;

    static const char*		attrnmstr;
    static const char*		infostr;
    static const char*		intdatacharstr;
    static const char*		int64datacharstr;
    static const char*		floatdatacharstr;
    static const char*		filetypestr;
    static const char*		shiftstr;

    static BufferString		createHovName(const char* base,int idx);

protected:

    bool			writeInt(int);
    bool			writeInt64(int64);
    bool			writeFloat(float);
    int				dataidx;
    const EM::Surface&		surf;
    const BinIDSampler*		sel;
  
    TypeSet<EM::SubID>		subids;
    TypeSet<float>		values;
    int				sectionindex;

    int				chunksize;
    int				nrdone;
    int				totalnr;
    BufferString		errmsg;

    std::ostream*		stream;
    bool			binary;
    BufferString		filename;
};


/*!\brief Reads auxdata from file */

class dgbSurfDataReader : public Executor
{
public:
    				dgbSurfDataReader(const char* filename);
				~dgbSurfDataReader();

    const char*			dataName() const;
    const char*			dataInfo() const;

    void			setSurface(EM::Surface& surf);

    virtual int			nextStep();
    virtual int			nrDone() const;
    virtual int			totalNr() const;
    virtual const char*		message() const;

protected:
    bool			readInt(int&);
    bool			readInt64(int64&);
    bool			readFloat(float&);
    BufferString		dataname;
    BufferString		datainfo;
    int				dataidx;
    float			shift;
    EM::Surface*		surf;
    const BinIDSampler*		sel;
  
    int				sectionindex;
    int				nrsections;
    EM::SectionID		currentsection;
    int				valsleftonsection;

    int				chunksize;
    int				nrdone;
    int				totalnr;
    BufferString		errmsg;

    std::istream*		stream;
    DataInterpreter<int>*	intinterpreter;
    DataInterpreter<int64>*	int64interpreter;
    DataInterpreter<float>*	floatinterpreter;
    bool			error;
};

};

#endif
