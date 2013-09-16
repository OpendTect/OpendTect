<?php

/**
 * Basic lint engine which just applies several linters based on the file types
 *
 * @group linter
 */
final class dGBLintEngine extends ArcanistLintEngine {

  public function buildLinters() {
    $linters = array();

    $paths = $this->getPaths();

    foreach ($paths as $key => $path) {
      if (preg_match('@^externals/@', $path)) {
        // Third-party stuff lives in /externals/; don't run lint engines
        // against it.
        unset($paths[$key]);
      }
    }

    $text_paths = preg_grep('/\.(h|cc|php|arcconfig|txt|cmake|rc)$/', $paths);
    $linters[] = id(new TextLinter())->setPaths($text_paths);
    $linters[] = id(new SVNLinter())->setPaths($paths);

    return $linters;
  }

}
