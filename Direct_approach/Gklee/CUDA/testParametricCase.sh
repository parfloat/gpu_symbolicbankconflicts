#!/bin/bash
#
# testParametricCase.sh - Run parametric-case benchmark with parametric mode (no BC checking)
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GKLEE_HOME="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$(dirname "$GKLEE_HOME")/bin"

BENCHMARK_DIR="$SCRIPT_DIR/Benchmarks/Misc_Test/parametric-case"
PROG="parametric-case"

cd "$BENCHMARK_DIR" || exit 1

echo "=== Running parametric-case with parametric mode (no BC checking) ==="
echo "Benchmark: $BENCHMARK_DIR/$PROG"
echo ""

# Run GKLEE with symbolic-config (parametric mode) without bank conflict checking
$BIN_DIR/gklee --symbolic-config "$PROG"
