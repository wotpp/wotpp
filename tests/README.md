# Tests

To add a new test, make a file in this directory and add it to the `test_cases` list in
`../meson.build`.

Test files have a special construct of the form `#[expect(<string>)]` where `<string>` is
a string of any valid utf-8 character.

When the tests are run using `run_test.py`, the script makes note of all of these test cases
and builds up a string of what the expected output _should_ be and compares it against the
actual output from wot++. If they differ, the test returns non-zero status and fails.

Example:
```
let foo "bar"

#[expect(bar)]
foo
```
