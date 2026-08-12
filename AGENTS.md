# Repository Guidelines

## Project Structure & Module Organization

This is a small, self-contained Python CLI project. The main implementation is
in [`json_to_c.py`](json_to_c.py): it loads a JSON example, infers a schema, and
generates matching C header/source files. There are currently no separate
packages, assets, test directories, or checked-in generated outputs. Keep new
tests in `tests/` and place reusable implementation code in `json_to_c.py`
unless the project grows enough to justify a package.

## Build, Test, and Development Commands

No build system or dependency manifest is currently present. Use these commands
for local development:

```bash
python3 -m py_compile json_to_c.py
python3 json_to_c.py --help
python3 json_to_c.py sample.json -o /tmp/json-to-c --base-name sample
```

The first command checks Python syntax, the second verifies the CLI interface,
and the third generates `sample.h` and `sample.c` in a disposable directory.
Compile generated C with a C compiler when appropriate, for example
`cc -std=c11 -Wall -Wextra -c /tmp/json-to-c/sample.c`.

## Coding Style & Naming Conventions

Use Python 3.7+, four-space indentation, type annotations, and standard-library
solutions where practical. Keep functions focused and preserve the existing
snake_case naming for Python symbols. Generated C identifiers are normalized to
lowercase snake_case; preserve that behavior when changing name allocation or
schema inference. Add concise docstrings for non-obvious transformation logic.

## Testing Guidelines

There is no test framework or coverage requirement yet. For changes, at minimum
run syntax validation and exercise representative JSON inputs covering objects,
arrays, scalars, nulls, mixed arrays, and names that collide with C keywords.
Verify generated files compile cleanly and inspect the generated API for
backward-compatible naming.

## Commit & Pull Request Guidelines

No repository commit history is available to establish an existing convention.
Use short, imperative subjects such as `Add mixed-array generation test`, and
keep unrelated changes separate. Pull requests should describe the input JSON
shapes affected, include the validation commands run, explain generated-code
changes, and attach a small before/after example when output changes.

## Generated Output & Safety

Treat generated `.c` and `.h` files as review artifacts unless explicitly
requested for check-in. Use a temporary output directory during experiments and
never overwrite user-owned source files without checking the selected output
paths first.
