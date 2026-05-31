# cbounce

`cbounce` is an experimental C99 single-header port of Bounce, a C++ 3D physics
engine by Irlan Robson.

This current release is intentionally small. It contains the generated
single-header API and one self-checking example.

Try it at https://cstaavetti.github.io/cbounce/

## Status

This is `v0.1.0`. There is no documented known functional parity gap against the
covered Bounce behavior, but broader randomized and long-running solver/contact
stress coverage is still pending. Treat this as an integration trial release,
not a stability guarantee. 

Initial tests seem to indicate that this port is
more performant and slightly more accurate in some of the originals tests.

## Usage

Include `cbounce.h` normally for declarations. In exactly one C translation
unit, define `CBOUNCE_IMPLEMENTATION` before including it:

```c
#define CBOUNCE_IMPLEMENTATION
#include "cbounce.h"
```

Build with the math library:

```sh
cc your_file.c -I. -std=c99 -Wall -Wextra -pedantic -O2 -lm -o your_app
```

## Example

Run the self-checking example:

```sh
make example
```

Both targets build `examples/example.c` into `build/example`.

## Web demo

Build the browser FPS collapse demo:

```sh
make web
```

This writes a static site to `build/web/site`. To run the smoke test:

```sh
make web-test
```

To play it locally:

```sh
make web-serve
```

Then open `http://localhost:8000`.

The demo uses `cbounce` compiled to WebAssembly for physics and loads Three.js
from a pinned jsDelivr CDN URL for rendering.

## GitHub Pages

The repository includes a GitHub Actions workflow at
`.github/workflows/pages.yml`. After the workflow is merged to `main`, set the
repository's Pages source to "GitHub Actions" in GitHub settings. Successful
pushes to `main` will publish the static demo to the repository Pages site.

## Files

- `cbounce.h` - generated single-header release artifact.
- `examples/example.c` - self-checking single-header usage example.
- `examples/web/` - WebAssembly FPS collapse demo.
- `license.txt` - zlib-style license from Bounce.
