## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.
## # The following are required to uses Dart and the Cdash dashboard

set(CTEST_PROJECT_NAME "OpendTect-6_6_rel")
set(CTEST_NIGHTLY_START_TIME "00:00:00 UTC")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "intranet")
set(CTEST_DROP_LOCATION "/cdash/submit.php?project=OpendTect-6.6_rel")
set(CTEST_DROP_SITE_CDASH TRUE)
