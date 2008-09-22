#ifndef attribdatacubeswriter_h
#define attribdatacubeswriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: attribdatacubeswriter.h,v 1.2 2008-09-22 13:06:42 cvskris Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "executor.h"
#include "multiid.h" 

class BinID;
class IOPar;
class SeisTrcWriter;
class SeisTrc;

namespace Attrib
{
    
class DataCubes;

class DataCubesWriter : public Executor
{
public:
    			DataCubesWriter(MultiID&,Attrib::DataCubes&,
				       const TypeSet<int>& cubeindices);
			~DataCubesWriter();

    od_int64		nrDone() const;
    od_int64		totalNr() const;		
    const char*		message() const		{ return "Writing out data!"; }
    const char*		nrDoneText() const	{ return "Traces written:"; }
    int			nextStep();
 	

private:

   int			nrdone_;    
   int			totalnr_;
   Attrib::DataCubes&	cube_;
   HorSamplingIterator	iterator_;
   BinID		currentpos_;
   MultiID		mid_;
   SeisTrcWriter*	writer_;
   SeisTrc&		trc_;
   TypeSet<int>		cubeindices_;
};

}; //namespace


#endif
