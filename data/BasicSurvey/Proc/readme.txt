This is the Proc directory - it contains all kinds of log files,
processing parameter files and reports (like SEG-Y reports). Every
multi-machine processing job creates a new directory here.

System/Data admininstrators:
During processing, OpendTect depends on the files in this directory. The
directory can be cleaned up when nobody is using the survey, or when you are sure nobody is doing bacth processing. Cleanup can however make it impossible
for users to reconstruct full processing flows and some objects that were used,
like the attribute sets that created their output cubes.

Developers:
This directory does not have support via the IO Manager. That means we are
talking about flat file access only, via a construction like:
#include "oddirs.h"
BufferString fname = GetProcFileName( "myprocess.log" );

