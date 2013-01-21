#ifndef gridcreator_h
#define gridcreator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"

#include "bufstring.h"

class CubeSampling;
class IOObj;
class IOPar;
class SeisTrcReader;
class SeisTrcWriter;
class TaskRunner;
namespace EM { class Horizon2D; }

/*!
\brief ExecutorGroup to create 2D seismic grid from 3D.
*/

mExpClass(EMAttrib) Seis2DGridCreator : public ExecutorGroup
{
public:
    			Seis2DGridCreator(const IOPar&);
			~Seis2DGridCreator();

    virtual const char*	nrDoneText() const;

    static const char*	sKeyInput();
    static const char*	sKeyOutput();
    static const char*  sKeySelType();
    static const char*	sKeyOutpAttrib();
    static const char*	sKeyInlSelType();
    static const char*	sKeyCrlSelType();

    static const char*	sKeyBaseLine();
    static const char*	sKeyStartBinID();
    static const char*	sKeyStopBinID();
    static const char*	sKeyInlSpacing();
    static const char*	sKeyCrlSpacing();

    static const char*	sKeyInlPrefix();
    static const char*	sKeyCrlPrefix();


protected:

    bool		init(const IOPar&);
    bool		initFromInlCrl(const IOPar&,const IOObj&,const IOObj&,
	    			       const CubeSampling&);
    bool		initFromRandomLine(const IOPar&,const IOObj&,
	    				   const IOObj&,const CubeSampling&);
};


/*!
\brief ExecutorGroup to create 2D horizon grid from 3D.
*/

mExpClass(EMAttrib) Horizon2DGridCreator : public ExecutorGroup
{
public:
    			Horizon2DGridCreator();
			~Horizon2DGridCreator();

    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual const char*	nrDoneText() const;

    bool		init(const IOPar&,TaskRunner* tr=0);
    bool		finish(TaskRunner* tr=0);

    static const char*	sKeyInputIDs();
    static const char*	sKeySeisID();
    static const char*	sKeyPrefix();

protected:

    ObjectSet<EM::Horizon2D> horizons_;
    od_int64		nrdone_;
    od_int64		totalnr_;

};


#endif

