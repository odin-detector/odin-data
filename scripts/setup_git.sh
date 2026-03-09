#!/usr/bin/env bash

# Setup the use of the git hook directory stored in the repository
git config core.hooksPath .githooks

# Setup git-blame to ignore the given list of commits
git config blame.ignoreRevsFile .git-blame-ignore-revs
