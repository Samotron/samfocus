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

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Build directory not found. Running initial setup...${NC}"
    meson setup build
fi

# Compile tests
echo -e "${YELLOW}Compiling tests...${NC}"
meson compile -C build

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}Compilation successful!${NC}"
echo ""

# Run tests
echo -e "${YELLOW}Running tests...${NC}"
echo ""

if [ "$1" == "-v" ] || [ "$1" == "--verbose" ]; then
    meson test -C build -v
else
    meson test -C build
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
