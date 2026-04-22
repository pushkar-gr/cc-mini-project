#!/bin/bash
# test_unionfs.sh — functional tests for Mini-UnionFS
set -e

make
BINARY="./bin/unionfs"
TEST_DIR="./unionfs_test_env"
LOWER="$TEST_DIR/lower"
UPPER="$TEST_DIR/upper"
MNT="$TEST_DIR/mnt"

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=== Mini-UnionFS Test Suite ==="

rm -rf "$TEST_DIR"
mkdir -p "$LOWER" "$UPPER" "$MNT"

echo "base_content"   > "$LOWER/base.txt"
echo "to_be_deleted"  > "$LOWER/delete_me.txt"

"$BINARY" "$LOWER" "$UPPER" "$MNT" &
FUSE_PID=$!
sleep 1

# Test 1: Layer Visibility — lower-layer file visible through mount
echo -n "Test 1: Layer Visibility ... "
if grep -q "base_content" "$MNT/base.txt" 2>/dev/null; then
    echo -e "${GREEN}PASSED${NC}"
else
    echo -e "${RED}FAILED${NC}"
fi

# Test 2: Copy-on-Write — modifying a lower file creates an upper copy
echo -n "Test 2: Copy-on-Write    ... "
echo "new_line" >> "$MNT/base.txt" 2>/dev/null
if grep -q "new_line" "$MNT/base.txt"   2>/dev/null &&
   grep -q "new_line" "$UPPER/base.txt" 2>/dev/null &&
   ! grep -q "new_line" "$LOWER/base.txt" 2>/dev/null; then
    echo -e "${GREEN}PASSED${NC}"
else
    echo -e "${RED}FAILED${NC}"
fi

# Test 3: Whiteout — deleting a lower-layer file hides it via .wh. marker
echo -n "Test 3: Whiteout         ... "
rm "$MNT/delete_me.txt" 2>/dev/null
if [ ! -f "$MNT/delete_me.txt" ] &&
   [   -f "$LOWER/delete_me.txt" ] &&
   [   -f "$UPPER/.wh.delete_me.txt" ]; then
    echo -e "${GREEN}PASSED${NC}"
else
    echo -e "${RED}FAILED${NC}"
fi

fusermount -u "$MNT" 2>/dev/null || umount "$MNT" 2>/dev/null || true
rm -rf "$TEST_DIR"
make clean
echo "=== Done ==="
