# Configuration for ctest's sending of the reports.
# 
# $Id: CTestConfig.cmake,v 1.1 2012-07-05 07:28:26 cvskris Exp $
#

set(CTEST_PROJECT_NAME "OpendTect")
set(CTEST_NIGHTLY_START_TIME "01:00:00 UTC")
set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "my.cdash.org")
set(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
set(CTEST_DROP_SITE_CDASH TRUE)
