#!/bin/bash
# test_all.sh - Test tất cả modules
# Sử dụng: sudo ./test_all.sh

set -e

echo "=========================================="
echo "Test All Modules - Network Programming"
echo "=========================================="
echo ""

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Script này cần quyền root"
    echo "Chạy: sudo $0"
    exit 1
fi

# Build tất cả
echo "[0] Build tất cả modules và programs..."
make clean
make
echo ""

# Đặt quyền executable cho các scripts
chmod +x test_*.sh

# Test counter
PASSED=0
FAILED=0
TOTAL=4

echo "=========================================="
echo "Bắt đầu test từng module..."
echo "=========================================="
echo ""

# Test 1: sk_buff demo
echo ">>> TEST 1/4: sk_buff Demo Module <<<"
if ./test_skbuff.sh; then
    echo "✓ Test 1 PASSED"
    ((PASSED++))
else
    echo "✗ Test 1 FAILED"
    ((FAILED++))
fi
echo ""
sleep 2

# Test 2: Loopback driver
echo ">>> TEST 2/4: Loopback Driver <<<"
if ./test_loopback.sh; then
    echo "✓ Test 2 PASSED"
    ((PASSED++))
else
    echo "✗ Test 2 FAILED"
    ((FAILED++))
fi
echo ""
sleep 2

# Test 3: HTTP detector
echo ">>> TEST 3/4: HTTP Password Detector <<<"
if ./test_http_detector.sh; then
    echo "✓ Test 3 PASSED"
    ((PASSED++))
else
    echo "✗ Test 3 FAILED"
    ((FAILED++))
fi
echo ""
sleep 2

# Test 4: Steganography
echo ">>> TEST 4/4: TCP/UDP Steganography <<<"
if ./test_steganography.sh; then
    echo "✓ Test 4 PASSED"
    ((PASSED++))
else
    echo "✗ Test 4 FAILED"
    ((FAILED++))
fi
echo ""

# Summary
echo ""
echo "=========================================="
echo "TEST SUMMARY"
echo "=========================================="
echo "Total tests:  $TOTAL"
echo "Passed:       $PASSED"
echo "Failed:       $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo ""
    echo "Modules được test:"
    echo "  ✓ sk_buff demo"
    echo "  ✓ Loopback network driver"
    echo "  ✓ HTTP password detector"
    echo "  ✓ TCP/UDP steganography"
    exit 0
else
    echo "⚠ SOME TESTS FAILED"
    echo "Xem log ở trên để biết chi tiết"
    exit 1
fi
