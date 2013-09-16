<?php

/**
 * Enforces basic text file rules.
 *
 * @group linter
 */
final class SVNLinter extends ArcanistLinter {

  const LINT_MISSING_PROPERTY	= 1;

  public function getLinterPriority() {
    return 0.5;
  }

  public function getLinterName() {
    return 'SVN';
  }

  public function getLintNameMap() {
    return array(
      self::LINT_MISSING_PROPERTY          => pht('Missing SVN property '),
    );
  }

  public function lintPath($path) {
    if (!strlen($this->getData($path))) {
      // If the file is empty, don't bother; particularly, don't require
      // the user to add a newline.
      return;
    }

    $this->lintSVNProperties($path);

    if ($this->didStopAllLinters()) {
      return;
    }

    if ($this->getEngine()->getCommitHookMode()) {
      $this->lintNoCommit($path);
    }
  }

  protected function lintSVNProperties($path) {
    $retval = 0;
    exec("svn proplist $path", $svn_return, $retval );
    if ( $retval!==0 )
      return;

    $eolfound = false;
    $keywordfound = false;

    foreach ( $svn_return as $line ) {
      if ( strpos($line,"svn:eol-style")!==false )
	$eolfound = true;
      if ( strpos($line,"svn:keywords")!==false )
	$keywordfound = true;
    }

    if ( $eolfound==false || $keywordfound==false ) {
      $this->raiseLintAtPath(
	self::LINT_MISSING_PROPERTY,
        "File \"$path\" is missing svn::eol-style and/or svn::keyword ".
	"subversion properties.");
    }
  }
}
