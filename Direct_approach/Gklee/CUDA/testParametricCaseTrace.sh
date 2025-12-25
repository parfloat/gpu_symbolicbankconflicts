#!/bin/bash
#
# testParametricCaseTrace.sh - Run parametric-case with parametric mode and verbose tracing
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GKLEE_HOME="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$(dirname "$GKLEE_HOME")/bin"

BENCHMARK_DIR="$SCRIPT_DIR/Benchmarks/Misc_Test/parametric-case"
PROG="parametric-case"
TRACE_FILE="$HOME/testParametricCase.txt"

cd "$BENCHMARK_DIR" || exit 1

echo "=== Running parametric-case with parametric mode + verbose tracing ==="
echo "Benchmark: $BENCHMARK_DIR/$PROG"
echo "Trace output: $TRACE_FILE"
echo ""

# Run GKLEE with:
# --symbolic-config : parametric mode
# --verbose-trace   : enable verbose execution tracing
# (no --check-BC)   : bank conflict checking disabled
$BIN_DIR/gklee --symbolic-config --verbose-trace="$TRACE_FILE" "$PROG"

echo ""
echo "=== Trace file saved to: $TRACE_FILE ==="
echo "Lines: $(wc -l < "$TRACE_FILE")"
echo "Size: $(ls -lh "$TRACE_FILE" | awk '{print $5}')"
