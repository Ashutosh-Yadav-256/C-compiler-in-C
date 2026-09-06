#!/usr/bin/env bash
set -e

echo "Building compiler..."
make all

TESTS_PASSED=0
TESTS_FAILED=0

for test_file in tests/*.c; do
    basename=$(basename "$test_file" .c)
    bin_file="tests/${basename}_bin"
    
    expected=0
    case "$basename" in
        hello) expected=42 ;;
        arithmetic) expected=10 ;;
        scopes) expected=25 ;;
        unary) expected=0 ;;
        control_flow) expected=33 ;;
        functions) expected=66 ;;
        early_return) expected=42 ;;
        pointers) expected=42 ;;
        chars_strings) expected=42 ;;
        bitwise) expected=42 ;;
        const_branch_safety) expected=42 ;;
        security_edge_cases) expected=42 ;;
        *) expected=0 ;;
    esac
    
    echo "Testing $basename (Expected: $expected)..."
    ./buildasm.sh "$test_file" "$bin_file" > /dev/null 2>&1
    
    EXIT_CODE=0
    if [ "$basename" = "fizzbuzz" ]; then
        ./"$bin_file" > "tests/${basename}.out" || EXIT_CODE=$?
        if [ "$EXIT_CODE" -eq 0 ] && [ "$(wc -l < "tests/${basename}.out")" -eq 15 ] && [ "$(tail -n 1 "tests/${basename}.out")" = "FizzBuzz" ]; then
            echo "  [PASS] $basename (Output verified)"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            echo "  [FAIL] $basename: Output mismatch or execution failed"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
        continue
    fi
    
    ./"$bin_file" || EXIT_CODE=$?
    
    if [ "$EXIT_CODE" -eq "$expected" ]; then
        echo "  [PASS] $basename"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo "  [FAIL] $basename: Expected $expected, got $EXIT_CODE"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
done

echo ""
echo "=============================="
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo "=============================="

if [ "$TESTS_FAILED" -gt 0 ]; then
    exit 1
fi
exit 0
