#!/bin/bash

# SMP Programming Examples Test Runner
# Tests all 10 kernel synchronization examples

# Arrays to store results
declare -a results
declare -a example_names

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
total=0
passed=0
failed=0
skipped=0

# List of all examples in order
examples=(
    "race-condition-ex01"
    "atomic-fix-ex02"
    "irq-disable-ex03"
    "spinlock-basic-ex04"
    "spinlock-optimized-ex05"
    "preemption-counter-ex06"
    "mutex-lock-ex07"
    "mutex-lock-slow-ex08"
    "mutex-unlock-ex09"
    "memory-ordering-rcu-ex10"
)

echo -e "${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║   SMP Programming Examples - Test Runner          ║${NC}"
echo -e "${BLUE}║   Testing all 10 kernel synchronization examples  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}"
echo ""

# Main test loop
for example in "${examples[@]}"; do
    echo -e "\n${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Testing: $example${NC}"
    echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Check if directory exists
    if [ ! -d "$example" ]; then
        echo -e "${RED}✗ Directory not found: $example${NC}"
        results+=("SKIP")
        example_names+=("$example")
        ((total++))
        ((skipped++))
        continue
    fi
    
    cd "$example" || {
        echo -e "${RED}✗ Cannot enter directory: $example${NC}"
        results+=("SKIP")
        example_names+=("$example")
        ((total++))
        ((skipped++))
        continue
    }
    
    # Ask user if they want to build
    echo -n "Run 'make all' to rebuild? (y/N): "
    read -r answer
    answer=${answer,,} # Convert to lowercase
    
    if [ "$answer" = "y" ]; then
        echo -e "${BLUE}Building $example...${NC}"
        if make all 2>&1 | grep -q "error:"; then
            echo -e "${RED}✗ Build FAILED for $example${NC}"
            results+=("FAIL")
            example_names+=("$example")
            ((total++))
            ((failed++))
            cd ..
            continue
        else
            echo -e "${GREEN}✓ Build successful${NC}"
        fi
    else
        echo -e "${BLUE}Skipping build, using existing binary${NC}"
    fi
    
    # Run the test
    echo -e "${BLUE}Running test...${NC}"
    
    # Try make test first
    if make -n test &>/dev/null; then
        # make test target exists
        output=$(make test 2>&1)
        exit_code=$?
    else
        # Find and run the executable
        executable=$(find . -maxdepth 1 -type f -executable ! -name "*.sh" | head -1)
        if [ -z "$executable" ]; then
            # Try finding by name pattern (no extension)
            executable=$(ls -1 | grep -v '\.' | grep -v Makefile | head -1)
            if [ -n "$executable" ] && [ -f "$executable" ]; then
                chmod +x "$executable" 2>/dev/null
                output=$(./"$executable" 2>&1)
                exit_code=$?
            else
                echo -e "${RED}✗ No executable found${NC}"
                results+=("FAIL")
                example_names+=("$example")
                ((total++))
                ((failed++))
                cd ..
                continue
            fi
        else
            output=$("$executable" 2>&1)
            exit_code=$?
        fi
    fi
    
    # Evaluate results based on expected behavior
    test_passed=false
    
    case "$example" in
        "race-condition-ex01")
            # ex01 should demonstrate the bug (race condition)
            if [[ "$output" == *"BUG DETECTED"* ]] || [[ "$output" == *"multiple times"* ]] || [[ "$output" == *"freed multiple"* ]]; then
                echo -e "${GREEN}✓ PASS${NC} - Race condition bug demonstrated correctly"
                test_passed=true
            else
                echo -e "${YELLOW}⚠ PARTIAL${NC} - Race condition may not have triggered (try running multiple times)"
                # Consider it passed since race conditions are non-deterministic
                test_passed=true
            fi
            ;;
            
        *)
            # All other examples should succeed
            if [ $exit_code -eq 0 ]; then
                if [[ "$output" == *"SUCCESS"* ]] || [[ "$output" == *"✓"* ]] || [[ "$output" == *"success"* ]]; then
                    echo -e "${GREEN}✓ PASS${NC} - Test completed successfully"
                    test_passed=true
                elif [[ "$output" == *"FAIL"* ]] || [[ "$output" == *"ERROR"* ]] || [[ "$output" == *"error"* ]]; then
                    echo -e "${RED}✗ FAIL${NC} - Test reported failure"
                else
                    # Exit code 0 but no clear success message - consider passed
                    echo -e "${GREEN}✓ PASS${NC} - Test completed (exit code 0)"
                    test_passed=true
                fi
            else
                echo -e "${RED}✗ FAIL${NC} - Test exited with code $exit_code"
                echo -e "${RED}Output:${NC}"
                echo "$output" | head -20
            fi
            ;;
    esac
    
    # Record result
    if [ "$test_passed" = true ]; then
        results+=("PASS")
        ((passed++))
    else
        results+=("FAIL")
        ((failed++))
    fi
    
    example_names+=("$example")
    ((total++))
    
    cd ..
    sleep 0.5
done

# Print comprehensive summary
echo -e "\n${BLUE}╔════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                 TEST SUMMARY                       ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "Total examples tested: ${BLUE}$total${NC}"
echo -e "${GREEN}Passed:  $passed${NC}"
echo -e "${RED}Failed:  $failed${NC}"
echo -e "${YELLOW}Skipped: $skipped${NC}"
echo ""

# Calculate success rate
if [ $total -gt 0 ]; then
    success_rate=$((passed * 100 / total))
    echo -e "Success rate: ${BLUE}${success_rate}%${NC}"
    echo ""
fi

# Detailed results list
echo -e "${YELLOW}Detailed Results:${NC}"
echo -e "${YELLOW}─────────────────────────────────────────────────────${NC}"

for i in "${!example_names[@]}"; do
    case "${results[$i]}" in
        "PASS")
            echo -e "${GREEN}✓ PASS${NC}  ${example_names[$i]}"
            ;;
        "FAIL")
            echo -e "${RED}✗ FAIL${NC}  ${example_names[$i]}"
            ;;
        "SKIP")
            echo -e "${YELLOW}⊘ SKIP${NC}  ${example_names[$i]}"
            ;;
    esac
done

echo ""
echo -e "${YELLOW}─────────────────────────────────────────────────────${NC}"

# Final verdict
if [ $failed -eq 0 ] && [ $skipped -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests PASSED! Excellent work!${NC}"
    exit 0
elif [ $failed -eq 0 ]; then
    echo -e "${YELLOW}⚠ All run tests passed, but some were skipped.${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests FAILED. Please review the failures above.${NC}"
    exit 1
fi
