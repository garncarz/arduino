#!/bin/bash

# Test runner script for Compressed Air Controller Android App
# This script runs all unit tests and displays a summary

echo "🧪 Running Compressed Air Controller Tests..."
echo "=============================================="

cd "$(dirname "$0")"

# Run the tests
./gradlew test

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo ""
    echo "✅ All tests passed successfully!"
    echo ""
    echo "🔍 Test Coverage:"
    echo "• Parser tests: Mode detection, barrel parsing, edge cases"
    echo "• Service tests: Auto-status functionality, state management"
    echo "• Integration tests: End-to-end mode detection scenarios"
    echo ""
    echo "📊 View detailed test report:"
    echo "file://$(pwd)/app/build/reports/tests/testDebugUnitTest/index.html"
else
    echo ""
    echo "❌ Some tests failed. Check the detailed report:"
    echo "file://$(pwd)/app/build/reports/tests/testDebugUnitTest/index.html"
fi

exit $exit_code
