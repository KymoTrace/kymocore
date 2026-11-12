# Contributing to KymoCore

**English** · [Deutsch](CONTRIBUTING.de.md)

Contributions are welcome: bug reports, suggestions and pull requests.

## Grant of rights for pull requests

KymoCore is offered under the MIT license (see [LICENSE](../LICENSE)). So that the
project can keep its license choices open for submitted code, a pull request is
only accepted with the following assurance:

> By submitting a contribution I confirm that
>
> 1. I created the contribution myself or hold the necessary rights to it, and
>    that it does not infringe any third-party rights;
> 2. I grant Christof Seidel a worldwide, perpetual, irrevocable, non-exclusive
>    and royalty-free right to use, modify, reproduce and distribute the
>    contribution, and to sublicense it under the MIT license as well as under
>    any other licenses.
>
> I retain my copyright in the contribution.

Please confirm this by ticking the box in the pull request template:
**"I agree to the grant of rights in CONTRIBUTING.md."** A check blocks
pull requests without this confirmation.

If the English and German versions differ, the German version prevails.

## Technical notes

- Format C/C++ files in `src/`, `examples/` and `test/` using the root
  `.clang-format` (verified with clang-format 23.1.1): four spaces, Allman braces
  and a 100-column code limit. Include order and Doxygen text stay unchanged;
  function separator rules remain 80 columns including indentation.
  Run `clang-format --style=file -i path/to/file.cpp`, or check without edits
  using `clang-format --style=file --dry-run --Werror path/to/file.cpp`.
- New source files start with the SPDX line
  `SPDX-License-Identifier: MIT`.
- Run the native suite with `python test/native_test.py` from the repository root.
- Check the exported package with `python tools/test_package.py` and the
  configurator with `python -m pytest -q tools/configurator/tests`.
- Additional application integration and hardware tests live in
  [KymoProbe](https://github.com/KymoTrace/kymoprobe). See the
  [publishing checklist](PUBLISHING.md) and [project overview](../README.md).
