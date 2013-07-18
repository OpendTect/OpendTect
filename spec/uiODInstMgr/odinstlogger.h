#ifndef odinstlogger_h
#define odinstlogger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: odinstlogger.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "bufstring.h"
#include <iostream>


namespace ODInst
{


/*!\brief Logs installer runs to a file. */


mDefClass(uiODInstMgr) Logger
{
public:

			Logger(const char*);

    BufferString	logfnm_;
    std::ostream&	strm_;

    void		flush();
    void		close();

    static void		setNoLogging();
    static Logger&	theInst(const char* fnmbase=0);

protected:

    std::ostream&	mkStream(const char*);

};

#define mODInstLogger()		ODInst::Logger::theInst()
#define mODInstLog()		mODInstLogger().strm_
#define mODInstToLog(s)		mODInstLog() << (s) << std::endl
#define mODInstToLog2(s1,s2)	mODInstLog() << (s1) << ' ' << (s2) << std::endl
#define mODInstToLogQuoted(p,s)	mODInstLog() << (p) << " '" << (s) << "' "
#define mODInstLogStartParagraph(clss) \
    mODInstLog() << "\n---> " << #clss << "\n\n"
#define mODInstLogEndParagraph(clss) \
    mODInstLog() << "\n" << #clss << "<---\n\n"


} // namespace

#endif

