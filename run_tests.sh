#!/bin/bash
# Test runner script for SamFocus

set -e

echo "======================================"
echo "   SamFocus Test Runner"
echo "======================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if zig is installed
if ! command -v zig &> /dev/null; then
    echo -e "${RED}Zig is not installed or not in PATH${NC}"
    echo "Please install Zig 0.14.0 or later from https://ziglang.org/download/"
    exit 1
fi

# Build and run tests
echo -e "${YELLOW}Building and running tests...${NC}"
echo ""

if [ "$1" == "-v" ] || [ "$1" == "--verbose" ]; then
    # Run test executables directly for verbose output
    zig build
    echo ""
    echo "Running unit tests..."
    ./zig-out/bin/test_database
    echo ""
    echo "Running integration tests..."
    ./zig-out/bin/test_workflows
else
    # Use zig build test for clean output
    zig build test
fi

TEST_RESULT=$?

echo ""
if [ $TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}======================================"
    echo "   All Tests Passed! ✓"
    echo "======================================${NC}"
    exit 0
else
    echo -e "${RED}======================================"
    echo "   Some Tests Failed! ✗"
    echo "======================================${NC}"
    echo ""
    echo "Run with -v or --verbose for detailed output"
    exit 1
fi
