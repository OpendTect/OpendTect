#ifndef attribdatacubeswriter_h
#define attribdatacubeswriter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
 RCS:		$Id: attribdatacubeswriter.h,v 1.4 2009-07-22 16:01:13 cvsbert Exp $
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

mClass DataCubesWriter : public Executor
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
