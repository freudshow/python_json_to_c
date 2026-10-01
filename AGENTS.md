# Repository Agent Guidelines

## Project Overview

- This repository is a small, standard-library-only Python command-line tool.
- `json_to_c.py` infers a schema from one JSON example and writes a C header
  and source file containing data structures and JSON encode/decode functions.
- The implementation and generator are intentionally kept in one module.
- `test/` contains fixture directories. Each normally has an `input.json` and
  checked-in generated `fixture.h` / `fixture.c` files.
- Fixture names exercise nested/typed values, nulls, empty values, variants,
  generic JSON and randomly shaped examples.
- There is no package layout, dependency manifest, build system, or configured
  Python test framework at present.
- Do not assume that files under `test/` are executable test programs; they are
  input and generated-output examples for manual regression checks.

## Repository Instructions

- Read this file before modifying code; keep its commands aligned with the
  actual CLI and fixture layout.
- No `.cursor/rules/`, `.cursorrules`, or
  `.github/copilot-instructions.md` was present when these guidelines were
  prepared. Recheck if adding or changing editor-specific instructions.
- Keep reusable implementation in `json_to_c.py` unless project growth makes
  introducing a package clearly useful.
- Put automated Python tests in `tests/` if a test framework is introduced;
  retain the existing `test/` fixture directory for JSON/C fixture pairs.

## Build, Lint, and Test Commands

There is no build command, configured linter, or test runner. Do not claim that
a lint or unit-test suite has passed when only running the checks below.

### Python checks

Run these from the repository root with Python 3.7 or newer:

```bash
python3 -m py_compile json_to_c.py
python3 json_to_c.py --help
```

`py_compile` is the available syntax check; no formatter, type checker, or
linting tool is configured. Use an installed compatible Python version rather
than adding dependencies just to perform routine validation.

### Run one fixture (single-case check)

For example, test only `test/typed_nested` by generating into a fresh temporary
directory, then compile that fixture's generated C source:

```bash
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
python3 json_to_c.py test/typed_nested/input.json \
  --output-dir "$tmpdir" --base-name fixture --root-name fixture
cc -std=c11 -Wall -Wextra -fsyntax-only "$tmpdir/fixture.c"
```

For cJSON-backed output, add `--backend cjson` and link/include cJSON, for
example `--cjson-header cJSON.h` when that is the installed header path, plus
the flags supplied by the local cJSON package. The generated typed codecs still
use the inferred schema; cJSON is the parse/print implementation, not a way to
accept shapes absent from the sample. cJSON stores numbers as doubles; this
backend rejects values outside its conservative safe numeric range. Keep using
the built-in backend when exact number lexemes or decimal precision must be
preserved. cJSON also represents strings as NUL-terminated buffers; this
backend rejects input containing an escaped `\\u0000` string value.

Replace `typed_nested` with a fixture directory such as `empty_and_null`,
`mixed_generic`, `object_array`, or `variant_object_array` to select one case.
The temporary output prevents accidental changes to checked-in generated C.
If compilation is not relevant to a change, run the generator alone and inspect
both emitted files and the generated API.

Generated files embed the current date, so a raw diff against checked-in
fixtures can show a date-only difference. Inspect substantive code/API changes
separately; do not update fixtures unless the task requires updated examples.

### Run all fixture cases

This optional shell loop exercises generation and C syntax for every fixture
with an `input.json` (requires `cc`):

```bash
set -eu
for input in test/*/input.json; do
  tmpdir="$(mktemp -d)"
  trap 'rm -rf "$tmpdir"' EXIT
  python3 json_to_c.py "$input" --output-dir "$tmpdir" \
    --base-name fixture --root-name fixture
  cc -std=c11 -Wall -Wextra -fsyntax-only "$tmpdir/fixture.c"
  rm -rf "$tmpdir"
  trap - EXIT
done
```

When a command fails, report which check failed. For deeper generated-code
validation, compile with `-c` instead of `-fsyntax-only`, or write a focused C
round-trip harness if the behavior change warrants runtime testing.

## Python Style and Design

- Support Python 3.7+; do not silently raise the minimum version without a
  deliberate compatibility decision.
- Use four spaces per indentation level, UTF-8 source, and readable lines.
- Follow the existing standard-library import style: module docstring, future
  imports, then standard-library imports at the top. Do not add third-party
  dependencies without a concrete need and explicit project-level support.
- Keep imports grouped after the future import; use aliases only when they
  clarify or avoid conflicts (the module uses `_datetime`).
- Use `snake_case` for functions, methods, variables, and module-level values;
  use `CapWords` for classes and `UPPER_SNAKE_CASE` for constants.
- Preserve type annotations on public and non-trivial helpers. Use the existing
  `typing` forms and `from __future__ import annotations` for Python 3.7 support.
- Prefer focused functions, explicit inputs/outputs, and standard-library
  facilities. Avoid adding classes or abstractions for one-off transformations.
- Use dataclasses for schema records where appropriate; retain explicit model
  fields rather than hiding the inferred schema in unstructured dictionaries.
- Write concise docstrings for public functions and non-obvious transformation
  logic. Match the surrounding source's language and terminology.
- Use f-strings for generated text, but keep generated-C assembly readable and
  preserve deterministic output ordering.
- Keep comments useful: explain invariants or non-obvious constraints, not
  obvious statements of what the next line does.

## Schema and Naming Compatibility

- Preserve stable generated identifiers: normalized lowercase `snake_case`,
  collision suffixes, C keyword avoidance, and existing root-name defaults.
- JSON property names in the generated API/output must remain the original
  names even when C field names are normalized.
- Preserve inference distinctions among booleans, integers, doubles, strings,
  null/generic values, objects, arrays, and heterogeneous array variants.
- Python's `bool` is a subclass of `int`; test booleans before integer handling
  whenever classifying JSON scalar values.
- Do not coerce mixed numeric arrays or large integers in ways that lose
  precision. Generic DOM fallback and number lexeme preservation are deliberate.
- When changing schema or name allocation, exercise keyword fields, normalized
  name collisions, empty inputs, nested arrays/objects, and mixed values.
- Treat generated C declarations and public function signatures as an API;
  avoid renaming or changing ownership/error contracts without a compatibility
  reason and corresponding fixture review.

## Error Handling and Generated C

- Keep CLI failures actionable and on `stderr`; `main()` uses status `2` for
  JSON/input read failures and `3` for generation/write failures.
- Catch expected errors at the CLI boundary; do not broadly suppress exceptions
  or turn programming errors into success-shaped output.
- Validate impossible model states with clear exceptions, following existing
  `ValueError` / `TypeError` use rather than silently emitting invalid C.
- Generated C targets C11 and uses `<stdbool.h>`, `<stddef.h>`, and
  `<stdint.h>` types. Compile representative output with `cc -std=c11`.
- Preserve generated C's lower-case snake-case names, `_t` type suffixes,
  ownership/free functions, and documented return codes.
- Check allocation, parse, and encode failures; follow existing cleanup paths
  so partially constructed values do not leak or become double-freed.
- Preserve the convention that `0` means success and nonzero means failure;
  respect the documented `-2` buffer-too-small result from `*_to_json`.

## Working Tree and Generated Artifacts

- Start by checking `git status`; treat existing edits as user-owned.
- Generate experimental outputs under a fresh temporary directory, not over a
  fixture or arbitrary source file.
- Checked-in `.c` and `.h` files under `test/` are fixtures, not disposable
  build output. Change them only when explicitly needed for the task.
- Keep changes scoped to the requested behavior and its necessary tests or
  fixtures; do not add generated artifacts, dependencies, or unrelated cleanup.
- Before finishing, inspect the diff and run the relevant Python and/or single
  fixture checks described above.
